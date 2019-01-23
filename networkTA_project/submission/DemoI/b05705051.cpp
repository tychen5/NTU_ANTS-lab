#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
using namespace std;


int main(int argc , char *argv[])
{
    //create a socket
    int sockfd = 0;
    char* IP = argv[1];
    char* port = argv[2];
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        cout << "Fail to create a socket.";
    }

    //the connection of the socket

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
 
    //localhost test 140.112.107.194 port 33120
    //info.sin_addr.s_addr = inet_addr("127.0.0.1");
    info.sin_addr.s_addr = inet_addr(IP);
    //info.sin_port = htons(8700);
    info.sin_port = htons(atoi(port));

    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){ // if connection error
        cout << "Connection error";
    }
    else{
        cout << "connection accepted\n";
    }

    char receiveMessage[500] = {};
    char regisSuccess[20] = {"100 OK\n"};
    char regisFail[20] = {"210 FAIL\n"};
    char logInFail[20] = {"220 AUTH_FAIL\n"};
    bool logInSuccess = false;
    char command[10] = {};
    
    //Send a message to server
    bool leaveServer = false;
    while(1){
        cout << "1 for register. 2 for log in. 3 for exit.\n";
        bzero(&command, sizeof(command));
        bzero(&receiveMessage, sizeof(receiveMessage));
        cin >> command;
        if (atoi(command) == 1){
            // register
            bzero(&command, sizeof(command));
            cout << "please enter your name.\n";
            char username[100] = {};
            cin >> username;
            char registerMessage[100] = {"REGISTER#"};
            strcat(registerMessage, username);
            send(sockfd,registerMessage,strlen(registerMessage),0);
            recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            if (strcmp(receiveMessage, regisFail) == 0){
                cout << "you have registered before.\n";
            }
            else if (strcmp(receiveMessage, regisSuccess) == 0){
                cout << "register successfully!!.\n";
            }
        }
        else if (atoi(command) == 2){
            // log in
            bzero(&command, sizeof(command));
            cout << "please enter your name.\n";
            char username[100] = {};
            cin >> username;
            cout << "please enter your port number.\n";
            cout << "your port number must >= 1024 and <= 65536.\n";
            char portNumber[10] = {};
            bool portRight = false;
            while(!portRight){
                bzero(&portNumber, sizeof(portNumber));
                cin >> portNumber;
                if (atoi(portNumber) >= 1024 && atoi(portNumber) <= 65536){
                    portRight = true;
                }
                else{
                    // port number out of range
                    cout << "your port number is out of range!!\n";
                    cout << "please type it again!!\n";
                }
            }
            char logInMessage[100] = {};
            strcat(logInMessage, username);
            strcat(logInMessage, "#");
            strcat(logInMessage, portNumber);
            send(sockfd,logInMessage,strlen(logInMessage),0);
            recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            if (strcmp(receiveMessage, logInFail) == 0){
                // 220 AUTH_FAIL
                cout << "this account has not be registered or its logging in.\n";
            }
            else{
                cout << "log in successfully!!\n";
                cout << receiveMessage << "\n";
                logInSuccess = true;
            }
        }
        else if (atoi(command) == 3){
            // exit the client can close the client
            bzero(&command, sizeof(command));
            cout << "exit the client!!\n";
            break;
        }
        else{
            // you are doing something wrong
            bzero(&command, sizeof(command));
            cout << "you are doing wrong!!\n";
        }
        if (logInSuccess){ // log in success
            cout << "now things you can do are as follows:\n";
            char receiveMessage2[500] = {};
            while(1){ // the loop while logging in success
                bzero(&receiveMessage2, sizeof(receiveMessage2));
                cout << "1 for list the users online. 2 for exit. 3 for ...\n";
                cin >> command;
                if (atoi(command) == 1){ // list
                    bzero(&command, sizeof(command));
                    char list[5] = {"List"};
                    send(sockfd,list,strlen(list),0);
                    recv(sockfd,receiveMessage2,sizeof(receiveMessage2),0);
                    cout << receiveMessage2 << "\n";
                }
                else if (atoi(command) == 2){ // exit
                    bzero(&command, sizeof(command));
                    logInSuccess = false;
                    char exit[5] = {"Exit"};
                    send(sockfd,exit,strlen(exit),0);
                    recv(sockfd,receiveMessage2,sizeof(receiveMessage2),0);
                    cout << receiveMessage2 << "\n";
                    cout << "now you leave the server!!\n";
                    leaveServer = true;
                    break;
                }
            }
        }
        if (leaveServer){
            break;
        }
    }
    close(sockfd);
    return 0;
}