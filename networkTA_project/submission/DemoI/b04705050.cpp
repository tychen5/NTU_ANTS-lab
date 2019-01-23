#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<string>
#include <iostream>

using namespace std;
int main(int argc , char *argv[])
{
	//socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket的連線
    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr("140.112.107.194");//server ip address
    info.sin_port = htons(33120);
    
    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        cout<<"Connection error";
        return 0;
    }
    else{
        cout<<"successfully connected"<<"\n";
    }
    //Send connection message to server
    char connectMessage[100] = {};
    recv(sockfd,connectMessage,sizeof(connectMessage),0);

    char message[100] = {}; 
    char name[100] = {};
    char port[10] = {};
    bool Login = false;
    bool Register = false;
    
    while(Login == false){
        cout<<"Enter 1 for register, 2 for login: ";
        cin>>message;
        string s(message);
        if (s.compare("1") == 0){
            cout<<"Enter the name you want to register: ";
            cin>>name;
            char registerResponse[100] = {};
            char registerMessage[100] = {"REGISTER#"};
            strcat(registerMessage, name);
            send(sockfd,registerMessage,sizeof(registerMessage, name),0);
            recv(sockfd,registerResponse,sizeof(registerResponse),0);
            printf("%s",registerResponse);
            cout<<"\n";
            Register = true;
        }
        else if (s.compare("2") == 0){
            if (Register == false){
                cout<<"You need to register first."<<"\n"<<"\n";
            }
            else{
                char loginName[100] = {};
                cout<<"Enter your name: ";
                cin>>loginName;
                cout<<"Enter the port number: ";
                cin>>port;
                if (std::stoi(port) < 1024 || std::stoi(port) > 65535){
                    cout<<"Please enter a port number between 1024 and 65535."<<"\n"<<"\n";
                }
                else{
                    string str_name(name);
                    string str_loginName(loginName);
                    char loginMessage[100] = {"#"};
                    strcat(loginName, loginMessage);
                    strcat(loginName, port);
                    send(sockfd,loginName,sizeof(loginName),0);
                    char loginResponse1[100] = {};
                    char loginResponse2[100] = {};
                    
                    if (str_name == str_loginName){
                        recv(sockfd,loginResponse1,sizeof(loginResponse1),0);
                        recv(sockfd,loginResponse2,sizeof(loginResponse2),0);
                        printf("%s",loginResponse1);
                        printf("%s",loginResponse2);
                        cout<<"\n"<<"\n";
                        Login = true;
                        break;
                    }
                    else{
                        cout<<"You didn't register that name."<<"\n";
                        recv(sockfd,loginResponse1,sizeof(loginResponse1),0);
                        printf("%s",loginResponse1);
                        cout<<"\n";
                    } 
                }
            }
        }
        else{
            cout<<"Invalid input."<<"\n"<<"\n";
        }
    }
    
    while (Login == true){
        cout<<"Enter the action you want to take. 1 to ask for latest list, 8 to exit: ";
        memset(message, 0, 100);
        cin>>message;
        string s2(message);
        if (s2.compare("1") == 0){
            char listMessage[100] = {"List"};
            char listResponse1[100] = {};
            char listResponse2[100] = {};
            send(sockfd,listMessage,sizeof(listMessage),0);
            recv(sockfd,listResponse1,sizeof(listResponse1),0);
            printf("%s",listResponse1);
            cout<<"\n";
        }
        else if(s2.compare("8") == 0){
            char exitMessage[100] = {"Exit"};
            char exitResponse[100] = {};
            send(sockfd,exitMessage,sizeof(exitMessage),0);
            recv(sockfd,exitResponse,sizeof(exitResponse),0);
            cout<<"Bye"<<"\n";
            printf("%s",exitResponse);
            break;
        }
        else{
            cout<<"Invalid input."<<"\n"<<"\n";
        }
    }
    
    printf("close Socket\n");
    close(sockfd);

    return 0;
}
