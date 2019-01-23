#include <stdio.h>  
#include <unistd.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
  
int main()  
{  
  int sock0;  
  struct sockaddr_in addr;  
  struct sockaddr_in client;  
  socklen_t len;  
  int sock_client;
  char registerBuffer[64] = {};
  char portnumBuffer[64] = {};
  char moneyBuffer[64] = {};
  char enterBuffer[64] = {};
  char receiveMessage[100] = {};//receivemessage's buffer 
    

  /* 製作 socket */  
  sock0 = socket(AF_INET, SOCK_STREAM, 0);  
  
  /* 設定 socket */  
  addr.sin_family = AF_INET;  
  addr.sin_port = htons(8700);  
  addr.sin_addr.s_addr = INADDR_ANY;  
  bind(sock0, (struct sockaddr*)&addr, sizeof(addr));  
  printf("\t[Info] binding...\n");  
  
  /* 設定成等待來自 TCP 用戶端的連線要求狀態 */  
  listen(sock0, 8);  
  printf("\t[Info] listening...\n");  
  
  /* 接受來自 TCP 用戶端地連線要求*/  
  printf("\t[Info] wait for connection...\n");  
  len = sizeof(client);  
  sock_client = accept(sock0, (struct sockaddr *)&client, &len);  
  printf("\t[Info] Testing...\n");  
  char *paddr_str = inet_ntoa(client.sin_addr);  
  printf("\t[Info] Receive connection from %s...\n", paddr_str);  
  
  while(1){
        write(sock_client, "Type the service you want.\n", 28); 
        bzero(&receiveMessage,sizeof(receiveMessage));
        recv(sock_client,receiveMessage,sizeof(receiveMessage),0);
        printf("%s",receiveMessage);
	if(strncmp(receiveMessage,"Register",8) == 0){
            write(sock_client, "Type your username\n", 20);
	    recv(sock_client,registerBuffer,sizeof(registerBuffer),0);
	    printf("\t[Info] Receive username of %s...\n", registerBuffer);
	    recv(sock_client,enterBuffer,sizeof(enterBuffer),0);
	    write(sock_client, "Type your portnum\n", 20);
	    recv(sock_client,portnumBuffer,sizeof(portnumBuffer),0);
	    printf("\t[Info] Receive portnum of %s...\n", portnumBuffer);
	    recv(sock_client,enterBuffer,sizeof(enterBuffer),0);
	    write(sock_client, "Type your money\n", 20);
	    recv(sock_client,moneyBuffer,sizeof(moneyBuffer),0);
	    printf("\t[Info] Receive money of %s...\n", moneyBuffer);
            recv(sock_client,enterBuffer,sizeof(enterBuffer),0);
	    continue;
	}
	if(strncmp(receiveMessage,"Login",5) == 0){
	    write(sock_client, "Type your name\n", 20);
	    recv(sock_client,receiveMessage,sizeof(receiveMessage),0);
	    if(strncmp(receiveMessage,registerBuffer,5) == 0){
	    	write(sock_client, "Your saving amount is(type ? to show it) \n", 40);
		recv(sock_client,enterBuffer,sizeof(enterBuffer),0);
		write(sock_client, moneyBuffer, sizeof(moneyBuffer));
	    	recv(sock_client,enterBuffer,sizeof(enterBuffer),0);
		continue;
		}
	    else{
		write(sock_client, "Wrong username\n", 28);
		recv(sock_client,enterBuffer,sizeof(enterBuffer),0);
		continue;
		}
	}

        if(strncmp(receiveMessage,"List",4) == 0){
            write(sock_client, registerBuffer, sizeof(registerBuffer));
            recv(sock_client,enterBuffer,sizeof(enterBuffer),0);
	    write(sock_client, portnumBuffer, sizeof(portnumBuffer));
	    recv(sock_client,enterBuffer,sizeof(enterBuffer),0);
	    write(sock_client, paddr_str, 20);
            recv(sock_client,enterBuffer,sizeof(enterBuffer),0);
            continue;
	}
        if(strncmp(receiveMessage,"Exit",4) == 0){
	    printf("\t[Info] Client log out...\n");
            break;
	}
    }
  /* 結束 TCP 對話 */  
  printf("\t[Info] Close client connection...\n");  
  close(sock_client);  
  
  /* 結束 listen 的 socket */  
  printf("\t[Info] Close self connection...\n");  
  close(sock0);  
  return 0;  
}  
