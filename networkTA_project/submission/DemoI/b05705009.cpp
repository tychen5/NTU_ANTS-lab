#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <string>
#include <arpa/inet.h>



using namespace std;
//#define LEN 9
char sendMessage[100];
char receiveMessage[100] = {};
int n = 0;

int main(int argc , char *argv[])
{
    cout << "plz enter ur ip & port" << endl;
    //socket的建立

    string urIp;
    int urPort;

    cin >> urIp;
    cin >> urPort;
    cout << endl;
    
    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;

    
    info.sin_addr.s_addr = inet_addr(urIp.c_str());
    info.sin_port = htons(urPort);
    // info.sin_addr.s_addr = inet_addr("140.112.107.194");
    // info.sin_port = htons(33120);

    int sockfd = 0;
    sockfd = socket(PF_INET , SOCK_STREAM , 0);
    if (sockfd == -1){
        printf("Fail to create a socket.");
        cout << endl;
    }

    
    //socket的連線
    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        printf("Connection error");
        exit(0);
    }
    
    recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
    cout << receiveMessage;
    //Send a message to server
    while(true){
        cout << "Enter 1 for Register, 2 for Login: ";
        string opt;
        cin >> opt;

        //register
        if(opt == "1"){
            bzero(&receiveMessage,sizeof(receiveMessage));
            cout << "Please write up ur RegisterID : ";
            string input = "REGISTER#";
            string IDname;
            cin >> IDname;
            input += IDname;
            send(sockfd,input.c_str(),input.length(),0);
            recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            cout << receiveMessage << endl;
        }
        else if(opt == "2"){
            //bzero(&sendMessage,sizeof(sendMessage));
            bzero(&receiveMessage,sizeof(receiveMessage));
            cout << "Give me your ID plz : ";
            string IDname;
            cin >> IDname;
            cout << "How about your port : ";
            string portNum;
            cin >> portNum;
            string input;
            input = IDname + "#" + portNum;
            send(sockfd,input.c_str(),input.length(),0);
            int n = recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            cout << receiveMessage << endl;

            while(strcmp(receiveMessage, "220 AUTH_FAIL\n")!=0)
            {
                string cmd;
                bzero(&receiveMessage,sizeof(receiveMessage));
                cout << "Enter the number of commands you want to take.\n";
                cout << "1 for the latest list, 8 to Exit: ";
                cin >> cmd;
                if(cmd == "1"){
                    string msg= "List";
                    bzero(&sendMessage,sizeof(sendMessage));
                    send(sockfd, msg.c_str(), msg.size(), 0);
                    cout << endl;
                    cout << "=========================="<< endl << endl;
                    int n = recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
                    cout <<receiveMessage << endl;
                    cout << "==========================" << endl;
                }

                else if (cmd == "8"){

                    string exitt = "Exit";

                    send(sockfd, exitt.c_str(), exitt.size(), 0);
                    //bzero(&receiveMessage,sizeof(receiveMessage));
                    int n = recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
                    cout << receiveMessage;
                    close(sockfd);
                    exit(0);
                    
                }
            }
        }
    }   
    
    return 0;
}