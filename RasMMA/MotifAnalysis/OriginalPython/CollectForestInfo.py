import pickle

class CollectForestInfo:
    intermediateDict = None
    residualDict = None
    descendant_dict = None
    groupMotif_dict = None
    treeList = None
    
    def __init__(self, intermidiatePicklePath, residualPicklePath, includePairwiseTree):
        
        # read the results from pickle files
        with open(intermidiatePicklePath, 'rb') as handle:
            self.intermediateDict = pickle.load(handle)
        with open(residualPicklePath, 'rb') as handle:
            self.residualDict = pickle.load(handle)
        
        self._setDescendantAndGroupMotifs()
        self._setTreeList(includePairwiseTree)
        
        
    # get descendant and motif information from pickle
    def _setDescendantAndGroupMotifs(self):

        descendant_dict = dict()
        groupMotif_dict = dict()
        intermediate_list = sorted(self.intermediateDict.items(), key=lambda x : x[0])

        for item in intermediate_list:
            value = item[1] # get original dict value
            score = value[0]
            clusterName = value[1][0]
            memberSet = value[2]
            motifs = value[1][1]

            descendants = set()
            for member in memberSet:
                if member[0] == "G":
                    for descendant in descendant_dict[member]:
                        descendants.add(descendant)
                else:
                    descendants.add(member)
            descendant_dict[clusterName] = descendants
            groupMotif_dict[clusterName] = motifs

        self.descendant_dict = descendant_dict
        self.groupMotif_dict = groupMotif_dict
    
    
    # get those residual trees which isn't sigular
    # collect their clusterName into notLonerList.
    def _setTreeList(self, includePairwiseTree):

        notLonerList = []

        for key, value in self.residualDict.items():
            clusterName = value[0][0]
            motifsList = value[0][1]
            members = value[1]

            notLoner = False

            if(len(members) != 0):
                if(includePairwiseTree):
                    notLoner = True

                else:   # remove 2-member pairs
                    if( len(members) == 2):
                        for member in members:
                            if member[0] == 'G':
                                notLoner = True
                                break
                    else:
                        notLoner = True

            if(notLoner):
                notLonerList.append((clusterName, members))

        notLonerList = sorted(notLonerList, key=lambda x: int(x[0][1::]), reverse=False)

        self.treeList = notLonerList

    def getGroupMotif_dict(self):
        return self.groupMotif_dict
        
    def getDescendant_dict(self):
        return self.descendant_dict
    
    def getTreeList(self):
        return self.treeList