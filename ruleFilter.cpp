/******************************************
 * 
 * ruleFilter.cpp
 *
 * Shane Bergsma
 * June 3, 2010
 *
 ******************************************/

#include <bitset>     // For filtering decisions

#include <iostream>   // For reading/writing STDIN
#include <sstream>    // For parsing the input
#include <time.h>     // For timing:

#include "filterCommon.h"

const std::string USAGE = "USAGE: cat taggedFile | ./ruleFilter";

// Apply the rule filters as appropriate to limit the decisions made
// by the linear filter values:
void applyFilters(const RuleLists& tabooHeads, const RuleLists& noLeftHead, const RuleLists& noRightHead, const RuleLists& tabooPairs,
				  const StrVec &words, const StrVec &tags
				  ) {

  // Store the other filter decisions here:  All bits are initially zero.
  FilterVals headF;
  FilterVals LxF, L1F, L5F;
  FilterVals RxF, R1F, R5F;

  int sentSize = tags.size();

  // Go through each word: 
  for (int i=1; i<sentSize; i++) {
    // Heads:
    if (tabooHeads.find(tags[i]) != tabooHeads.end())
	  headF.set(i, 1);
    // Left-filtering:
    if (noLeftHead.find(tags[i]) != noLeftHead.end())
      LxF.set(i, 1);
    // Right-filtering:
    if (noRightHead.find(tags[i]) != noRightHead.end())
      RxF.set(i, 1);
  }

  // Now write the output decisions:
  std::string possiblePairs = "";
  // An example for every modifier but the root:
  for (int mod = 1; mod < sentSize; mod++) {
    // the list of heads for mod-i:
    std::vector<int> headList;
    // Go through all the possible heads:
    for (int head = 0; head < sentSize; head++) {
	  // Sort these roughly by how often they should apply:	  
      if (mod == head) continue;  // Words can't link to themselves:
      if (head != 0 && headF.test(head)) continue; // The root is always a head
      if (LxF.test(mod) && head < mod) continue; // Roots are on the left...
      if (RxF.test(mod) && head > mod) continue;
      if (L1F.test(mod) && head != mod-1) continue;
      if (R1F.test(mod) && head != mod+1) continue;
      if (L5F.test(mod) && (head > mod || (mod-head>5))) continue;
      if (R5F.test(mod) && (head < mod || (head-mod>5))) continue;
	  // Finally, the pair rules:
      std::string pairStr;
      if (mod > head) {
		pairStr = "h" + tags[head] + "<m" + tags[mod];
      } else {
		pairStr = "m" + tags[mod] + "<h" + tags[head];
      }
      if (tabooPairs.find(pairStr) == tabooPairs.end()) {
		// If we don't filter anything, put this on as an option:
		headList.push_back(head);
	  }
    }
    // Now, for each mod, add on its possible heads:
    if (headList.empty() == 0) {
      if (possiblePairs != "") possiblePairs += "\t";
      possiblePairs += fastInt2Str(mod) + ":" + fastInt2Str(headList[0]);
      for (int i=1; i<(int)(headList.size()); i++) {
		possiblePairs += "," + fastInt2Str(headList[i]);
      }
    }
  }
  std::cout << possiblePairs << std::endl;
}


////////////////////////////////////////////////
////////////////////////////////////////////////
// Run program
////////////////////////////////////////////////
////////////////////////////////////////////////
int main(int nargin, char** argv) {
  if (nargin != 1) {
    std::cerr << USAGE << std::endl;
	exit(-1);
  }

  ////////////////////////////////////////////////
  // First, load the simple rule lists:
  ////////////////////////////////////////////////
  RuleLists tabooHeads;  RuleLists noLeftHead;  RuleLists noRightHead;  RuleLists tabooPairs;
  initializeTaboos(tabooHeads, noLeftHead, noRightHead, tabooPairs);

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
				 words, tags);
  }

  // Report timing
  clock_t endTime = clock(); //record time that predicting ends
  float time_task = ((double)(endTime - startTime)) / CLOCKS_PER_SEC;    //compute elapsed time of task
  std::cerr << time_task << " seconds for filtering" << std::endl;

  return 1;
}

