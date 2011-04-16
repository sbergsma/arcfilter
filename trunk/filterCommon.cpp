/******************************************
 * 
 * filterCommon.cpp
 *
 * Shane Bergsma
 * June 3, 2010
 *
 ******************************************/

#include "filterCommon.h"

#include <cassert>    // For error checking
#include <iostream>   // For reading/writing STDIN
#include <fstream>    // For reading files
#include <iterator>   // For debugging

const int MAXDIST = 5;       // For the span of the neighbour tag inclusion in linear
const int WIDTH = 5;         // For the scope of between-tag finding in quadratic

// Quantize the distance into several ranges, currently used for
// head-mod links and mod-root links.
inline std::string binDistance(int d) {
  if (d < 15 && d > -15) {
    return fastInt2Str(d);
  } else if (d < 25 && d > -25) {
	d = d/2;
    return fastInt2Str(d * 2);
  } else if (d >= 25) {
    return ">25";
  } else if (d <= -25) {
    return "<-25";
  }
  // Should be unreachable:
  assert(0);
  return "ERROR";
}

// Load in all the rules for simple filtering of arcs
void initializeTaboos(RuleLists& tabooHeads, RuleLists& noLeftHead, RuleLists& noRightHead, RuleLists& tabooPairs) {
  // unlikely heads:
  tabooHeads.insert("''");  tabooHeads.insert(","); tabooHeads.insert("."); 
  tabooHeads.insert(";"); tabooHeads.insert("CC"); tabooHeads.insert("PRP$"); 
  tabooHeads.insert("PRP"); tabooHeads.insert("``"); tabooHeads.insert("-RRB-"); 
  tabooHeads.insert("-LRB-"); tabooHeads.insert("EX"); tabooHeads.insert("|");
  // definite left-right constraints:
  noLeftHead.insert("EX"); noLeftHead.insert("LS"); noLeftHead.insert("POS"); noLeftHead.insert("PRP$");
  noRightHead.insert("."); noRightHead.insert("RP");
  // unlikely pairs:
  tabooPairs.insert("hCD<mCD"); tabooPairs.insert("hROOT<m,"); tabooPairs.insert("mIN<hJJ"); 
  tabooPairs.insert("mJJ<hDT"); tabooPairs.insert("hNNP<mNNS"); tabooPairs.insert("hDT<mJJ"); 
  tabooPairs.insert("mDT<hDT"); tabooPairs.insert("hDT<m."); tabooPairs.insert("hDT<mNNS"); 
  tabooPairs.insert("hROOT<mDT"); tabooPairs.insert("hNN<mDT"); tabooPairs.insert("hNN<mNNP"); 
  tabooPairs.insert("hDT<mDT"); tabooPairs.insert("mNNP<hDT"); tabooPairs.insert("hDT<mNN"); 
  tabooPairs.insert("hDT<mNNP"); tabooPairs.insert("hNNP<mDT"); tabooPairs.insert("hNNP<mNN"); 
  tabooPairs.insert("mIN<hDT"); tabooPairs.insert("mNN<hDT"); tabooPairs.insert("mNNP<hIN");
}

