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
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    //請問使用者要註冊或登入
    char name[64];
    char port_num_char[64];
    int port_num = 0;

    printf("Welcome to the P2P Micropayment System!\n");
    printf("Please register first.\n");

    while(1)
    {
      printf("Please type in your account name: ");
      scanf("%s", name);
      printf("Please type in a number(from 1024 to 65535) to be your port number: ");
      scanf("%s", port_num_char);
      port_num = atoi(port_num_char);
      if(port_num < 1024 || port_num > 65535)
      {
        port_num = 0;
        printf("You type a invalid number, idiot.\n");
        continue;
      }
      else
      {
        printf("Please wait for connection...\n");
        break;
      }
    }

    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1)
    {
        printf("Fail to create a socket.");
    }

    //server
    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(server_ip);
    info.sin_port = htons(server_port);

    //client
    struct sockaddr_in clientInfo;
    socklen_t addrlen = sizeof(clientInfo);
    bzero(&clientInfo,sizeof(clientInfo));
    clientInfo.sin_family = PF_INET;
    clientInfo.sin_addr.s_addr = INADDR_ANY;
    clientInfo.sin_port = htons(port_num);
    bind(sockfd,(struct sockaddr *)&clientInfo,sizeof(clientInfo));

    //socket的連線
    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err == -1)
    {
        printf("Connection error\n");
        close(sockfd);
        return 0;
    }

    //Register
    char message[128];
    strcpy(message, "REGISTER#");
    strcat(message, name);
    send(sockfd,&message,sizeof(message),0);

    char receiveMessage[128];
    recv(sockfd, &receiveMessage, sizeof(receiveMessage),0);
    printf("%s\n",receiveMessage);

    if(strcmp(receiveMessage, "210 FALL\n") == 0)
    {
      return 0;
    }

    //sign in
    printf("Now, you can login.\n");
    char name2[64];
    char port_num_char2[64];
    int port_num2 = 0;
    printf("Please type in your account name: ");
    scanf("%s", name2);
    printf("Please type in your port number: ");
    scanf("%s", port_num_char2);
    port_num2 = atoi(port_num_char2);

    char message2[128];
    strcpy(message2, name2);
    strcat(message2, "#");
    strcat(message2, port_num_char2);
    send(sockfd,&message2,sizeof(message2),0);

    char receiveMessage2[128];
    recv(sockfd, &receiveMessage2, sizeof(receiveMessage2),0);
    //收到驗證失敗訊息
    if(strcmp(receiveMessage2, "220 AUTH_FALL") == 0)
    {
      printf("%s", receiveMessage2);
      return 0;
    }
    //收到驗證成功訊息
    else
    {
      printf("Account balance: %s\n", receiveMessage2);

      char receiveMessage3[128];
      recv(sockfd, &receiveMessage3, sizeof(receiveMessage3),0);
      printf("Number of accounts online: %s\n", receiveMessage3);

      char receiveMessage4[128];
      recv(sockfd, &receiveMessage4, sizeof(receiveMessage4),0);
      printf("%s\n",receiveMessage4);
    }

    //if login successfully
    int action = 0;
    while(1)
    {
      printf("\nPlease type in 1 to get the latest list, 2 to exit: ");
      char temp[64];
      scanf("%s", temp);
      action = atoi(temp);

      if(action == 1)
      {
        printf("Send List Request\n");
        char message[128];
        strcpy(message, "List");
        send(sockfd,&message,sizeof(message),0);

        char receiveMessage[128];
        recv(sockfd, &receiveMessage, sizeof(receiveMessage),0);
        printf("Account balance: %s\n", receiveMessage);

        char receiveMessage2[128];
        recv(sockfd, &receiveMessage2, sizeof(receiveMessage2),0);
        printf("Number of accounts online: %s\n", receiveMessage2);

        char receiveMessage3[128];
        recv(sockfd, &receiveMessage3, sizeof(receiveMessage3),0);
        printf("%s\n",receiveMessage3);
      }
      else if(action == 2)
      {
        printf("Send Exit Request\n");
        char message5[128];
        strcpy(message5, "Exit");
        send(sockfd,&message5,sizeof(message5),0);

        char receiveMessage3[128];
        recv(sockfd, &receiveMessage3, sizeof(receiveMessage3),0);
        printf("%s\n",receiveMessage3);
        
        break;
      }
      else
      {
        printf("You type a invalid number, idiot.\n");
        continue;
      }
    }
    close(sockfd);
    return 0;
}
