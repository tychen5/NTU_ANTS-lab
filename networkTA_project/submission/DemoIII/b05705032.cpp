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
#include <openssl/ssl.h>
#include <openssl/err.h>
using namespace std;

#define MAXCONN 100
#define FAIL -1

SSL_CTX *ctx; // ssl communication environment CTX
SSL *ssl;
char client_msg[1024];

//Init server CTX
SSL_CTX* InitServerCTX(void){   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc. 
    SSL_load_error_strings();   // load all error messages */
    method = SSLv23_server_method();  // create new server-method instance 
    ctx = SSL_CTX_new(method);   // create new context from method
    if ( ctx == NULL ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

//Load the certificate with cert.key and private.key 
void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile){
    //set the local certificate from CertFile
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    //set the private key from KeyFile
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    //verify private key 
    if ( !SSL_CTX_check_private_key(ctx) ){
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

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
        		
			char clientName[clients[i].name.length()+1];  	
       		strcpy(clientName, clients[i].name.c_str()); // to change clients.name(string) into nameSend(char)
        	
			// combine all the data into temp	
			snprintf(tmp, 1024, "%d \t%s # %s # %d\r\n", clients[i].index, clientName, inet_ntoa(clients[i].addr.sin_addr), clients[i].port);
			
			// collect data of EACH account into accountOnline
			strcat(accountOnline, tmp);
		}
    } // end for
    
    snprintf(numOnline, 100, "%d\r\n", online); // convert online(int) into numOnline(char)
    strcat(list, numOnline);
   	strcat(list, accountOnline); // collect both numOnline and accountOnline to listReply
   	return list;
}// end char

// Transfer balance between clients
int transfer(char payer[], char payee[], int amount){
	int canTransfer = -1;
	for (int i = 0 ; i < MAXCONN ; i++){
		if (string(payer) == clients[i].name){	
			if(clients[i].accountBalance < amount){
				canTransfer = 1;
				break;
			}
			else{
				canTransfer = 0;
				break;
			}
		}
	}
	if(canTransfer == 0){
		for(int i = 0 ; i < MAXCONN ; i++){
			if (string(payer) == clients[i].name){	
				clients[i].accountBalance -= amount;
				cout << clients[i].name << " accBalance: " << clients[i].accountBalance << endl;
			}
			if(string(payee) == clients[i].name){
				clients[i].accountBalance += amount;
				cout << clients[i].name << " accBalance: " << clients[i].accountBalance << endl;
			}
		}
	}
	return canTransfer;
}

