def stageMatrix(matchMatrix, gapSeqList):
    len_BASE = len(gapSeqList)-1
    hkName_li = [hk for hk, stat, api in matchMatrix[1]]
    stageMatrix = {hk:[] for hk in hkName_li}

    for hk in stageMatrix:
        strBuf_li = []
        j = 0

        while j < len_BASE+1:
            #----GapSeqList: check if gapSeqList is empty. 
            if (gapSeqList[j]) :
                if (strBuf_li): stageMatrix[hk].append(strBuf_li)

                gapli =  list(filter(lambda tup:tup[0]==hk, gapSeqList[j]))
                strBuf_li = gapli[0][1] if gapli else ['=']
                stageMatrix[hk].append(strBuf_li) 
                strBuf_li = []

            j+=1
            if (j > len_BASE): 
                if (strBuf_li): stageMatrix[hk].append(strBuf_li)
                break

            #----MatchMatrix: check if the stat of two col. are equal.
            api = [api for hkN, stat, api in matchMatrix[j] if hkN==hk][0]
            strBuf_li.append(api)

            if (j+1 < len_BASE+1):
                curColStat_li = [stat for hkN, stat, api in matchMatrix[j]]
                nextColStat_li = [stat for hkN, stat, api in matchMatrix[j+1]]
                if curColStat_li != nextColStat_li:
                    stageMatrix[hk].append(strBuf_li)
                    strBuf_li = []
    
    # transform multiple gap segment into one gap segment                    
    for hk in stageMatrix:
        for ii,seg in enumerate(stageMatrix[hk]):
            if set(seg) == set(['=']):
                stageMatrix[hk][ii] = ['=']

    return stageMatrix