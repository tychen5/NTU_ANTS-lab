#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;


int register_request(int argc , char *argv[]);
int login_request(int argc , char *argv[]);

int main(int argc , char *argv[])
{
    cout << "Welcome to the P2P Micropayment System!\n";

    int sockfd = 0;
    while(1)
    {
        cout << "Type 1 to register or 2 to login: ";
        int action = 0;
        cin >> action;
        if(action == 1)
        {
            sockfd = register_request(argc, argv);
            if (sockfd == 0) {
                return 0;
            }
            break;
        }
        else if(action == 2)
        {
            sockfd = login_request(argc, argv);
            if (sockfd == 0) {
                return 0;
            }
            break;
        }
        else
        {
            cout << "I don't understand...\n";
            continue;
        }
    }


    while(1){
        cout << "Type 3 to get list, 4 to pay, or 5 to exit: ";
        int action = 0;
        cin >> action;
        if(action == 3)
        {
            char msg_sent[1024];
            strcpy(msg_sent, "List");
            send(sockfd,&msg_sent,sizeof(msg_sent),0);

            char msg_recieved[1024];
            recv(sockfd,&msg_recieved,sizeof(msg_recieved),0);
            cout << msg_recieved << endl;
        }
        else if(action == 4)
        {
            char msg_sent[1024];
            strcpy(msg_sent, "Pay#");
            cout << "Payee account name: ";
            char payee_name[1024];
            cin >> payee_name;
            strcat(msg_sent, payee_name);
            cout << "Pay amount: ";
            int money = 0;
            cin >> money;
            char money_char[1024];
            sprintf(money_char,"%d", money);
            strcat(msg_sent, money_char);
            send(sockfd,&msg_sent,sizeof(msg_sent),0);

            char msg_recieved[1024];
            recv(sockfd,&msg_recieved,sizeof(msg_recieved),0);
            cout << msg_recieved << endl;
        }
        else if(action == 5)
        {
            char msg_sent[1024];
            strcpy(msg_sent, "Exit");
            send(sockfd,&msg_sent,sizeof(msg_sent),0);

            char msg_recieved[1024];
            recv(sockfd,&msg_recieved,sizeof(msg_recieved),0);
            cout << msg_recieved << endl;

            cout << "close Socket\n";
            close(sockfd);
            break;
        }
        else
        {
            cout << "I don't understand...\n";
            continue;
        }
    }

    return 0;
}


int register_request(int argc , char *argv[])
{
    char name[1024];
    int port;
    while(1)
    {
      cout << "Please type in your account name: ";
      cin >> name;
      cout << "Please type in a number(from 1024 to 65535) to be your port number: ";
      cin >> port;

      if(port < 1024 || port > 65535)
      {
        port = 0;
        cout << "You type a invalid number, idiot.\n";
        continue;
      }
      else
      {
        cout << "Please wait for connection...\n";
        break;
      }
    }

    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        cout << "Fail to create a socket.";
        return 0;
    }

    //server setting
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(server_ip);
    info.sin_port = htons(server_port);

    //client setting
    struct sockaddr_in clientInfo;
    bzero(&clientInfo,sizeof(clientInfo));
    clientInfo.sin_family = PF_INET;
    clientInfo.sin_addr.s_addr = INADDR_ANY;
    clientInfo.sin_port = htons(port);
    bind(sockfd,(struct sockaddr *)&clientInfo,sizeof(clientInfo));

    //socket的連線
    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        cout << "Connection error" << endl;
        return 0;
    }
    else{
        cout << "Connect to server successfully" << endl;
    }

    //register
    cout << "Please wait for register..." << endl;
    char msg_sent[1024];
    strcpy(msg_sent, "REGISTER#");
    strcat(msg_sent, name);
    send(sockfd,&msg_sent,sizeof(msg_sent),0);

    char msg_recieved[1024];
    recv(sockfd,&msg_recieved,sizeof(msg_recieved),0);
    cout << msg_recieved << endl;
    if(strcmp(msg_recieved, "210 FALL") == 0)
    {
        return 0;
    }
    else
    {
        char money[1024];
        while (1) {
            cout << "Please type in the number of money you want to deposit: ";
            cin >> money;

            if (atoi(money) > 1000000) {
                cout << "You don't have so much money...\n";
                memset(money, 0, sizeof(money));
            }
            else {
                break;
            }
        }

        char msg_sent[1024];
        strcpy(msg_sent, "DEPOSIT#");
        strcat(msg_sent, name);
        strcat(msg_sent, "#");
        strcat(msg_sent, money);
        send(sockfd,&msg_sent,sizeof(msg_sent),0);

        char msg_recieved[1024];
        recv(sockfd,&msg_recieved,sizeof(msg_recieved),0);
        cout << msg_recieved << endl;
        if(strcmp(msg_recieved, "DEPOSIT_FALL") == 0)
        {
            return 0;
        }
    }
    return sockfd;
}

int login_request(int argc , char *argv[])
{
    char name[1024];
    int port;
    while(1)
    {
      cout << "Please type in your account name: ";
      cin >> name;
      cout << "Please type your port number: ";
      cin >> port;

      if(port < 1024 || port > 65535)
      {
        port = 0;
        cout << "You type a invalid number, idiot.\n";
        continue;
      }
      else
      {
        cout << "Please wait for connection...\n";
        break;
      }
    }

    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        cout << "Fail to create a socket.";
        return 0;
    }

    //server setting
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(server_ip);
    info.sin_port = htons(server_port);

    //client setting
    struct sockaddr_in clientInfo;
    bzero(&clientInfo,sizeof(clientInfo));
    clientInfo.sin_family = PF_INET;
    clientInfo.sin_addr.s_addr = INADDR_ANY;
    clientInfo.sin_port = htons(port);
    bind(sockfd,(struct sockaddr *)&clientInfo,sizeof(clientInfo));

    //socket的連線
    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        cout << "Connection error" << endl;
        return 0;
    }
    else{
        cout << "Connect to server successfully" << endl;
    }

    //login
    cout << "Please wait for login..." << endl;
    char msg_sent[1024];
    strcpy(msg_sent, name);
    strcat(msg_sent, "#");
    char port_char[1024];
    sprintf(port_char,"%d",port);
    strcat(msg_sent, port_char);
    send(sockfd,&msg_sent,sizeof(msg_sent),0);

    char msg_recieved[1024];
    recv(sockfd,&msg_recieved,sizeof(msg_recieved),0);
    cout << msg_recieved << endl;
    if(strcmp(msg_recieved, "220 AUTH_FALL") == 0)
    {
        return 0;
    }

    return sockfd;
}
