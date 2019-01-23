#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
//set maxN the maximum users simultaneously
const int maxN = 5;
char ret[100] = {};
int usrN = 0;
char *foo = NULL;
char *bar = NULL;
//reply message
const char s[]="#";
const char ca[] = "connection accepted\n";
const char mes_ok[] = "100 OK\n";
const char mes_fail[] = "210 FAIL\n";
const char mes_auth[] = "220 AUTH_FAIL\n";
const char mes_bye[] = "Bye\n";
const char mes_error[] = "Please follow the rule and try again.\n";

//user list structure
struct usrList{
  bool check;
  char accountName[32];
  char *ipAddr;
  char portN[6];
};
struct usrList usrlist[maxN];

//arg to parse into new thread
struct arg_struct {
    int fd;
    char *ip;
};

//find if '#' exist or not
bool findChr(char *tar)
{
  const char *invalid_characters = "#";
  char *c = tar;
  while (*c)
  {
      if (strchr(invalid_characters, *c))
        return true;
      c++;
  }
  return false;
}

// new thread
void * socketThread(void *arg)
{
  struct arg_struct *args = arg;
  int newSocket = args->fd;
  char *ipAddr = args->ip;
  send(newSocket, ca, sizeof(ca), 0);
  printf("[INFO] Accepted. Waiting for registering......\n");

  char usr[32] = {};
  char tempRet[100] = {};
  bool login = false;

  while(1){
    memset(ret, 0, sizeof(ret));
    memset(tempRet, 0, sizeof(tempRet));
    recv(newSocket, ret, sizeof(ret), 0);
    strcpy(tempRet, ret);
    //After login
    if(login)
    {
      //return list
      if(strcmp(tempRet, "List") == 0)
      {
        char mes_list[100] = "10000\nnumber of account online: ";
        char partStr[100] = "\n";
        int usrNN = 0;
        for(int n = 0; n < maxN; n++)
        {
          if(usrlist[n].check)
          {
            usrNN++;
            strcat(partStr, usrlist[n].accountName);
            strcat(partStr, "#");
            strcat(partStr, usrlist[n].ipAddr);
            strcat(partStr, "#");
            strcat(partStr, usrlist[n].portN);
            strcat(partStr, "\n");
          }
        }
        char nn = usrNN+'0';
        strcat(mes_list, &nn);
        strcat(mes_list, partStr);
        send(newSocket, mes_list, sizeof(mes_list), 0);
        printf("[INFO] Sending list to %s.\n", usr);
      }
      //Exit and logout
      else if(strcmp(tempRet, "Exit") == 0)
      {
        send(newSocket, mes_bye, sizeof(mes_bye), 0);
        for (int n = 0; n < maxN; n++)
        {
          if(strcmp(usrlist[n].accountName, usr) == 0)
          {
            memset(usrlist[n].accountName, 0, sizeof(usrlist[n].accountName));
            memset(usrlist[n].portN, 0, sizeof(usrlist[n].portN));
            usrlist[n].ipAddr = 0;
            usrlist[n].ipAddr = NULL;
            usrlist[n].check = false;
          }
        }
        login = false;
        printf("[INFO] Disconnect from %s.\n", usr);
        printf("[INFO] Exit socketThread. \n");
        close(newSocket);
        pthread_exit(NULL);
      }
      //error
      else
      {
        send(newSocket, mes_error, sizeof(mes_error), 0);
        printf("[INFO] Invalid request.\n");
      }
    }
    //Before login
    else
    {
      //find #
      if(findChr(tempRet))
      {
        foo = strtok(tempRet,s);
        bar = strtok(NULL,s);
        //registering
        if(strcmp(foo, "REGISTER") == 0)
        {
          if(strcmp(bar, usr)==0 || usrN >= maxN)
          {
            send(newSocket, mes_fail, sizeof(mes_fail), 0);
            printf("[INFO] Registration failed. 210\n");
          }
          else
          {
            strcpy(usr, bar);
            usrN++;
            send(newSocket, mes_ok, sizeof(mes_ok), 0);
            printf("[INFO] Registration success. 100 %s\n", bar);
          }
        }
        //login process
        else {
          if(strcmp(foo, usr)==0)
          {
            //return list
            char mes_list[100] = "10000\nnumber of account online: ";
            char partStr[100] = "\n";
            int usrNN = 0;

            for(int n = 0; n < maxN; n++)
            {
              if(usrlist[n].check == false)
              {
                usrlist[n].check = true;
                strcpy(usrlist[n].accountName, usr);
                usrlist[n].ipAddr = ipAddr;
                strcpy(usrlist[n].portN, bar);
                break;
              }
            }

            for(int n = 0; n < maxN; n++)
            {
              if(usrlist[n].check)
              {
                usrNN++;
                strcat(partStr, usrlist[n].accountName);
                strcat(partStr, "#");
                strcat(partStr, usrlist[n].ipAddr);
                strcat(partStr, "#");
                strcat(partStr, usrlist[n].portN);
                strcat(partStr, "\n");
              }
            }
            char nn = usrNN+'0';
            strcat(mes_list, &nn);
            strcat(mes_list, partStr);

            send(newSocket, mes_list, sizeof(mes_list), 0);
            printf("[INFO] Login success. %s\n", foo);
            printf("UsrN: %d\n", usrN);
            login = true;
          }
          else
          {
            send(newSocket, mes_auth, sizeof(mes_auth), 0);
            printf("[INFO] Login failed. 220 %s\n", foo);
          }
        }
      }
      //invalid input
      else
      {
        send(newSocket, mes_error, sizeof(mes_error), 0);
        printf("[INFO] Invalid request. \n");
      }
    }


  }
  //Exit and close connection
  printf("[INFO] Exit socketThread \n");
  usrN--;
  close(newSocket);
  pthread_exit(NULL);
}


int main()
{
  int serverSocket, newSocket;
  struct sockaddr_in serverAddr;
  struct sockaddr_in serverStorage;
  socklen_t addr_size;
  int sock_client;

  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(12345);
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
  bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
  if(listen(serverSocket, maxN) == 0)
    printf("[INFO] Listening......\n");
  else
    printf("[INFO] Error\n");
  pthread_t tid[10];
  int i = 0;

  for (int i = 0; i < maxN; i++)
    usrlist[i].check = false;

  while(1)
  {
    addr_size = sizeof serverStorage;
    newSocket = accept(serverSocket, (struct sockaddr *)&serverStorage, &addr_size);
    struct arg_struct tempArg;
    //parse ip address and sockfd
    tempArg.fd = newSocket;
    tempArg.ip = inet_ntoa(serverStorage.sin_addr);
    if(pthread_create(&tid[i++], NULL, socketThread, (void*)&tempArg))
      printf("[INFO] Failed to create thread");
    if(i >= maxN)
    {
      i = 0;
      while(i < maxN)
      {
        pthread_join(tid[i++],NULL);
      }
      i = 0;
    }
  }

  close(serverSocket);

  return 0;
}
