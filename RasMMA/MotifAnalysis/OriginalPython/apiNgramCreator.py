class FamilyTree:
    familyName = None
    treeIndex = None
    apiSequences = None
    nGramList = []
    
    def __init__(self, familyName, treeIndex, apiSequences, windowSize):
        self.familyName = familyName
        self.treeIndex = treeIndex
        self.apiSequences = apiSequences
        self.setNgram(windowSize)
        
    def setNgram(self, windowSize):
#         for api in self.apiSequences:
        sequenceLen = len(self.apiSequences)
        self.nGramList = [ self.apiSequences[ i : i+windowSize ] for i in range(sequenceLen - windowSize + 1) ]
    
    def getFamilyName(self):
        return self.familyName
    
    def getTreeIndex(self):
        return self.treeIndex
    
    def getAPISequences(self):
        return self.apiSequences
    
    def getNGramList(self):
        return self.nGramList
    
    def getNGramListLen(self):
        return len(self.nGramList)