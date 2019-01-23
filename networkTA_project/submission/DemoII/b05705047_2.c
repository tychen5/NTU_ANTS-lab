
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <unistd.h>
#define PORT 33120
// just for testing port number
int Type2Send (int fd) // command send to server
{
    char input[200];
    memset(&input, '\0', sizeof(input)); // clear the input 
    scanf("%s" , input) ; 
    if (strstr(input,"Stop") != NULL)    // my own stop command
         return -1;     
    input[strlen(input)]='\n';           // solution of <CRLF>
    send(fd , input , strlen(input) , 0); 
    sleep(1);                            // Wait server to respond
    if (strstr(input,"Exit") != NULL)    // If we send Exit than 99
    {
        return 99 ;
    }
    return 1;
}

void Receive2Print (int fd) // command receive from server
{
    int valread=0;
    char buffer[1024];
    memset(&buffer, '\0', sizeof(buffer));  // clear buffer every time
    valread = recv(fd, buffer, sizeof(buffer), 0);
    if (valread > 0)  // if buffer has sth then print it 
        printf("%s\n", buffer);
   return;
}
   
int main(int argc, char const *argv[]) 
{ 
    struct sockaddr_in address; 
    int sock = 0, valread, result=0; 
    struct sockaddr_in serv_addr; 
    //char *Register = "REGISTER#<B05705047>\n"; 
    //char *NameSet = "<B05705047>#<33120>\n"; 
    //char *List = "List\n"; 
    //char *Exit = "Exit\n"; 
    char input[200];
    char buffer[1024] = {0};     

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)  // test socket
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    memset(&serv_addr, '\0', sizeof(serv_addr)); // clear address's buffer
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(atoi(argv[2]));  //convert portnumber 

    //printf("%s, %s\n", argv[1], argv[2]);
       

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0) // test IP
    { 
        printf("\nInvalid address \n"); 
        return -1; 
    } 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return -1; 
    } 

    valread = recv(sock, buffer, sizeof(buffer), 0);
    if (valread > 0) 
    {             // the "connection accepted"
        printf("%s\n", buffer);
    }

    memset(&buffer, '\0', sizeof(buffer)); // reset buffer

while((result=Type2Send(sock)) > 0) 
{
    Receive2Print(sock);
    if (result == 99)    // when enter Exit then break 
      break;
}
close(sock);
return 0;
}
