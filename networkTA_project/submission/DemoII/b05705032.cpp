#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
using namespace std;

#define MAXCONN 100

char client_msg[1024];

typedef struct Client{
    struct sockaddr_in addr;
    bool isReg;
    int isConn;
    int clientfd;
    int index;
    string name;
    int port;
    int accountBalance;
} Client;

//pthread_mutex_t activeConnMutex;
pthread_mutex_t lock[MAXCONN];
pthread_t threadID[MAXCONN];
Client clients[MAXCONN]; // to save the data of each client

char* getList(char list[1024]){ // use to find the number of accounts online
    int online = 0;
    char numOnline[100]; // save number of accounts onliene
	char accountOnline[1024]; // save data of ALL accounts online
	bzero(numOnline,100);
	bzero(accountOnline,1024);
    	
   	for(int i = 0 ; i < MAXCONN ; i++){
       	if(clients[i].isConn){	// if the client is connected
       		online++; // count account online
       		char tmp[1024]; // save data of EACH accounts online
        	bzero(tmp,1024);
        	
			/*	FOR CHECKING
        	cout << "checking............" << endl;
        	cout << "index: " << clients[i].index << endl;
       		cout << "name: " << clients[i].name << endl;
        	cout << "port: " << clients[i].port << endl;
        	*/
        		
			char clientName[clients[i].name.length()+1];  	
       		strcpy(clientName, clients[i].name.c_str()); // to change clients.name(string) into nameSend(char)
        	
			// combine all the data into temp	
			snprintf(tmp, 1024, "%d \t%s # %s # %d\r\n", clients[i].index, clientName, inet_ntoa(clients[i].addr.sin_addr), clients[i].port);
			//cout << "check temp: " << temp << endl;
			
			// collect data of EACH account into accountOnline
			strcat(accountOnline, tmp);
		}
    } // end for
    
    snprintf(numOnline, 100, "%d\r\n", online); // convert online(int) into numOnline(char)
    strcat(list, numOnline);
   	strcat(list, accountOnline); // collect both numOnline and accountOnline to listReply
   	//cout << "check list: " << numOnline << " " << accountOnline << " " << listReply << endl;
   	return list;
}// end char

void *socketThread(void* arg){
    Client *client = (Client *)(arg);
    char name[20];
    char port[20];
    
    char accountBalance[20] = {"10000\n"};
	char registerFail[20] = {"210 FAIL"};
	char loginFail[20] = {"220 AUTH_FAIL"};
	char success[20] = {"100 OK"};
	char exit[20] = {"Bye"};
    
    // to make code below more simple and clear
    struct sockaddr_in addr = client->addr;
	bool isRegister = client->isReg;
    int clientfd = client->clientfd;
    int clientIndex = client->index;
    int accBalance = client->accountBalance;
    
    while(1){  
        cout << endl;
        bzero(client_msg, 1024);
		recv(clientfd, client_msg, 1024 , 0);
        cout << "Client request: " << client_msg << endl; 
        
        // ------  to split message by delimiter # ------
        int length = 0;
		char * running = strdupa(client_msg);
		char * part1; // message
		char * part2;
		
		//client_message[strcspn(client_message, "\n")] = '\0';
		length = strlen(client_msg);

		if(length > 4){  // means that the command is not request for list and exit (their length are both > 4)
 			part1 = strsep(&running, "#"); 
			part2 = strsep(&running, "#"); 
		//	cout << "Msg1: " << part1 << endl; 
		//	cout << "Msg2: " << part2 << endl; 
		}
		// -----------------------------------------------
                
		int now = clientIndex;
		// analyze the command...
    	if(strcmp(part1, "REGISTER") == 0){ // to register
        	if(!isRegister){ // haven register
        		bool check = true;
				strcat(name, part2); // name = part2
     			clients[now].name = string(name); // save name(char -> string) into client data
     			// check whether the name already existed
     			for(int i = 0 ; i < MAXCONN ; i++){
     				//cout << clients[now].name << " " << clients[i].name << endl;
     				if(clients[now].name == clients[i].name and now != i){
     					cout << "Server: Name Duplicate\n";
     					send(clients[now].clientfd, registerFail, sizeof(registerFail), 0);
     					isRegister = false;
     					check = false;
     					bzero(name, 20);
					}
     			}
     			
     			if(check){
 		  			cout << "Server: Approve to REGISTER" << endl;
 		  			send(clients[now].clientfd, success, sizeof(success), 0);
 		  			isRegister = true;
 		  		}
 			}
 			else{ // already registered
 				cout << "Server: Fail on REGISTER" << endl;
				send(clients[now].clientfd, registerFail, sizeof(registerFail), 0);
		 	}
	 	}
        else if(strcmp(client_msg, "List") == 0){ // request for online list
			char onlineList[1024] = {"\nnumber of accounts online: "};
			char listReply[1024];
			getList(listReply);

			strcat(onlineList, listReply); // combine
			//cout << "check for TMP: " << onlineList << endl;
            send(clients[now].clientfd, onlineList, strlen(onlineList), 0);
            cout << "Server: ONLINE LIST sent" << endl;
            bzero(onlineList, 1024);
            bzero(listReply, 1024);
		}
    	else if(strcmp(client_msg, "Exit") == 0){ // to exit
	 		//cout << "Server: Approve to EXIT" << endl;
	 		send(clients[now].clientfd, exit, strlen(exit), 0);
	 		
	 		client->isConn = 0; // disconnected
	 		cout << "One client EXIT\n";
  			close(clients[now].clientfd);
  			pthread_exit(NULL);
	 	}
    	else{ // to login
	 		if(isRegister){
	 			//cout << "check name: " << part1 << " " << name << "\n";
	 			if(strcmp(part1, name) == 0){ // the name is registered
	 				bool check = true;
	 				cout << "Server: Client existed! Approve to LOGIN" << endl;
	 				strcat(port, part2);	// save the port number
	 				clients[now].port = atoi(port);
	 				if(clients[now].port >= 1024 and clients[now].port <= 65535){
	 					// check whether the port already in used
	 					for(int i = 0 ; i < MAXCONN ; i++){
     						if(clients[now].port == clients[i].port and now != i){
     							cout << "Server: Port Duplicate\n";
     							send(clients[now].clientfd, loginFail, sizeof(loginFail), 0);
     							check = false;
     						//	bzero(name, 20);
     							bzero(port, 20);
							}
     					}
     					if(check)
	 						send(clients[now].clientfd, accountBalance, sizeof(accountBalance), 0);	
					}
	 				else
	 					send(clients[now].clientfd, loginFail, sizeof(loginFail), 0);
	 				
	 				if(check){
						// a hidden request for list (send if online num > 1)
	 					bzero(client_msg, 1024);
	 					recv(clients[now].clientfd, client_msg, 1024, 0);
	 					if(strcmp(client_msg, "List") == 0){
	 						int online = 0;
	 						for(int i = 0 ; i < MAXCONN ; i++){
       							if(clients[i].isConn){	
									online++;
								}
							}
							//cout << "Acc online: " << online << endl;
							char tmp[10];
							bzero(tmp, 10);
							sprintf(tmp, "%d", online); // convert int to char
							cout << "NOW online: " << tmp << endl;
							send(clients[now].clientfd, tmp, sizeof(tmp), 0); // send number of account to client
						
							if(online > 1){
								char onlineList[1024] = {"\nnumber of accounts online: "};
								char listReply[1024];
								getList(listReply);
	
								strcat(onlineList, listReply); // combine
								//cout << "check for TMP: " << onlineList << endl;
            					send(clients[now].clientfd, onlineList, strlen(onlineList), 0);
        					    cout << "Server: ONLINE LIST sent" << endl;
        					    bzero(onlineList, 1024);
         						bzero(listReply, 1024);
							}// end if on
						}// end if List
					} // end check
				}//end if
			 	else{
			 		cout << "Wrong name LOGIN" << endl;
			 		send(clients[now].clientfd, loginFail, sizeof(loginFail), 0);;
			 	}
			}
			else{ // haven register
				cout << "Fail on LOGIN" << endl;
				send(clients[now].clientfd, loginFail, sizeof(loginFail), 0);
			}
		}//end else    
    } //end while
} // end socketThread

