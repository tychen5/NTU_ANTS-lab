#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

int BUFFER = 1024;

int main(int argc, char *argv[]){
    //creat a socket
    int sockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        cout << "ERROR, socket not created.";
    }
    
    //connect to server
    struct sockaddr_in serverAddr;
    char recv_buf[BUFFER];
    char send_buf[BUFFER];

    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddr.sin_port = htons(atoi(argv[2]));

    int err = connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if(err < 0){
        cout << "ERROR, connection fail.";
        return 0;
    }
    
    //deal with the connection message
    recv(sockfd, recv_buf, sizeof(recv_buf), 0);
    bzero(&recv_buf, sizeof(recv_buf));
    bool registered = 0;
    
    //user interface
    while(true){ 
        cout << "Please enter R to register, L to login: ";
        char mode; // R or L
        char r_name[20]; // register user name
        cin >> mode;
        //register
        if(mode == 'R' || mode == 'r'){
            cout << "==REGISTER==" << endl;
            cout << "Enter user name: ";
            cin >> r_name;
            strcpy(send_buf, "REGISTER#");
            strcat(send_buf, r_name);
            send(sockfd, send_buf, strlen(send_buf), 0);
            recv(sockfd, recv_buf, sizeof(recv_buf), 0);
            if (strncmp(recv_buf, "100 OK", 6)){
                cout << "Registered successfully as "<< r_name << '.' << endl;
                cout << "==========" << endl;
                registered = 1;
            }
        }
        //login
        else if(mode == 'L' || mode == 'l'){
            bool flag = 0;
            char l_name[20], p_num[100]; // login name
            while(flag == 0){
                cout << "Please enter user name and port number: ";
                cin >> l_name >> p_num;
                //check if port is legal
                if(atoi(p_num) < 1024 || atoi(p_num) > 65535){
                    cout << "Invalid port number." << endl;
                    cout << "Port number should be between 1024 and 65535.";
                    cout << "Please try again." << endl;
                    cout << "==========" << endl;
                }
                else flag = 1;
            }  
            bzero(&send_buf, sizeof(send_buf));
            bzero(&recv_buf, sizeof(recv_buf));
            strcpy(send_buf, l_name);
            strcat(send_buf, "#");
            strcat(send_buf, p_num);
            send(sockfd, send_buf, strlen(send_buf), 0);
            recv(sockfd, recv_buf, sizeof(recv_buf), 0);
            cout << recv_buf;
            break;
        }
        //Input not R nor L
        else{
            cout << "ERROR, please register or login first." << endl;
        }
    }
    cout << "===Login successfully===" << endl;
    
    //Things to do after login
    while(true){
        bzero(&send_buf, sizeof(send_buf));
        bzero(&recv_buf, sizeof(recv_buf)); 
        cout << send_buf << recv_buf;
        cout << "Please type in what to do next." << endl;
        cout << "1 to get the latest list, 0 to exit:";
        int task;
        cin >> task;

        if(task == 1){
            strcpy(send_buf, "List");
            send(sockfd, send_buf, strlen(send_buf), 0);
            recv(sockfd, recv_buf, sizeof(recv_buf), 0);
            cout << recv_buf << endl;
        }
        else if(task == 0){
            strcpy(send_buf, "Exit");
            send(sockfd, send_buf, strlen(send_buf), 0);
            recv(sockfd, recv_buf, sizeof(recv_buf), 0);
            cout << recv_buf;
            
            return 0;
        }
        else{
            cout << "Invalid input, please try again!" << endl;
            cout << "==========" << endl;
        }
    }    
    return 0;
}


