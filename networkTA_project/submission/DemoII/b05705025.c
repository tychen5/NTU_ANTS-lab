#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "threadpool.h"

#define ser_ip "127.0.0.1"
#define ser_port 3698
#define max_thread_size 20  //initializing maximun size of pools

typedef struct ClientInfo
{
    struct sockaddr_in addr;
    int clientfd;
    int isConn;
    int index;
} ClientInfo;

/*global variable*/
char user[50][100] = {};
char user_port[50][6] = {};
char user_addr[50][20] = {};
char user_dep[50][15] = {};
int user_login[50] = {0};
int userCnt = 0;
int onlineCnt = 0;

/*
    server jobs handling
 */
void* serverWork(void* argv)
{
    ClientInfo *client = (ClientInfo *)(argv);
    int clientfd = client->clientfd;
    struct sockaddr_in addr = client->addr;
    char message[] = {"connection successful.\n"};
    char *delim = "#";
    int login = 0;
    int userId = 0;
    char inputBuffer[256] = {};
    send(clientfd,message,strlen(message),0);

    while(1){
        bzero(inputBuffer, sizeof(inputBuffer));

        if(recv(clientfd,inputBuffer,sizeof(inputBuffer),0) == 0){
            //printf("this is input buffer1: %s\n", inputBuffer );
            if(user_login[userId] == 1){
                onlineCnt --;
                printf("%s", user[userId]);
                printf("%s\n", "log out");
                printf("%d\n", userId);
                user_login[userId] = 0;
                close(client->clientfd);
                break;
            }
        }
        else{
        //printf("this is input buffer2: %s\n", inputBuffer );       
            char output[256] = {};
            if(login == 0){
                char *spl;
                spl = strtok(inputBuffer, delim);
                if(strcmp(spl, "REGISTER") == 0){
                    int repeat = 0;
                    spl = strtok (NULL, delim);
                    for(int i = 0; i <= userCnt; i++){
                        if(strcmp(user[i], spl) == 0){
                            send(clientfd, "repeated user number", strlen("repeated user number"), 0);
                            repeat = 1;
                            break;
                        }
                    }
                    if(repeat == 0){
                        char money[15] = {};
                        char dep_message[100] = {};
                        strcpy(user[userCnt], spl);
                        strcpy(output, user[userCnt]);
                        send(clientfd, "100 OK", strlen("100 OK"), 0);
                        userCnt++;
                        if(recv(clientfd,money,sizeof(money),0) != 0){
                            strcpy(dep_message, "your deposit amount is: ");
                            strcat(dep_message, money);
                            send(clientfd, dep_message, strlen(dep_message), 0);
                            strcpy(user_dep[userCnt-1], money); 
                        }
                   }
                }

                else if(spl != NULL){
                    int identify = 0;
                    for(int i = 0; i < userCnt; i++){
                        if(strcmp(spl, user[i]) == 0){
                            identify = 1;
                            userId = i;
                            break;
                        }
                    }
                    if(identify == 0){
                        strcpy(output, "220 AUTH_FAIL");
                        send(clientfd, output, strlen(output), 0);
                        //printf("%s\n", output);
                    }
                    else{
                        spl = strtok(NULL, delim);
                        int port = atoi(spl);
                        if(port > 1024 && port <= 69999){
                            user_login[userId] = 1;
                            onlineCnt++;
                            strcpy(user_port[userId], spl);
                            printf("%s\n", spl);
                            printf("%s\n", user_port[userId]);
                            strcpy(output, user_dep[userId]);
                            send(clientfd, output, strlen(output), 0);
                            //printf("%s\n", output);
                            login = 1;
                            strcpy(user_addr[userId], inet_ntoa(addr.sin_addr));
                            /*handle list*/
                            char num_of_user[5] = {};
                            char list[200] = {};
                            sprintf(num_of_user, "%d", onlineCnt);
                            strcpy(list, "number of accounclient online: ");
                            strcat(list, num_of_user);
                            strcat(list, "\n");
                            for(int i = 0; i < userCnt; i++){
                                if(user_login[i] == 1){
                                    strcat(list, user[i]);
                                    strcat(list,"#");
                                    strcat(list, inet_ntoa(addr.sin_addr));
                                    strcat(list,"#");
                                    strcat(list,user_port[i]);
                                    strcat(list, "\n");
                                }
                            }
                            send(clientfd, list, strlen(list), 0);
                        }
                        else{
                            strcpy(output, "port num out of range, please re-assign.\n");
                            send(clientfd, output, strlen(output), 0);
                        }
                    }
                }
            }
            else{
                if(strcmp(inputBuffer, "Exit") == 0){
                    strcpy(output, "Bye\n");
                    send(clientfd, output, strlen(output), 0);
                }
                else if(strcmp(inputBuffer, "List") == 0){
                        char num_of_user[5] = {};
                        char list[200] = {};
                        sprintf(num_of_user, "%d", onlineCnt);
                        strcpy(list, "number of client online: ");
                        strcat(list, num_of_user);
                        strcat(list, "\n");
                        for(int i = 0; i < userCnt; i++){
                            if(user_login[i] == 1){
                                strcat(list, user[i]);
                                strcat(list,"#");
                                strcat(list, inet_ntoa(addr.sin_addr));
                                strcat(list,"#");
                                strcat(list,user_port[i]);
                                strcat(list, "\n");
                            }
                        }
                        send(clientfd, list, strlen(list), 0);
                }
                else{
                    strcpy(output, "What are u doing?");
                    send(clientfd, output, strlen(output), 0);
                }
            }
        }
    }
    return NULL;
}

int main()
{
    struct sockaddr_in ser_addr,client_addr;
    int serv_sock,client_sock;
    socklen_t client_sz; 
    thread_pool * pool;
    struct ClientInfo clients[256];
    int i = 0;

    pool = (thread_pool*)malloc(max_thread_size*sizeof(thread_pool));

    serv_sock = socket(AF_INET,SOCK_STREAM,0);
    bzero(&ser_addr,sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ser_addr.sin_port = htons(ser_port);

    thread_pool_init(pool,max_thread_size);
    
    if(bind(serv_sock,(struct sockaddr*)& ser_addr,sizeof(ser_addr)) == -1)
    {
        perror("bind() ");
        exit(1);
    }

    if(listen(serv_sock,20) == -1)
    {
        perror("listen() ");
        exit(1);
    }

    //while(int j = 0;j<max_thread_size;j++)
    while(1)
    {
        client_sz = sizeof(client_addr);
        client_sock = accept(serv_sock,(struct sockaddr*)& client_addr,&client_sz);
        clients[i].clientfd = client_sock;
        clients[i].addr = client_addr;

        //deal with server's work and assign to each pool
        threadpool_add_runner(pool,serverWork,(void*)&clients[i]);
        i++;
    }
    return 0;
}