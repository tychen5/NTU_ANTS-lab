#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
using namespace std;

int main(int argc, char *argv[])
{
    // create socket
    int sockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
        cout << "Fail to create a socket.";
        
    // buffer for storing message from server
    char buffer[256];
    char buffer1[256];

    // initialize
    struct sockaddr_in info;
    bzero(&info, sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(argv[1]);  //IP address
    info.sin_port = htons(atoi(argv[2]));       //port number

  
    // connect to server
    int cnt = connect(sockfd, (struct sockaddr *)&info, sizeof(info));
    if(cnt == -1){
        cout << "Connection error";
        return -1;
    }

    recv(sockfd, buffer, sizeof(buffer), 0);
    cout << buffer;
    
    bool flag = true;
    while(flag){
        int instruction = 0;
        cout << "Enter 1 for register, 2 for login: ";
        cin >> instruction;

        if(instruction == 1){
            // register new account
            char reg[30] = "REGISTER#";
            char accountName[30] = {};
            cout << "Please enter your account name: ";
            cin >> accountName;
            strcat(reg, accountName);
            strcat(reg, "\n");

            bzero(buffer, 256);
            send(sockfd, reg, sizeof(reg), 0);
            recv(sockfd, buffer, sizeof(buffer), 0);
            cout << buffer;
        }

        else if(instruction == 2){
            // Login
            char portNum[10] = {};
            char accountName[30] = {};
            cout << "Please enter your account name: ";
            cin >> accountName;
            cout << "Please enter port number: ";
            cin >> portNum;
            strcat(accountName, "#");
            strcat(accountName, portNum);
            strcat(accountName, "\n");

            bzero(buffer, 256);
            send(sockfd, accountName, sizeof(accountName), 0);
            recv(sockfd, buffer, sizeof(buffer), 0);
            cout << buffer;
            flag = false;
        }
        else
            cout << "please enter right number!" << endl;
    }
    
    flag = true;
    while(flag){
        cout << "Enter '1' to browse online list, '8' to exit: ";
        int num = 0;
        cin >> num;
        if(num == 1){
            // list
            char List[6] = "List\n";
            bzero(buffer, 256);
            send(sockfd, List, sizeof(List), 0);
            recv(sockfd, buffer, sizeof(buffer), 0);
            cout << buffer;
        }
        else if(num == 8){
            // exit
            char exit[6] = "Exit\n";
            bzero(buffer, 256);
            send(sockfd, exit, sizeof(exit), 0);
            recv(sockfd, buffer, sizeof(buffer), 0);
            cout << "Bye\n";
            flag = false;
            break;
        }
        else
            cout << "Please enter right number!" << endl;
    }
    close(sockfd);
    return 0;
}