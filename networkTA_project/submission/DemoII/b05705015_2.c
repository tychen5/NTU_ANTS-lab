#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//<netinet/in.h>所定義的函式
//struct sockaddr_in {
//    short            sin_family;   // AF_INET,因為這是IPv4;
//    unsigned short   sin_port;     // 儲存port No
//    struct in_addr   sin_addr;     // 參見struct in_addr
//   char             sin_zero[8];  // Not used, must be zero */
//};

//struct in_addr {
//    unsigned long s_addr;          // load with inet_pton()
//
//};
int main(int argc , char *argv[])
{

    //socket的建立
    int clientsocket = 0;
    clientsocket = socket(AF_INET , SOCK_STREAM , 0);

    if (clientsocket == -1){
        printf("Fail to create a socket.");
    }

    //socket的連線
    struct sockaddr_in info;
    bzero(&info,sizeof(info)); //初始化，將struct涵蓋的bits設為0
    info.sin_family = PF_INET;//sockaddr_in為Ipv4結構

    //localhost test
    info.sin_addr.s_addr = inet_addr("127.0.0.1");//IP address
    info.sin_port = htons(8700);//port number


    int errror = connect(clientsocket,(struct sockaddr *)&info,sizeof(info));
    if(errror==-1){
        printf("Connection error"); //If connect error
    }

    char receiveMessage[100] = {};//receivemessage's buffer 
    recv(clientsocket,receiveMessage,sizeof(receiveMessage),0);
    printf("%s",receiveMessage);

    while(1){
        bzero(&receiveMessage,sizeof(receiveMessage));
        char message[100] = {};
        scanf("%s",message);
        send(clientsocket,message,sizeof(message),0);
        recv(clientsocket,receiveMessage,sizeof(receiveMessage),0);
        printf("%s",receiveMessage);
        if(strncmp(message,"Exit",4) == 0)//if log out
            break;
    }
    
    printf("Close Socket\n");
    close(clientsocket);
    return 0;
}