// Build a feature vector given the current words and tags:
void buildLinearFeatureVector(int pos, const StrVec &words, const StrVec &tags, int sentSize, StrVec &feats) {
  // Get all the relevant information:
  std::string wh = words[pos];
  std::string th = tags[pos];
  std::string whl, thl, whr, thr, thll, thrr;
  safeNeighbourDoubleGet(pos-1, words, tags, sentSize, &whl, &thl);
  safeNeighbourDoubleGet(pos+1, words, tags, sentSize, &whr, &thr);
  safeNeighbourTagGet(pos-2, tags, sentSize, &thll);
  safeNeighbourTagGet(pos+2, tags, sentSize, &thrr);

  // First: get the prefix and suffix, if possible:
  std::string pref = "";
  std::string suff = "";
  int whLen = wh.size();
  if (whLen > 2) {
	suff = wh.substr(whLen-2);
	if (whLen > 4) {
	  pref = wh.substr(0, 4);
	}
  }

  // Then, the shape of the word:
  std::string tmp(wh);
  for (std::string::iterator cItr=tmp.begin(); cItr != tmp.end(); cItr++) {
	if (isupper(*cItr))
	  *cItr = 'A';
	else if (islower(*cItr))
	  *cItr = 'a';
  }
  // This is just a unique:
  std::string wordShape = ""; 
  wordShape += tmp[0];
  for (int j=1; j<(int)(tmp.size()); j++) {
	if (tmp[j] != tmp[j-1] || 
		(tmp[j] != 'A' && tmp[j] != 'a')
		)
	  wordShape += tmp[j];
  }
  if (wordShape.size() > 5) {
	wordShape = wordShape.substr(0, 5);
  }
  
  // I deem these things to be worth conjoining with everyone:
  // H$wh, h$th, <$pref, >$suff, //$wordShape:

  // First, handle their conjunctions with each other, where not every
  // possibility is needed, i.e., conjoining the prefix and word itself
  // will buy you nothing.

  std::string featstr;

  // First, do the prefix:
  if (pref != "") {
	std::string featstrP = "{" + pref; feats.push_back(featstrP);
	featstr = featstrP + "^h" + th; feats.push_back(featstr);
	featstr = featstrP + "^>" + suff; feats.push_back(featstr);
	featstr = featstrP + "^#" + wordShape; feats.push_back(featstr);
  }

  // Then the suffix:
  if (suff != "") {
	std::string featstrS = "}" + suff; feats.push_back(featstrS);
	featstr = featstrS + "^h" + th; feats.push_back(featstr);
	featstr = featstrS + "^#" + wordShape; feats.push_back(featstr);
  }

  // Then the shape:
  featstr = "#" + wordShape;  feats.push_back(featstr);
  featstr += "^h" + th;  feats.push_back(featstr);

  // The word itself:
  featstr = "H" + wh;  feats.push_back(featstr);
  featstr += "^t" + th;  feats.push_back(featstr);

  // The tag itself:
  featstr = "h" + th;  feats.push_back(featstr);

  // Now, do their conjunctions with everything else:
  StrVec conjoinFeats;
  featstr = "H" + wh; conjoinFeats.push_back(featstr);
  featstr = "h" + th; conjoinFeats.push_back(featstr);
  featstr = "<" + pref; conjoinFeats.push_back(featstr);
  featstr = ">" + suff; conjoinFeats.push_back(featstr);
  featstr = "#" + wordShape; conjoinFeats.push_back(featstr);

  StrVec atomicFeats;
  // Little conjunctions on sentence size, position:
  featstr = "P" + fastInt2Str(pos); atomicFeats.push_back(featstr);
  featstr = "P" + fastInt2Str(pos) + "^S" + fastInt2Str(sentSize); atomicFeats.push_back(featstr);

  // From the end:
  int reverseDist = sentSize - pos;
  std::string reverseHeadPos = binDistance(reverseDist);
  //  std::cout << reverseDist << " = " << sentSize << " - " << pos << " = " << reverseHeadPos << std::endl;
  
  featstr = "V" + fastInt2Str(reverseDist); atomicFeats.push_back(featstr);
  featstr = "v" + reverseHeadPos; atomicFeats.push_back(featstr);

  // New: all the tags on the left/right.
  std::tr1::unordered_set<std::string> nbhTags;

  int startSpot = 1;
  if (pos - MAXDIST > 1) startSpot = pos - MAXDIST;
  for (int currSpot=startSpot; currSpot < pos; currSpot++) {
	featstr = "L" + tags[currSpot]; nbhTags.insert(featstr);
	int tagDist = pos-currSpot;
	featstr += "." + fastInt2Str(tagDist); nbhTags.insert(featstr);
  }
  int endSpot = sentSize;
  if (pos + MAXDIST + 1 < sentSize) endSpot = pos + MAXDIST + 1;
  for (int currSpot=pos+1; currSpot < endSpot; currSpot++) {
	featstr = "R" + tags[currSpot]; nbhTags.insert(featstr);
	int tagDist = currSpot-pos;
	featstr += "." + fastInt2Str(tagDist); nbhTags.insert(featstr);
  }
  for (std::tr1::unordered_set<std::string>::const_iterator itr = nbhTags.begin(); itr != nbhTags.end(); ++itr) {
	atomicFeats.push_back(*itr);
  }

  featstr = "G" + whl; atomicFeats.push_back(featstr);
  featstr = "I" + whr; atomicFeats.push_back(featstr);
  featstr = "h" + th + ".g" + thl; atomicFeats.push_back(featstr);
  featstr = "h" + th + ".i" + thr; atomicFeats.push_back(featstr);
  featstr = "g" + thl + ".i" + thr; atomicFeats.push_back(featstr);
  featstr = "f" + thll + ".g" + thl; atomicFeats.push_back(featstr);
  featstr = "i" + thr + ".j" + thrr; atomicFeats.push_back(featstr);

  // Now make all the feature conjunctions:
  for (StrVec::const_iterator aItr = atomicFeats.begin(); aItr != atomicFeats.end(); aItr++) {
    feats.push_back(*aItr);
	for (StrVec::const_iterator cItr = conjoinFeats.begin(); cItr != conjoinFeats.end(); cItr++) {
      featstr = *aItr + *cItr; feats.push_back(featstr);
    }
  }

  // And the bias term:
  feats.push_back("bias");
}

