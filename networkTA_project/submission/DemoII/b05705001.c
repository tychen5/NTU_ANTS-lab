#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>
#include <stdbool.h>

int serverSocket, newSocket;
struct sockaddr_in serverAddr;
struct sockaddr_in serverInfo,clientInfo;
struct sockaddr_storage serverStorage;


char client_message[8000];
char buffer[1024];
pthread_mutex_t mutex_uList = PTHREAD_MUTEX_INITIALIZER;
char* user_list[10];
int usercnt = 0;
int account_list[10] = {0};
char* IP_List[10] = {0};
char* port_list[10] = {0};
int online_list[10] = {0};
int online_user_cnt = 0;


char * slice_str(char * str, int start, int end)
{
  int len = end - start + 1;
  char * copied_str = malloc(len * sizeof(char)); 
  memset(copied_str, '\0', 200);

  for (int i = start; i <= end; ++i)
  {
    copied_str[i - start] = str[i];
  }
  // printf("%s\n", copied_str);
  free(copied_str);
  return(copied_str);
}

char * split_str(char * str) //split string before token '#'
{ 
  const char *tok = "#";
  char *token;
  token = strtok(str, tok); //username
  return(token);
}

int getlen(char * str)
{
  // int res = 0;
  for (int i = 0; i < sizeof(str); ++i)
  {
    if(str[i] == '\0')
    {
      return(i);
    } 
  }
  return(200);
}

int find(char * element, char* list[10], int listlen)
{
  for (int i = 0; i < listlen; ++i)
  {
    if(strcmp(list[i], element) == 0)
    {
      return i; //return user ID
    }
  }
  return -1; //can't find
}


