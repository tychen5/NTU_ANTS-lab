# % run Alignment_Fast3.ipynb
# % run StructMatchGap3.ipynb
# % run StageMatrix.ipynb
# % run Motif.ipynb
# % run OutputStage.ipynb
# % run CommonMotifAnalysis_Tmp.ipynb

import Aligment_Fast3 as alignment
import StructMatchGap3
import StageMatrix
from Motif import Motif
from OutputStage import OutputStage
from CommonMotifAnalysis_Tmp import CommonMotif


# Doing global alignment and Calculate common motif.
# will return a common motif dict
def do_globalAlignment(rep1, rep2):
    # Aligment
    align_dict = dict()
    BASE = "rep1"
    align_dict['rep1'] = alignment.pairwise_NW( rep1, rep1, 2, -1, -3, 1)[2]
    align_dict['rep2'] = alignment.pairwise_NW( rep1, rep2, 2, -1, -3, 1)[2]
    
    # get 'Match Matrix' and 'Gap List'
    matchMatrix, gapSeqList = StructMatchGap3.structMatchGap(align_dict, BASE)
    stageMatrixResult = StageMatrix.stageMatrix(matchMatrix, gapSeqList)
    Motif_Obj = Motif(stageMatrixResult, BASE)
    outputStage = OutputStage(stageMatrixResult, None, BASE, Motif_Obj)
    
    executionTrace_dict = {"rep1":rep1, "rep2":rep2}
    
    commonMotif = CommonMotif(stageMatrixResult, Motif_Obj, executionTrace_dict, outputStage)
    
    # comMotifdict= {'s<stage>_<motif>': [CMS], oriIdxRange1, oriIdxRange2},
    comMotif_dict = commonMotif.getComMotifDict()  
    return comMotif_dict


from FeatureHooklog3 import FeatureHooklog3
# % run FeatureHooklog3.ipynb
#******************** the output toMergeCandidate_Dict have to change to set

# initialize all hooklogs as "to merge candidates clusters"
def initialCandidateDict(data_directory):
    
#     toMergeCandidate_List = list()
    toMergeCandidate_Dict = dict()
    
    # get feature hooklogs
    Hooklog = FeatureHooklog3
    hkName_list = list(filter(lambda f:f.endswith('.trace.hooklog'), os.listdir(data_directory))) # hooklog Name List
    hk_count = 0
    for hkName in hkName_list:
        featureHooklog = Hooklog(data_directory + hkName, 1).getHkli_noContainTS()
#         featureHooklog = [line.rstrip('\n') for line in open(data_directory + hkName)]
        clusterName = "G"+str(hk_count)
        # R = tuple( clusterName, list(  tuple(featureHooklog, fhStartIdx, fhEndIdx) ) ), the representative of cluster.
        R = (clusterName, [(featureHooklog, 0, len(featureHooklog)-1)] )
        clusterMembers = set()
        hkName = shortenHooklogName(hkName)
        clusterMembers.add(hkName)
        
        toMergeCandidate_Dict[hk_count] = (R, clusterMembers)
        
        hk_count+=1
        
    print("-- Finish Initializing --")
    return toMergeCandidate_Dict
#     return toMergeCandidateSet

# shorten Name to first 6 charactors
def shortenHooklogName(hkName):
    hashValue = hkName[0:6]
    pid = hkName.split("_")[1].split(".")[0]
    return hashValue+"_"+pid

# input: two R
# output: new RepresentativeR of inputs;
def get_Representative(Ri, Rj):
    rep1 = list()
    rep2 = list()

