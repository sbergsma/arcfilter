/******************************************
 * 
 * linearFilter.cpp
 *
 * Shane Bergsma
 * June 3, 2010
 *
 ******************************************/

#include <iostream>   // For reading/writing STDIN
#include <sstream>    // For parsing the input
#include <time.h>     // For timing:

#include "filterCommon.h"

const std::string USAGE = "USAGE: cat taggedFile | ./linearFilter ultraLinearWeights ultraPairWeights";

const bool runTiming = 0;

void applyFilters(const float noneBias, const float pairBias,
				  const StrVec &words, const StrVec &tags,
				  const LinearWeightsMap &linWeights, const UltraPairWeightsMap &pairWeights) {
  int sentSize = tags.size();

  ////////////////////////////////////////////////////////////////////////
  // STEP 1: Precompute what you can from the sentence in linear time (one pass)
  ////////////////////////////////////////////////////////////////////////
  // 1) Predetermine which of the nodes might be roots and store in here:
  std::vector<bool> possibleRootBool; possibleRootBool.push_back(0); // No need to check artificial root
  // 2) Predetermine the token-role filter scores for all the nodes in linear time:
  FilterScores headS, LxS, RxS, L1S, L5S, R1S, R5S, rootS;
  // Push on zeros on these so you don't have to re-adjust the offset later: (these correspond to the artificial root)
  headS.push_back(0); rootS.push_back(0);
  LxS.push_back(0); L1S.push_back(0); L5S.push_back(0);
  RxS.push_back(0); R1S.push_back(0); R5S.push_back(0);
  // 3) For speed, preproduce the head-markers that you join with the pairs:
  StrVec leftHeadMarkers; 	leftHeadMarkers.push_back(""); // Never look up the root in this
  StrVec rightHeadMarkers;  rightHeadMarkers.push_back("");
  // 4) Now get the linear-pass information:
  for (int i=1; i<sentSize; i++) {  // Go through each word: 
    // a) First, check if root:
    if (tags[i] == "VBD" || tags[i] == "VBZ" || tags[i] == "VBP" || tags[i] == "MD") possibleRootBool.push_back(1);
    else possibleRootBool.push_back(0);
    // b) Then, build those head markers:
    std::string lhstr = "h" + tags[i]; std::string rhstr = "<h" + tags[i];
    leftHeadMarkers.push_back(lhstr); rightHeadMarkers.push_back(rhstr);
    // c) Then, get the scores:
    StrVec linFeats; // i) Create Feature Vector
    buildLinearFeatureVector(i, words, tags, sentSize, linFeats);
    eightF preds;    // ii) Get the filter predictions (scores):
	getUltraLinearFilterScores(linWeights, linFeats, preds);
    headS.push_back(preds[0]); rootS.push_back(preds[1]); /// iii) Stick on the scores
    LxS.push_back(preds[2]); L1S.push_back(preds[3]); L5S.push_back(preds[4]);
    RxS.push_back(preds[5]); R1S.push_back(preds[6]); R5S.push_back(preds[7]);
  }
  ////////////////////////////////////////////////////////////////////////
  // STEP 2: Go through all arcs (quadratic loop), finding and writing possible heads for each mod
  ////////////////////////////////////////////////////////////////////////
  std::string possiblePairs = "";
  for (int mod = 1; mod < sentSize; mod++) {  // An example for every modifier but the root:
    std::vector<int> headList;    // the list of heads for mod-i:
    // Precompute the mod strings, for efficiency:
    std::string rightModMarker = "<m" + tags[mod];  std::string leftModMarker = "m" + tags[mod];
    // For speed, do everything knowing the order of head and mod, in three blocks:
    ////////////////////////////////////////////////////////////////////////
    // BLOCK 1: head == 0
    ////////////////////////////////////////////////////////////////////////
    { // int head = 0
      bool filtered = 0;
      std::string pairStr = "hROOT" + rightModMarker;
      // Look up the weights on this tag pair + distance for none/pair:
      UltraPairWeightsMap::const_iterator finder = pairWeights.find(pairStr);
      float noneScore = noneBias; float pairScore = pairBias;
      if (finder != pairWeights.end()) {
		noneScore += (finder->second)[0] * mod; pairScore += (finder->second)[1] * mod;
      }
      if ( pairScore > noneScore || LxS[mod] > noneScore || R1S[mod] > noneScore || R5S[mod] > noneScore ||
		   (L1S[mod] > noneScore && mod != 1) || (L5S[mod] > noneScore && mod>5) )
		filtered = 1;
      else
		// See if there's another root, in which case this guy can't be the root:
		for (int i=1; i<sentSize && !filtered; i++)
		  if (i != mod && possibleRootBool[i] && rootS[i] > noneScore) filtered = 1;
      if (filtered == 0) headList.push_back(0);
    }

    ////////////////////////////////////////////////////////////////////////
    // BLOCK 2: head < mod:
    ////////////////////////////////////////////////////////////////////////
    for (int head = 1; head < mod; head++) {    // Go through all the possible heads:
      bool filtered = 0;
      std::string pairStr = leftHeadMarkers[head] + rightModMarker;
      // Look up the weights on this tag pair + distance for none/pair:
      UltraPairWeightsMap::const_iterator finder = pairWeights.find(pairStr);
      float noneScore = noneBias; float pairScore = pairBias;
      if (finder != pairWeights.end()) {
		int distance = mod - head;
		noneScore += (finder->second)[0] * distance; pairScore += (finder->second)[1] * distance;
      }
      if ( pairScore > noneScore || headS[head] > noneScore || LxS[mod] > noneScore ||  R1S[mod] > noneScore ||
		   R5S[mod] > noneScore || (L1S[mod] > noneScore && head != mod-1) || (L5S[mod] > noneScore && (mod-head>5)) )
		filtered = 1;
      else
		// Now, see if there's a root that can filter you: Go through
		// all the nodes between mod and head (or head and mod):
		for (int i = head + 1; i < mod && !filtered; i++)
		  if (possibleRootBool[i] && rootS[i] > noneScore) // Check: can this node be a root?
			filtered = 1;
      if (filtered == 0) headList.push_back(head);
    }

    ////////////////////////////////////////////////////////////////////////
    // BLOCK 3: mod < head:
    ////////////////////////////////////////////////////////////////////////
    for (int head = mod+1; head < sentSize; head++) {    // Go through all the possible heads:
      bool filtered = 0;
      std::string pairStr = leftModMarker + rightHeadMarkers[head];
      // Look up the weights on this tag pair + distance for none/pair:
      UltraPairWeightsMap::const_iterator finder = pairWeights.find(pairStr);
      float noneScore = noneBias; float pairScore = pairBias;
      if (finder != pairWeights.end()) {
		int distance = head - mod;
		noneScore += (finder->second)[0] * distance; pairScore += (finder->second)[1] * distance;
      }
      if ( pairScore > noneScore || headS[head] > noneScore || RxS[mod] > noneScore ||
		   L1S[mod] > noneScore || L5S[mod] > noneScore ||
		   (R1S[mod] > noneScore && head != mod+1) || (R5S[mod] > noneScore && (head-mod>5)) ) 
		filtered = 1;
      else
		// Look for a root between them
		for (int i = mod + 1; i < head && !filtered; i++)
		  if (possibleRootBool[i] && rootS[i] > noneScore)
			filtered = 1;
      if (filtered == 0) headList.push_back(head);
    }
	if (!runTiming) {
	  // Now, for each mod, add on its possible heads:
	  if (mod > 1) possiblePairs += "\t";
	  // possiblePairs += fastInt2Str(mod) + ":" + fastInt2Str(headList[0]);  Ditch the index -- not needed
	  if (!headList.empty()) possiblePairs += fastInt2Str(headList[0]);
	  for (int i=1; i<(int)(headList.size()); i++) possiblePairs += "," + fastInt2Str(headList[i]);
	}
  }
  if (!runTiming) std::cout << possiblePairs << std::endl;

}

