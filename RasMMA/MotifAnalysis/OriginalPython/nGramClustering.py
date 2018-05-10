
# coding: utf-8

# In[2]:

from CollectForestInfo import CollectForestInfo
from apiNgramCreator import FamilyTree
import RasMMA as rasmma

import pickle


# In[1]:


picklePath = "../output/3family/1/"
nGramSize = 20


# In[3]:



forestDict = dict()

import os

for rootDir, dirs, files in os.walk(picklePath):
    if(files):
        dirName = (os.path.split(rootDir)[-1]) 
        familyName = dirName
        
        intermediatePicklePath, residualPicklePath = files
        
        forestInfo = CollectForestInfo(rootDir+'/'+intermediatePicklePath,
                                       rootDir+'/'+residualPicklePath,
                                       True) # one pickle is a forest
        groupMotif_dict = forestInfo.getGroupMotif_dict()
        notLonerList = forestInfo.getTreeList()

        for notLonerGroup in notLonerList:
            treeIndex = notLonerGroup[0]
            group_motifs = groupMotif_dict[treeIndex]

            apiSequence = list()
            for motif in group_motifs:
                apiSequence.extend(motif[0])

            if(len(apiSequence) < nGramSize):
                print("APISequenceLength Smaller Than Window:", familyName, treeIndex)
                continue
            tree = FamilyTree(familyName, treeIndex, apiSequence, nGramSize)
            forestDict[familyName + '_' + treeIndex] = tree


# In[7]:


featuredApiGrams_dict = dict()

gramCounter = 0 # index of total NGrams
for tName, tree in forestDict.items():
    for gramIdx in range(tree.getNGramListLen()):
        featureApiGram = tree.getNGramList()[gramIdx]
        clusterName = 'G' + str(gramCounter) # naming each initial clusters
        clusterMembers = {tName + '_gram' + str(gramIdx)}
        R = (clusterName, [(featureApiGram, 0, len(featureApiGram)-1)] )
        featuredApiGrams_dict[gramCounter] = (R, clusterMembers)
        gramCounter += 1


# In[8]:


len(featuredApiGrams_dict)


# In[10]:


tag = "3family1_main_20gram"
outputPath = "output/RasMMA/6family/"
out_pickleDir = outputPath + "pickle/"

thresholdValue = 0.8

import os
if not os.path.isdir(outputPath): os.makedirs(outputPath)
if not os.path.isdir(out_pickleDir): os.makedirs(out_pickleDir)


# In[11]:


from datetime import datetime, timedelta


if len(featuredApiGrams_dict.keys()) > 1: # at least two trees
    
    date_time1 = datetime.now()

    intermediatePool, initialDict, roundInfos, residualMessage = rasmma.clusterInitializedReps(featuredApiGrams_dict, tag, outputPath, thresholdValue)

    date_time2 = datetime.now()
    datetime = date_time2
    print("Time spending: ",date_time)
    
    # saving intermediatePool as pickle file
    with open(out_pickleDir + tag + '_intermediate.pickle', 'wb') as handle:
        pickle.dump(intermediatePool, handle, protocol=pickle.HIGHEST_PROTOCOL)

    # saving initialNames dict as pickle file
    with open(out_pickleDir + tag + '_initialDict.pickle', 'wb') as handle:
        pickle.dump(initialDict, handle, protocol=pickle.HIGHEST_PROTOCOL)

    # saving round information dict as pickle file
    with open(out_pickleDir + tag + '_roundInfos.pickle', 'wb') as handle:
        pickle.dump(roundInfos, handle, protocol=pickle.HIGHEST_PROTOCOL)

    if(residualMessage is not None): # this shouldn't occur when thresholdValue is 0
        with open(out_pickleDir + tag + '_residual.pickle', 'wb') as handle:
            pickle.dump(residualMessage, handle, protocol=pickle.HIGHEST_PROTOCOL)
else:
    print("no residual tree candidates.")
