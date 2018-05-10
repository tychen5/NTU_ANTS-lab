from OrderedSet import OrderedSet

class Motif:
    motif_apiparam = None
    motif_lb2seg = None
    motif_seg2lb = None
    motif_stageMatrixIndex = None
    motif_gaplb = None #======1/15
    totalStageLen = None  #======1/15
    
    def __init__(self, stageMatrix, BASE):
        self._setMotif(stageMatrix, BASE)
        
    
    def _setMotif(self, stageMatrix, BASE):
        segment_set = OrderedSet()
        for i in range(len(stageMatrix[BASE])): 
            for hk in stageMatrix:
                segment_set.add(tuple(stageMatrix[hk][i]))
#         print("total segment:", len(segment_set) )

        #apiparam set
        apiparam_set = set()
        for hk in stageMatrix: 
            for seg in stageMatrix[hk]:
                for a in seg:
                    apiparam_set.add(a)

        #label each segment (motif)
        lb2seg_dict = dict()
        for i in range(len(segment_set)):
            lb2seg_dict['M'+str(i+1)]= tuple(list(segment_set)[i])
        seg2lb_dict = dict()
        for k in lb2seg_dict:
            seg2lb_dict[lb2seg_dict[k]] = k

        #set stageMatrixIndex
        #record every motif in the every stage of a hooklog; the start&end indext of hk is for API deduplication
        stageMatrixIndex = dict()
        for hklg in stageMatrix:
            idxtuple_list = []
            last_start = 0
            last_end = 0
            for motif in stageMatrix[hklg]:
                if motif == ['=']:
                    idxtuple_list.append((-1, -1))
                    continue
                last_end = last_start + len(motif) - 1
                idxtuple_list.append((last_start, last_end))
                last_start = last_end + 1
            stageMatrixIndex[hklg] = idxtuple_list

        self.motif_apiparam = apiparam_set
        self.motif_lb2seg = lb2seg_dict
        self.motif_seg2lb = seg2lb_dict
        self.motif_stageMatrixIndex = stageMatrixIndex
        if tuple(['=']) not in seg2lb_dict.keys():
            self.motif_gaplb = 'NONE'
#             print("No Gap.")
        else:
            self.motif_gaplb = seg2lb_dict[tuple(['='])]
            
        self.totalStageLen = len(stageMatrix[BASE])
        
    def getStageMatrixIndex(self):
        return self.motif_stageMatrixIndex
    
    def getMoti_apiPar(self):
        return self.motif_apiparam
    
    def getMoti_lb2seg(self):
        return self.motif_lb2seg
    
    def getMoti_seg2lb(self):
        return self.motif_seg2lb
    
    def getMoti_gaplb(self):
        return self.motif_gaplb
    
    def getTotalStageLen(self):
        return self.totalStageLen