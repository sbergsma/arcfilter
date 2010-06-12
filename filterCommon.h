/******************************************
 * 
 * filterCommon.h
 *
 * Shane Bergsma
 * June 3, 2010
 *
 ******************************************/

#ifndef FILTERCOMMON_H
#define FILTERCOMMON_H

#include <tr1/unordered_map>   // For storing the weights
#include <tr1/unordered_set>   // For storing the taboo lists
#include <string>
#include <vector>
#include <bitset>     // For filtering decisions, FilterVals type

const int MAXSENTSIZE = 999;  // For the efficient bitvector, and for int2str

typedef std::tr1::unordered_set<std::string> RuleLists;
typedef std::vector<std::string> StrVec;
typedef std::bitset<MAXSENTSIZE> FilterVals;

typedef std::vector<bool> eightB;

typedef std::vector<float> eightW;
typedef std::tr1::unordered_map<std::string,eightW> LinearWeightsMap;

typedef std::tr1::unordered_map<std::string,float> QuadWeightMap;
typedef std::vector< std::pair<std::string,float> > RealFeats;

// Load in all the rules for simple filtering of arcs
void initializeTaboos(RuleLists& tabooHeads, RuleLists& noLeftHead, RuleLists& noRightHead, RuleLists& tabooPairs);

// Quickly turn an integer into a string: For efficiency: Use fact we
// never have a distance or index > 999
inline std::string fastInt2Str(int d) {
  static char buf[3];
  sprintf(buf, "%d", d);
  return buf;
}

// Preprocess the input lines as I did when training:
inline void normLines(std::string &input) {
  for (std::string::iterator cItr=input.begin(); cItr != input.end(); cItr++) {
	if (*cItr == '#')
	  *cItr = '|'; // '#' means comments elsewhere
	else if (*cItr == ':')
	  *cItr = ';'; // ':' separates features and weights elsewhere
  }
}

// Preprocess the input words to convert digits to 0
inline void normWords(std::string &input) {
  for (std::string::iterator cItr=input.begin(); cItr != input.end(); cItr++)
	if (isdigit(*cItr))
	  *cItr = '0';
}

inline void safeNeighbourDoubleGet(int nbh, const StrVec &words, const StrVec &tags, int sentSize,
								   std::string *wordNbh, std::string *tagNbh) {
  if (nbh > 0 && nbh < sentSize) {
	*wordNbh = words[nbh];
	*tagNbh = tags[nbh];
	return;
  } 
  //else { // if (nbh < 1 || nbh >= words.size()) {
  *wordNbh = "~";
  *tagNbh = "~";
  //  }
}

inline void safeNeighbourTagGet(int nbh, const StrVec &tags, int sentSize,
								std::string *tagNbh) {
  if (nbh > 0 && nbh < sentSize) {
	*tagNbh = tags[nbh];
	return;
  }// else { // if (nbh < 1 || nbh >= tags.size()) {
  *tagNbh = "~";
}

// Quantize the distance into several ranges, currently used for
// head-mod links and mod-root links.
inline std::string binDistance(int d);

// Build a feature vector given the current words and tags:
void buildLinearFeatureVector(int pos, const StrVec &words, const StrVec &tags, int sentSize, StrVec &feats);

// Build a feature vector given a pair of words and tags
void buildQuadraticFeatureVector(int h, int m, const StrVec &words, const StrVec &tags, int sentSize, 
								 const std::vector<float> &logPrecomputes, StrVec &binfeats, RealFeats &realfeats);

// Get the 0/1 predictions for each filter
void getLinearFilterPredictions(const LinearWeightsMap &linWeights, const StrVec &feats, eightB &preds);

// Get the 0/1 prediction for the quadratic
bool getQuadraticFilterPredictions(const QuadWeightMap &quadWeights, const StrVec &binFeats, const RealFeats &realFeats);

// Load the weight matrix from file:
void initializeLinearWeights(char *filename, LinearWeightsMap &linWeights);

// Load the weight vector from file:
void initializeQuadWeights(char *filename, QuadWeightMap &quadWeights);

#endif // FILTERCOMMON_H
