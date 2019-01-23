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
void receive(int sockfd)
{
    char receiveMessage[1024] = {0};
    memset(receiveMessage, '\0',sizeof(receiveMessage));
    recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
    printf("%s",receiveMessage);

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
    receive(sockfd);
    
    char command[100];
    char username[100] = {};
    char portnum[100] = {};
    char message[1000] = {};
    

    // Register
    // printf("%s\n", "Enter 1 for register, 2 for Login: ");
    // memset(command, '\0',100);
    // scanf("%s", command);

    while(1){

        printf("%s\n", "Enter 1 for register, 2 for Login: ");
        // cleanBuf();
        memset(command, '\0',100);
        scanf("%s", command);

        // printf("=== %s\n", command);
        if(strcmp(command, "1") == 0){
            printf("%s", "Enter the name you want to register: ");
            scanf("%s", username);
            strcpy(message, "REGISTER#");
            strcat(message, username);
            // strcat(message, "\n");
            send(sockfd,message, sizeof(message),0);

            memset(message, '\0',100);
            memset(username, '\0',10);

            //100 OK or 210 FAIL
            receive(sockfd);

        }



        // Login

        else if(strcmp(command, "2") == 0){
            printf("%s", "Enter your name: "); cleanBuf();
            scanf("%s", username);
            printf("%s", "Enter the port number: "); cleanBuf();
            scanf("%s",portnum);
            strcpy(message, username);
            strcat(message, "#");
            strcat(message, portnum);
            // strcat(message, "\n");
            send(sockfd,message, sizeof(message),0);
            memset(message, '\0',100);

            //Please enter: or 210 AUTH_FAIL 
            char receiveMessage[1024] = {0};
            memset(receiveMessage, '\0',sizeof(receiveMessage));
            recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            printf("%s",receiveMessage);
            if(strcmp(receiveMessage,"220 AUTH_FAIL\n") == 0)
            {
                continue;
            }

            scanf("%s", message);
            send(sockfd,message,sizeof(message),0);

            //account balance
            receive(sockfd);

            //number of accounts online
            receive(sockfd);
            
            //online list
            receive(sockfd);
            break;


        }

        else
        {
            printf("%s\n", "<<Please enter 1 or 2>>");
            continue;
        }


    }



    // Action or Exit

    while(1){
        printf("%s", "Enter the number of action you want to take:\n1 to ask for the latest list, 8 to exit: ");
        cleanBuf();
        memset(command, '\0',100);
        scanf("%s", command);
        // printf("--- %s\n", command);
        if(strcmp(command, "1") == 0)
        {
            char askList[] = {"List\n"};
            send(sockfd,askList, sizeof(askList),0);
            memset(askList, '\0', sizeof(askList));

            //account balance
            receive(sockfd);


            //number of accounts online
            receive(sockfd);
            
            //online list
            receive(sockfd);

            
        }

        else if(strcmp(command, "8") == 0)
        {
            strcpy(message, "Exit\n");
            send(sockfd,message, sizeof(message),0);
            memset(message, '\0', sizeof(message));

            //Bye!
            receive(sockfd);
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
