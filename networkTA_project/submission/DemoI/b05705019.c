#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"

int sockfd = 0;
char command[100] = {};
void regAndLog();
int reg_time = 0;

int main(int argc , char *argv[])
{
    // create socket
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd < 0)
    {
        printf("Fail to create a socket.");
        return -1;
    }
    
    // IPv4 AF_INET sockets:
    // IPv6參見 sockaddr_in6
    // struct sockaddr_in {
    //     short            sin_family;   // AF_INET,因為這是IPv4;
    //     unsigned short   sin_port;     // 儲存port No
    //     struct in_addr   sin_addr;     // 參見struct in_addr
    //     char             sin_zero[8];  // Not used, must be zero */
    // };

    // struct in_addr {
    //     unsigned long s_addr;          // load with inet_pton()
    // };
    
    struct sockaddr_in info_client;
    bzero(&info_client,sizeof(info_client)); // initialize struct to 0 bit
    info_client.sin_family = PF_INET; // sockaddr_in為Ipv4結構
    info_client.sin_addr.s_addr = inet_addr(argv[1]); // IP address of server
    info_client.sin_port = htons(atoi(argv[2])); // port number of server

    // connect to server
    char connectReveive[100] = {};
    int error = connect(sockfd,(struct sockaddr *)&info_client,sizeof(info_client));
    if(error < 0)
    {
        printf("Connection failed");
        return -1;
    }
    else
    {
        recv(sockfd, connectReveive, sizeof(connectReveive), 0);
        printf("\n%s", connectReveive);
    }
           
    // while not exit, keep making commands
    while(1)
    {
        regAndLog();
    }
    
    return 0;
}

void regAndLog(){
    // register or login
    char Regcommand[10];
    printf("\nEnter" GREEN " <R> " RESET "for register, " GREEN "<L> " RESET "for login, " GREEN "<EXIT>" RESET "for exit. ");
    scanf("\n%s", Regcommand);
    while(strcmp(Regcommand, "R") != 0 && strcmp(Regcommand, "L") != 0 && strcmp(Regcommand, "E") != 0)
        regAndLog();
       
  
    char username[100];
    char sendMessage[100] = {};
    char receiveMessage[100] = {};
    // register
    if(strcmp(Regcommand, "R") == 0)
    {
        // input user name
        printf("\nEnter the name you want to register: ");
        scanf("%s", username);
        if(reg_time > 0)
        {
            printf("\nYou have registered before. \nPlease use another name to register.\n");
            regAndLog();
        }
        else
        {
            strcpy(sendMessage, "REGISTER#");
            strcat(sendMessage, username);
            send(sockfd, sendMessage, sizeof(sendMessage), 0);
            recv(sockfd, receiveMessage, sizeof(receiveMessage), 0);
            printf("\n%s", receiveMessage);
            reg_time++;
        }
    }
    else if(strcmp(Regcommand, "L") == 0)  // login
    {
        // enter user name
        printf("\nWhat's your name? ");
        scanf("%s", username);

        // enter port number
        int portnum = 0;
        _Bool flag_port = 1;
        while(flag_port)
        {
            printf("\nEnter your port number: ");
            scanf("%d", &portnum);
            if(portnum < 1024 || portnum > 65535)
                printf("\nPlease enter valid port number.");
            else
                flag_port = 0;
        }
        // printf("%s", "portnum");
        // printf("%d", portnum);
        strcpy(sendMessage, username);
        strcat(sendMessage, "#");
        char portnum_str[10];
        sprintf(portnum_str, "%d", portnum);

        strcat(sendMessage, portnum_str);
        strcat(sendMessage, "\n");
        send(sockfd, sendMessage, sizeof(sendMessage), 0);
        recv(sockfd, receiveMessage, sizeof(receiveMessage), 0);
        // printf("%s", sendMessage);
        // if not registered before
        if(strcmp(receiveMessage, "220 AUTH_FAIL\n") == 0)
        {
            printf("%s\n", receiveMessage);
            printf("Please register first. \n");
            regAndLog();
        }
        else
        {
            printf("Successfully login.\n");
            printf("\n%s", receiveMessage);
            char latestInfo[100] = {};
            recv(sockfd, latestInfo, sizeof(latestInfo), 0);
            printf("%s\n", latestInfo);

            // request latest information or EXIT
            printf("Enter" GREEN " <LIST> " RESET "for requesting the latest information, " GREEN " <EXIT> " RESET "to exit. ");
            scanf("%s", command);
            while(strcmp(command, "LIST") != 0 && strcmp(command, "EXIT") != 0)
            {
                if(strcmp(command, "LIST") == 0)
                {
                    char latestInfo[100] = {};
                    recv(sockfd, latestInfo, sizeof(latestInfo), 0);
                    printf("%s\n", latestInfo);
                }
                else if(strcmp(command, "EXIT") == 0) //
                {
                    send(sockfd, command, sizeof(command), 0);
                    recv(sockfd, receiveMessage, sizeof(receiveMessage), 0);
                    printf("%s\n", receiveMessage);
                    close(sockfd);
                    break;
                }
            }
        }
    }
    else if(strcmp(Regcommand, "EXIT") == 0)
    {
        send(sockfd, Regcommand, sizeof(Regcommand), 0);
        recv(sockfd, receiveMessage, sizeof(receiveMessage), 0);
        printf("%s\n", receiveMessage);
        close(sockfd);
    }   
}