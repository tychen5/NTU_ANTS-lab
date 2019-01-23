#include <string.h>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <deque>
#include "threadPool.h"
#include <pthread.h>
using namespace std;

char userAccountRegistered[100][20];
char userOnline[100][50];
int balance[100];
int countRegistered = 0;
int countLogIn = 0;
int sockfd = 0;
int TOTALTHREAD = 10;

int lookForBalanceNum(char name[]);
char* list(char nowUser[]);
char* regis(char inputBuffer[]);
char* logIn(char inputBuffer[], char clientIP[]);
char* getNowUser(char inputBuffer[]);
int main(int argc , char *argv[])
{
    //pthread_t tid[TOTALTHREAD];       /* the thread identifier */
    //pthread_attr_t attr; /* set of attributes for the thread */
    ThreadPool pool(TOTALTHREAD);
    //int sockfd = 0, forClientSockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket的連線
    //struct sockaddr_in serverInfo,clientInfo;
    struct sockaddr_in serverInfo, clientInfo;
    socklen_t addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(8877);
    size_t bn = ::bind(sockfd,(struct sockaddr *)&serverInfo, sizeof(serverInfo));
    if (bn < 0) {
        cerr << "Bind error: " << errno << endl; exit(-1);
    }

    listen(sockfd,TOTALTHREAD);
    
    while(1){
        // struct sockaddr_in clientInfo;
        // socklen_t addrlen = sizeof(clientInfo);
        int forClientSockfd = 0;
        forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);
        pool.enqueue([&](int forClientSockfd){
        while(1){
            cout << "client" << forClientSockfd << "accepted" << endl;
            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientInfo.sin_addr), clientIP, INET_ADDRSTRLEN);
            char inputBuffer[256] = {'\0'};
            char nowUser[20]; 
            while(1){
                bzero(&inputBuffer, sizeof(inputBuffer));
                int r = recv(forClientSockfd,inputBuffer,sizeof(inputBuffer),0);
                if (r == -1) {
                    cerr << "recv error, errno: " << errno << endl; return;
                }
                if (r == 0){
                    cout << "client" << forClientSockfd << "shut down" << endl;
                    char tempName[20];
                    char tempOnlineInfo[100];
                    for (int i = 0; i < 100; i++){
                        strcpy(tempOnlineInfo, userOnline[i]);
                        int tempNamelen = 0;
                        for (int j = 0; j < strlen(tempOnlineInfo); j++){
                            if (tempOnlineInfo[j] == ' '){
                                tempNamelen = j;
                                    break;
                            }
                            else{
                                tempName[j] = tempOnlineInfo[j];
                            }
                        }
                        tempName[tempNamelen] = '\0';
                        if (strcmp(tempName, nowUser) == 0){
                            cout << "delete" << endl;
                            bzero(&userOnline[i],sizeof(userOnline[i]));
                            break;
                        }
                        bzero(&tempName,sizeof(tempName));
                        bzero(&tempOnlineInfo,sizeof(tempOnlineInfo));
                    }
                    countLogIn -= 1;
                    return;
                }
                else if (r != 0){
                    if (strcmp(inputBuffer, "List") == 0){
                        cout << "Server receive command: List" << endl;
                        send(forClientSockfd,list(nowUser),strlen(list(nowUser)),0);
                    }
                    // user register
                    else if (strncmp(inputBuffer, "REGISTER#", 9) == 0){
                        cout << "Server receive command: REGISTER" << endl;
                        char goal[20] = {};
                        strcpy(goal, regis(inputBuffer));
                        send(forClientSockfd,goal,strlen(goal),0);
                        cout << "finishREGISSend" << endl;
                    }
                    // end register
                    else if (strncmp(inputBuffer, "Exit", 4) == 0){
                        cout << "Server receive command: Exit" << endl;
                        char bye[] = {"Bye\n"};
                        //cout << "now leaving : " << nowUser << endl;
                        char tempName[20];
                        char tempOnlineInfo[100];
                        for (int i = 0; i < 100; i++){
                            strcpy(tempOnlineInfo, userOnline[i]);
                            int tempNamelen = 0;
                            for (int j = 0; j < strlen(tempOnlineInfo); j++){
                                if (tempOnlineInfo[j] == ' '){
                                    tempNamelen = j;
                                    break;
                                }
                                else{
                                    tempName[j] = tempOnlineInfo[j];
                                }
                            }
                            tempName[tempNamelen] = '\0';
                            if (strcmp(tempName, nowUser) == 0){
                                bzero(&userOnline[i],sizeof(userOnline[i]));
                                break;
                            }
                            bzero(&tempName,sizeof(tempName));
                            bzero(&tempOnlineInfo,sizeof(tempOnlineInfo));
                        }
                        countLogIn -= 1;
                        send(forClientSockfd,bye,strlen(bye),0);
                        return;
                    }
                
                    else{ // log
                        cout << "Server receive command: log" << endl;
                        char goal[20] = {};
                        strcpy(nowUser, getNowUser(inputBuffer));
                        strcpy(goal, logIn(inputBuffer, clientIP));
                        send(forClientSockfd, goal, strlen(goal), 0);
                    }
                }
            }
        }
        }, forClientSockfd);
    }
    return 0;
}

