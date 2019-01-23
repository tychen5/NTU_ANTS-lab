#ifndef DEF_H
#define DEF_H

#define BUF_SIZE 512

#define STAGE_LOGIN 0
#define STAGE_MENU 1
#define STAGE_EXIT 2

#define STAGE_CONNECT 0
#define STAGE_DISCONNECT 1

#define LEN(x) (strlen(x)+1)

typedef int bool;
#define true 1
#define false 0

#ifndef N_DEBUG
#define DEBUG_ZU(x); fprintf(stderr, "%s: %zu\n", #x, x);
#define DEBUG_S(x); fprintf(stderr, "%s: %s\n", #x, x);
#define DEBUG_D(x); fprintf(stderr, "%s: %d\n", #x, x);
#define DEBUG(s); fprintf(stderr, "%s\n", s);
#endif 

#ifdef N_DEBUG
#define DEBUG_ZU(x); 
#define DEBUG_S(x); 
#define DEBUG_D(x); 
#define DEBUG(s);
#endif

#endif