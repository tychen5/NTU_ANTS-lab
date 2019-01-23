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
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"
#include "openssl/ssl.h"
#include "openssl/err.h"

typedef int bool;
#define true 1
#define false 0
char cert[] = "/Users/shihyunchen/Documents/computer networks/assignment/b05705019_part3/cert.pem";
char key[] = "/Users/shihyunchen/Documents/computer network/assignment/b05705019_part3/key.pem";
int transfer_from_user=0;
void * listening(void* data);

void send_to(int sockfd, char *message);
void recv_from(int sockfd);
void regLog();
_Bool bool_recv = false;

// initilalize certificate
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

void ShowCerts(SSL* ssl);
void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile);
void LoadCertificates_server(SSL_CTX* ctx, char* CertFile, char* KeyFile);
SSL *ssl;
int main(int argc, char *argv[])
{
    // create socket
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd < 0){
        printf("Fail to create a socket.");
        return -1;
    }

    //connect to server
    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(argv[1]);
    info.sin_port = htons(atoi(argv[2]));

    SSL_CTX *ctx;
    SSL_library_init();
    ctx = InitCTX();
    LoadCertificates(ctx,cert, key);
    ssl = SSL_new(ctx);      // create new SSL connection state
    SSL_set_fd(ssl, socketfd);    // attach the socket descriptor 
    int error = connect(sockfd, (struct sockaddr *)&info, sizeof(info));
    if(SSL_connect(error < 0)){
        printf("Connection failed");
        return -1;
    }
    else
    {
        printf("Connected with %s encryption\n", SSL_get_cipher(ssl)); //show the cipher alogrithm
        ShowCerts(ssl); 
        SSL_read(ssl, recv_buf, local_buffer);
        recv_from(sockfd);
    }
        
    

    // while not exit
    _Bool flag_exit = false;
    while(!flag_exit)
    {
        // register and login
        char command[1024];
        _Bool reg = 0;
        while(reg == 0)
        {
            printf("\nEnter <1> to register, <2> to login, <4> to exit: ");
            scanf("%s", command);
            if(strcmp(command, "1") == 0 || strcmp(command, "2") == 0 || strcmp(command, "4") == 0)
                reg = 1;
        }
        if(strcmp(command, "1") == 0)
        {
            printf("Enter the account name you want to register: ");
            char name[100];
            scanf("%s", name);
            printf("Enter the money you want to deposit: ");
            char deposit[100];
            scanf("%s", deposit);
            char message[1024];
            strcpy(message, "REGISTER#");
            strcat(message, name);
            strcat(message, "#");
            strcat(message, deposit);
            send_to(sockfd, message);
            recv_from(sockfd);
        }
        if(strcmp(command, "2") == 0)
        {
            _Bool login = false;
            char username[100];
            printf("Enter your name: ");
            scanf("%s", username);
            int portnum;
            while(!login)
            {
                printf("Enter your port number: ");
                scanf("%d", &portnum);
                if(portnum >= 1024 && portnum = 65536)
                    login = true;
                else
                    printf("Please enter valid port number(between 1024~65535).");
            }
            char portnum_str[1024];
            sprintf(portnum_str, "%d", portnum);
          
            char loginMessage[1024];
            strcpy(loginMessage, username);
            strcat(loginMessage, "#");
            strcat(loginMessage, portnum_str);
            // printf("%sloginmessage", loginMessage);
            send_to(sockfd, loginMessage);
            recv_from(sockfd);
            if(!bool_recv)
                continue;
                // continue; // go to the very beginning: enter 1 to register, 2 to login, 4 to exit 
            else
            {
                // Create socket2 for other users
                pthread_t tid;
                pthread_create(&tid, NULL, &listening, NULL);
                while(!flag_exit)
                {
                    _Bool flag_list = false;
                    char command2[1024];
                    while(!flag_list)
                    {
                        printf("\nEnter <1> for requesting latest information, <4> for exit, <5> to transfer data: ");
                        scanf("%s", command2);
                        if(strcmp(command2, "1") == 0 || strcmp(command2, "4") == 0)
                            flag_list = true;
                    }
                    if(strcmp(command2, "1") == 0)
                    {
                        send_to(sockfd, command2);
                        recv_from(sockfd);
                    }
                    else
                    {
                        flag_exit = true;
                        char exit_str[1024];
                        strcpy(exit_str, "Exit");
                        send_to(sockfd, exit_str);
                        recv_from(sockfd);
                        close(sockfd);
                    }
                }
            }
        }
        // encryp connection
        if(strcmp(command, "5") == 0)
        {
            int sockfd_transfer;
   			struct sockaddr_in serv_addr;
   			char buff[local_buffer];
   			string nameforpayee;
   			cout <<"Please enter the account name that you want to transfer: ";
   			cin >> nameforpayee;
   			bool success_get = false;
   			while(!success_get)
            {
   				string sendtran = "MONEY_TRANSFER#" + nameforpayee;
   				(ssl, sendtran.c_str(), sendtran.length());
                memset(recv_buf, 0, local_buffer);
                SSL_read(ssl, recv_buf, local_buffer);
                if(recv_buf[0]=='2' && recv_buf[1]=='2' && recv_buf[2]=='0')
                {
                    cout << "Can't find the user. Please enter another user's account name: ";
                    cin >> nameforpayee;
                }
                else
                {
                    success_get = true;
                }	
   			}
   			string transfer_name(recv_buf);
            portnum = stoi(transfer_name);
   			// Create a socket point
   			sockfd_transfer = socket(PF_INET, SOCK_STREAM, 0);
   			if (sockfd_transfer < 0) 
            {
                perror("ERROR opening socket");
                exit(1);
   			}
   			
   			bzero((char *) &serv_addr, sizeof(serv_addr));
   			serv_addr.sin_family = AF_INET;
   			serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
   			serv_addr.sin_port = htons(portnum);
   			cout <<"Please enter the money you want to transfer: ";
   			string amount;
   			cin >> amount;
   			
   			if (connect(sockfd_transfer, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
            {
      		    perror("ERROR connecting");
    			exit(1);
   			}

   			SSL_CTX *ctx_1;
    		SSL *ssl_1;
    		ctx_1 = InitCTX();
            LoadCertificates(ctx_1,cert, key);
    		ssl_1 = SSL_new(ctx_1);
    		SSL_set_fd(ssl_1, sockfd_transfer);
    		if ( SSL_connect(ssl_1) == FAIL )
    			ERR_print_errors_fp(stderr);
    		else
            {
       		    printf(" Connected with %s encryption\n", SSL_get_cipher(ssl_1));
       		    ShowCerts(ssl_1);
   				strcat(buffer, "#");
                strcat(buffer, to_string(amount).c_str());
                strcat(buffer, "#");
                strcat(buffer, to_string(portnum).c_str());
   				SSL_write (ssl_1, buffer.c_str(), buffer.length());
                memset(recv_buf, 0, local_buffer);
                SSL_read (ssl_1, recv_buf, local_buffer);
                cout<<endl<<recv_buf<<endl;
   				SSL_free(ssl_1);
   			}
            close(sockfd_transfer);
            SSL_CTX_free(ctx_1);
		}
        if(strcmp(command, "4") == 0)
        {
            flag_exit = true;
            char exit_str[1024];
            strcpy(exit_str, "Exit");
            send_to(sockfd, exit_str);
            recv_from(sockfd);
            SSL_free(ssl);
            close(sockfd);
        }

    }

	SSL_CTX_free(ctx);
    return 0;
}

void send_to(int sockfd, char *message)
{
    char sendMessage[1024];
    strcpy(sendMessage, message);
    strcat(sendMessage, "\n");
    SSL_write(ssl, sendMessage, strlen(sendMessage));
}

void recv_from(int sockfd)
{
    bool_recv = false;
    char receiveMessage[10000] = {};
    SSL_read(ssl, receiveMessage, sizeof(receiveMessage));
    if(strcmp(receiveMessage, "210 FAIL\n") == 0)
        printf("This account name is used, please use another one to try again.\n");
    else if(strcmp(receiveMessage, "220 AUTH_FAIL\n") == 0)
        printf("Please register first.\n");
    else if(strcmp(receiveMessage, "100 OK\n") == 0)
        printf("100 OK: successfully registered.\n");
    else
    {
        printf("%s", receiveMessage);
        bool_recv = true;
    }
}

// listening on the socket
void* listening(void * data)
{
  	int sockfd_others, clientfd, clientLEN;
    struct sockaddr_in serv_addr, cli_addr;
    struct sockaddr_in addr;
    socklen_t addr_len;
   	
    //Create a server socket point
    sockfd_others = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd_others < 0) 
     	perror("Failed to open a socket");
    // Initialize socket structure in serv_addr
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portnum);
    // Assign a port number to server
    if (bind(sockfd_others, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)         
        perror("Failed to bind");

    SSL_CTX *ctx_2;
    SSL_library_init();
    printf("Initialize SSL library.\n");
    SSL *ssl_2;
    
    t sockaddr *) &addr, &addr_len)))
    {

        if(sockfd_new > -1)
        {
            pthread_mutex_lock(&inlock);
            ctx_2 = InitServerCTX(); 
            LoadCertificates_server(ctx_2, cert, key);
            ssl_2 = SSL_new(ctx_2);
            SSL_set_fd(ssl_2, sockfd_new);
            SSL_accept(ssl_2); 
        
            char recv_buf[local_buffer];
            cout << endl <<"connecting message: ";
        
            memset(recv_buf,0,local_buffer);
            SSL_read(ssl_2, recv_buf, local_buffer);
            printf("%s\n", recv_buf);
            string recv_message(recv_buf);
            string tran = "transfer#" + recv_message;

            //transfer_from_user = 1;
            SSL_write(ssl, tran.c_str(), tran.length());
            bzero(recv_buf, local_buffer);
            SSL_read(ssl, recv_buf, local_buffer);
            printf("%s\n", recv_buf);
            SSL_write(ssl_2, recv_buf, local_buffer);
        } 
    }
        // close socket
        close(sockfd_new);
        SSL_CTX_free(ctx_2);     
}   

//Load the certificate 
void ShowCerts(SSL* ssl){   
    X509 *cert;
    char *line;
 
    cert = SSL_get_peer_certificate(ssl); // get the server's certificate
    if ( cert != NULL ){
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       // free the malloc'ed string 
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);\
        free(line);       // free the malloc'ed string 
        X509_free(cert);     // free the malloc'ed certificate copy */
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
        fprintf(stderr, "Private key and the public certificate isn't match.\n");
        abort();
    }
}
//Load the certificate 
void LoadCertificates_server(SSL_CTX* ctx, char* CertFile, char* KeyFile){
    
    if (SSL_CTX_load_verify_locations(ctx, CertFile, KeyFile) != 1)
        ERR_print_errors_fp(stderr);
    
    if (SSL_CTX_set_default_verify_paths(ctx) != 1)
        ERR_print_errors_fp(stderr);
    
    
    // local certificate
    if (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
    }
    // private key
    if (SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
    }
    // verify private key
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