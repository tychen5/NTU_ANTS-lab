#ifndef USER_H_
#define USER_H_

#include <stdio.h>
#include <pthread.h>
#include "../packet/packet.h"
#include "def.h"

typedef struct
{
  Cookie* cookieP;  
  size_t balance;
  size_t port;
  pthread_t threadId;
} User;

typedef struct
{
  User users[THREAD];
  size_t users_n;
} OnlineUsers;

#endif