#     print(Ri[0], Rj[0])
    for i in range(len(Ri[1])): # get length of R's common motif seqs  (p.s. Ri[0] is clusterName)
        rep1 += Ri[1][i][0]
    for i in range(len(Rj[1])):
        rep2 += Rj[1][i][0]
    
    repNew = list() 
    
    if(rep1 and rep2):
        comMotif_dict = do_globalAlignment(rep1, rep2) # do Alignment
        newStartIdx = 0

        for m in sorted(comMotif_dict.keys(), key = lambda x : int(x.split('_')[0][1:])): # sorted by stages
            cmsList = comMotif_dict[m]
            newEndIdx = newStartIdx + len(cmsList[0]) - 1
            repNew.append((cmsList[0], newStartIdx, newEndIdx, cmsList[1], cmsList[2]))
                      # [CMS, newCMSStartIdx, newCMSEndIdx, oriIdxRange1, oriIdxRange2]
            newStartIdx = newEndIdx + 1
    rep1.clear()
    rep2.clear()
    del comMotif_dict
    return repNew

# return a dictionary that contains the initializing informations
#
# initialDict = {clusterName : (originalName, initialLength)}

def getInitialDict(toMergeCandidateDict):
    initialDict = dict()
    for key, value in toMergeCandidateDict.items():
        clusterName = value[0][0]
        initialLen = value[0][1][0][2] + 1
        originalName = value[1].pop()
        initialDict[clusterName] = (originalName, initialLen)
        value[1].add(originalName)
    return initialDict

# return a dict that contains only original name
# nameDict = {clusterName: original name}

def getInitialNameDict(initialDict):
    nameDict = dict()
    for key, value in initialDict.items():
        name = value[0]
        nameDict[key] = name
    return nameDict

import functools

# compute score of Rnew
# the score calculate method is the length ratio of new to origin one
def compute_Score(Ri, Rj, Rnew):
    
    if(Rnew[1]):
        L_Ri = functools.reduce(lambda x,y:x+y, [len(i[0]) for i in Ri[1]])
        L_Rj = functools.reduce(lambda x,y:x+y, [len(j[0]) for j in Rj[1]])
    
        Lorg = max(L_Ri, L_Rj)
        Lnew = functools.reduce(lambda x,y:x+y, [len(n[0]) for n in Rnew[1]]) 
        return float(Lnew)/Lorg
    else:
        return 0

# get score list of toMergeCandidateDict(single iteration) from highest to lowest

def findMergeCandidateScoreList(toMergeCandidateDict, generatedSeqNum):
    scoreList = list()
    dictKeys = list(toMergeCandidateDict.keys())
    
    sensitiveAPIs = {"CreateProcessInternal", "OpenProcess", "WinExec", "CreateThread", "OpenThread", "CreateRemoteThread",
                     "CopyFile", "CreateFile", "WriteFile", "ReadFile", "DeleteFile", "RegCreateKey", "RegSetValue",
                     "InternetOpen", "InternetConnect", "HttpSendRequest", "WinHttpOpen", "WinHttpSendRequest", "WinHttpWriteData", "WinHttpCreateUrl"}
    
    for i in range(len(dictKeys)):
        for j in range(i+1, len(dictKeys)):
            
            # toMergeCandidateDict[i][1] is memberSet
            Ri = toMergeCandidateDict[ dictKeys[i] ][0] # Ri is a tuple like (('G0', [[['A#A', 'C#C'], 0, 1, (0, 1), (1, 2)]]))
            Rj = toMergeCandidateDict[ dictKeys[j] ][0]
            
            print(toMergeCandidateDict[ dictKeys[i] ][1], toMergeCandidateDict[ dictKeys[j] ][1])
            
            # create Rnew = (clusterName , repNew)
            repNew = get_Representative(Ri, Rj)
            clusterTempName = "G" + str(generatedSeqNum)
            Rnew = (clusterTempName , repNew)
            couldBeMergedFlag = True
            
#             couldBeMergedFlag =False
#             RnewSequenceLen = 0
#             for motifInfo in Rnew[1]:
#                 motifLen = len(motifInfo[0])
                
#                 # Check if any API in rep contains sensitive API
#                 for motif in motifInfo[0]:
#                     if motif.split('#')[0] in sensitiveAPIs:
#                         couldBeMergedFlag = True
#                         break
                