void * socketThread(void *arg)
{
  printf("%s\n", "----- A new thread. -----");
  int newSocket = *((int *)arg);  
  int msgcnt = 0;
  char message[1024] = {0};
  int login = 0;
  int userID = -1; 
  char* client_name;

  while(recv(newSocket , client_message ,1024 , 0) > 0)
  {
    
    printf("No. %d msg: ", msgcnt++);
    printf("%s\n", client_message);

    //Register
    if (strstr(client_message, "REGISTER#") != NULL) //strcmp(slice_str(client_message,0,8), "REGISTER#") == 0
    {
      //client name!!!
        client_name = slice_str(client_message, 9, sizeof(client_message));
        memset(message, '\0', sizeof(message)); 

        //check if the username has been used
        if (find(client_name, user_list, usercnt) != -1)
        {
          strcpy(message, "210 FAIL\n");
          send(newSocket,message,sizeof(message),0);
          continue;
        }

        //register successfully
        pthread_mutex_lock(&mutex_uList);
        userID = usercnt;
        user_list[userID] = strdup(client_name);
        usercnt += 1;
        pthread_mutex_unlock(&mutex_uList);
        
        strcpy(message, "100 OK\n");
        send(newSocket,message,sizeof(message),0); 
        memset(message, '\0', sizeof(message));   
        continue;
    }

    //List
    else if((strcmp(client_message, "List\n") == 0) && login == 1)
    {

      //Account balance
      char money2[20];
      strcpy(message, "Account balance: ");
      sprintf(money2, "%d", account_list[userID]); 
      strcat(message, money2);
      strcat(message, "\n");
      send(newSocket,message,sizeof(message),0);
      memset(message, '\0', sizeof(message));


      //format list
      char list2[1024] = {};
      int online_cnt2 = 0;
      for (int i = 0; i < usercnt; ++i)
      {

        if(online_list[i] == 1)
        {
          online_cnt2++;
          char message_list2[1024];
          memset(message_list2, '\0', sizeof(message_list2));
          snprintf(message_list2, 1024, "%s#%s#%s\r\n", user_list[i], inet_ntoa(clientInfo.sin_addr), port_list[i]); //online list
          strcat(list2, message_list2);   
        }        
      }

      //online accounts
      sprintf(message, "number of accounts online: %d\n", online_cnt2); 
      send(newSocket,message,sizeof(message),0);
      memset(message, 0, sizeof(message));


      //online list
      strcpy(message, list2);
      send(newSocket,message,sizeof(message),0);
      memset(message, 0, sizeof(message));

    }
    else if((strcmp(client_message, "Exit\n") == 0) && login == 1)
    {
      strcpy(message, "Bye!\n");
      send(newSocket,message,sizeof(message),0);
      memset(message, '\0', sizeof(message));
      online_list[userID] = 0;
      online_user_cnt--;

      
      printf("User %s exit.\n", user_list[userID]);

    }

    //Login

    else
    {
      client_name = split_str(client_message);

      if (find(client_name, user_list, usercnt) != -1)
      {

        login = 1; 
        userID = find(client_name, user_list, usercnt); //will be used in List

        port_list[userID] = slice_str(client_message, getlen(client_name)+1, 200);
        online_list[userID] = 1;
        online_user_cnt++;   

        strcpy(message, "Please enter the amount you want to deposit: ");
        send(newSocket,message,sizeof(message),0);  
        memset(message, '\0', sizeof(message));


        //deposit
        memset(client_message, '\0', sizeof(client_message));
        recv(newSocket , client_message ,sizeof(client_message) , 0);
        int deposit_amount = atoi(client_message);
        memset(client_message, '\0', sizeof(client_message));

        //account message
        char money[50] = {};
        account_list[userID] = deposit_amount;
        strcpy(message, "Account balance: ");
        sprintf(money, "%d", account_list[userID]); 
        strcat(message, money);
        strcat(message, "\n");
        send(newSocket,message,sizeof(message),0);
        memset(message, '\0', sizeof(message));

        printf("%s ", user_list[userID]);
        printf("deposit amount: %d\n", account_list[userID]);

        //show list
        char list[1024] = {};
        int online_cnt = 0;
        for (int i = 0; i < usercnt; ++i)
        {

          if(online_list[i] == 1)
          {
            online_cnt++;
            char message_list[1024];
            memset(message_list, '\0', sizeof(message_list));
            // sprintf(port, "%u", ntohs(clientInfo.sin_port));
            snprintf(message_list, 1024, "%s#%s#%s\r\n", user_list[i], inet_ntoa(clientInfo.sin_addr), port_list[i]); //online list
            strcat(list, message_list);   
          }        
        }

        //online accounts
        sprintf(message, "number of accounts online: %d\n", online_cnt); 
        send(newSocket,message,sizeof(message),0);
        memset(message, 0, sizeof(message));

        //online list
        strcpy(message, list);
        send(newSocket,message,sizeof(message),0);
        memset(message, 0, sizeof(message));



        continue;

      }
      else
      {
        strcpy(message, "220 AUTH_FAIL\n");
        send(newSocket,message,sizeof(message),0);
        memset(message, 0, sizeof(message));
      }
    }  
  }
  online_list[userID] = 0;
  printf("Exit socketThread \n");
  close(newSocket);
  pthread_exit(NULL);
}
int main(int argc , char *argv[]){

  socklen_t addr_size;
  serverSocket = socket(PF_INET, SOCK_STREAM, 0);
  serverAddr.sin_family = AF_INET;
  // srand(time(NULL)); 
  // int servPort = (rand() % 2000) + 7000;
  int servPort = atoi(argv[1]);
  serverAddr.sin_port = htons(servPort);
  printf("Server port number: %d\n", servPort);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
  bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));


  if(listen(serverSocket,10)==0)
    printf("Listening\n");
  else
    printf("Listening error\n");
  pthread_t tid[10];
  int i = 0;

    while(1)
    {

        //Accept call creates a new socket for the incoming connection
        addr_size = sizeof serverStorage;
        newSocket = accept(serverSocket, (struct sockaddr *) &clientInfo, &addr_size);
        char message[] = {"Connected accepted. Welcome to Yi Chin's server.\n"};
        send(newSocket,message,sizeof(message),0);

        //for each client request creates a thread and assign the client request to it to process
        //so the main thread can entertain next request
        if( pthread_create(&tid[i], NULL, socketThread, &newSocket) != 0 )
        {
          printf("Failed to create thread\n");
          i++;
        }
        // else
        // {
        //   printf("client IP: %s\n",inet_ntoa(clientInfo.sin_addr));
        //   printf("client port: %u\n",ntohs(clientInfo.sin_port));   
        // }   
        // if( i >= 50)
        // {
        //   i = 0;
        //   while(i < 50)
        //   {
        //     pthread_join(tid[i++],NULL);
        //   }
        //   i = 0;
        // }
        
    }
  return 0;
}