// Build a feature vector given a pair of words and tags
void buildQuadraticFeatureVector(int h, int m, const StrVec &words, const StrVec &tags, int sentSize, 
								 const std::vector<float> &logPrecomputes, StrVec &binFeats, RealFeats &realFeats) {
  // These get used in multiple places:
  std::string wh = words[h];  std::string wm = words[m];
  std::string th = tags[h];   std::string tm = tags[m];
  std::string thl, tml, thr, tmr;
  safeNeighbourTagGet(h-1, tags, sentSize, &thl);  safeNeighbourTagGet(h+1, tags, sentSize, &thr);
  safeNeighbourTagGet(m-1, tags, sentSize, &tml);  safeNeighbourTagGet(m+1, tags, sentSize, &tmr);

  int distance;  std::string featStr;
  std::string direction;  // Direction: like a backed-off distance
  if (m < h) {
	direction = "<";
	distance = (h-m);
  } else {
	direction = ">";
	distance = (m-h);
  }
  binFeats.push_back(direction);
  featStr = "D" + direction; realFeats.push_back( std::pair<std::string,float>(featStr,logPrecomputes[distance]) );
    // Distance tied to tags:
  featStr = th + tm; realFeats.push_back( std::pair<std::string,float>(featStr,logPrecomputes[distance]) );
  // words and tags and direction
  featStr = th + "~" + wm + direction;  binFeats.push_back(featStr);
  featStr = wh + "*" + tm + direction;  binFeats.push_back(featStr);
  std::string keyTriple = th + tm + direction;  binFeats.push_back(keyTriple);
  // mod tags
  featStr = "l" + tml + "." + keyTriple;  binFeats.push_back(featStr);
  featStr = "n" + tmr + "." + keyTriple;  binFeats.push_back(featStr);
  if (h != 0) {	// head ones:
	featStr = "g" + thl + "." + keyTriple;  binFeats.push_back(featStr);
	featStr = "i" + thr + "." + keyTriple;  binFeats.push_back(featStr);
	// Look for barriers:
	int start;
	int end;
	if (h < m) {
	  start = h;
	  end = m;
	} else {
	  start = m;
	  end = h;
	}
	// And tags within +-WIDTH of h and m, but less than m:
	std::tr1::unordered_map<std::string,int> btwTags;
	std::tr1::unordered_map<std::string,int> btwWords;
	for (int i=start+1; i<end && i<=start+WIDTH; i++) {
	  btwTags[tags[i]]++;
	  btwWords[words[i]]++;
	}
	for (int i=end-1; i>start && i>=end-WIDTH && i>start+WIDTH; i--) {
	  btwTags[tags[i]]++;
	  btwWords[words[i]]++;
	}
	for (std::tr1::unordered_map<std::string,int>::const_iterator itr = btwTags.begin(); itr != btwTags.end(); ++itr) {
	  realFeats.push_back( std::pair<std::string,float>(keyTriple + itr->first,logPrecomputes[itr->second]) );
	}
	for (std::tr1::unordered_map<std::string,int>::const_iterator itr = btwWords.begin(); itr != btwWords.end(); ++itr) {
	  realFeats.push_back( std::pair<std::string,float>(keyTriple + "!" + itr->first,logPrecomputes[itr->second]) );
	}
  }
  binFeats.push_back("bias");
}
  
// Get the 0/1 predictions for each filter
void getLinearFilterPredictions(const LinearWeightsMap &linWeights, const StrVec &feats, eightB &preds) {
  // Otherwise, build the eight scores for each filter type:
  float scores[8] = {0,0,0,0,0,0,0,0};  
  for (StrVec::const_iterator itr=feats.begin(); itr != feats.end(); itr++) {
	// Get the weights for each feature:
	LinearWeightsMap::const_iterator finder = linWeights.find(*itr);
	// If there are weights for this feature:
	if (finder != linWeights.end()) {
	  for (int i=0; i<8; i++) {
		scores[i] += finder->second[i];
	  }
	}
  }
  // And output the scores as 1/0 decisions:
  for (int i=0; i<8; i++) {
	preds.push_back(scores[i] > 0.000001);
  }
}
  
