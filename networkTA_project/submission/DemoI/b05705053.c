// Client side C/C++ program to demonstrate Socket programming 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include "color.h"
#define PORT 8080 
#define _GNU_SOURCE

void prompt(void)
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    fp = fopen("prompt.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    while ((read = getline(&line, &len, fp)) != -1) {
        printf(ANSI_COLOR_YELLOW);
        printf("%s", line);
    }
    fclose(fp);
    if (line)
        free(line);
}

int main(int argc, char const *argv[]) 
{
	struct sockaddr_in address; 
	int sock = 0, valread; 
	struct sockaddr_in serv_addr; 
	char buffer[1024] = {0};
	char ip_address[24] = {0};
	int port_num = 0;
	char msg[1024] = {0};
	char str[1024] = {0};
    char command[1024] = {0};

	if (argc >= 2) {
		sscanf(argv[1], "%s", ip_address);
	}
	if (argc >= 3){
		sscanf(argv[2], "%d", &port_num);
	}	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 
	
	memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(port_num); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, ip_address, &serv_addr.sin_addr)<=0) 
	{
		printf("\nInvalid address/ Address not supported \n"); 
		return -1; 
	}

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n"); 
		return -1; 
	} 

    prompt();
    printf(ANSI_COLOR_RESET); 
    valread = recv(sock , buffer, sizeof(buffer), 0);
    if (strcmp(buffer ,"connection accepted") < 0){
        printf(ANSI_COLOR_RED "Server Connection Fail");
        return -1;
    }
    printf("\n%s", buffer);
	while(&free){
		memset(buffer,'\0',sizeof(buffer));
		memset(msg,'\0', sizeof(msg));
        printf(ANSI_COLOR_RESET "\nclient>");
        scanf("%s", command);
        if (strstr(":h", command) != NULL){
            printf(ANSI_COLOR_YELLOW);
            prompt();
        }
        else if(strstr(":r", command) != NULL){
            char account[48]={0};
            printf(ANSI_COLOR_CYAN);
            printf("Enter your user account to register:" ANSI_COLOR_RESET);
            scanf("%s", account);
            sprintf(msg,"%s#%s","REGISTER",account);
            send(sock , &msg , strlen(msg) , 0);
            if(recv(sock , buffer, sizeof(buffer), MSG_PEEK) > 0){
                memset(buffer,'\0',sizeof(buffer)); 
                valread = read(sock , buffer, sizeof(buffer)); 
            } 
            if(strcmp(msg,"100 OK")){
                printf("%s\n", "register successfully!");
            }
            else{
                printf("%s",ANSI_COLOR_MAGENTA  "register fail!" ANSI_COLOR_RESET); 
            }
        }
        else if(strstr(":l", command) != NULL){
            char account[48]={0};
            int port = 0;
            printf(ANSI_COLOR_GREEN "Enter your user account to login:" ANSI_COLOR_RESET);
            scanf("%s", account);
            printf(ANSI_COLOR_GREEN "Please enter your port:" ANSI_COLOR_RESET);
            scanf("%d", &port);
            while((port <= 1024 ) || (port > 65535)){
                printf(ANSI_COLOR_MAGENTA "Please enter your port between 1024 and 65535: " ANSI_COLOR_RESET);
                scanf("%d", &port);
            }
            sprintf(msg,"%s#%d",account,port);
            memset(buffer,'\0',sizeof(buffer));
            send(sock, &msg, strlen(msg), 0);
            recv(sock, buffer, sizeof(buffer), 0);
            if (strstr(buffer, "220 AUTH_FAIL") != NULL){
                printf(ANSI_COLOR_MAGENTA "Wrong Password or Have not register yet!" ANSI_COLOR_RESET);
                continue;
            }
            printf("%s\n", buffer);
            memset(buffer,'\0',sizeof(buffer));
            recv(sock, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT);
            if (strlen(buffer) == 0){
                continue;
            }
            recv(sock, buffer, sizeof(buffer), 0);
            printf("%s\n", buffer); 
        }
        else if(strstr(":L", command) != NULL){
            memset(msg,'\0',sizeof(msg));
            strcat(msg, "List");
            send(sock, &msg, strlen(msg), 0);
            memset(buffer,'\0',sizeof(buffer));
            recv(sock, buffer, sizeof(buffer), 0);
            printf("%s\n", buffer); 
        }
        else if(strstr(":html", command) != NULL){
            memset(msg, "\0", sizeof(msg));
            send(sock , &msg , strlen(msg) , 0); 
            int cnt = 0;
            do{
                cnt += 1;
                memset(buffer,'\0',sizeof(buffer));
                if (recv(sock, buffer, sizeof(buffer), MSG_PEEK) > 0){
                    valread = read(sock , buffer, sizeof(buffer)); 
                    printf("%s\n",buffer); 
                }

            }while(strlen(buffer) > 0);
            printf("%d", cnt);
        }
        else if(strstr(command, "e") != NULL){
            memset(msg, "\0", sizeof(msg));
            strcat(msg, "Bye~");
            printf("%s", msg);
            close(sock);
            return 0; 
        }
        else{
            printf(ANSI_COLOR_MAGENTA "Please input :h to help\n");
        }
	};
	return 0; 
} 


