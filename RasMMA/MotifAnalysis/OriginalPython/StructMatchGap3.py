def structMatchGap(ali_dict, BASE = None):
    if BASE == None:
        len_BASE = ali_dict[list(ali_dict.keys())[0]][-1][1][0] #get baseline length
    else:
        len_BASE = ali_dict[BASE][-1][1][0] #get baseline length
    matchMatrix={i+1:[] for i in range(len_BASE)} 
    gapSeqList={i:[] for i in range(len_BASE +1)} 

    for hk in ali_dict:
        ali_hk = ali_dict[hk]
        len_aHK = len(ali_hk)
        
        j = 0
        while (j < len_aHK):
            # BASE:(iB,apiB); Others:(iO,apiO)
            ((iB,apiB),(iO,apiO)) = ali_hk[j] 
            if apiB=='=' and apiO!='=':
                lastIndex = ali_hk[j-1][0][0] if j!=0 else 0
                strBuf_li = [apiO]
                
                # 20170827 - WJ: What this loop??? the j index might cause index out of bounds exception before.
                # I add j<(len_aHK-1) instead of j<len_aHK to avoid the exception.
                
                while (j < (len_aHK-1)) and (ali_hk[j+1][0][1]=='='):
                    strBuf_li.append(ali_hk[j+1][1][1])
                    j += 1
                gapSeqList[lastIndex].append((hk, strBuf_li))

            else:
                symbol = 'gap' if (apiO=='=' and apiB!='=') else ('mismatch' if apiB!=apiO else 'match')
                symbolAPI = '=' if symbol =='gap' else apiO
                matchMatrix[iB].append((hk,symbol,symbolAPI)) 
                
            j += 1
                    
    return matchMatrix, gapSeqList