// Get the floating-point scores for each of the nine ultra filters:
void getUltraLinearFilterScores(const LinearWeightsMap &linWeights, const StrVec &feats, eightF &preds) {
  // Build the eight scores for each filter type:
  float scores[8] = {0,0,0,0,0,0,0,0};  
  for (StrVec::const_iterator itr=feats.begin(); itr != feats.end(); itr++) {
    // Get the weights for each feature:
    LinearWeightsMap::const_iterator finder = linWeights.find(*itr);
    // If there are weights for this feature:
    if (finder != linWeights.end()) {
      for (int i=0; i<8; i++) {
		scores[i] += finder->second[i];
      }
    }
  }
  // And output the scores as values:
  for (int i=0; i<8; i++) {
    preds.push_back(scores[i]);
  }
}

// Get the 0/1 prediction for the quadratic
bool getQuadraticFilterPredictions(const QuadWeightMap &quadWeights, const StrVec &binFeats, const RealFeats &realFeats) {
  float score = 0;
  for (StrVec::const_iterator itr=binFeats.begin(); itr != binFeats.end(); itr++) {
	// Get the weights for each feature:
	QuadWeightMap::const_iterator finder = quadWeights.find(*itr);
	// If there are weights for this feature:
	if (finder != quadWeights.end()) {
	  score += finder->second;
	}
  }
  for (RealFeats::const_iterator itr=realFeats.begin(); itr != realFeats.end(); itr++) {
	// Get the weights for each feature:
	QuadWeightMap::const_iterator finder = quadWeights.find(itr->first);
	// If there are weights for this feature:
	if (finder != quadWeights.end()) {
	  score += finder->second * itr->second;
	}
  }
  return (score > 0.00000001);
}

// Load the weight matrix from file:
void initializeLinearWeights(char *filename, LinearWeightsMap &linWeights) {
  std::cerr << "Loading linear weights ";
  // We pass zero for blank weight files, so load nothing:
  if (filename[0] == '0') return;

  // open the file:
  std::ifstream file(filename);
  if (!file) {
    std::cerr << "Error! Weight file " << filename << " can not be opened" << std::endl;
    exit(-1);
  }

  // parse each input line:
  std::string feat;
  while (file >> feat) {
    // Read in the eight weights for this feature
	eightF wtset;
    float wt;
    for (int i=0; i<8; i++) {
      file >> wt; wtset.push_back(wt);
    }
    // Add to the weight matrix:
    linWeights[feat] = wtset;
  }
  
  // close the file
  file.close();
  std::cerr << "> done" << std::endl;
}

// Load the ultra 2-d pair matrix from file:
void initializeUltraPairWeights(char *filename, UltraPairWeightsMap &pairWeights) {
  std::cerr << "Loading ultra-pair weights ";
  // We pass zero for blank weight files, so load nothing:
  if (filename[0] == '0') return;

  // open the file:
  std::ifstream file(filename);
  if (!file) {
    std::cerr << "Error! Weight file " << filename << " can not be opened" << std::endl;
    exit(-1);
  }

  // parse each input line:
  std::string feat;
  while (file >> feat) {
    // Read in the two weights for this feature
    twoW wtset;
    float wt;
    file >> wt; wtset.push_back(wt);
    file >> wt; wtset.push_back(wt);
    // Add to the weight matrix:
    pairWeights[feat] = wtset;
  }
  
  // close the file
  file.close();
  std::cerr << "> done" << std::endl;
}

// Load the weight vector from file:
void initializeQuadWeights(char *filename, QuadWeightMap &quadWeights) {
  std::cerr << "Loading quadratic weights ";
  // We pass zero for blank weight files, so load nothing:
  if (filename[0] == '0') return;

  // open the file:
  std::ifstream file(filename);
  if (!file) {
    std::cerr << "Error! Weight file " << filename << " can not be opened" << std::endl;
    exit(-1);
  }

  // parse each input line:
  std::string feat;
  while (file >> feat) {
	float wt;
	file >> wt;
    // Add to the weight matrix:
    quadWeights[feat] = wt;
  }
  
  // close the file
  file.close();
  std::cerr << "> done" << std::endl;
}
