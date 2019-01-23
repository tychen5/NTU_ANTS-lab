#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc , char *argv[]){

    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //server
    struct sockaddr_in serverinfo;
    bzero(&serverinfo,sizeof(serverinfo));
    serverinfo.sin_family = PF_INET;
    serverinfo.sin_addr.s_addr = inet_addr(argv[1]);
    serverinfo.sin_port = htons(atoi(argv[2]));


    //socket的連線

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;

    //localhost test
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = INADDR_ANY;
    info.sin_port = htons(9999);
    bind(sockfd,(struct sockaddr *)&info,sizeof(info));

    int err = connect(sockfd,(struct sockaddr *)&serverinfo,sizeof(serverinfo));
    if(err==-1){
        printf("Connection error");
    }

    char re[28];
    recv(sockfd,&re,sizeof(re),0);
    printf("%s",re);

    

    while(1){
        //Send a message to server
        //char message[1000];
        //scanf("%s",message);
        printf("please enter 1 for register, 2 for login, or 3 for exit:");
        int option = 0;
        scanf("%d",&option);
        
        if(option == 1){
            printf("please enter name you want for registration:");
            char name[20];
            scanf("%s",name);
            char regisName[100];
            strcpy(regisName,"REGISTER#");
            strcat(regisName,name);
            
            send(sockfd,regisName,sizeof(regisName),0);

            char msg[100];
            recv(sockfd,msg,sizeof(msg),0);
            printf("%s", msg);
            continue;
        }
        else if(option == 2){
            printf("please enter your name:");
            char name[20];
            scanf("%s",name);

            //port between 1024 ~ 65535
            printf("please enter your port number:");
            char portNum[20];
            scanf("%s",portNum);

            if(atoi(portNum)>65535 || atoi(portNum)<1024){
                printf("port number not exists\n");
                continue;
            }

            char loginAcc[100];
            strcpy(loginAcc,name);
            strcat(loginAcc,"#");
            strcat(loginAcc,portNum);

            send(sockfd,loginAcc,sizeof(loginAcc),0);

            char receiveMessage[1000];
            recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            printf("%s",receiveMessage);

            if(strcmp(receiveMessage,"220 AUTH_FAIL") == 0){
                continue;
            }
            else{
                printf("%s",receiveMessage);
                char receiveMessage1[1000];
                recv(sockfd,receiveMessage1,sizeof(receiveMessage1),0);
                printf("%s",receiveMessage1);
                while(1){
                    printf("please enter 1 for latest list, 2 for logout:");
                    int option = 0;
                    scanf("%d",&option);
                    if(option == 1){
                        char msg[5] = "List";
                        send(sockfd,msg,sizeof(msg),0);

                        char receiveM[1000];
                        recv(sockfd,&receiveM,sizeof(receiveM),0);
                        printf("%s", receiveM);
                    }
                    else if(option == 2){
                        printf("Bye");
                        break;
                    }
                    else{
                        printf("option deny, please enter options again\n");
                        continue;
                    }
                }
            }
        }
        else if(option == 3){
            printf("Bye");
            break;
        }
        else{
            printf("option deny, please enter options again\n");
            continue;
        }
        // send(sockfd,message,sizeof(message),0);

        // const char delim[2] = "#";
        // char *token;
        // token = strtok(message, delim);



        // if(strcmp(token, "REGISTER") == 0){
        //     char msg[1000];
        //     recv(sockfd,&msg,sizeof(msg),0);
        //     printf("%s", msg);
        // }
        // else if(strcmp(token,"LIST") == 0){
        //     char msg[1000];
        //     recv(sockfd,&msg,sizeof(msg),0);
        //     printf("%s", msg);
        // }
        // else if(strcmp(token, "Exit") == 0){
        //     char msg[1000];
        //     recv(sockfd,&msg,sizeof(msg),0);
        //     printf("%s", msg);
        //     break;
        // }
        // else{
        //     char receiveMessage[1000];
        //     recv(sockfd,receiveMessage,sizeof(receiveMessage),0);

        //     if(strcmp(receiveMessage,"220 AUTH_FAIL") == 0){
        //         printf("%s",receiveMessage);
        //         continue;
        //     }
        //     else{
        //         printf("%s",receiveMessage);

        //         char receiveMessage1[1000];
        //         recv(sockfd,receiveMessage1,sizeof(receiveMessage1),0);
        //         printf("%s",receiveMessage1);
        //         // int userNum = atoi(receiveMessage1);
        //         // for(int i = 0; i < userNum; i++){
        //         //     char receiveMessage[1000];
        //         //     recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
        //         //     printf("%s",receiveMessage);
        //         // }
        //     }
        // }
        
    }


    close(sockfd);
    return 0;
}