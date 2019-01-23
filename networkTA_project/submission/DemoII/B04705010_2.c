#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  struct sockaddr_in server;
  int sock;
  int n, prt;
  char receiveMessage[100] = {};
  char test[] = "Exit";
  const char s[]="#";

  char *IPAddr = argv[1];
  prt = atoi(argv[2]);

  //build up socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  server.sin_family = AF_INET;
  server.sin_port = htons(prt);
  inet_pton(AF_INET, IPAddr, &server.sin_addr.s_addr);

  socklen_t len = sizeof(server);
  //connect TCP
  int err = connect(sock, (struct sockaddr *)&server, len);
  recv(sock,receiveMessage,sizeof(receiveMessage),0);
  printf("%s\n",receiveMessage);

  char ret[100] = {};
  char mes[32] = {};
  char temp[32] = {};
  char *foo = NULL;
  char *bar = NULL;
  char user[32] = {};
  //main
  //connection success
  if(err != -1){
    while(1){
      foo = NULL;
      bar = NULL;
      //reset all string array
      memset(ret, 0, sizeof(ret));
      memset(mes, 0, sizeof(mes));
      memset(temp, 0, sizeof(temp));

      scanf("%s", mes);

      strcpy(temp, mes);
      foo = strtok(temp,s);
      bar = strtok(NULL,s);

      //store user name when registering
      if(strcmp(foo, "REGISTER") == 0)
        strcpy(user, bar);

      send(sock, mes, sizeof(mes), 0);
      recv(sock, ret, sizeof(ret), 0);
      printf("%s", ret);
      if(strcmp(mes,test) == 0)
        break;
    }
  }
  //connection fail
  else{
    printf("connection error");
  }

  //close TCP connection
  close(sock);

  return 0;
}
