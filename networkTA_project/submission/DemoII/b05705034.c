#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define CLEN 1024

struct users{
	char username[20];
	int port;
	int checklogin;
};

struct users u[50];
int cnt = 0;
int online = 0;
int nowuser = 0;

char regok[20] = "100 OK\n";
char regero[20] = "210 FAIL\n";
char logero[20] = "220 AUTH_FAIL\n";
char sbye[10] = "Bye\n";
char accbal[10] = "10000\n";
char disonline[30] = "number of accounts online:";

void *client_handler(void *sd);

int main(){
	int sd;
	int conn_sd;
	struct sockaddr_in server;
	struct sockaddr_in cserver;
	int c_server_len = sizeof(cserver);
	
	char acc[30] = "connection accepted\n";
	
	/* make a socket */
	if((sd = socket(AF_INET, SOCK_STREAM,0)) < 0){
		perror("socket() failed.");
		exit(1);
	}
	
	memset(&server, '0', sizeof(server));
	
	/* port and IP address */
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(33120);
	
	/* create a bind */
	if(bind(sd, (struct sockaddr*)&server, sizeof(server)) < 0){
		perror("reader: bind");
		exit(1);
	}
	
	if(listen(sd, 5) < 0){
		perror("listen_error");
		exit(1);
	}
	
	pthread_t worker;
	
	while(1){
		if( (conn_sd = accept(sd, (struct sockaddr*)&cserver, &c_server_len)) < 0){
			perror("accept_error");
			exit(1);
		}
		
		send(conn_sd, acc, strlen(acc), 0);
		
		
		if(pthread_create(&worker, NULL, client_handler, &conn_sd) != 0){
			perror("pthread_create error");
			exit(1);
		}
		
	}
	
	close(sd);
	
	return 0;
}


void *client_handler(void *sd){
	int conn_sd = *(int*)sd;
	
	int rsize;
	char buf[CLEN];
	
	while(1){
		int torf = 0;
		int ucnt = 0;
		char *tp;
		char x[30];
		memset(buf,0,sizeof(buf));
		rsize = recv(conn_sd, buf, CLEN, 0);
		
		if(rsize == 0){
			break;
		}
		else if(rsize == -1){
			perror("recv_error");
			exit(1);
		}
		else{
			int slen = strlen(buf);
			char copuser[30];
			if((strncmp(buf, "REGISTER#", 9) == 0) && buf[slen-1] == '\n'){
				strncpy(copuser, buf+9, slen-1);
				int j = 0;
				while(1){
				  	if(copuser[j] == '\n'){
				  		copuser[j] = '\0';
				  		break;
					}
					j++;
				}
				for(int i = 0; i < cnt; i++){
				  if(strcmp(copuser, u[i].username) == 0){
				    	torf = 1;
				  }
			    }
				if(torf == 1){
				  send(conn_sd, regero, strlen(regero), 0);
				  continue;
				}
				else{
				  int j = 0;
				  while(1){
				  	if(buf[j] == '\n'){
				  		buf[j] = '\0';
				  		break;
					}
					j++;
				  }
				  strncpy(u[cnt].username, buf+9, slen-1);
				  cnt++;
				  send(conn_sd, regok, strlen(regok), 0);
				  continue;	
				}
			}
			
			torf = 0;
			tp = strtok(buf, "#");
			strcpy(x,tp);
			for(int i = 0; i < cnt; i++){
				if(strcmp(x, u[i].username) == 0){
					torf = 1;
					ucnt = i;
				}
			}
			nowuser = ucnt;
		 	
			if(torf == 1){
				char onlineuser[50];
				char onnum[12];
				online++;
				tp = strtok( NULL, "#" );
				u[ucnt].port = atoi(tp);
				u[ucnt].checklogin = 1;
				send(conn_sd, accbal, strlen(accbal), 0);
				send(conn_sd, disonline, strlen(disonline),0);
				snprintf(onnum, 12, "%d\n", online);
				send(conn_sd, onnum, strlen(onnum),0);
				for(int i = 0; i < cnt; i++){
					if(u[i].checklogin == 1){
						sprintf(onlineuser, "%s%s%d\n",u[i].username, "#127.0.0.1#", u[i].port);
						send(conn_sd, onlineuser, strlen(onlineuser),0);
					}
				}
				break;
			}
			else{
				send(conn_sd, logero, strlen(logero), 0);
				continue;
			}
			
		}
	}
	
	while(1){
		memset(buf,0,sizeof(buf));
		rsize = recv(conn_sd, buf, CLEN, 0);
		
		if(rsize == 0){
			break;
		}
		else if(rsize == -1){
			perror("recv_error");
			exit(1);
		}
		else{
			int slen = strlen(buf); 
			if((strncmp(buf, "Exit", 4) == 0) && buf[slen-1] == '\n'){
				send(conn_sd, sbye, strlen(sbye), 0);
				u[nowuser].checklogin = 0;
				online--;
				nowuser--;
				break;
			}
			else if((strncmp(buf, "List", 4) == 0) && buf[slen-1] == '\n'){
				char onlineuser[50]; 
				char onnum[12];
				send(conn_sd, accbal, strlen(accbal), 0);
				send(conn_sd, disonline, strlen(disonline),0);
				snprintf(onnum, 12, "%d\n", online);
				send(conn_sd, onnum, strlen(onnum),0);
				for(int i = 0; i < cnt; i++){
					if(u[i].checklogin == 1){
						sprintf(onlineuser, "%s%s%d\n",u[i].username, "#127.0.0.1#", u[i].port);
						send(conn_sd, onlineuser, strlen(onlineuser),0);
					}
				}
				continue;
			}
			else{
				printf("Server error");
				break;
			}
		}
	}
	
	if(close(conn_sd) < 0){
		perror("close");
		exit(1);
	}
}



