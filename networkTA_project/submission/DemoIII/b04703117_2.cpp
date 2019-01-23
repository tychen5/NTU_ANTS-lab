#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <arpa/inet.h> 
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <resolv.h>
#include <list>

#include <openssl/ssl.h>
#include <openssl/err.h>

using namespace std;

#define FAIL    -1
#define local_buffer 1024
#define thread_handler 5
pthread_mutex_t inlock;
list<int>Queue;
std::string str1 = "mycert.pem";
std::string str2 = "mykey.pem";
char *a = new char[str1.length()+1];
char *b = new char[str2.length()+1];
  

int transfer_from_user=0;
void * socket_others(void* data);
int socketfd;
int portno;

SSL_CTX* InitCTX(void){   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = TLSv1_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

//Load the certificate 
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

//Init server instance and context
SSL_CTX* InitServerCTX(void){   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc. 
    SSL_load_error_strings();   // load all error messages */
    method = TLSv1_server_method();  // create new server-method instance 
    ctx = SSL_CTX_new(method);   // create new context from method
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

//Load the certificate 
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
//Load the certificate 
void LoadCertificates_server(SSL_CTX* ctx, char* CertFile, char* KeyFile){
    //New lines
    if (SSL_CTX_load_verify_locations(ctx, CertFile, KeyFile) != 1)
        ERR_print_errors_fp(stderr);
    
    if (SSL_CTX_set_default_verify_paths(ctx) != 1)
        ERR_print_errors_fp(stderr);
    //End new lines
    
    /* set the local certificate from CertFile */
    if (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if (SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if (!SSL_CTX_check_private_key(ctx))
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
    
    //New lines - Force the client-side have a certificate
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, 4);
    //End new lines

}

SSL *ssl;
int main(int argc, char *argv[]){ //input 執行
  	if (argc != 3){
		cout << endl << "Wrong Input." << endl;
		return 0;
	}
  	struct sockaddr_in server;

  	//Create socket
  	socketfd = socket(AF_INET , SOCK_STREAM , 0);
  	if (socketfd == -1){
		cout<<"Could not create socket";
  	}

  	struct hostent *serv;
  	serv = gethostbyname(argv[1]);

  	memset((char *) &server, 0, sizeof(server));
  	server.sin_family = AF_INET;
  	memmove((char *)&server.sin_addr.s_addr,(char *)serv->h_addr,serv->h_length);
  	server.sin_port = htons(atoi(argv[2]));
 
  	//Connect to remote server
  	if (connect(socketfd , (struct sockaddr *)&server , sizeof(server)) < 0){
		close(socketfd);
		abort();
  	}
  
	SSL_CTX *ctx;
	SSL_library_init();
	ctx = InitCTX();
	strcpy(a,str1.c_str());
	strcpy(b,str2.c_str());
	LoadCertificates(ctx,a, b);
	char recv_buf[local_buffer];

	ssl = SSL_new(ctx);      // create new SSL connection state
	SSL_set_fd(ssl, socketfd);    // attach the socket descriptor 
	if(SSL_connect(ssl) == FAIL)   // perform the connection
		ERR_print_errors_fp(stderr);
	else{
		printf("Connected to :%d\n", atoi(argv[2])); //show the cipher alogrithm
		printf("Connected with %s encryption\n", SSL_get_cipher(ssl)); //show the cipher alogrithm
		ShowCerts(ssl);        // get any certs
		memset(recv_buf, 0, local_buffer);
		SSL_read(ssl, recv_buf, local_buffer);
		cerr << recv_buf;
		cout << endl<<"Enter 1 for register, 2 for login:";
		//register
		string choice;
		cin >> choice;
		string name;
		string balance;
		bool login_suc=false;
		while(!login_suc){
			if(choice == "1") {//REGISTER
				cout << "Enter the name you want to register:";
				cin >> name;
				cout << "Enter the initial balance:";
				cin >> balance; 
				cout << "--------------------------------------" << endl;
				string reg="REGISTER#" + name + "#" + balance + "\n";
				//send the message to server
				SSL_write (ssl, reg.c_str(), reg.length());
        		memset(recv_buf, 0, local_buffer);
				SSL_read (ssl, recv_buf, local_buffer);
				if(recv_buf[0] == '1'){
					for(int i = 0;i < 6;i++)
						cout << recv_buf[i];
					cout<<endl;
					} //print the received message
				if(recv_buf[0]=='2' || recv_buf[0]=='y'){
					for(int i = 0;i < 8;i++)
						cout << recv_buf[i];
					cout << endl;
					cout << "Please Register Again." << endl;
				} // if the accunt have been registered, user should register again.
				cout << endl << "Enter 1 for register, 2 for login:";
				cin >> choice;
			} 
			//LOGIN
			else if (choice == "2"){
				cout << "Enter your name:";
				cin >> name;
				cout << "Enter the port number:";
				cin >> portno;
   				while (portno < 1024 || portno > 65535) {
      				cout << "The port is not acceptable. Please enter another port number: ";
      				cin >> portno;
   				}
   			
				cout << "--------------------------------------" << endl;
				sprintf(recv_buf, "%s#%d\n", name.c_str(), portno);
				SSL_write(ssl, recv_buf, strlen(recv_buf));
				if(SSL_read(ssl, recv_buf, local_buffer) < local_buffer){
					for(int i = 0;i < local_buffer;i++){	
						if(recv_buf[i] == '#')
							recv_buf[i] = '\t';
					} // to print blank space instead of #
					cout << recv_buf << endl;
				}else{
					cout << "Buffer Full" << endl;
					recv_buf[0] = '2';
				}
				if(recv_buf[0] == '2' || recv_buf[0] == 'y'){
					cout << "The account doesn't exist. Please try again." << endl;
					// If the account doesn't exist. User should try it again.
					cout << endl << "Enter 1 for register, 2 for login:";
					cin >> choice;
				}
				else 
					login_suc = true;
			} else { // error detection
				cout << "Wrong Input! Try again." << endl;
				cout << "Enter 1 for register, 2 for login:";
				cin >> choice;
			}
		}

        
		/* Create socket2 for other users */
		pthread_t tid1;
		pthread_create(&tid1, NULL, &socket_others, NULL);

		cout << "log in successfully." << endl;
		sleep(1);
		bool exit_socket = false;
		while(!exit_socket){
			cout << "Enter the number of actions u want to take." << endl;
			cout << "1 to ask for the latest list, 8 to exit, 10 to transfer data:";
			string command;
			cin >> command;
			if (command == "1"){//ask the list
				SSL_write (ssl, "List", 4);
				memset(recv_buf, 0, local_buffer);
				if(SSL_read(ssl, recv_buf, local_buffer)<local_buffer){
					for(int i = 0;i < local_buffer;i++){	
						if(recv_buf[i] == '#')
							recv_buf[i] = '\t';
					} // to print blank space instead of #
					cout << recv_buf << endl;
				}else{
					cout << "Buffer Full" << endl;
					exit_socket = true;
				}
			
			} else if (command == "8"){//quit
				SSL_write (ssl, "Exit", 4);
				memset(recv_buf, 0, local_buffer);
				SSL_read(ssl, recv_buf, local_buffer);
				for(int i = 0;i < 3;i++){
					cout << recv_buf[i];
				}
				cout << endl;
				exit_socket = true;
			} else if(command == "10"){
        		int sockfd_transfer;
				struct sockaddr_in serv_addr;
				char buff[local_buffer];
				string payeename;
				cout << "Please enter the payee's port number:";
				cin >> portno;
				cout << "Please enter the payee's name:";
				cin >> payeename;
				while (portno < 1024 || portno > 65535) {
					cout << "The port is not acceptable. Please enter another port number: ";
					cin >> portno;
				}
				bool success_get = false;
				while(!success_get){
					string sendtotransfer = "MONEY_TRANSFER#" + to_string(portno);
					SSL_write (ssl, sendtotransfer.c_str(), sendtotransfer.length());
						memset(recv_buf, 0, local_buffer);
						SSL_read(ssl, recv_buf, local_buffer);
						if(recv_buf[0] == '2' && recv_buf[1] == '2' && recv_buf[2] == '0'){
							cout << " Please try again port number: ";
							cin >> portno;
						}else{
							success_get = true;
						}
				}
				string transfer_name(recv_buf);
				/* Create a socket point */
				sockfd_transfer = socket(PF_INET, SOCK_STREAM, 0);    // create socket
				if (sockfd_transfer < 0) {
					perror("ERROR opening socket");
					exit(1);
				}
				/* initialize value in serv_addr */
				bzero((char *) &serv_addr, sizeof(serv_addr));
				serv_addr.sin_family = AF_INET;
				serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // given server IP
				serv_addr.sin_port = htons(portno);
				cout << "Please enter the transfer amount:";
				string amount;
				cin >> amount;
				/* Connect to the server */
				if (connect(sockfd_transfer, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
					cout << "connection to server fail:";
					perror("ERROR connecting");
					exit(1);
				}

				SSL_CTX *ctx_1;
				SSL *ssl_1;
				ctx_1 = InitCTX();
				LoadCertificates(ctx_1,a, b);
				ssl_1 = SSL_new(ctx_1);
				SSL_set_fd(ssl_1, sockfd_transfer);
				if ( SSL_connect(ssl_1) == FAIL ){   // perform the connection
				cout << "connection to ssl server fail:";
					ERR_print_errors_fp(stderr);
				}else{
					cout << "Connected to port:" << portno; //show the cipher alogrithm
					printf(" Connected with %s encryption\n", SSL_get_cipher(ssl_1)); //show the cipher alogrithm
					ShowCerts(ssl_1);        // get any certs
					cout << "name = " << name << endl;
					cout << "amount = " << amount << endl;
					cout << "payeename = " << payeename << endl;
					string buffer = name + "#" + amount + "#" + payeename;
					SSL_write (ssl_1, buffer.c_str(), buffer.length());
					memset(recv_buf, 0, local_buffer);
					SSL_read (ssl_1, recv_buf, local_buffer);
					cout << endl << recv_buf << endl;
					SSL_free(ssl_1);
				}
				close(sockfd_transfer);
   				SSL_CTX_free(ctx_1);
			} else {//error detection
				cout << "Command does not exist. Try it again." << endl;
			}
		}
		SSL_free(ssl);
	}
	close(socketfd);
	SSL_CTX_free(ctx);	
	return 0;
}

void* socket_others(void * data){

  	int sockfd_others, clientfd, clientLEN;
    struct sockaddr_in serv_addr, cli_addr;
    struct sockaddr_in addr;
    socklen_t addr_len;
   	
    /* Create a server socket point */
    sockfd_others = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd_others < 0) {
     	perror("ERROR opening socket");
      exit(1);
    } 
    /* Initialize socket structure in serv_addr */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;      // ...
    serv_addr.sin_port = htons(portno);
    /* Assign a port number to server */
    if (bind(sockfd_others, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR on binding");
    	exit(1);
    }
    SSL_CTX *ctx_2;
    SSL_library_init(); //init SSL library
    printf("Initialize SSL library.\n");
    SSL *ssl_2;
     
    /* Listen for other users with max 3 connections */
    int sockfd_new;
    listen(sockfd_others,3);
    printf("\nListening on port number: %d\n", portno);
    while((sockfd_new = accept(sockfd_others, (struct sockaddr *) &addr, &addr_len))){

      if(sockfd_new >= 0){
        pthread_mutex_lock(&inlock);
        ctx_2 = InitServerCTX();  //initialize SSL 
        strcpy(a,str1.c_str());
        strcpy(b,str2.c_str());
        LoadCertificates_server(ctx_2, a, b); // load certs and key
        ssl_2 = SSL_new(ctx_2);              // get new SSL state with context 
        SSL_set_fd(ssl_2, sockfd_new);      // set connection socket to SSL state
        SSL_accept(ssl_2);         // service client connection 
      
        char recv_buf[local_buffer];
        cout << endl << "A user connects with message: ";
      
        memset(recv_buf,0,local_buffer);
        SSL_read(ssl_2, recv_buf, local_buffer);
			printf("%s\n", recv_buf);
			string recv_message(recv_buf);
			string totransfer = "DO_transfer#" + recv_message;

			//transfer_from_user = 1;
			SSL_write(ssl, totransfer.c_str(), totransfer.length());
			bzero(recv_buf, local_buffer);
			SSL_read(ssl, recv_buf, local_buffer);
			printf("%s\n", recv_buf);
			SSL_write(ssl_2, recv_buf, local_buffer);
			cerr << "1 to ask for the latest list, 8 to exit, 10 to transfer data:";
        
      } 
    }
    close(sockfd_new);  // close server socket
    SSL_CTX_free(ctx_2);     
  
}    