#                 # if no any sensitive API, check sequence length of rep bigger than 26
#                 if(couldBeMergedFlag == False):
#                     RnewSequenceLen += motifLen
#                     if(RnewSequenceLen > 26):
#                         couldBeMergedFlag = True
#                         break
                
            # if the rep could be merged, then put into scoreList.
            if( couldBeMergedFlag ):
            
                # compute merge score of Rnew
                score = compute_Score(Ri, Rj, Rnew)
                Ri_name = Ri[0]
                Rj_name = Rj[0]
                scoreList.append((score, Rnew, Ri_name, Rj_name))
                
#             else:
#                 print("Rep Sequence Length smaller than 26! Length: ", RnewSequenceLen)

    if(len(scoreList) > 0):
        scoreList.sort(key=lambda tup:tup[0], reverse=True) # sorting by score (from biggest to smallest) 
    else:
        print("No common motif")
    
    return scoreList # list = [(score, Rnew, Ri_name, Rj_name), (score, Rnew, Ri_name, Rj_name), ...]

def checkExactlySameCandidates(scoreList):
    globalPoolDict = dict() # a dict contains many sets.  dict = {index0: memberSet, 1: memberSet, 2:...}
    newScoreList = list() # list = [(score, R, memberSet), (score, R, memberSet), ...]
    scoreListIdx = 0
    for rank in scoreList:
        score = rank[0]
       
        if(score == 1.0):
            
            Ri_name = rank[2]
            Rj_name = rank[3]
            
            duplicate = False
            for key, memberSet in globalPoolDict.items():
                if(Ri_name in memberSet) or (Rj_name in memberSet):
                    memberSet.add(Ri_name)
                    memberSet.add(Rj_name)
                    
                    # update newScoreList 'memberSet' element
                    newScoreList[key] = (newScoreList[key][0], newScoreList[key][1], memberSet)
                    duplicate = True
                    
            # Find new independent pair, add into newScoreList and create new dict key
            if(duplicate is False):
                memberSet = set()
                memberSet.add(Ri_name)
                memberSet.add(Rj_name)
                globalPoolDict[scoreListIdx] = memberSet
                
                Rnew = rank[1]
                newScoreList.append((score, Rnew, memberSet))
                scoreListIdx += 1
        else:
            Rnew = rank[1]
            Ri_name = rank[2]
            Rj_name = rank[3]
            memberSet = set()
            memberSet.add(Ri_name)
            memberSet.add(Rj_name)
            newScoreList.append((score, Rnew, memberSet))
            scoreListIdx += 1
            
    return newScoreList # list = [(score, R, memberSet), (score, R, memberSet), ...]
        
# add Rnew into toMergeCandidateDict and remove member of Rnew from candidates.

def mergeCandidateClusters_new(toMergeCandidateDict, intermediatePoolDict, scoreList, generatedSeqNum, initialDict, definedThreshold):
    initialNameDict = getInitialNameDict(initialDict) # get original name for reference in output.
    
    currentMergedSet = set()
    for rank in scoreList:
        score = rank[0]
        memberSet = rank[2] # memberSet of highest score

        # the minmum score this round is smaller than threshold
        if(score < definedThreshold):
            break
        
        exclusiveness = False
        
        # check exclusiveness
        for member in memberSet:
            if(member in currentMergedSet):
                exclusiveness = True
                break
                
        if(not exclusiveness):
            clusterMembers = set() # create cluster member set with original Name
            for member in memberSet:
                nameOfMember = int(member.split('G')[1])
                del toMergeCandidateDict[nameOfMember]
                
                if member in initialNameDict:
                    clusterMembers.add(initialNameDict[member])
                else:
                    clusterMembers.add(member)
                    
                # Mark elements are merged
                currentMergedSet.add(member) # update currentMergedSet
            
            Rnew = rank[1][1] # representative without old clusterName (i.e., rank[1] = (Name, Rep.))
            newName = "G" + str(generatedSeqNum)
            new_Cluster = (newName, Rnew)
            
            toMergeCandidateDict[generatedSeqNum] = (new_Cluster, clusterMembers)
            intermediatePoolDict[generatedSeqNum] = (score, new_Cluster, clusterMembers) # (score, newCluster, members)
            generatedSeqNum += 1
        
    return toMergeCandidateDict, intermediatePoolDict, generatedSeqNum

