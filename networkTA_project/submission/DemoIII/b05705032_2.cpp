#include <stdio.h>
#include <stdlib.h>
#include <string.h> //bzero
#include <string> 
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h> //htons
#include <openssl/ssl.h>
#include <openssl/err.h>
#define FAIL -1

using namespace std; 

bool isLogIn = false;
bool isRegister = false;
SSL *ssl;

int Connecting(const char* ip, int port){
	struct sockaddr_in dest; // an Internet endpoint address
	int sokfd; //socket descripter

	memset(&dest, 0, sizeof(dest)); 
	dest.sin_port = htons(port);
	dest.sin_addr.s_addr = inet_addr(ip);
	dest.sin_family = AF_INET;

	sokfd = socket(AF_INET, SOCK_STREAM, 0);
	if(connect(sokfd, (struct sockaddr*)&dest, sizeof(dest)) < 0){
		cout << "Connection ERROR\n";
		abort();
	}
	return sokfd;
}

SSL_CTX* InitCTX(void){   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms(); 
    SSL_load_error_strings(); 
    method = SSLv23_client_method(); 
    ctx = SSL_CTX_new(method); 
    if ( ctx == NULL ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

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

void ShowCerts(SSL* ssl){   
    X509 *cert;
    char *line;
 
    cert = SSL_get_peer_certificate(ssl); // get the server's certificate
    if ( cert != NULL ){
        printf("---------------------------------------------\n");
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       // free the malloc'ed string 
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);\
        free(line);       // free the malloc'ed string 
        X509_free(cert);     // free the malloc'ed certificate copy */
        printf("---------------------------------------------\n");
    }
    else
        printf("No certificates.\n");
}

void* socketThread(void* arg){ //thread to receive the transaction messages
	SSL_CTX *ctxClient;
	ctxClient = InitServerCTX();

	char* port = (char*)arg;
	char CertFile[] = "mycert.pem";
    char KeyFile[] = "mykey.pem";
	LoadCertificates(ctxClient, CertFile, KeyFile); // load certs and key

	struct sockaddr_in dest;
	dest.sin_port = htons(atoi(port));
	dest.sin_addr.s_addr = INADDR_ANY;
	dest.sin_family = AF_INET;

	int sock_Client = socket(AF_INET, SOCK_STREAM, 0);

	if(bind(sock_Client, (struct sockaddr*)&dest, sizeof(dest)) < 0){
		cout << "ERROR binding socket";
        exit(0);
	}

	if(listen(sock_Client, 10) < 0){
		cout << "ERROR listening socket";
        exit(0);
	}

	while(isLogIn){
		SSL *ssl_Client;
		int clientfd; //client descriptor
		struct sockaddr_in client_addr;
		int addrlen = sizeof(client_addr);
		clientfd = accept(sock_Client, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen);

		ssl_Client = SSL_new(ctxClient);              // get new SSL state with context 
        SSL_set_fd(ssl_Client, clientfd);      // set connection socket to SSL state

        if ( SSL_accept(ssl_Client) == FAIL ) //do SSL-protocol accept
        	ERR_print_errors_fp(stderr);
        else{
        	char buf[1024];
			bzero(buf, 1024);
			SSL_read(ssl_Client, buf, sizeof(buf));
			cout << "Transaction with client: " << buf << endl;
			SSL_write(ssl, buf, strlen(buf)); //send to server
			//isLogIn = true;
			//cout << "again";
		//	bzero(buf, 1024);
		//	SSL_read(ssl, buf, strlen(buf)); //send to server
		//	cout << "Transaction reply: " << buf << endl;
        }
	}
	SSL_CTX_free(ctxClient);        // release context
}

int main(int argc, char const *argv[]){
	SSL_library_init();
	SSL_load_error_strings();
	
	pthread_t tid[1];       // open only one thread
   	pthread_attr_t attr;
	
    struct hostent *server = gethostbyname(argv[1]);
    int portNum = atoi(argv[2]);

	isLogIn = 0;

	if(argc < 3)
		printf("Please enter IP address and port number\n");
	
	int sd = Connecting(argv[1], portNum);//connection

	SSL_CTX *ctx;
	ctx = InitCTX();
	ssl = SSL_new(ctx);      // SSL connection
    SSL_set_fd(ssl, sd); 

    if ( SSL_connect(ssl) == FAIL )   // perform the connection
        ERR_print_errors_fp(stderr);
    else{   
    	char buf[128];
    	bzero(buf, 128);
    	SSL_read(ssl, buf, sizeof(buf)); // get reply & decrypt 
		cout << "Connection State: " << buf << endl;
		
		char server_reply[1024];
		
		char name[20] = {};
		char askFor[20] = {"List"};
		char toExit[20] = {"Exit"};
		
		while(1){
			int option;
			cout << "\nEnter 1 for Register, 2 for Login: ";
        	cin >> option;
        	bzero(server_reply, 64);
        	
			if (option == 1){ //register
				char toRegister[20] = {"REGISTER#"};
				char balance[10] = {};
				cout << "Enter the name you want to register: ";
				cin >> name;
				
				strcat(toRegister, name); // combine [name] to the [toRegister]
				SSL_write(ssl, toRegister, strlen(toRegister));
				SSL_read(ssl, server_reply, sizeof(server_reply)); // get reply & decrypt 
				
				if(strcmp(server_reply, "100 OK") == 0){
            		cout << "Enter the initial balance: ";
            		cin >> balance;
            		SSL_write(ssl, balance, strlen(balance));
            		bzero(server_reply, 1024);
            		SSL_read(ssl, server_reply, sizeof(server_reply));
            		isRegister = true;
            	}
            	else isRegister = false;
				
				cout << "Server reply: ";
        		cout << server_reply << endl; 
			}
			else if(option == 2){ //isLogIn
				char name_port[20]; // name#port
				char port[20];
				
				cout << "Enter your name: ";
				cin >> name_port; 
				cout << "Enter the port number: ";
            	cin >> port;
            	
            	strcat(name_port, "#");
        		strcat(name_port, port);

				SSL_write(ssl, name_port, strlen(name_port));
				SSL_read(ssl, server_reply, sizeof(server_reply)); 
				
				// ACC balance
            	cout << "Server reply: ";
            	cout << server_reply ; 
            	
				if(strcmp(server_reply, "220 AUTH_FAIL") != 0){
					isLogIn = true; // so we can do the actions after client is login

					SSL_write(ssl, askFor, strlen(askFor));
					bzero(server_reply, 1024);
					SSL_read(ssl, server_reply, sizeof(server_reply)); // get reply & decrypt 
					if(strcmp(server_reply, "1") != 0 and strcmp(server_reply, "0") != 0){
						cout << "///auto print online list!///";
						SSL_write(ssl, askFor, strlen(askFor));
						bzero(server_reply, 1024);
						SSL_read(ssl, server_reply, sizeof(server_reply)); // get reply & decrypt
						cout << server_reply;  // auto online list
					}
					pthread_attr_init(&attr);
					pthread_create(&tid[0], &attr, socketThread, (void*)(port));

					while(isLogIn){
						int action;
						bzero(server_reply, 1024);
						cout << "\nEnter the number of actions yo want to take." << endl;
						cout << "1 to ask for the latest list, 2 for transaction, 8 to exit: ";
    					cin >> action;
    					
						if (action == 1){ // request for List
							SSL_write(ssl, askFor, strlen(askFor));
							memset(&server_reply, 0, sizeof(server_reply));
							bytes = SSL_read(ssl, server_reply, sizeof(server_reply)); // get reply & decrypt 
							cout << "Server reply: ";
							cout << server_reply;  // auto online list
						}
						else if(action == 2){ // transfer
							char Bname[20];
							char Bip[10]; 
							char Bport[10];
							char transferToB[100]; // use char not int ************
					
							cout << "\nEnter the name of the client: " ;
							cin >> Bname;
							if(strcmp(Bname, name) != 0){ // cant transaction with myself
								//Bip = {"127.0.0.1"}; // expected, easy to work, use in c++
								strcpy(Bip, "127.0.0.1");  // use in c
								//cout << "\nEnter the IP address of the client: " ;
								//cin >> Bip;
					
								cout << "Enter the port number of the client: " ;
								cin >> Bport;
	
								cout << "Enter the amount transfers to this client: " ;
								cin >> transferToB;

									//another ssl connection
									SSL_CTX *ctx2;
									ctx2 = InitCTX();
									SSL *ssl2;

									int sd2 = Connecting(Bip, atoi(Bport));

									ssl2 = SSL_new(ctx2);
						    		SSL_set_fd(ssl2, sd2);

						    		if ( SSL_connect(ssl2) == FAIL ) 
						        		ERR_print_errors_fp(stderr);
						    		else{
						    			//send transaction message client B
						    			char msg[1024];
						    			bzero(msg, 1024);
										snprintf(msg, 1024, "%s#%s#%s", name, transferToB, Bname);
										SSL_write(ssl2, msg, strlen(msg));
						    		}
						    		SSL_free(ssl2);        // release connection state 
						    		SSL_CTX_free(ctx2);        // release context
							}// end if
						}
						else if(action == 8){ // transaction
							SSL_write(ssl, toExit, strlen(toExit));
							bzero(server_reply, 1024);
							SSL_read(ssl, server_reply, sizeof(server_reply)); // get reply & decrypt 
							cout << "Server reply: ";
							cout << server_reply;  // auto online list
							
        					isLogIn = false;
							return 0;
						}
						else
							cout << "wrong action.";
					}
				}
			}
			else
				cout << "wrong number";	
		}
		SSL_free(ssl);  
	}
	SSL_CTX_free(ctx);        // release

	return 0;
}
