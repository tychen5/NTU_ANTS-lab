#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{

    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;

    //localhost test
    info.sin_addr.s_addr = inet_addr("140.112.106.45");
    info.sin_port = htons(33220);


    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        printf("Connection error");
    }


    //Send a message to server
    char message[100] = {};
    char receiveMessage[100] = {};
    int command = 0;
    char account[100] = {};
    char regis_msg[20] = {"REGISTER#"};
    char port[] = {};
    char aaa[] = {"#"};
    /*
    recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
    printf("%s",receiveMessage);
    printf("\n");*/

    while(1){
        command = 0;
        bzero(&port,sizeof(port));
        bzero(&message,sizeof(message));
        bzero(&receiveMessage,sizeof(receiveMessage));
        bzero(&account,sizeof(account));

        printf("%s", "Enter 1 for register, 2 for log in:\n");
        scanf("%d", &command);
        if(command == 1){
            printf("%s","Enter the name you want to regist:\n");
            scanf("%s", account);
            strcat(regis_msg, account);
            strcat(message, regis_msg);
            //printf("%s", message);
            send(sockfd,message,strlen(message),0);
            recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            printf("%s",receiveMessage);
            printf("\n");
        }
        else if(command == 2){
            printf("%s","Enter your name:\n");
            scanf("%s", account);
            printf("%s","Enter your port number:\n");
            scanf("%s", port);
            int port_int = 0;
            port_int = atoi(port);
            if(port_int <= 65535 && port_int >= 1024){
             
                strcat(message, account);
                strcat(message, aaa);
                strcat(message, port);
                send(sockfd,message,strlen(message),0);
                recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
                printf("%s",receiveMessage);
                printf("\n");

                
                while(1){
                bzero(&receiveMessage,sizeof(receiveMessage));
                /*recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
                printf("%s",receiveMessage);*/
                bzero(&message,sizeof(message));

                command = 0;
                printf("%s", "Enter 3 for the latest list, 4 for exit:\n");
                printf("\n");
                scanf("%d", &command);
                if(command == 3){
                    char list[] = {"List"};
                    strcpy(message, list);
                    send(sockfd,message,strlen(message),0);
                    recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
                    printf("%s",receiveMessage);
                }
                else if(command == 4){
                    char Exit[] = {"Exit"};
                    strcpy(message, Exit);
                    send(sockfd,message,strlen(message),0);
                    recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
                    printf("%s",receiveMessage);
                    break;
                }
                else{
                    printf("%s","the command is not available.\n");
                }
                }
                break;
            }
            else{
                printf("%s","the port number is not available.\n");
            }
        }
        else{
             printf("%s","the command is not available.\n");
        }
    }
    
    printf("close Socket\n");
    close(sockfd);
    return 0;
}