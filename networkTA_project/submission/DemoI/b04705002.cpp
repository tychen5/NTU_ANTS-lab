//
//  main.cpp
//  socket_program_hw1
//
//  Created by Apple on 2018/11/30.
//  Copyright © 2018年 Apple. All rights reserved.
//

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <cstdlib>
using namespace std;



int main(int argc, const char * argv[]) {
    char addr [100];
    strcpy(addr, argv[1]);
    int port = atoi(argv[2]);
    // insert code here...
    int sock_FD = 0;
    sock_FD = socket(PF_INET , SOCK_STREAM , 0); //AF_INET : IPv4 , SOCK_STREAM: TCP connection
    
    if (sock_FD == -1){
        printf("Fail to create a socket.");
    }
    else{
        string res = "Client: socket created" + to_string(sock_FD);
        cout<<res<<endl;
    }
    struct sockaddr_in connect_info;
    bzero(&connect_info,sizeof(connect_info)); //initailize
    connect_info.sin_family = AF_INET;  //AF = Address Family for address ,PF = Protocol Family for socket initialize ,AF_INET = PF_INET
    connect_info.sin_addr.s_addr =  inet_addr(addr); // convert ip address to integer format
    connect_info.sin_port = htons(port);
    
    int err = connect(sock_FD, (struct sockaddr *) &connect_info,sizeof(connect_info));
    if(err==-1){
        printf("Connection error");
        cout<<endl;
        return 0;
    }
    else{
        cout<< "Successfully connected" <<endl;
    }
    char first[100] = {};
    recv(sock_FD,first,sizeof(first),0);
    cout<< first;
    bzero(&first,sizeof(first)); //initailize
    
    cout<<"Enter 1 for register, 2 for Login: ";
    char decision[10];
    cin>> decision;
    char receiveMessage[2000] ={};
    string user_name ;
    string member_list;
    bool flag = 0;
    bool flag_decision = 0;
    bool login = 0;
    while(cin){
        if(strcmp(decision,"1") == 0){
            cout<<"Enter the name you want to Register: ";
        }
        else if(strcmp(decision,"2") ==0 ){
            cout<<"Enter your name: ";
        }
        else if (strcmp(decision,"3") ==0 && login){
            cout<<"Enter the number of actions you want to take: ";
            cout<<"1 for ask the latest list, 8 for Exit :";
        }
        else{
            cout<<"please enter 1 for register, 2 for login: ";
            cin>> decision;
            continue;
        }
        // int start = 0, end =0;
        
        char message[200] = {};
        bzero(&message,sizeof(message)); //initailize
        cin>>message;
        char * send_mes ;
        bzero(&send_mes,sizeof(send_mes));
        if (strcmp(decision,"1") ==0 ){
            char pre[] = "REGISTER#";
            send_mes = (char *) malloc(strlen(pre)+ strlen(message) );
            strcpy(send_mes, pre);
            strcat(send_mes, message);
            strcpy(decision, "100");
            
        }
        else if(strcmp(decision,"2") ==0 ){
            char port[20] = {};
            cout<< "Enter your port number: ";
            cin>> port;
            bool is_digit = 0;
            for(int i=0;i<strlen(port);i++){
                if (isdigit(port[i])==0){
                    is_digit =1;
                }
            }
            while (is_digit){
                cout<<"port number should be number!!! ";
                cout<< "Please enter your port number again: ";
                cin>> port;
                is_digit = 0;
                for(int i=0;i<strlen(port);i++){
                    if (isdigit(port[i])==0){
                        is_digit =1;
                    }
                }
            }
            send_mes = (char *) malloc(2+strlen(message) + strlen(port) );
            strcpy(send_mes, message);
            strcat(send_mes, "#");
            strcat(send_mes, port);
        }
        else if(strcmp(decision,"3") ==0 ){
            send_mes = (char *) malloc(10);
            if (strcmp(message,"1") ==0){
                strcpy(send_mes, "List");
            }
            else if (strcmp(message,"8") ==0){
                strcpy(send_mes, "Exit");
            }
            else{
                continue;
            }
        }
        else{
            cout<<"Please enter once (1 for register, 2 for Login: )";
            continue;
        }
        send(sock_FD, send_mes,strlen(send_mes),0);
        bzero(&receiveMessage,sizeof(receiveMessage)); //initailize
        recv(sock_FD,receiveMessage,sizeof(receiveMessage),0);
        
        cout<<receiveMessage;
        string res = receiveMessage;
        
        string temp = send_mes;
        if(strstr(send_mes,"REGISTER") && res.substr(0,3) == "100"){
            int start = temp.find("#");
            temp = temp.substr(start+1);
            user_name.append(temp);
        }
        else if (res.substr(0,3) == "210"){
            cout<<"error while register"<<endl;
            continue;
        }
        else if (res.substr(0,3) == "220"){
            cout<<"error happened while log in, please make sure you registed and try again"<<endl;
            cout<<"Enter 1 for register, 2 for Login: ";
            cin>>decision;
            continue;
        }
        else{
            int start = temp.find("#");
            temp = temp.substr(0, start);
            if(user_name.find(temp)!= std::string::npos){
                flag =1;
            }
        } 
        if (flag ==1 && res.substr(0,3) != "220"){
            bzero(&receiveMessage,sizeof(receiveMessage)); //initailize
            recv(sock_FD,receiveMessage,sizeof(receiveMessage),0);
            cout<<receiveMessage;
            bzero(&receiveMessage,sizeof(receiveMessage)); //initailize
            strcpy(decision, "3");
            flag =0;
            login = 1;
        }
    }
    
    close(sock_FD);
    printf("close Socket\n");
    
    return 0;
}
