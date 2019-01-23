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
#include<pthread.h>
using namespace std;

bool alive =false;
int serverfd=0;
void* asServer(void* p)
{

    char* port = (char*)p;

    struct sockaddr_in dest;
    dest.sin_port = htons(atoi(port));
    dest.sin_addr.s_addr = INADDR_ANY;
    dest.sin_family = AF_INET;

    int sd3 = socket(AF_INET, SOCK_STREAM, 0);
    int b = bind(sd3, (struct sockaddr*)&dest, sizeof(dest));
    listen(sd3, 10); //listen to the port

    while(alive)
    {
        int clientfd;
        struct sockaddr_in client_addr;
        int addrlen = sizeof(client_addr);
        clientfd = accept(sd3, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen);

        char buf[100];
        bzero(buf, 100);
        recv(clientfd, buf, sizeof(buf), 0);
        cout << "Peers chatting";
        printf("%s\n",buf);

        send(serverfd, buf, sizeof(buf), 0);
    }
}

int main(int argc , char *argv[])
{
	//for transaction
	pthread_t tid;       /* the thread identifier */
    pthread_attr_t attr; /* set of attributes for the thread */
//create socket
    int sock = 0;
    sock = socket(AF_INET , SOCK_STREAM , 0);//create socket

//server information

    struct sockaddr_in my_addr;
	struct sockaddr_in info;

    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr("10.0.2.15");
    info.sin_port = htons(2018);

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
	int read_size=0;
	
    while (connected){
        printf("Enter 1 for Register, 2 for Login: ");
        std::cin >> signal;
		if(signal == 1){
            //Steps for Register
			printf("Enter the name you want to register: ");
            char name[10] = {};
			char value[10] = {};
			std::cin >> name;
			printf("$: ");
			std::cin >> value;
            strcat(s0,name);//REGISTER#NAME
            send(sock,s0,sizeof(s0),0);
            
			memset(toRe,'\0', sizeof(toRe));//clean
            recv(sock,toRe,sizeof(toRe),0);
            printf("Server reply: %s\n",toRe);
			send(sock,value,sizeof(value),0);
        }//END REGISTER
        else if(signal == 2){
			printf("Enter your name: ");
			std::cin >> s1;
            
            printf("Enter the port number: ");
            char portnum[10] = {};
            std::cin >> portnum;
            
			strcat(s1,"#");//NAME#
            strcat(s1,portnum);//NAME#PORTNUM
            
            send(sock,s1,strlen(s1),0);
		
           //read 10000 & list
	    	memset(toRe,'\0',sizeof(toRe));
            recv(sock,toRe,sizeof(toRe),0);
            printf("Server reply: %s\n",toRe);

	    	//read list
  	   		memset(toRe,'\0',sizeof(toRe));
            recv(sock,toRe,sizeof(toRe),0);
            for(int i = 0; i < sizeof(toRe); i ++){
			cout << toRe[i];
			}
			
			pthread_attr_init(&attr);
            pthread_create(&tid, &attr, asServer, (void*)(portnum));
            alive =true;
			
			while(signal != 8){
				//chech if someone wants to talk to you
				//struct timeval tv;
				//fd_set readfds;
				//tv.tv_sec = 10;
				//tv.tv_usec = 500000;
				//FD_ZERO(&readfds);
				//FD_SET(sock, &readfds);
				//int rv = select(sock+1, &readfds, NULL, NULL, &tv);

				//if(rv > 0){
				//	memset(toRe,'\0',sizeof(toRe));
            	//	recv(sock,toRe,sizeof(toRe),0);
            	//	printf("Server reply: %s\n",toRe);
				//	cout << "Communication!";
				//	rv=0;
				//}
				if(true){            	
					printf("Enter the action you want to take, 1 to ask for the latest list, 6 to communicate with other client, 8 to Exit: ");
					std::cin >> signal;
					
					if(signal == 1){
						send(sock,s2,sizeof(s2),0);	
						memset(toRe,'\0',sizeof(toRe));
            			recv(sock,toRe,sizeof(toRe),0);
            			printf("Server reply: %s\n",toRe);
					}
					
					if(signal== 6){
						char chat[100];
						cout << "Enter CHAT#HisName#$";
						std::cin>> chat;
						send(sock,chat,sizeof(chat),0);
					
						//memset(toRe,'\0',sizeof(toRE));
            			//recv(sock,toRe,sizeof(toRe),0);
            			//printf("Server reply: %s\n",toRe);
						//cout<<"server reply:" << toRe << "'\n'";
						
						char ip[20];
						strcpy(ip, "10.0.2.15");
						//char port[20];
int port = 1075;
						cout << "Enter port num";
						cin >> port;
cout << port;
						
						struct sockaddr_in dest;
    					int sd2;

    					memset(&dest, 0, sizeof(dest)); 
    					dest.sin_port = htons(port);
    					dest.sin_addr.s_addr = inet_addr(ip);
    					dest.sin_family = AF_INET;

    					sd2 = socket(AF_INET, SOCK_STREAM, 0);
    					if(connect(sd2, (struct sockaddr*)&dest, sizeof(dest)) < 0){
        					printf("WRONG!\n");
        					abort();
    					}
    					
    					char to_peer[100];
                        memset(to_peer,'\0',sizeof(to_peer));
                        for(int i=0; i <sizeof(to_peer); i++){
                        	to_peer[i] = chat[i];
						}
						send(sd2, to_peer, sizeof(to_peer), 0);
					}//end signal = 6
					else if(signal == 8){
						send(sock,s3,sizeof(s3),0);
	   					memset(toRe,'\0',sizeof(toRe));
            			recv(sock,toRe,sizeof(toRe),0);
            			printf("Server reply: %s\n",toRe);
						close(sock);
            			connected = false;
					}//end signal = 8
				}//END an action
	   		}//end signal
		}//END LOG IN
	}//end while connected
    close(sock);
    return 0;
}//end main
