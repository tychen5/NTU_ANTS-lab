#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string> 
#include <iostream>

int main(int argc , char *argv[])
{

//create socket
    int sock = 0;
    sock = socket(AF_INET , SOCK_STREAM , 0);//create socket

//server information

    struct sockaddr_in info;

    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr("140.112.107.194");
    info.sin_port = htons(33120);

//connect to server
    connect(sock,(struct sockaddr *)&info,sizeof(info));



    int signal = 0;//determine action to take
    char s0[256] = {"REGISTER#"};
    char s1[20] = {0};//Name#Port
    char s2[10] = {"List"};
    char s3[10] = {"Exit"};
    char toRe[1024] = {0};//message from server

//receive the "connection accepted" message
    recv(sock,toRe,sizeof(toRe),0);
    printf("Server reply: %s\n",toRe);//Server:conncetion accepted
	
    bool connected = true;
	
    while (connected){
        printf("Enter 1 for Register, 2 for Login: ");
        std::cin >> signal;
		if(signal == 1){
            //Steps for Register
			printf("Enter the name you want to register: ");
            char name[10] = {};
			std::cin >> name;
            strcat(s0,name);//REGISTER#NAME
            
            send(sock,s0,sizeof(s0),0);
            
			memset(toRe,'\0', sizeof(toRe));//clean
            recv(sock,toRe,sizeof(toRe),0);
            printf("Server reply: %s\n",toRe);
        }
        else if(signal == 2){
			printf("Enter your name: ");
			std::cin >> s1;
            
            printf("Enter the port number: ");
            char portnum[10] = {};
            std::cin >> portnum;
            
			strcat(s1,"#");//NAME#
            strcat(s1,portnum);//NAME#PORTNUM
            
            send(sock,s1,strlen(s1),0);
		
           //read 10000
	    memset(toRe,'\0',sizeof(toRe));
            recv(sock,toRe,sizeof(toRe),0);
            printf("Server reply: %s\n",toRe);

	    //read list
  	    memset(toRe,'\0',sizeof(toRe));
            recv(sock,toRe,sizeof(toRe),0);
           printf("Server reply1: %s\n",toRe);

            while(signal != 8){
            	printf("Enter the action you want to take, 1 to ask for the latest list, 8 to Exit: ");
				std::cin >> signal;
				if(signal == 1){
					send(sock,s2,sizeof(s2),0);
					
					memset(toRe,'\0',sizeof(toRe));
            		recv(sock,toRe,sizeof(toRe),0);
            		printf("Server reply: %s\n",toRe);
				}
				else if(signal == 8){
					send(sock,s3,sizeof(s3),0);

	    memset(toRe,'\0',sizeof(toRe));
            recv(sock,toRe,sizeof(toRe),0);
            printf("Server reply: %s\n",toRe);

            		close(sock);
            		connected = false;
				}
			}
        }
    }
    close(sock);
    return 0;
}
