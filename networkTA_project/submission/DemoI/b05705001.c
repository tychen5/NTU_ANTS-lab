#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>

void cleanBuf()
{
    char c = '0';
    do 
    {
        char c = getchar();
    } while (c == '\n');
}
int main(int argc , char *argv[])
{ 
    int port = atoi(argv[2]); 

    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket的連線

    struct sockaddr_in info;
    memset(&info,'\0',sizeof(info));
    info.sin_family = PF_INET;

    //localhost test
    info.sin_addr.s_addr = inet_addr(argv[1]);
    info.sin_port = htons(port);

    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        printf("Connection error\n");
        return 0;
    }

    // connected accepted
    char receiveMessage[100] = {};
    recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
    printf("Receive: %s",receiveMessage);
    memset(receiveMessage, '\0',100);
    
    char command = '0';
    char username[10] = {};
    char portnum[10] = {};
    char message[100] = {};
    

    // Register
    printf("%s\n", "Enter 1 for register, 2 for Login: ");
    // cleanBuf();
    scanf("%c", &command);
    while(1){

        if(command == '1'){
            printf("%s", "Enter the name you want to register: ");
            scanf("%s", username);
            strcpy(message, "REGISTER#");
            strcat(message, username);
            strcat(message, "\n");
            send(sockfd,message, sizeof(message),0);

            memset(message, '\0',100);
            memset(username, '\0',10);

            recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            printf("receive: %s",receiveMessage);
            memset(receiveMessage, '\0',100);

        }



        // Login

        else if(command == '2'){
            printf("%s", "Enter your name: "); cleanBuf();
            scanf("%s", username);
            printf("%s", "Enter the port number: "); cleanBuf();
            scanf("%s",portnum);
            strcpy(message, username);
            strcat(message, "#");
            strcat(message, portnum);
            strcat(message, "\n");
            // printf("%s", message);
            send(sockfd,message, sizeof(message),0);
            memset(message, '\0',100);

            memset(receiveMessage,'\0',100);
            recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            printf("%s",receiveMessage);
            
            recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            printf("%s",receiveMessage);
            memset(receiveMessage, '\0',100);
            break;


        }

        else
        {
            printf("%s\n", "<<Please enter 1 or 2>>");
            printf("%s\n", "Enter 1 for register, 2 for Login: ");
            cleanBuf();
            scanf("%c", &command);
            continue;
        }
        printf("%s\n", "Enter 1 for register, 2 for Login: ");
        cleanBuf();
        scanf("%c", &command);

    }


        // send(sockfd,message, sizeof(message),0);
        // memset(message, '\0',100);

        // char message1[] = {"REGISTER#qqq\n"};
        // send(sockfd, message1,sizeof(message1),0);
        // memset(message1, '\0',sizeof(message1));

        // memset(receiveMessage,'\0',sizeof(receiveMessage));
        // recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
        // printf("%s",receiveMessage);
        
        // char message2[] = {"qqq#2222\n"};
        // send(sockfd,message2, sizeof(message2),0);
        // memset(message2, '\0',sizeof(message2));

        // memset(receiveMessage,'\0',sizeof(receiveMessage));
        // recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
        // printf("%s",receiveMessage);
        
        // memset(receiveMessage,'\0',sizeof(receiveMessage));
        // recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
        // printf("%s",receiveMessage);

        // printf("size: %lu\n", sizeof(receiveMessage));
        // printf("last: %c\n", receiveMessage[27]);



    // Action or Exit
    command = '0';

    while(1){
        printf("%s", "Enter the number of action you want to take:\n1 to ask for the latest list, 8 to exit: ");
        cleanBuf();
        scanf("%c", &command);

        if(command == '1')
        {
            char askList[] = {"List\n"};
            send(sockfd,askList, sizeof(askList),0);
            memset(askList, '\0', sizeof(askList));

            recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            printf("%s",receiveMessage);
            printf("last: %c\n", receiveMessage[0]);
            memset(receiveMessage, '\0',100);

            // int number_online = receiveMessage[27] - '0';
            
            // for (int i = 0; i < number_online; ++i)
            // {
                recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
                printf("%s",receiveMessage);
                memset(receiveMessage, '\0',100);
            // }
            
        }

        else if(command == '8')
        {
            strcpy(message, "Exit\n");
            send(sockfd,message, sizeof(message),0);
            memset(message, '\0', sizeof(message));

            recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            printf("%s",receiveMessage);
            memset(receiveMessage, '\0',100);
            break ;      
        }
        else{
            printf("%s\n", "<<Please enter 1 or 8.>>"); 
        }
    }
        
            
    printf("\nclose Socket\n");
    close(sockfd);
    return 0;
}
