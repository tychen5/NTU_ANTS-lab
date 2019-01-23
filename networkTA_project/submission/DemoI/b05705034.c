#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

#define BLEN 120
#define CLEN 1024

int main(){
	int sd;
	struct sockaddr_in server;
	char buf[BLEN];
	char *bptr;
	int  n;
	int  buflen;
	
	bptr = buf;
	buflen = BLEN;
	
	char buff[CLEN];
	
	/* make a socket */
	if((sd = socket(AF_INET, SOCK_STREAM,0)) < 0){
		perror("socket() failed.");
		exit(1);
	}
	
	memset(&server, '0', sizeof(server));
	
	/* port and IP address */
	server.sin_family = AF_INET;
	server.sin_port = htons(33120);
	inet_pton(AF_INET, "140.112.107.194", &server.sin_addr.s_addr);
	
	/* connect server */
	if(connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
		perror("connect() failed.");
		exit(1);
	}
	
	memset(buf,0,sizeof(buf));
    recv(sd, buf, 120, 0);
    printf("%s\n",buf);
    
    while(1){
    	int mode;
    	char register1[30] = "REGISTER#";
	    char login[30];
	    char logname[20];
	    
	    char username[20];
	    char portnum[10];
	    
    	printf("Enter 1 for Register, 2 for Login: ");
    	scanf("%d",&mode);
        if(mode == 1){
        	printf("Enter the name you want to register: ");
        	scanf("%s", logname);
        	memset(buf,0,sizeof(buf));
        	sprintf(buf, "%s%s\n",register1, logname);
        	send(sd, buf, strlen(buf),0);
        	memset(buf,0,sizeof(buf));
        	recv(sd, buf, 120, 0);
        	if(strcmp(buf, "100 OK\n") == 0){
        		printf("%s\n", buf);
				continue;
			}
			else if(strcmp(buf, "210 FAIL\n") == 0){
				printf("%s\n", buf);
				continue;
			}
            else{
            	perror("Register fialed!");
            	exit(1);
			}
		}
    	if(mode == 2){
        	printf("Enter your name: ");
        	scanf("%s", username);
        	
        	printf("Enter the port number: ");
        	scanf("%s", portnum);
        	
        	sprintf(login, "%s#%s\n", username, portnum);
        	send(sd, login, strlen(login),0);
        	
        	memset(buf,0,sizeof(buf));
        	recv(sd, buf, 120, 0);
        	if(strcmp(buf, "10000\n") == 0){
        		printf("%s\n", buf); 
        		memset(buff,0,sizeof(buff));
        	    recv(sd, buff, 256, 0);
        	    printf("%s\n", buff);
				break;
			}
            else if(strcmp(buf, "220 AUTH_FAIL\n") == 0){
            	printf("%s\n", buf); 
				printf("Please register first\n");
				continue;
			}
			else{
            	perror("Login fialed\n");
            	exit(1);
			}
		}
        else{
        	printf("Please enter 1 or 2\n");
        	continue;
		}
	}
	
	while(1){
		int list;
		char menu[20] = "List\n";
	    char laeve[10] = "Exit\n";
		
		printf("Enter the number of actions you want to take.\n");
		printf("1 to ask for the latest list, 8 to Exit: ");
		scanf("%d",&list);
        if(list == 1){
        	send(sd, menu, strlen(menu),0);
        	memset(buff,0,sizeof(buff));
        	recv(sd, buff, 256, 0);
        	printf("%s\n", buff);
		    continue;
		}
		else if(list == 8){
			send(sd, laeve, strlen(laeve),0);
        	memset(buf,0,sizeof(buf));
        	recv(sd, buf, 120, 0);
        	printf("%s\n", buf);
			break;
		}
		else {
			printf("Please enter 1 or 8\n");
			continue;
		}
	}
	
	/* close */
	close(sd);
	
	return 0;
}