int main(int argc, char const *argv[]){
	int sockfd = 0, portNum = 0;
	
	for(int i = 0 ; i < MAXCONN ; i++)
		clients[i].isConn = 0; // haven connected
    
   // CREATE SOCKET
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		cout << "ERROR creating socket";
        exit(0);
	}
	portNum = atoi(argv[1]);
   
	// CONNECT SOCKET DESCRIPTOR
	struct sockaddr_in  serv_addr;
	// initialize the value of [struct sockaddr_in] 
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET; //AF_INET means using TCP protocol
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //any in address
	serv_addr.sin_port = htons(portNum);
   
	// BINDING server address the socket
	if(bind(sockfd, (struct sockaddr*)(&serv_addr), sizeof(serv_addr)) < 0){
		cout << "ERROR binding socket";
        exit(0);
	}

	// LISTENING to the socket
	if(listen(sockfd, 10) < 0){
		cout << "ERROR listening socket";
        exit(0);
	}
	else{
		cout << "Listening...\n";
	}
    
	while(1){
		
		// find the last index allow for new connection
		int i = 0;
		while(i < MAXCONN){
			if(!clients[i].isConn){
            	break;
        	}
        	++i;           
		}   
       
		// ACCEPT 
		struct sockaddr_in addr;
		socklen_t sin_size = sizeof(struct sockaddr_in);
		int clientfd;
		if((clientfd = accept(sockfd, (struct sockaddr*)(&addr), &sin_size)) < 0){   
			cout << "ERROR accepting socket";
        	exit(0);
    	}

        char connection[20] = "Connection Accepted";
    	send(clientfd, connection, sizeof(connection), 0); // send to client so they know that success in connection
    	cout << "Server: " << connection << endl;
    	
    	// assign according data
    	clients[i].clientfd = clientfd;
    	clients[i].addr = addr;
    	clients[i].isConn = 1;
    	clients[i].isReg = false;
    	clients[i].index = i;
       
       	// CREATE a thread for a client
    	pthread_create(&threadID[i], NULL, &socketThread, &clients[i]);   // each threadID run socketThread  
    	//pthread_join(threadID[i], NULL);  // cant put this side else, no two clients can access same server in same time
    	
	} //end while
	
	return 0;
}//end main
