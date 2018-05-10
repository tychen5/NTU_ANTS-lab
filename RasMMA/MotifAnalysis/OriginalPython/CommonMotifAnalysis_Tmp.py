class CommonMotif:
    stageMatrix = None
    stageMatrixIndex = None
    executionTrace = None # include "tick" featureprofile

    stage2commonDi = None
    stage2HkDi = None
    stage2nodeDi = None
    stageGaplb = None
    
    comMotif_dict = None
    disMotif_dict = None
    
    def __init__(self, stageMatrix, Motif, execTrace_dit, outputStage):
#         print("==Calculate common moitf==")
        self.stageMatrix = stageMatrix
        self.stageMatrixIndex = Motif.getStageMatrixIndex()
        self.executionTrace = execTrace_dit
        
        self.stage2commonDi = outputStage.getStage2common()
        self.stage2HkDi = outputStage.getStage2Hk()
        self.stage2nodeDi = outputStage.getStage2node()
        self.stageGaplb = outputStage.getStageGap()

        self.comMotif_dict = dict()
        self.disMotif_dict = dict()
        
        self.__setComMotif()
        self.__setDisMotif()
        
        
    #===private function 
    
    # Capture complete info of common motif from executionTrace
    def __setComMotif(self):
        for stage in self.stage2commonDi:
            if self.stage2commonDi[stage]:
                motifID = list(self.stage2HkDi[stage+1].keys())[0][1:] #because common stage only have one motif
                hooklog_names = self.stage2HkDi[stage+1]['M'+motifID]
                rep1_name = hooklog_names[0]
                rep1_indexRange = self.stageMatrixIndex[rep1_name][stage] #will capture the range of executionTrace
                rep2_indexRange = self.stageMatrixIndex[ hooklog_names[1] ][stage] # hoolong_name[1] = rep2_name
                
                # comMotif_object is a tuple which contains: (ComAPIs, ori_range1, ori_range2)
                comMotif_object = (self.executionTrace[rep1_name][rep1_indexRange[0]:rep1_indexRange[1]+1] , rep1_indexRange, rep2_indexRange)
                self.comMotif_dict['s'+str(stage+1)+'_'+motifID] = comMotif_object
            

    # Capture complete info of distinct motif from executionTrace
    def __setDisMotif(self):        
        for stage in self.stage2nodeDi:
            for stage1 in self.stage2HkDi: 
                for motifID in self.stage2HkDi[stage1]:
                    if motifID != self.stageGaplb:
                        hk = self.stage2HkDi[stage1][motifID][0]  
                        start = self.stageMatrixIndex[hk][stage][0]
                        end = self.stageMatrixIndex[hk][stage][1]
                        self.disMotif_dict['s'+str(stage+1)+'_'+motifID[1:]] = self.executionTrace[hk][start:end+1]
    
#         print("--- 2 set DistinctMotif dict fin---")
    
    #===public function
    def getComMotifDict(self):
        return self.comMotif_dict
    
    def getDisMotiDict(self):
        return self.disMotif_dict