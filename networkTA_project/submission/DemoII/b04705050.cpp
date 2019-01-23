#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<string>
#include <iostream>
#include <string.h>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;
void *task1(void *);
static int newsockfd;
vector <string> usernames;
vector <string> userports;
vector <string> ip;
int usersOnline = 0;
char Info[100] = {};
int main(int argc , char *argv[])
{
    //socket的建立
    int sockfd = 0;
    struct sockaddr cli_addr;
    socklen_t clilen;
    pthread_t threadA[3];
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket的連線
    struct sockaddr_in serverInfo,clientInfo;
    int addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));
    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(8700);
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sockfd,5);

    //vector <string> usernames; 
    int noThread = 0;
    while(noThread < 3){
        cout<<"Listening"<<endl;
        newsockfd = accept(sockfd, &cli_addr, &clilen);
        if (newsockfd < 0){
            cout<<"Cannot connect."<<endl;
        }
        else{
            cout<<"Successfully connected. "<<endl;
            struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&cli_addr;
            struct in_addr ipAddr = pV4Addr->sin_addr;
            char str[100];
            inet_ntop(AF_INET, &ipAddr, str, 100);
            ip.push_back(str);   
        }
        pthread_create(&threadA[noThread], NULL, task1, NULL); 
        noThread++;
    }
    for(int i = 0; i < 3; i++){
        pthread_join(threadA[i], NULL);
    }
    return 0;
}

void *task1 (void *arg)
{
    int thread_socket = newsockfd;
    cout << "Thread No: " << pthread_self() << endl;
    bool loop = false;
    bool registered = false;
    bool login = false;
    char inputBuffer[256] = {};
    char message[] = {"Hi,this is server.\n"};
    string name = "";
    string inputString (inputBuffer);
    
    while(!loop)
    {    
        if (registered == false && login == false){//first connection
            send(thread_socket,message,sizeof(message),0);
        }
        recv(thread_socket,inputBuffer,sizeof(inputBuffer),0);
        string input_str(inputBuffer);
        string delimiter = "#";
        string command = input_str.substr(0, input_str.find(delimiter));
        printf("Recieve from client: %s\n",inputBuffer);
        if (command == "REGISTER"){//REGISTER#jess
            char registerResponse[100] = {"100 OK"};
            send(thread_socket,registerResponse,sizeof(registerResponse),0);
            name = input_str.erase(0,9);
            registered = true;
        }
        else if(command == "Exit"){
            send(thread_socket,"Bye \n",100,0);
            int index = -1;
            for(int i = 0; i<usersOnline; i++){
                if (usernames[i] == name){
                    index = i;
                    break;
                }
            }
            usernames.erase (usernames.begin()+index);
            userports.erase(userports.begin()+index);
            ip.erase(ip.begin()+index);
            ::usersOnline--;

            char onlineInfo[100] = {"number of accounts online: "};
            char userInfo[100] = {""};
            for (int i = 0; i < usersOnline;i++){
                const char *name_c = usernames[i].c_str();
                strcat(userInfo,name_c);
                strcat(userInfo,"#");
                const char *ip_c = ip[i].c_str();
                strcat(userInfo,ip_c);
                strcat(userInfo,"#");
                const char *port_c = userports[i].c_str();
                strcat(userInfo,port_c);
                strcat(userInfo,"\n"); 
            }
            char usersOnline_s[10];
            sprintf(usersOnline_s, "%d", usersOnline);
            strcat(onlineInfo,usersOnline_s);//number of accounts online: 3
            strcat(onlineInfo,"\n");
            strcat(onlineInfo,userInfo);//name#IP#port
            strcpy(Info,onlineInfo);
            break;
        }     
        else {
            if (login == false){
                if (command == name && registered == true){//jess#11111
                    string port = input_str.erase(0,name.length()+1);
                    ::usersOnline++;
                    ::usernames.push_back(name);
                    ::userports.push_back(port);
                    char userInfo[100] = {""};
                    for (int i = 0; i < usersOnline;i++){
                        cout<<"users: "<<usernames[i]<<endl;
                        cout<<"portsss: "<<userports[i]<<endl;
                        const char *name_c = usernames[i].c_str();
                        strcat(userInfo,name_c);
                        strcat(userInfo,"#");
                        const char *ip_c = ip[i].c_str();
                        strcat(userInfo,ip_c);
                        strcat(userInfo,"#");
                        const char *port_c = userports[i].c_str();
                        strcat(userInfo,port_c);
                        strcat(userInfo,"\n"); 
                    }
                    send(thread_socket,"10000 \n",sizeof("10000 \n"),0);//account balance
                    char onlineInfo[100] = {"number of accounts online: "};
                    char usersOnline_s[10];
                    sprintf(usersOnline_s, "%d", usersOnline);
                    strcat(onlineInfo,usersOnline_s);//number of accounts online: 3
                    strcat(onlineInfo,"\n");
                    strcat(onlineInfo,userInfo);//name#IP#port
                    send(thread_socket,onlineInfo,sizeof(onlineInfo),0);
                    strcpy(Info,onlineInfo);
                    cout<<"onlineInfo: "<<onlineInfo<<endl;
                    login = true;
                }
                else{
                    cout<<"User not correctly registered..."<<endl;
                    char registerResponse[100] = {"220 AUTH_FAIL"};
                    send(thread_socket,registerResponse,sizeof(registerResponse),0);   
                }  
            }
            else{
                if (command == "List"){
                    send(thread_socket,Info,sizeof(Info),0);
                }
                else{
                    cout<<"Invalid input"<<endl;
                }
            }
        }
    }
    send(thread_socket,"Bye \n",sizeof("Bye \n"),0);
    cout << "\nClosing thread and conn" << endl;
    close(thread_socket);
    return 0;
    
}