////////////////////////////////////////////////
////////////////////////////////////////////////
// Run program
////////////////////////////////////////////////
////////////////////////////////////////////////
int main(int nargin, char** argv) {
  if (nargin != 3) {
    std::cerr << USAGE << std::endl;
    exit(-1);
  }

  ////////////////////////////////////////////////
  // First, load the weight vectors:
  ////////////////////////////////////////////////
  LinearWeightsMap linWeights;
  initializeLinearWeights(argv[1], linWeights);

  UltraPairWeightsMap pairWeights;
  initializeUltraPairWeights(argv[2], pairWeights);

  ////////////////////////////////////////////////
  // Get the bias of the pair and none filters:
  ////////////////////////////////////////////////
  UltraPairWeightsMap::const_iterator finder = pairWeights.find("bias");
  if (finder == pairWeights.end()) {
    std::cerr << "Error: no bias feature for the pair/none filters." << std::endl;
    exit(-1);
  }
  float noneBias = finder->second[0];
  float pairBias = finder->second[1];

  // Start timing of program
  clock_t startTime = clock();

  ////////////////////////////////////////////////
  // Next, go through each line (sentence) of thie input:
  ////////////////////////////////////////////////
  std::string input;
  while (getline(std::cin, input, '\n')) {
    // Preprocess the lines
    normLines(input);
    ////////////////////////////////////////////////
    // Read the line into the word and tag arrays:
    ////////////////////////////////////////////////
    StrVec words;	StrVec tags;
    // Parse this line with a string stream:
    std::stringstream line(input);
    // For each element (word+tag+head) in this line:
    std::string element;
    while (getline(line, element, ' ')) {
      std::stringstream triple(element);
      std::string word, tag;
      getline(triple, word, '_');
      normWords(word);
      words.push_back(word);
      getline(triple, tag, '_');
      tags.push_back(tag);
    }

    ////////////////////////////////////////////////
    // Apply filters and output decisions:
    ////////////////////////////////////////////////
    applyFilters(noneBias, pairBias, words, tags, linWeights, pairWeights);
  }

  // Report timing
  clock_t endTime = clock(); //record time that predicting ends
  float time_task = ((double)(endTime - startTime)) / CLOCKS_PER_SEC;    //compute elapsed time of task
  std::cerr << time_task << " seconds for filtering" << std::endl;

  return 1;
}