int lookForBalanceNum(char name[]){
    for (int i = 0; i < countRegistered; i++){
        if (strncmp(name, userAccountRegistered[i], strlen(name)) == 0){
            return(balance[i]);
        }
    }
    return 0;
}
char* list(char nowUser[]){
    char* listReturn = new char[1000];
    char buffer[10] = {};
    int userBalance = lookForBalanceNum(nowUser);
    sprintf(buffer, "%d", userBalance);
    strncat(listReturn, buffer, strlen(buffer));
    strcat(listReturn, "\n");
    for (int i = 0; i < 100; i++){
        if (strcmp(userOnline[i], "") != 0)
            strcat(listReturn, userOnline[i]);
    }
    return listReturn;
}
char* regis(char inputBuffer[]){
    char temp[20];
    for (int i = 9; i < strlen(inputBuffer); i++){
        temp[i - 9] = inputBuffer[i];
    }
    temp[strlen(inputBuffer) - 9] = '\0';
    bool hasRegistered = false;
    for (int i = 0; i < countRegistered; i++){
        if (strcmp(userAccountRegistered[i], temp) == 0){
            hasRegistered = true;
        }
    }
    if (!hasRegistered){
        strcpy(userAccountRegistered[countRegistered], temp);
        balance[countRegistered] = 10000;
        countRegistered += 1;
    }
    char* successRegis = new char[20];
    char* failRegis = new char[20];
    strcpy(successRegis, "100 OK\n");
    strcpy(failRegis, "210 FAIL\n");
    if (!hasRegistered){
        cout << "sucRegis" << endl;
        return successRegis;
    }
    else{
        cout << "failRegis" << endl;
        return failRegis;
    }
}
char* logIn(char inputBuffer[], char clientIP[]){
    char* failLogIn = new char[20];
    strcpy(failLogIn, "220 AUTH_FAIL\n");
    int flag = 0;
    bool afterSharp = false;
    char temp1[20];
    char clientPort[10];
    int lengthName = 0;
    int lengthPort = 0;
    char tempUserOnline[1000];
    for (int i = 0; i < strlen(inputBuffer); i++){
        if (strncmp(&inputBuffer[i], "#", 1) == 0){
            afterSharp = true;
            continue;
        }
        if (!afterSharp){
            temp1[i] = inputBuffer[i];
            lengthName += 1;
        }
        else{
            clientPort[lengthPort] = inputBuffer[i];
            lengthPort += 1;
        }
    }
    for (int i = 0; i < countRegistered; i++){ //check whether the user registered
        if (strncmp(userAccountRegistered[i], temp1, lengthName) == 0){
            flag = 1;
        }
    }
    for (int i = 0; i < 100; i++){ //check whether the user is logging in now
        if (strncmp(userOnline[i], temp1, lengthName) == 0){
            cout << "there are 220_AUTH_FAIL generate" << endl;
            flag = 0;
            break;
        }
    }
    temp1[lengthName] = '\0';
    clientPort[lengthPort] = '\0';
    if (flag == 1){
        strcat(tempUserOnline, temp1);
        strcat(tempUserOnline, " ");
        strcat(tempUserOnline, clientIP);
        strcat(tempUserOnline, " ");
        strcat(tempUserOnline, clientPort);
        strcat(tempUserOnline, "\n");
        for (int i = 0; i < 100; i++){
            if (strcmp(userOnline[i], "") == 0){
                cout << "success" << endl;
                strcat(userOnline[i], tempUserOnline);
                countLogIn += 1;
                break;
            }
        }
        //strcat(userOnline[countLogIn], tempUserOnline);
        char* listReturn = new char[1000];
        char buffer[10] = {};
        int userBalance = lookForBalanceNum(temp1);
        sprintf(buffer, "%d", userBalance);
        strncat(listReturn, buffer, strlen(buffer));
        strcat(listReturn, "\n");
        for (int i = 0; i < 100; i++){
            if (strcmp(userOnline[i], "") != 0){
                strcat(listReturn, userOnline[i]);
            }
        }
        return listReturn;
    }
    else
        return failLogIn;
}
char* getNowUser(char inputBuffer[]){
    char* temp1 = new char[20];
    int lengthName = 0;
    for (int i = 0; i < strlen(inputBuffer); i++){
        if (strncmp(&inputBuffer[i], "#", 1) == 0){
            break;
        }
        else{
            temp1[i] = inputBuffer[i];
            lengthName += 1;
        }
    }
    temp1[lengthName] = '\0';
    return temp1;
}