#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string> 
#include <iostream>

int main(int argc , char *argv[])
{

//-----socket connection---------
    int sock = 0;
    sock = socket(AF_INET , SOCK_STREAM , 0);//create socket

    if (sock == -1){//check socket performance
        printf("Fail to create a socket.");
    }

    char* serverIP = argv[1];
    char* serverPort = argv[2];
    int severPort_i = atoi(serverPort);
//------server ip----

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr("127.0.1");
    info.sin_port = htons(severPort_i);
    int err = connect(sock,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        printf("Connection error");
    }
    printf("connect commit");
//-----get my ip----

    int myPort, n;
    struct sockaddr_in my_addr;
    char myPortC[10] = {};
    char receiveMessage[2000] = {0};

    // bzero(&my_addr, sizeof(my_addr));
    // unsigned int len = sizeof(my_addr);
    // getsockname(sock, (struct sockaddr *) &my_addr, &len);
    // myPort = ntohs(my_addr.sin_port);
    // n = sprintf (myPortC, "%d",myPort);
// 

    int tmp = 0;
    char message1[100] = {"REGISTER#"};
    char tag[2] = {"#"};
    while (tmp == 0){
        printf("Enter 1 for register, Enter 2 for login : ");
        std::cin >> tmp;
        if(tmp == 1){
            //-----register----
            char name[20] = {};
            printf("register\n");
            // recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s",receiveMessage);
            printf("Enter the name you want to register :");
            std::cin >> name;
            strcat(message1,name);
            //send register message
            send(sock,message1,sizeof(message1),0);
            memset(receiveMessage, '\0', sizeof(receiveMessage));
            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s",receiveMessage);
            tmp = 0;
        }
        else if(tmp == 2){
            //-----login------
            char message2[20] = {};
            // char myPortC[20] = {};
            printf("login\n");
            printf("Enter your name:");
            std::cin >> message2;
            strcat(message2,"#");
            printf("Enter your port name:\n");
            char portname[20] = {0};
            std::cin >> portname;
            int portname_i = 0;
            portname_i = atoi(portname);
            while(portname_i < 1024 || portname_i > 655355){
                printf("wrong port number\n" );
                printf("Enter your port name again:\n");
                memset(portname, '\0', sizeof(portname));
                std::cin >> portname;
                portname_i = atoi(portname);
            }
            printf("%s\n", portname);
            strcat(message2,portname);
            // printf("%s\n",message2);
            //send login message
            send(sock,message2,strlen(message2),0);
            // clear data
            // char buffer[100] = {};
            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s\n",receiveMessage);
            char money[2] = {0};
            std::cin >> money;
            send(sock,money,strlen(money),0);

            memset(receiveMessage,'\0',sizeof(receiveMessage));
            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s\n",receiveMessage);

            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s\n",receiveMessage);
            memset(receiveMessage,'\0',sizeof(receiveMessage));

            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s\n",receiveMessage);
            memset(receiveMessage,'\0',sizeof(receiveMessage));
            
            tmp = 0;

            
        }
        else if (tmp == 3){
            char list[20] = {0};
            std::cin >> list;
            send(sock,list,strlen(list),0);
            memset(receiveMessage,'\0',sizeof(receiveMessage));
            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s\n",receiveMessage);

            memset(receiveMessage,'\0',sizeof(receiveMessage));
            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s\n",receiveMessage);

            memset(receiveMessage,'\0',sizeof(receiveMessage));
            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s\n",receiveMessage);
            tmp = 0;
            int recvCount = 0;
        }
        else if (tmp == 4){
            char list[20] = {0};
            std::cin >> list;
            send(sock,list,strlen(list),0);
            memset(receiveMessage,'\0',sizeof(receiveMessage));
            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s\n",receiveMessage);
            close(sock);
            tmp = 0;
            break;
        }
        else if(tmp == 0){
            printf("close Socket\n");
            close(sock);
            break;
        }
        else{
            printf("wrong number");
            tmp = 0;
        }
    }
    return 0;
}
