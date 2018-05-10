# output_Stage
import pygraphviz as pgv
import math
from IPython.display import SVG

# output_moreInfo
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np

class OutputStage:
    famStageMatrix = None
    outputPath = None
    famBASE = None 
    famSeg2lb = None
    famLb2seg = None
    
    g = None
    gapLb = None
    
    stageLen = None
    stage2node_dict = None
    stage2Hk_dict = None
    stage2common_dict = None
    
    threshold = 0.0
    
    def __init__(self, stageMatrix, outputPath, BASE, Motif):
#         print("==Starting drawing stages==")
        self.famStageMatrix = stageMatrix
        self.outputPath = outputPath
        self.famBASE = BASE
        self.famSeg2lb = Motif.getMoti_seg2lb()
        self.famLb2seg= Motif.getMoti_lb2seg()
        
        self.g = pgv.AGraph(directed=True)
        
        self.stageLen = len( self.famStageMatrix[self.famBASE])
        self.stage2node_dict = dict() 
        self.stage2Hk_dict = dict()
        self.stage2common_dict = dict()
        
        self.g = self.__nodeBuilt(self.g)
        self.g = self.__nodeConnect(self.g)
        self.g = self.__setLabel(self.g)
        if(outputPath != None): # output is not necessary in some usage
            self.__saveOutput(self.g)
        
    #def __len__(self):
    #def __iter__(self):
    #def __getitem__(self, key):
    #def __str__(self):
    
    #===private function 
    def __nodeBuilt(self, g): # stage2node, stage2Hk
#         stage2node_dict = dict()
        stage2node_dict = {i : [] for i in range(self.stageLen)}
        stage2Hk_dict = dict() 

        for i in range(self.stageLen):
            n_dict = dict()

            for k in self.famStageMatrix:
                lb_key = self.famSeg2lb[tuple(self.famStageMatrix[k][i])] #===== lb_key=> M1.M2
                if lb_key not in n_dict.keys():
                    n_dict[lb_key] = [k]
                else:
                    n_dict[lb_key] = n_dict[lb_key] + [k]

            for lb in n_dict:
                num_stage2HK  = len(n_dict[lb])
                num_stage2moti = len(self.famLb2seg[lb])

                if g.number_of_nodes()==0: 
                    g.add_node(1,label=lb+ ':'+ str(num_stage2HK)+ r"\nlen:"+str(num_stage2moti)
                               ,id=';'.join(n_dict[lb])) 
#                     stage2node_dict[i]=stage2node_dict[i]+[1] if i in stage2node_dict else []+[1]
                    stage2node_dict[i]=stage2node_dict[i]+[1]

                else:
                    g.add_node(g.number_of_nodes()+1, label=lb+':'+str(num_stage2HK) +r"\nlen:" 
                               +str(num_stage2moti), id=';'.join(n_dict[lb]))
#                     stage2node_dict[i]=stage2node_dict[i]+[g.number_of_nodes()] if i in stage2node_dict else []+[g.number_of_nodes()]
                    stage2node_dict[i]=stage2node_dict[i]+[g.number_of_nodes()]
            stage2Hk_dict[i+1] = n_dict 
            
        self.stage2node_dict = stage2node_dict
        self.stage2Hk_dict = stage2Hk_dict
#         print("---Node Build Done---")
        return g
        
    
    def __nodeConnect(self, g):
        for k in self.stage2node_dict:
#             print(k, "stage") 
            if k < self.stageLen -1:
                cur_list = [g.get_node(i) for i in self.stage2node_dict[k]]
                next_list = [g.get_node(i) for i in self.stage2node_dict[k+1]]
                for n_cur in cur_list:
                    for n_next in next_list:
                        for log_cur in n_cur.attr['id'].split(';'):
                            for log_next in n_next.attr['id'].split(';'):
                                if log_cur == log_next:
                                    if not g.has_edge(n_cur,n_next):
                                        common_set=set(n_cur.attr['id'].split(';')).intersection(
                                            set(n_next.attr['id'].split(';')))
                                        g.add_edge(n_cur,n_next,label=len(common_set))

        #relabel gap
        import re
        if tuple(['=']) in self.famSeg2lb:
            self.gapLb = self.famSeg2lb[tuple(['='])]
            for n in g.nodes():
                if n.attr['label'].split(':')[0] == self.gapLb:
                    tok = re.split('\W+n*',n.attr['label'])
                    n.attr['label']='gap'+ ':' + str(tok[1]) + r"\n" + tok[2] + ':' + tok[3]
                    n.attr['shape']='diamond'
                    n.attr['fillcolor']='yellow'
#         else:
#             print("Warning!")
#             print("\tDoes not run others, because only one stage is unnecessary if there is no gap!")
    
        return g
        
    
    def __setLabel(self, g):
        # add extra labels (entropy per stage)
        last_node = ''
        for i in self.stage2node_dict:
            # stage label and entropy
            g.add_node(len(g.nodes())+1)
            g.get_node(len(g.nodes())).attr['shape'] = 'plaintext'
            
            n_list = [len(g.get_node(n).attr['id'].split(';')) for n in self.stage2node_dict[i] 
                      if g.get_node(n).attr['id']]
            #for n in stage2node_dict[i]: print i,n,g.get_node(n)
            e = str(round(self.__entropy(n_list), 2))
            g.get_node(len(g.nodes())).attr['label'] = 'stage '+str(i+1)+'\nH='+e
            g.get_node(len(g.nodes())).attr['id'] = 'extra'
            # make them same level
            g.add_subgraph([len(g.nodes())] + [n for n in self.stage2node_dict[i]], rank='same')

        stage2common_dict = {l:0 for l in self.stage2node_dict}
        for n in g.nodes():
            if g.get_node(n).attr['label'].startswith('stage'):
                stage = int(g.get_node(n).attr['label'].split()[1]) - 1
                entropy = float(g.get_node(n).attr['label'].split('=')[1]) 
                if entropy <= self.threshold: # if entropy == 0, then common[stage] : 1
                    stage2common_dict[stage] = 1

        # align stage label
        for n in sorted(g.nodes()): 
            if n.attr['id'] == 'extra':
                if int(n) < len(g.nodes())-1:
                    g.add_edge(n, int(n)+1, weight=10, style='invis')
                    

        self.stage2common_dict = stage2common_dict
        return g
    
    
    def __saveOutput(self ,g):
        family_name = self.outputPath.split('/')[-1]
        
        g.draw(self.outputPath +'/' +family_name +'_output.svg', format='svg',prog='dot')
        g.draw(self.outputPath +'/' +family_name +'_output.pdf', format='pdf',prog='dot')
        g.draw(self.outputPath +'/' +family_name +'_output.dot', format='dot',prog='dot')

        #SVG(filename= self.fam_path +'/' +family_name +'_output.svg')
        print("---Save Output Done---")
    
    # entropy function
    def __entropy(self ,l):       
        e = 0
        n = sum(list(l))
        for i in l:
            e += float(i)/n*math.log(float(i)/n)
        return abs(e)
    
    
        
    # public function
    def getStageLen(self):
        return self.stageLen
    
    def getStage2node(self):
        return self.stage2node_dict
    
    def getStage2Hk(self):
        return self.stage2Hk_dict
    
    def getStage2common(self):
        return self.stage2common_dict
    
    def getStageGap(self):
        return self.gapLb
    
    def getGraph(self):
        return self.g
   