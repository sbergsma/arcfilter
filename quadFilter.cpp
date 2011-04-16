/******************************************
 * 
 * quadFilter.cpp
 *
 * Shane Bergsma
 * June 7, 2010
 *
 ******************************************/

#include <time.h>     // For timing:
#include <math.h>     // For floor and log

#include <iostream>   // For reading/writing STDIN
#include <sstream>    // For parsing the input


#include "filterCommon.h"

const std::string USAGE = "USAGE: cat taggedFile | ./quadFilter linearWeights quadWeights";

// Apply the rule filters as appropriate to limit the decisions made
// by the linear filter values, then apply the quad to the stragglers.
void applyFilters(const RuleLists& tabooHeads, const RuleLists& noLeftHead, const RuleLists& noRightHead, const RuleLists& tabooPairs,
				  const StrVec &words, const StrVec &tags, const LinearWeightsMap &linWeights, const QuadWeightMap &quadWeights, const std::vector<float> &logPrecomputes) {
  std::vector<int> rootIndices;  // Store any root indices here:
  FilterVals headF, LxF, L1F, L5F, RxF, R1F, R5F;  // Store the other filter decisions here:
  int sentSize = tags.size();
  if (sentSize > MAXSENTSIZE) {
	std::cerr << "Error: exceeding maximum sentence size\n" << std::endl;
	std::cout << "\n" << std::endl;	// For now, just don't produce any output and move to next one:
	return;
  }

  // Predetermine which are heads, roots, and while we're at it, the left-right mods:
  for (int i=1; i<sentSize; i++) {  // Go through each word: 
	// Create Feature Vector
	StrVec linFeats;
	buildLinearFeatureVector(i, words, tags, sentSize, linFeats);
	// Get the filter predictions
	eightB preds;
	getLinearFilterPredictions(linWeights, linFeats, preds);
	// Use Predictions in conjunction with the Rules
    if (tabooHeads.find(tags[i]) != tabooHeads.end()) {    // Heads:
	  headF.set(i, 1);
	} else {
	  headF.set(i, preds[0]);
	}
	if (preds[1]) rootIndices.push_back(i);	// Roots:
    if (noLeftHead.find(tags[i]) != noLeftHead.end()) {    // Left-filtering:
      LxF.set(i, 1);
    } else {
	  L1F.set(i, preds[3]);
	  L5F.set(i, preds[4]);
    }
    if (noRightHead.find(tags[i]) != noRightHead.end()) {    // Right-filtering:
      RxF.set(i, 1);
    } else {
	  R1F.set(i, preds[6]);
	  R5F.set(i, preds[7]);
	  // If the rules said nothing about neither no-left nor no-right heads:
	  if (noLeftHead.find(tags[i]) == noLeftHead.end()) { // -- already true: && noRightHead.find(tags[i]) == noRightHead.end()) {
		RxF.set(i, preds[5]);
		LxF.set(i, preds[2]);
	  }
    }
  }

  // Now find and write the arc possibilities for each mod:
  std::string possiblePairs = "";
  for (int mod = 1; mod < sentSize; mod++) {  // An example for every modifier but the root:
    std::vector<int> headList;    // the list of heads for mod-i:
    for (int head = 0; head < sentSize; head++) {    // Go through all the possible heads:
	  // Sort these roughly by how often they should apply:	  
      if (mod == head) continue;  // Words can't link to themselves:
      if (head != 0 && headF.test(head)) continue; // The root is always a head
      if (LxF.test(mod) && head < mod) continue; // Roots are on the left...
      if (RxF.test(mod) && head > mod) continue;
      if (L1F.test(mod) && head != mod-1) continue;
      if (R1F.test(mod) && head != mod+1) continue;
      if (L5F.test(mod) && (head > mod || (mod-head>5))) continue;
      if (R5F.test(mod) && (head < mod || (head-mod>5))) continue;
      // The root-filter is most interesting.  It affects things in two ways:
      // a) in a projective parser, things can't cross it:
      bool isARoot = false;
	  bool doFilter = false;
      for (std::vector<int>::iterator rItr = rootIndices.begin(); rItr != rootIndices.end(); rItr++) {
	if ((head < *rItr && *rItr < mod) ||
	    (mod < *rItr && *rItr < head))
		  doFilter = true;
		if (*rItr == mod) isARoot = true;		// Also, check if this mod is on the list of roots:
      }
	  if (doFilter) continue;
      //  b) if we've picked out a root, no one else can be the root:
      if (head == 0 && // We are consiering whether it's this guy:
		  !rootIndices.empty() && // and there is definitely a root somewhere
		  !isARoot) {  // but it's not this guy
		continue;
      }
      std::string pairStr;	  // Finally, the pair rules:
	  if (mod > head)
		pairStr = "h" + tags[head] + "<m" + tags[mod];
	  else
		pairStr = "m" + tags[mod] + "<h" + tags[head];
      if (tabooPairs.find(pairStr) != tabooPairs.end())
		continue;

	  // If you made it this far, it's time to build and use the quadratic filter:
	  StrVec binaryQuadFeats;
	  RealFeats realQuadFeats;
	  buildQuadraticFeatureVector(head, mod, words, tags, sentSize, logPrecomputes, binaryQuadFeats, realQuadFeats);
	  bool qpred = getQuadraticFilterPredictions(quadWeights, binaryQuadFeats, realQuadFeats);
	  if (qpred)
		headList.push_back(head); 	// If we don't filter anything, put this on as an option
    }
	if (mod > 1) possiblePairs += "\t";
	if (!headList.empty()) possiblePairs += fastInt2Str(headList[0]);
	for (int i=1; i<(int)(headList.size()); i++) possiblePairs += "," + fastInt2Str(headList[i]); // int to avoid warns
  }
  std::cout << possiblePairs << std::endl;
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
  // First, load the simple rule lists:
  ////////////////////////////////////////////////
  RuleLists tabooHeads;  RuleLists noLeftHead;  RuleLists noRightHead;  RuleLists tabooPairs;
  initializeTaboos(tabooHeads, noLeftHead, noRightHead, tabooPairs);

  ////////////////////////////////////////////////
  // Then, load the linear weight vectors:
  ////////////////////////////////////////////////
  LinearWeightsMap linWeights;
  initializeLinearWeights(argv[1], linWeights);

  ////////////////////////////////////////////////
  // Then, load the quad weight vector:
  ////////////////////////////////////////////////
  QuadWeightMap quadWeights;
  initializeQuadWeights(argv[2], quadWeights);

  // Also, to save time, precompute log values up to 200 for direct addressing:
  std::vector<float> logPrecomputes;
  logPrecomputes.push_back(0);
  for (int i=1; i<=100; i++) {
	// Take it to the same number of sigdigs as you used in training:
	float roundedFloat = floor(log(i+1) * 1000 + .5) / 1000;
	logPrecomputes.push_back(roundedFloat);
  }
  
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
    applyFilters(tabooHeads, noLeftHead, noRightHead, tabooPairs,
				 words, tags, linWeights, quadWeights, logPrecomputes);
  }

  // Report timing
  clock_t endTime = clock(); //record time that predicting ends
  float time_task = ((double)(endTime - startTime)) / CLOCKS_PER_SEC;    //compute elapsed time of task
  std::cerr << time_task << " seconds for filtering" << std::endl;

  return 1;
}