# add Rnew into toMergeCandidateDict and remove member of Rnew from candidates.

def mergeCandidateClusters(toMergeCandidateDict, intermediatePoolDict, scoreList, generatedSeqNum, initialDict):
    currentMergedSet = set()
    
    initialNameDict = getInitialNameDict(initialDict)
    
    for rank in scoreList:
        Ri_name = rank[2] # member1 of highest score
        Rj_name = rank[3] # member2 of highest score
        
        # check exclusiveness that candidate have been merged in current scoreList.
        # if both two element haven't been processed then create new cluster.
        if((Ri_name not in currentMergedSet) and (Rj_name not in currentMergedSet)):
            # remove candidates in @toMergeCandidateDict
            keyOfRi = int(Ri_name.split('G')[1])
            keyOfRj = int(Rj_name.split('G')[1])
            del toMergeCandidateDict[keyOfRi], toMergeCandidateDict[keyOfRj]

            Rnew = rank[1] # get representative of highest score
            newName = "G" + str(generatedSeqNum) # update clusterName
        
            new_Cluster = (newName, Rnew[1])

            clusterMembers = set() # create cluster member set
            if Ri_name in initialNameDict:
                clusterMembers.add(initialNameDict[Ri_name])
            else:
                clusterMembers.add(Ri_name)
            
            
            if Rj_name in initialNameDict:
                clusterMembers.add(initialNameDict[Rj_name])
            else:
                clusterMembers.add(Rj_name)
            
            
            toMergeCandidateDict[generatedSeqNum] = (new_Cluster, clusterMembers)
            intermediatePoolDict[generatedSeqNum] = (rank[0], new_Cluster, clusterMembers) # (score, newCluster, members)

            generatedSeqNum += 1
        
        # Mark elements are merged
        currentMergedSet.add(Ri_name) # update currentMergedSet
        currentMergedSet.add(Rj_name)
        
    return toMergeCandidateDict, intermediatePoolDict, generatedSeqNum

### Main Function of SBBGCA ###

import pickle

def do_SBBGCA_clustering(data_directory, tag, outputPath, thresholdValue):
#     testDict = {0: (('G0', [[['A#A', 'B#B','B#B', 'C#C','D#D'], 0, 2]]),{"a.trace.hooklog"}),
#                 1:(('G1', [[['A#A','B#B','C#C','D#D'], 0, 2]]),{"b.trace.hooklog"}),
#                    2:(('G2', [[['F#F','C#C','D#D', 'G#G'], 0, 2]]),{"c.trace.hooklog"}),
#                       3:(('G3', [[['Q#Q','C#C','D#D','G#G'], 0, 2]]),{"d.trace.hooklog"}),
#                            4:(('G4', [[['A#A'], 0, 2]]),{"e.trace.hooklog"})}
    intermediatePool = dict()
    roundInfos = dict()
    residual = None # used to save residual candidate when algorithm stop.
