def pairwise_NW(hl_list1, hl_list2, score_matched, score_mismatched, score_gap, isret):

    isH1Long = True if len(hl_list1) >= len(hl_list2) else False 
    long_list = hl_list1 if isH1Long else hl_list2
    short_list = hl_list2 if isH1Long else hl_list1
    
    m = len(long_list)
    n = len(short_list)
    
    # create a  score matrix and initial first vertical row
    # scoreMatrix element = (score, directionsList())
    # directionsList contains direction
    # (because there may be multiple directions in one score cell)
    #
    # direction: 0-matched, 1-mismatched, 2-insert verticalGap, 3-insert horizontalGap
    scoreMatrix = [[(score_gap*i,list()) if j==0 else (0,list()) for j in range(m+1)] for i in range(n+1)]
    scoreMatrix[0] = [(0+score_gap*j,list()) for j in range(m+1)] # initial first horizontal column values
    
    # run Needleman-Wunsch algorithm
    for i in range(1, n+1): # vertical(short)
#         s = ""
        for j in range(1, m+1): # horizontal(long)
            # count gap scores
            verticalGapScore = scoreMatrix[i][j-1][0] + score_gap
            horizontalGapScore = scoreMatrix[i-1][j][0] + score_gap
            
            matchScore = scoreMatrix[i-1][j-1][0] # initial
            isMatchOrNot = 0
            
            # count match/mismatch score
            if compareSequence( long_list[j-1], short_list[i-1] ):
                matchScore = matchScore + score_matched
                isMatchOrNot = 1
            else:
                matchScore = matchScore + score_mismatched
                isMatchOrNot = -1
            
            maxScore = max(verticalGapScore, horizontalGapScore, matchScore) # find maximum score
            
            directions = list()
            # add (direction to each cell of scoreMatrix) into directions
            if(maxScore == matchScore):
                if isMatchOrNot==1: # matched
                    directions.append(0)
                else:               # mismatched
                    directions.append(1)
            if(maxScore == verticalGapScore): # vertical add gap
                directions.append(2)
            if(maxScore == horizontalGapScore): # horizontal add gap
                directions.append(3)
            
            scoreMatrix[i][j] = (maxScore,directions)

    output_longList = []
    output_shortList = []

    # walk sequence path(from end cell to first cell)

    
    while(m>0 and n>0) : # if a direction touch margin than finish walking
        
        ## only pick up one path
        ## if want to get all paths, go through all directions in 'scoreMatrix[n][m][1]'
        direction = scoreMatrix[n][m][1][0]

        # the condition of direction check
        if direction == 0:
            output_longList.append((m, long_list[m-1]))
            output_shortList.append((n, short_list[n-1]))
            m=m-1
            n=n-1

        elif direction == 1:
            output_longList.append((m, long_list[m-1]))
            output_shortList.append((n, short_list[n-1]))
            m=m-1
            n=n-1

        elif direction == 2:
            output_shortList.append((-1, "="))
            output_longList.append((m, long_list[m-1]))
            m=m-1

        else:
            output_longList.append((-1, "="))
            output_shortList.append((n, short_list[n-1]))
            n=n-1
    
    # two below loop are for walking path which didn't stop at (0,0)
    # if not stop at (0,0), should insert gap to push back until (0,0)
    while(n>0):
        output_longList.append((-1, "="))
        output_shortList.append((n, short_list[n-1]))
        n=n-1
        
    while(m>0):
        output_shortList.append((-1, "="))
        output_longList.append((m, long_list[m-1]))
        m=m-1
                
    alignment_result = list(zip(output_longList, output_shortList)) if isH1Long else list(zip(output_shortList, output_longList))
    alignment_result = alignment_result[::-1]
    
    return None, None, alignment_result # MIKE: 20170811, for the compatibility

def compareSequence(seq1, seq2):
    return True if seq1 == seq2 else False