void *socketThread(void* arg){
    Client *client = (Client *)(arg);
    char name[20];
    char port[20];
    
    char accountBalance[20] = {};
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
        bzero(client_msg, 1024);
		SSL_read(ssl, client_msg, sizeof(client_msg)); 
		if(strlen(client_msg)!=0){
		cout << endl;
        cout << "Client request: " << client_msg << endl; 
        
        // ------  to split message by delimiter # ------
        int length = 0;
		char * running = strdupa(client_msg);
		char * part1; // message
		char * part2;
		char * part3;
		
		//client_message[strcspn(client_message, "\n")] = '\0';
		length = strlen(client_msg);
		string jus = string(client_msg);
		int l = 0; 
		for(int i = 0; i < jus.length() ; i++)
			if(jus[i] == '#')	l++;
		
		if(length > 4){  // means that the command is not request for list and exit (their length are both > 4)
 			part1 = strsep(&running, "#"); 
			part2 = strsep(&running, "#"); 
			if(l == 2)	part3 = strsep(&running, "#"); 
		}
		// -----------------------------------------------
        l == 0;   
		int now = clientIndex;
		// analyze the command...
    	if(strcmp(part1, "REGISTER") == 0){ // to register
        	if(!isRegister){ // haven register
        		bool check = true;
				strcat(name, part2); // name = part2
     			clients[now].name = string(name); // save name(char -> string) into client datab
				
				//checking
				//cout << "checking name: " << clients[now].name << endl;
        	
     			// check whether the name already existed
     			for(int i = 0 ; i < MAXCONN ; i++){
     				if(clients[now].name == clients[i].name and now != i){
     					cout << "Server: Name Duplicate\n";
						SSL_write(ssl, registerFail, strlen(registerFail));
     					isRegister = false;
     					check = false;
     					bzero(name, 20);
					}
     			}
     			
     			if(check){
 		  			cout << "Server: Approve to REGISTER" << endl;
					SSL_write(ssl, success, strlen(success));// 100 OK
					
					// receive account balance
					bzero(client_msg, 1024);
					SSL_read(ssl, client_msg, sizeof(client_msg)); 
        			clients[now].accountBalance = atoi(client_msg); // char* to int and then save to to clients
        			SSL_write(ssl, success, strlen(success));// 100 OK send again
 		  			isRegister = true;
 		  		}
 			}
 			else{ // already registered
 				cout << "Server: Fail on REGISTER" << endl;
				SSL_write(ssl, registerFail, sizeof(registerFail));// 210 FAIL
		 	}
	 	}
        else if(strcmp(client_msg, "List") == 0){ // request for online list
        	char balance_online[1024];
			char onlineList[1024] = {"\nnumber of accounts online: "};
			char listReply[1024];
			getList(listReply);
			
			snprintf(balance_online, 1024, "%d", clients[now].accountBalance); // int to char
			strcat(onlineList, listReply); // combine
			strcat(balance_online, onlineList); // combine
            SSL_write(ssl, balance_online, strlen(balance_online));
            cout << "checking balance: " << clients[now].accountBalance << endl;
            cout << "Server: ACC BALANCE and ONLINE LIST sent" << endl;
            bzero(balance_online, 1024);
			bzero(onlineList, 1024);
            bzero(listReply, 1024);
		}
    	else if(strcmp(client_msg, "Exit") == 0){ // to exit
	 		SSL_write(ssl, exit, strlen(exit));
	 		
	 		client->isConn = 0; // disconnected
	 		cout << "One client EXIT\n";
  			close(clients[now].clientfd);
  			pthread_exit(NULL);
	 	}
    	else if(l == 1){ // to login    --> l = number of '#'
	 		if(isRegister){
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
     							SSL_write(ssl, loginFail, strlen(loginFail));
     							check = false;
     						//	bzero(name, 20);
     							bzero(port, 20);
							}
     					}
     					if(check){
     						// combine all the data into temp	
     						bzero(accountBalance, 10);
							snprintf(accountBalance, 10, "%d\r\n", clients[now].accountBalance); // int to char and then reply to the client
	 						cout << "checking balance after: " << accountBalance << endl;
							SSL_write(ssl, accountBalance, strlen(accountBalance));
	 					}
					}
	 				else
	 					SSL_write(ssl, loginFail, strlen(loginFail));
	 					
	 				if(check){
						// a hidden request for list (send if online num > 1)
	 					bzero(client_msg, 1024);
	 					SSL_read(ssl, client_msg, sizeof(client_msg));
	 					
	 					if(strcmp(client_msg, "List") == 0){
	 						int online = 0;
	 						for(int i = 0 ; i < MAXCONN ; i++){
       							if(clients[i].isConn){	
									online++;
								}
							}

							char tmp[10];
							bzero(tmp, 10);
							sprintf(tmp, "%d", online); // convert int to char
							cout << "NOW online: " << tmp << endl;
							SSL_write(ssl, tmp, strlen(tmp));
							
							if(online > 1){
								char onlineList[1024] = {"\nnumber of accounts online: "};
								char listReply[1024];
								getList(listReply);
	
								strcat(onlineList, listReply); // combine
            					SSL_write(ssl, onlineList, strlen(onlineList));
        					    cout << "Server: ONLINE LIST sent" << endl;
        					    bzero(onlineList, 1024);
         						bzero(listReply, 1024);
							}// end if on
						}// end if List
					} // end check
				}//end if
			 	else{
			 		cout << "Wrong name LOGIN" << endl;
			 		SSL_write(ssl, loginFail, strlen(loginFail));
			 	}
			}
			else{ // haven register
				cout << "Fail on LOGIN" << endl;
				SSL_write(ssl, loginFail, strlen(loginFail));
			}
		}//end login else
		else{
			cout << "Client Transaction Done\n";
	 		int success = -1;
			success = transfer(part1, part3, atoi(part2));
			if(success == 0){
				cout << "Transfer successfully\n";
			//	SSL_write(ssl, "Transfer Done", strlen("Transfer Done"));
			}
			else if(success == 1)
				cout << "Transfer fail\n";
		}
	}
    } //end while
} // end socketThread

int main(int argc, char const *argv[]){
   	SSL_library_init();
	SSL_load_error_strings();
	char CertFile[] = "mycert.pem";
    char KeyFile[] = "mykey.pem";

	ctx = InitServerCTX();  //initial
    LoadCertificates(ctx, CertFile, KeyFile); 
	
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
    	
    	ssl = SSL_new(ctx);              // new SSL state
        SSL_set_fd(ssl, clientfd);       // SSL connection with socket

        if ( SSL_accept(ssl) == FAIL )
        	ERR_print_errors_fp(stderr);
    	else{
        	char connection[20] = "Connection Accepted";
        	SSL_write(ssl, connection, strlen(connection)); // send to client so they know that success in connection
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
    	}// end ssl
    	
	} //end while
	
	SSL_CTX_free(ctx);         // release SSL
	return 0;
}//end main