#     toMergeCandidateDict = testDict
    toMergeCandidateDict = initialCandidateDict(data_directory) # initialize @toMergeCandidateDict

    # initialDict = {clusterName : (originalName, initialLength)}
    initialDict = getInitialDict(toMergeCandidateDict)
    
    roundProduct = list()
    for key, value in initialDict.items():
        roundProduct.append(key)
    roundInfos[0] = roundProduct # record product in round 0 (i.e., initialization)
    
    generatedSeqNum = len(toMergeCandidateDict) # counter after initialize. Used to naming clusters.

    print("-- Start Clustering --")
    print("Threshold set =", thresholdValue)
    roundCounter = 1
    
    while(1):
        if(len(toMergeCandidateDict) == 1):
            residual = toMergeCandidateDict # output residual candidates.
            break

        # calculate scoreList in candidate clusters
        scoreList = findMergeCandidateScoreList(toMergeCandidateDict, generatedSeqNum)
        
        # check and merge exactly the same candidates before merge clusters
        scoreList = checkExactlySameCandidates(scoreList)
        
        # generated Clusters in This Round:
        nameIdxStart = generatedSeqNum
        
        toMergeCandidateDict, intermediatePool, generatedSeqNum = mergeCandidateClusters_new(
            toMergeCandidateDict, intermediatePool, scoreList, generatedSeqNum, initialDict, thresholdValue)
        
        # check if algorithm should stop when merge score under threshold
        # if a score smaller than threshold, then it will break out when merging.
        # Hense, if the 'generatedSeqNum' equals than 'nameIdxStart', means that no any new generated cluster.
        # (if occurr a new cluster, generatedSeqNum will add one.)
        if(generatedSeqNum == nameIdxStart):
            residual = toMergeCandidateDict # output residual candidates.
            break # end algorithm
        
        nameIdxEnd = generatedSeqNum
        
        # Record clusters generated in this round
        for idx in range(nameIdxStart, nameIdxEnd):
            if roundInfos.get(roundCounter) is None:
                roundProduct = list()
                roundProduct.append(intermediatePool[idx][1][0])
                roundInfos[roundCounter] = roundProduct
            else:
                roundInfos[roundCounter].append(intermediatePool[idx][1][0])
                
        roundCounter += 1
        scoreList.clear()
    print("-- Finish Clustering --")

    return intermediatePool, initialDict, roundInfos, residual

def clusterInitializedReps(initializedReps_dict, tag, outputPath, thresholdValue):
    intermediatePool = dict()
    roundInfos = dict()
    residual = None # used to save residual candidate when algorithm stop.
#     toMergeCandidateDict = testDict
    toMergeCandidateDict = initializedReps_dict # using residualRepsDict as toMergeCandidateDict (skip initialization)

    # initialDict = {clusterName : (originalName, initialLength)}
    initialDict = getInitialDict(toMergeCandidateDict)
    
    roundProduct = list()
    for key, value in initialDict.items():
        roundProduct.append(key)
    roundInfos[0] = roundProduct # record product in round 0 (i.e., initialization)
    
    generatedSeqNum = len(toMergeCandidateDict) # counter after initialize. Used to naming clusters.

    print("-- Start Clustering --")
    print("Threshold set =", thresholdValue)
    roundCounter = 1
    
    while(1):
        print("Current Round : Round ", roundCounter)
        if(len(toMergeCandidateDict) == 1):
            residual = toMergeCandidateDict # output residual candidates.
            break

        # calculate scoreList in candidate clusters
        scoreList = findMergeCandidateScoreList(toMergeCandidateDict, generatedSeqNum)
        print("-- Finish scoring --")
        
        # check and merge exactly the same candidates before merge clusters
        scoreList = checkExactlySameCandidates(scoreList)
        print("-- Finish checking 100% same candidates --")
        
        # generated Clusters in This Round:
        nameIdxStart = generatedSeqNum
        
        toMergeCandidateDict, intermediatePool, generatedSeqNum = mergeCandidateClusters_new(
            toMergeCandidateDict, intermediatePool, scoreList, generatedSeqNum, initialDict, thresholdValue)
        print("-- Finish merging clusters --")
        # check if algorithm should stop when merge score under threshold
        # if a score smaller than threshold, then it will break out when merging.
        # Hense, if the 'generatedSeqNum' equals than 'nameIdxStart', means that no any new generated cluster.
        # (if occurr a new cluster, generatedSeqNum will add one.)
        if(generatedSeqNum == nameIdxStart):
            residual = toMergeCandidateDict # output residual candidates.
            break # end algorithm
        
        nameIdxEnd = generatedSeqNum
        
        # Record clusters generated in this round
        for idx in range(nameIdxStart, nameIdxEnd):
            if roundInfos.get(roundCounter) is None:
                roundProduct = list()
                roundProduct.append(intermediatePool[idx][1][0])
                roundInfos[roundCounter] = roundProduct
            else:
                roundInfos[roundCounter].append(intermediatePool[idx][1][0])
                
        roundCounter += 1
        scoreList.clear()
    print("-- Finish Clustering --")

    return intermediatePool, initialDict, roundInfos, residual