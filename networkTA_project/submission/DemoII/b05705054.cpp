#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <queue>
#include <string>

using namespace std;


int sockfd = 0,forClientSockfd = 0;
char clientMessage[10000]={0}; 
char clientName[200][200]={0};
char clientIP[200][200]={0};
int clientNum = 0;
char clientPort[200][200]={0};
int clientBalance[200]={0};
int clientOnline[200] = {0};
string list();

queue<int> client_sock_queue;
pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_thread = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_clientName = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_clientIP = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_clientPort = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_clientBalance = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_clientNum = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_online = PTHREAD_MUTEX_INITIALIZER;

string list(){
    string msg;
    int onlineNum = 0;
    pthread_mutex_lock(&mutex_online);
    for (int i = 0; i < 200; i++){
        if(clientOnline[i] == 1){
            msg += clientName[i];
            msg += "#";
            msg += clientIP[i];
            msg += "#";
            msg += clientPort[i];
            msg += "\n";
        }
    }
    pthread_mutex_unlock(&mutex_online);
    return msg;
}


void* threadDealer(void * socket_desc){
    while(true){
        int forClientSockfd = -1 ;
        pthread_mutex_lock(&mutex_thread);
        if (client_sock_queue.size() <= 0){
            pthread_mutex_unlock(&mutex_thread);
            continue;
        }
        forClientSockfd = client_sock_queue.front();
        client_sock_queue.pop();
        pthread_mutex_unlock(&mutex_thread);

        char useronline[200] = {0};

        //queue is empty
        if (forClientSockfd <= 0){
            continue;
        }
        char inputBuffer[10000] = {0};

        while(recv(forClientSockfd,inputBuffer,sizeof(inputBuffer),0) > 0){
            puts("recv message from client");
            puts(inputBuffer);

            const char delim[2] = "#";
            char *token;
            token = strtok(inputBuffer, delim);

            struct sockaddr_in news;
            socklen_t newslen = sizeof(news);
            getpeername(forClientSockfd, (struct sockaddr*) &news, &newslen);

            if(strcmp(token, "REGISTER") == 0){
                token = strtok(NULL, delim);
                int accountExist = 0;
                for(int i = 0; i < 200; i ++){
                    pthread_mutex_lock(&mutex_clientName);
                    if(strcmp(token, clientName[i]) == 0){
                        pthread_mutex_unlock(&mutex_clientName);
                        accountExist = 1;
                        break;
                    }
                    pthread_mutex_unlock(&mutex_clientName);
                }
                if(accountExist == 1){
                    char msg[10] = "210 FAIL";
                    send(forClientSockfd,msg,strlen(msg),0);
                }
                else{
                    pthread_mutex_lock(&mutex_clientName);
                    pthread_mutex_lock(&mutex_clientNum);
                    strcpy(clientName[clientNum], token);
                    pthread_mutex_unlock(&mutex_clientName);
                    pthread_mutex_unlock(&mutex_clientNum);

                    int ba = atoi(strtok(NULL, delim));
                    pthread_mutex_lock(&mutex_clientBalance);
                    pthread_mutex_lock(&mutex_clientNum);
                    clientBalance[clientNum] = ba;
                    pthread_mutex_unlock(&mutex_clientBalance);
                    pthread_mutex_unlock(&mutex_clientNum);

                    char msg[10] = "100 OK\n";
                    send(forClientSockfd,msg,strlen(msg),0);
                    pthread_mutex_lock(&mutex_clientNum);
                    clientNum += 1;
                    pthread_mutex_unlock(&mutex_clientNum);
                }
                
                
            }
            else if(strcmp(token, "List") == 0){
                string clientlist;
                clientlist = list();
                send(forClientSockfd,clientlist.c_str(),clientlist.length(),0);
            }
            else if(strcmp(token, "Exit") == 0){
                for(int i = 0; i < 200; i++){
                    pthread_mutex_lock(&mutex_clientName);
                    pthread_mutex_lock(&mutex_online);
                    if(strcmp(useronline, clientName[i]) == 0){
                        clientOnline[i] = 0;
                    }
                    pthread_mutex_unlock(&mutex_online);
                    pthread_mutex_unlock(&mutex_clientName);
                    
                }

                char msg[] = "Bye\n";
                send(forClientSockfd,msg,strlen(msg),0);
                forClientSockfd = -1;
                break;
            }
            else{
                puts("i receive login");
                int clientFound = 0;
                for(int i = 0; i <= 200; i++){
                    pthread_mutex_lock(&mutex_clientName);
                    if(strcmp(token, clientName[i]) == 0){
                        clientFound = 1;
                    }
                    pthread_mutex_unlock(&mutex_clientName);
                }
                if(clientFound == 0){
                    char msg[100] = "220 AUTH_FAIL\n";
                    send(forClientSockfd,msg,strlen(msg),0);
                }
                else{
                    for(int i = 0; i <= 200; i++){
                        pthread_mutex_lock(&mutex_clientName);
                        bzero(useronline,sizeof(useronline));
                        strcpy(useronline, clientName[i]);
                        pthread_mutex_unlock(&mutex_clientName);

                        if(strcmp(token, useronline) == 0){
                            puts("find the user");
                            pthread_mutex_lock(&mutex_online);
                            clientOnline[i] = 1;
                            pthread_mutex_unlock(&mutex_online);
                            
                            char balancemsg[10000];
                            pthread_mutex_lock(&mutex_clientBalance);
                            sprintf(balancemsg, "%d", clientBalance[i]); 
                            pthread_mutex_unlock(&mutex_clientBalance);

                            strcat(balancemsg, "\n");
                            pthread_mutex_lock(&mutex_clientIP);
                            strcpy(clientIP[i], inet_ntoa(news.sin_addr));
                            pthread_mutex_unlock(&mutex_clientIP);

                            token = strtok(NULL, delim);
                            pthread_mutex_lock(&mutex_clientPort);
                            strcpy(clientPort[i],token);
                            pthread_mutex_unlock(&mutex_clientPort);


                            string clientlist (balancemsg);
                            clientlist += list();
                            send(forClientSockfd,clientlist.c_str(),clientlist.length(),0);
                            break;
                        }
                    }                
                }  
            }
            bzero(inputBuffer, sizeof(inputBuffer));  
        }
        
    }
}

int main(int argc , char *argv[]){
    //socket的建立

    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket的連線
    struct sockaddr_in serverInfo,clientInfo;
    socklen_t addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(8700);
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sockfd,5);

    pthread_t *client_thread;
    client_thread = new pthread_t[5];
    for (int i = 0; i < 5; i++){
        if (pthread_create(&client_thread[i], NULL, threadDealer, nullptr) < 0){
            return -1;
        }
    }
    while(true){
        forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);
        // pthread_mutex_lock(&mutex_queue);
        client_sock_queue.push(forClientSockfd);
        // pthread_mutex_unlock(&mutex_queue);
    }
    
    
    
    return 0;
}
