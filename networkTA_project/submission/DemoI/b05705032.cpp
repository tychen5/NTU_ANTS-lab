#include <stdio.h>
#include <stdlib.h>
#include <string.h> //bzero
#include <string> 
#include <iostream>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h> //htons

using namespace std; 


int main(int argc , char *argv[]){
	// ** According to TCP, we can only use send and recv, but not other(eg. write, read) 
	
	string server_name = argv[0];
    struct hostent *server = gethostbyname(argv[1]);
    int portNum = atoi(argv[2]);
    
    if (server == NULL){
        cout << "No such HOST!";
        exit(0);
    }
	
	// CREATE SOCKET
	// prototype: int socket(int domain, int type, int protocol)
	int sockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        cout << "Cannot create the socket!";
        exit(0);
    }
    
	// CONNECT SOCKET DESCRIPTOR
	struct sockaddr_in serv_addr; // [struct sockaddr_in] is used to save the data of the socket
	
	// initialize the value of [struct sockaddr_in] 
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;  // check : 
    //bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);  // cant used [server] that saved argv[1] before as the type is different
    serv_addr.sin_port = htons(portNum);
    
    // int connect(int sockfd, struct sockaddr *serv_addr,int addrlen)
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        cout << "ERROR connecting";
        exit(0);
    }

    // CONNECTION REPLY
    char server_reply[256]; // the message reply from server
    
    bzero(server_reply, 256);
	//read(sockfd, server_reply, 255);
	recv(sockfd, server_reply, sizeof(server_reply), 0);
    cout << "Connection State: " << server_reply;
    
	bool isLogIn = false;
	bool isRegister = false;
	char askFor[20] = {"List"};
	char toExit[20] = {"Exit"};
	
    while (1){
    	int choice = 0;
		
        cout << "\nEnter 1 for Register, 2 for Login: ";
        cin >> choice;
        
        if(choice == 1){ // register
			
			char toRegister[20] = {"REGISTER#"};
            char name[20] = {};
            cout << "Enter the name you want to register: ";
            cin >> name;
            
            strcat(toRegister, name); // combine [name] to the [toRegister]
            
            // cout << toRegister; 
            
            // send [toRegister] to the server and read the reply from server
            send(sockfd, toRegister, sizeof(toRegister), 0);
            bzero(server_reply, 256);
            recv(sockfd, server_reply, sizeof(server_reply), 0);
            
            cout << "Server reply: ";
        	cout << server_reply; 
        	
        	isRegister = true;
        }
      /*  else if(choice == 1 and isRegister){
        	cout << "Already Register!";
		}*/
        else if(choice == 2){ // login

            char name_port[20] = {}; // name#port
            cout << "Enter your name: ";
            cin >> name_port; 
            
            cout << "Enter the port number: ";
            char port[20] = {0};
            cin >> port;
            
            strcat(name_port, "#");
            strcat(name_port, port);

		//	cout << name_port;
			
			// send [name_port] to the server and read the reply from server
            send(sockfd, name_port, sizeof(name_port), 0);
			bzero(server_reply, 256);
            recv(sockfd, server_reply, sizeof(server_reply), 0);

            cout << "Server reply: ";
        	cout << server_reply;  
        	
        	server_reply[strcspn(server_reply, "\n")] = '\0';  // remove the \n in the server_reply
        	if(strcmp(server_reply, "10000") == 0){
        	//	cout << "We are login!";
        		isLogIn = true; // so we can do the actions after client is login
        		
        		//send(sockfd, askFor, sizeof(askFor), 0);
				bzero(server_reply, 256);
           		recv(sockfd, server_reply, sizeof(server_reply), 0);
            	
           		cout << "Server reply: ";
       			cout << server_reply;  
        		
			}
			
			while(isLogIn){
				int act_num = 0;
				cout << "\nEnter the number of actions yo want to take." << endl;
				cout << "1 to ask for the latest list, 8 to exit: ";
    			cin >> act_num;

				if (act_num == 1){ // request for List
					send(sockfd, askFor, sizeof(askFor), 0);
					bzero(server_reply, 256);
            		recv(sockfd, server_reply, sizeof(server_reply), 0);
            		
            		cout << "Server reply: ";
        			cout << server_reply;  
        			
				}
				else if(act_num == 8){ // Exit
					send(sockfd, toExit, sizeof(toExit), 0);
					bzero(server_reply, 256);
            		recv(sockfd, server_reply, sizeof(server_reply), 0);
        			
        			cout << "Server reply: ";
        			cout << server_reply;
        			
        			isLogIn = false;
        			return 0;
				}
				else{
					cout << "wrong action.";
				}
			}	
        }
        else{
            cout << "wrong number";
        }
    }


    cout << "close Socket\n";
    close(sockfd);
    return 0;
}
