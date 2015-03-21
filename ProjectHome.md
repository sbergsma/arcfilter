# Code for "Fast and Accurate Arc Filtering for Dependency Parsing" #

This code provides tools related to our COLING 2010 and ACL 2011 papers on fast and accurate arc filtering to speed up dependency parsing.  See the papers for an explanation of the different filters.

## Paper ##

If you use this material in your work, please cite as:

  * **Shane Bergsma and Colin Cherry,**<a href='http://www.clsp.jhu.edu/~sbergsma/Pubs/bergsmaCherry_ArcFilter_COLING2010.pdf'>Fast and Accurate Arc Filtering for Dependency Parsing</a>, In Proc. COLING 2010, Beijing, China, August 2010.

and/or

  * **Colin Cherry and Shane Bergsma,**<a href='http://www.clsp.jhu.edu/~sbergsma/Pubs/cherryBergsma_JointFilt_ACL2011.pdf'>Joint Training of Dependency Parsing Filters through Latent Support Vector Machines</a>, In Proc. ACL 2011 Short Papers, Portland, Oregon, July 2011.

depending on which aspects of the material you are using.

## Code and Models ##

You can download the project with SVN using:
`svn checkout http://arcfilter.googlecode.com/svn/trunk/ arcfilter-read-only`

This repository includes a small sample file called sampleInput.dat.

Compile the code by typing make at the command line.

## Input ##
To create the input, provide sentences as underscore-separated triples of POS-tag, word, and head. Note the program doesn't use the head (it can be replaced by a dummy char, as in sampleInput.dat), but it is included for evaluation convenience. Include a special root node that says `ROOT_ROOT_-1`:

`ROOT_ROOT_-1 Speculators_NNS_2 are_VBP_0 calling_VBG_2 for_IN_3 a_DT_6 degree_NN_4 of_IN_6 liquidity_NN_7 that_WDT_6 is_VBZ_9 not_RB_10 there_RB_10 in_IN_10 the_DT_15 market_NN_13 ._._2 ''_''_2`

## Output ##
The classifier produces output as a list of possible heads for each mod, starting at 1 (the first word) and using zero for words that can have the ROOT node as head:

`1:2,3,4,5 2:3,4,5 3:0,2,4,5 4:5 5:0,2,3,4 6:2,3,4,5`

## Sample Operation ##

The simplest kind of filtering is rule filtering:

`./ruleFilter < sampleInput.dat`

For the classifier-based filters, there are currently two sets of models: ones trained using L2 regularization and ones trained using L1 regularization. The L1 ones are smaller models and run faster, but with a slightly lower amount of filtering.

For L2 operation, run:

`./linearFilter linWeights.L2 < sampleInput.dat`

`./quadFilter linWeights.L2 quadWeights.L2 < sampleInput.dat`

For L1 operation, run:

`./linearFilter linWeights.L1 < sampleInput.dat`

`./quadFilter linWeights.L1 quadWeights.L1 < sampleInput.dat`

There is also now a faster, more accurate, hybrid linear/rule filter based on joint training (described in the ACL paper).  We call this improved filter the "ultraFilter:"

`./ultraFilter ultraLinWeights ultraPairWeights < sampleInput.dat`

## Contact ##

Please send an e-mail to <a href='http://www.clsp.jhu.edu/~sbergsma/'>Shane Bergsma</a> at sbergsma@jhu.edu if you use this material. We'd also be happy to help if you need any assistance.

Shane Bergsma

Original: June 11, 2010

Updated: April 16, 2011