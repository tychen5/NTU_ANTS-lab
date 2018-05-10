import os

class Hooklog3(object):
    path = None
    par = None
    
    digitname = None
    length = None
    li = None
    
    def __init__(self, path, par):
        self.path = path
        self.par = par
        self.li = list()
        self.length = 0
        
        self._parseDigitname()
        self._parseHooklog()
        
    def isExist(self):
        if os.path.isfile(self.path):
            return True
        else:
            return False
    
    def __len__(self):
        return self.length

    def __iter__(self):
        return iter(self.li)
    
    def __getitem__(self, key):
        return self.li[key]
    
    def __str__(self):
        return "class Hooklog3, %s, par = %d, len = %d, digit name = %s" % (self.path, self.par, self.length, self.digitname)
    
    def _parseDigitname(self): # Mike: make it protected for class
        if self.path != "":
            self.digitname = self.path.split('/')[-1][:6] + '-' + self.path.split('_')[-1].split('.')[0]
    
    # private functions
    def _parseHooklog(self): # Mike: make it protected for class
        if self.isExist():
            handle = open(self.path, 'rb') # MIKE: change mode from 'r' to 'rb', Windows cannot handel seek() in text(r) mode
            
            while 1:
                (tick, api) = self._getNextPair(handle)
                if tick == 0:
                    break # end of file or fail to read
                self.li.append((tick, api))
                self.length += 1
            if handle: handle.close()
            
            self.li.sort(key = lambda tup: tup[0]) # sort by tick
            #self.li = list(filter(lambda (tick, api): api != '', self.li)) #=====2/17 # 20170708 MIKE: who add this?
        else:
            pass
    
    def _skipAPI(self, api): # MIKE: 20170708, skip some apis
        return False # Hooklog3 does not skip API
    
    def _getNextPair(self, handle): # Mike: make it protected for class
        tick = 0
        api = '' # 20170708 MIKE: = 'api' why?
        
        while 1:
            line = handle.readline().decode('ISO 8859-1') # MIKE: 20170616, for python 3
            if not line: break # end of file
            if line[0] == '#': # start a new call
                tick = line[1:].strip()
                api = handle.readline().decode('ISO 8859-1').strip() # MIKE: 20170616, for python 3
                
                # hack
                if api.startswith('=Reg'): api = api[1:]
                    
                # MIKE: 20170708, skip some apis
                if self._skipAPI(api): continue
                
                if self.par == True:
                    api = self._getParValue(api, handle)
                break # get one call
            else:
                continue
        
        return (tick, api)
    
    # protected functions
    # MIKE: 20160307, basic par. I move it out for better creating new class FeatureHooklog. It has to be protected!
    def _getParValue(self, api, handle):
        if api.startswith('Reg') or api == 'LoadLibrary' or True: # read first par
            api += handle.readline().decode('ISO 8859-1').strip()
        return api # MIKE: 20170616, was it a bug? It was returned in if
    
    # public functions
    def getWinSet(self, win):
        li_set = set()
        for i in range(self.length - win + 1):
            key = ''
            for w in range(win):
                key += self.li[i+w][1] # api
            li_set.add(key)
        return li_set