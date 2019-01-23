#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int main(int argc , char *argv[])
{

    //construct socket sd
    int sd = 0;
    sd = socket(PF_INET, SOCK_STREAM, 0);
    if (sd == -1){
        printf("Fail to create a socket.");
    }

    //socket connection

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = AF_INET;

    /*localhost test*/
    // info.sin_addr.s_addr = inet_addr("127.0.0.1");
    // info.sin_port = htons(8700);
    /*temp server*/
    // info.sin_addr.s_addr = inet_addr("140.112.107.194");
    // info.sin_port = htons(33120);
    /*jacob's server*/
    // info.sin_addr.s_addr = inet_addr("140.112.106.45");
    // info.sin_port = htons(33220);
    /*operatable IP and port*/
    int pnum = 0;
    char ip[20] = {};
    info.sin_addr.s_addr = inet_addr(argv[1]);
    info.sin_port = htons(atoi(argv[2]));


    //made a connection and check for mistakes
    int retcode = connect(sd,(struct sockaddr *)&info,sizeof(info));
    if(retcode==-1){
        printf("%s\n","Connection error");
        return 0;
    }

    char id[100] = {};
    char receiveMessage[100] = {};
    recv(sd,receiveMessage,sizeof(receiveMessage),0);
    printf("%s",receiveMessage);
    int login = -1;
    
    while(1){
    //Send a message to server
        // char command[20] = {};
        // bzero(receiveMessage, sizeof(receiveMessage));
        // char message[100] = {};
        // scanf("%s", message);
        // send(sd,message,sizeof(message),0);
        // recv(sd,receiveMessage,sizeof(receiveMessage),0);
        // printf("%s",receiveMessage);
        // strncpy(command, message, 9);
        // if(strcmp(command, "REGISTER#") == 0){
        //     //printf("%s\n", "REGISTER SUCCESS, PLEASE LOGIN:");
        // }
        // else if(strcmp(message,"exit") == 0){
        //     printf("%s\n","break" );
        //     break;
        // }
        // else{
        //     char temp[100] = {};
        //     recv(sd,temp,sizeof(temp),0);
        //     printf("%s",temp);
        // }

        //for every loop need to reinicialize 
        bzero(receiveMessage, sizeof(receiveMessage));
        int option = 0;
        char message[100] = {};
        //if not login
        if(login == -1){
            printf("%s\n", "enter 1 to register or 2 to login");
            scanf("%d", &option);

            //register
            if(option == 1){
                    printf("%s", "enter your name: ");
                    scanf("%s",id);
                    strcpy(message,"REGISTER#");
                    strcat(message, id);
                    send(sd,message,sizeof(message),0);
                    recv(sd,receiveMessage,sizeof(receiveMessage),0);
                    printf("%s",receiveMessage);
                }
                else if(option == 2){
                    char temp[100] = {};
                    printf("%s", "enter your name: ");
                    scanf("%s",temp);
                    int port = 0;
                    char num[10] = {};
                    printf("%s", "enter port number: ");
                    scanf("%d", &port);
                    sprintf(num, "%d", port);
                    strcpy(message,temp);
                    strcat(message, "#");
                    strcat(message, num);
                    send(sd,message,sizeof(message),0);
                    recv(sd,receiveMessage,sizeof(receiveMessage),0);
                    printf("%s",receiveMessage);
                    if(strcmp(receiveMessage, "220 AUTH_FAIL") < 0){
                        login = 1;
                        char temp2[100] = {};
                        recv(sd,temp2,sizeof(temp),0);
                        printf("%s",temp2);
                    }
                    else{
                        printf("%s\n", "Wrong Account Name! Please re-register or re-login");
                    }
                }
        }
        else{
            printf("%s", "enter option code: ");
            scanf("%d", &option);
            if(option == 1){
                strcpy(message,"List");
                send(sd,message,sizeof(message),0);
                recv(sd,receiveMessage,sizeof(receiveMessage),0);
                printf("%s",receiveMessage);
            }
            else if(option == 8){
                break;
            }
            else{
                printf("%s\n", "option number not exist");
            }
        }

    }
    printf("close Socket\n");
    close(sd);
    return 0;
}