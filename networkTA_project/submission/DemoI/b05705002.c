#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


int main(int argc , char *argv[])
{
	/*create socket*/
    int sock = 0;
    sock = socket(AF_INET , SOCK_STREAM , 0);	//create socket, socket(domain, type, protocol)
	
    if (sock < 0){		//check socket performance
        printf("Fail to create a socket.");
    }
    
    /*client connect to server*/
    struct sockaddr_in info;
    bzero(&info,sizeof(info));		//initialize 
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr("140.112.107.194");	//字串IP轉換成整數IP 
    info.sin_port = htons(33120);		//htons:Host TO Network Short integer
    
    int err = connect(sock,(struct sockaddr *)&info,sizeof(info));
    if(err == -1){
        printf("Connection error");
    }
	
    struct sockaddr_in my_addr;
	char receiveMessage[2000] = {0};
    /*Get my IP
    int myPort, n;
    char myPortC[10] = {};
    

    bzero(&my_addr, sizeof(my_addr));
    unsigned int len = sizeof(my_addr);
    getsockname(sock, (struct sockaddr *) &my_addr, &len);
    myPort = ntohs(my_addr.sin_port);
    n = sprintf (myPortC, "%d",myPort);*/
    

	bool connected;
    connected = true;
    
    int tmp = 0;
	int flag=0;
    char message1[100] = {"REGISTER#"};		//message sent to server
    char tag[2] = {"#"};		//message sent to server
	char list[100] = {"List"};
	char exit[100] = {"Exit"};
	while (connected){
		      
		printf("Enter 1 for Register, Enter 2 for Login : ");
        std::cin >> tmp;
        if(tmp == 1){
            //register
            char name[20] = {};
            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s",receiveMessage);
            
            printf("Enter the name you want to register : ");
            std::cin >> name;
            strcat(message1,name);		//把name接到register#, message1="register#name" 
            
            //send register message to client
            send(sock,message1,sizeof(message1),0);
            memset(receiveMessage, '\0', sizeof(receiveMessage));	//把recieveMessage的值clear 
            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s",receiveMessage);

        }
        
        else if(tmp == 2){
            //-----login------
            char message2[20] = {};
            char portname[20] = {0};
			
            printf("Enter your name: ");
            std::cin >> message2;
            strcat(message2,"#");
            
            printf("Enter the port name: ");
            std::cin >> portname;
            strcat(message2,portname);
            
            //send login message
            send(sock,message2,strlen(message2),0);
			
            memset(receiveMessage,'\0',sizeof(receiveMessage));
            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s\n",receiveMessage);

			
			memset(receiveMessage,'\0',sizeof(receiveMessage));
            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s\n",receiveMessage);
			

			printf("Enter the number of actions you want to take. 1 to ask for the latest list, 8 to Exit: ");
						
			
			flag=0;
            
			while(flag!=8){
				std::cin >> flag;
				if(flag==1){
					send(sock,list,strlen(list),0);

					memset(receiveMessage,'\0',sizeof(receiveMessage));
            		recv(sock,receiveMessage,sizeof(receiveMessage),0);
            		printf("%s\n",receiveMessage);
				}
				else if (flag==8){
					send(sock,exit,strlen(list),0);
			
					memset(receiveMessage,'\0',sizeof(receiveMessage));
            		recv(sock,receiveMessage,sizeof(receiveMessage),0);
            		printf("%s\n",receiveMessage);
				}
				else{
					printf("Wrong\n");
				}
			}
			connected=false;
        }
		
        else{
            printf("Wrong Number\n");
            
        }
    }
    
    printf("close Socket\n");
    close(sock);
    return 0;
} 
