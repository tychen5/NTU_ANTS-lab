#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <arpa/inet.h>
using namespace std;

/*struct sockaddr_in {
    short            sin_family;   // AF_INET,因為這是IPv4;
    unsigned short   sin_port;     // 儲存port No
    struct in_addr   sin_addr;     // 參見struct in_addr
    char             sin_zero[8];  // Not used, must be zero 
};

struct in_addr {
    unsigned long s_addr;          // load with inet_pton()
};*/

int main(int argc , char *argv[])
{

    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        cout<<"Fail to create a socket.";
    }

    //socket的連線

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;

    //localhost test
    //info.sin_addr.s_addr = inet_addr("127.0.0.1");
    info.sin_addr.s_addr = inet_addr("140.112.106.45");
    info.sin_port = htons(33220);
    //info.sin_addr.s_addr = inet_addr("140.112.107.194");
    //info.sin_port = htons(33120);
    //050705070507050705070507050705007

    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        cout<<"Connection error";
    }


    //Send a message to server
    
    char receiveMessage[10000] = {};
    /*
    recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
    cout<<receiveMessage;*/
    cout<<"R for REGISTER. "<<"LOG for LOGIN. "<<"bye for EXIT. "<<"\n";
    while(1){
        bzero(&receiveMessage,sizeof(receiveMessage));
        char message[100] = {};

        scanf("%s",message);
        if(strncmp(message,"R",1) == 0){
            cout<<"enter your name!!"<<"\n";
            char name[40]={};
            scanf("%s",name);
            char REGISTER[100]={"REGISTER#"};
            strcat(REGISTER,name);
            send(sockfd,REGISTER,strlen(REGISTER),0);
            recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            if(strncmp(receiveMessage,"100 OK",6) == 0){
                cout<<"REGISTER sucessfull"<<"\n";
            }
            else{
                cout<<"There is a same account name"<<"\n";
            }
        }
        if(strncmp(message,"LOG",3) == 0){
            cout<<"enter your name!!"<<"\n";
            char LOGIN[40]={};
            scanf("%s",LOGIN);
            cout<<"enter your portnumber!!"<<"\n";
            char port[10]={};
            scanf("%s",port);
            strcat(LOGIN,"#");
            strcat(LOGIN,port);
            send(sockfd,LOGIN,strlen(LOGIN),0);
            recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            if(strncmp(receiveMessage,"220 AUTH_FAIL",13) == 0){
                cout<<"LOG IN Fail"<<"\n";
            }
            else{
                cout << "LOG IN SUCESS"<<"\n";
                cout << receiveMessage<<"\n";

                char LIST[40]={};
                scanf("%s",LIST);
                if(strncmp(LIST,"List",4)==0){
                    bzero(&receiveMessage,sizeof(receiveMessage));
                    send(sockfd,LIST,strlen(LIST),0);
                    recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
                    cout<<receiveMessage<<"\n";
                }

            }
        }
        if(strncmp(message,"bye",3) == 0){
            char EXIT[10]={"Exit"};
            send(sockfd,EXIT,strlen(EXIT),0);
            recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            cout<<receiveMessage;
            break;
        }

        //send(sockfd,message,strlen(message),0);
        //recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
        //cout<<receiveMessage;
    }
    
    cout<<"close Socket"<<'\n';
    close(sockfd);
    return 0;
}
