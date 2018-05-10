# MotifAnalysis

1. The process of constructing an hierarchical clustering tree is:

* Hooklog3 # construct the basic data structure of a hooklog (from a file to a python list)
* distanceMatrixJaccard # calculate the distance matrix of n hooklogs by using Jaccard distance method
* UPGMA # draw the hierarchical clustering tree based on the distance matrix
* groupingZ.ipynb # group the hooklogs the result of UPGMA (a data structure named Z)

Each individual program has a "function"Example.ipynb to demostrate the usage of the class or function.
AllExample.ipynb is an aggregation of all above examples.

* hooklogStatisticsExample.ipynb shows some statistics after you obtain Z.


2. /Alignment

StageMatrix.ipynb is the entry point of the alignment process. Change the testDir in the prrgram. Note that the Featurehooklog.ipynb used in the program is a little bi different from Featurehooklog3.ipynb (and it will be merged later).
