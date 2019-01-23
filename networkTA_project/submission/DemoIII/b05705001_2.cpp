#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>
#include <string>
#include <iostream>
#include <vector>
#include <errno.h>
#include "openssl/ssl.h"
#include "openssl/err.h"
using namespace std;

pthread_mutex_t mutex_peer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ssl = PTHREAD_MUTEX_INITIALIZER;
char portnum[100];

SSL *ssl; //client to server
int transfer_from_user=0;
void * socket_others(void* data);
int socketfd;
int portno;
char mycert[] = "./client1cert.pem"; 
char mykey[] = "./client1key.pem";

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



/// to other peer ***************

void* connectToPeer(void* data);
void recvFromPeer(SSL* ssl);

void cleanBuf()
{
    char c = '0';
    do 
    {
        char c = getchar();
    } while (c == '\n');
}
void receive()
{
    char receiveMessage[4000] = {0};
    memset(receiveMessage, '\0',sizeof(receiveMessage));
    SSL_read(ssl,receiveMessage,sizeof(receiveMessage));
    printf("%s",receiveMessage);
    // printf("--end function\n");

}


int main(int argc , char *argv[])
{ 
    int port = atoi(argv[2]); 

    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    struct sockaddr_in info;
    memset(&info,'\0',sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(argv[1]);
    info.sin_port = htons(port);
    
    //socket的連線
    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        printf("Connection error\n");
        return 0;
    }

    /// SSL ***************
    SSL_CTX *ctx;
    SSL_library_init();
    ctx = InitCTX();
    LoadCertificates(ctx,mycert, mykey);
    ssl = SSL_new(ctx);      // create new SSL connection state
    SSL_set_fd(ssl, sockfd);    // attach ssl to the socket descriptor


    char command[100];
    char username[100] = {};
    char message[1000] = {};
    string login_user;

    if ( SSL_connect(ssl) <= 0 )   // perform the connection
        ERR_print_errors_fp(stderr);
    else
    {
        ShowCerts(ssl);
        // connected accepted
        receive();
        while(1){

            printf("%s\n", "Enter 1 for register, 2 for Login: ");
            memset(command, '\0',100);
            scanf("%s", command);

            if(strcmp(command, "1") == 0){
                printf("%s", "Enter the name you want to register: ");
                scanf("%s", username);
                strcpy(message, "REGISTER#");
                strcat(message, username);
                SSL_write(ssl,message, sizeof(message));

                memset(message, '\0',100);
                memset(username, '\0',100);

                //100 OK or 210 FAIL
                receive();

            }

            // Login

            else if(strcmp(command, "2") == 0){
                printf("%s", "Enter your name: "); cleanBuf();
                scanf("%s", username);
                printf("%s", "Enter the port number: "); cleanBuf();
                scanf("%s",portnum);
                strcpy(message, username);
                strcat(message, "#");
                strcat(message, portnum);
                // strcat(message, "\n");
                SSL_write(ssl,message, sizeof(message));
                memset(message, '\0',100);

                //Please enter: or 210 AUTH_FAIL 
                char receiveMessage[1024] = {0};
                memset(receiveMessage, '\0',sizeof(receiveMessage));
                SSL_read(ssl,receiveMessage,sizeof(receiveMessage));
                printf("%1s",receiveMessage);
                if(strcmp(receiveMessage,"220 AUTH_FAIL\n") == 0)
                {
                    continue;
                }

                scanf("%s", message);
                SSL_write(ssl,message,sizeof(message));

                //account list
                receive();
                break;


            }

            else
            {
                printf("%s\n", "<<Please enter 1 or 2>>");
                continue;
            }


        }

        // create thread for peer to peer communication
        pthread_t tid;
        if( pthread_create(&tid, NULL, connectToPeer, NULL) != 0 )
        {
            printf("Failed to create thread\n");
            return 0;
        }
        sleep(1);

        // Already logined. Action or Exit

        while(1){
            printf("%s", "Enter the number of action you want to take.\nEnter 1 to ask for the latest list, 2 to  pay to other client, 8 to exit: ");
            cleanBuf();
            memset(command, '\0',100);
            scanf("%s", command);
            if(strcmp(command, "1") == 0)
            {
                char askList[] = {"List\n"};
                SSL_write(ssl,askList, sizeof(askList));
                memset(askList, '\0', sizeof(askList));

                //online list
                receive();
                continue;
            
            }
            else if(strcmp(command, "2") == 0)  //transaction
            {
                string transToPeer, transToServer;
                string payee, payAmount;
                char server_message[2000];
                memset(server_message, '\0',sizeof(server_message));
                login_user = username;
                cout << "Please enter the user you want to pay: ";
                cin >> payee;
                cout << "Please enter the amount you want to pay: ";
                cin >> payAmount;


                transToServer = "PayRequest#" + login_user + "#" + payAmount + "#" + payee + "\0";
                
                SSL_write(ssl, transToServer.c_str(), transToServer.size()); //send transaction message
                SSL_read(ssl, server_message, sizeof(server_message));
                
                if(strstr(server_message, "FAIL") != NULL) //transaction fail
                {
                    cout << server_message;
                    continue;
                }
                else //transaction available
                {  
                    
                    int payeePort = atoi(server_message);
                    cout << "Payee's port: " << payeePort << "\n"; 

                    //connect to another payee
                    int transfd = 0;
                    transfd = socket(AF_INET , SOCK_STREAM , 0);
                    if (transfd == -1){
                        printf("Fail to create a socket.");
                    }

                    struct sockaddr_in trans;
                    memset(&trans,'\0',sizeof(trans));
                    trans.sin_family = PF_INET;
                    trans.sin_addr.s_addr = inet_addr("127.0.0.1");
                    trans.sin_port = htons(payeePort);
                    
                    //socket的連線
                    int err3 = connect(transfd,(struct sockaddr *)&trans,sizeof(trans));
                    if(err3==-1){
                        printf("Connection error\n");
                        continue;
                    }

                    printf("Connection to other client success.\n");

                    SSL* ssl3;
                    SSL_CTX *ctx3;
                    SSL_library_init();
                    ctx3 = InitCTX();
                    LoadCertificates(ctx3,mycert, mykey);
                    ssl3 = SSL_new(ctx3); 
                    SSL_set_fd(ssl3, transfd); 
                    if(SSL_connect(ssl3) <= 0)
                    {
                        cout << "SSL connection FAIL.\n";
                        ERR_print_errors_fp(stderr);
                        continue;
                    }
                    else
                    {
                        //start chatting as a payer
                        transToPeer.clear();
                        transToPeer = login_user + "#" + payAmount + "#" + payee + "\n";
                        SSL_write(ssl3, transToPeer.c_str(), transToPeer.size());
                        cout << "finish transaction\n";
                        continue; //finish transaction
                    }

                }


            }

            else if(strcmp(command, "8") == 0)
            {
                strcpy(message, "Exit\n");
                SSL_write(ssl,message, sizeof(message));
                memset(message, '\0', sizeof(message));

                //Bye!
                receive();
                exit(0);
                break;      
            }
            else{
                printf("%s\n", "<<Please enter 1, 2 or 8.>>"); 
            }
        }
    }


            
    printf("\nclose Socket\n");
    close(sockfd);
    return 0;
}

void* connectToPeer(void * data) //Another thread. As a server(payee), connect to payer
{
    int mySocket;
    struct sockaddr_in myAddr;
    struct sockaddr_in myInfo, peerInfo;
    struct sockaddr_storage myStorage;

    int port = atoi(portnum);
    cout << "portnum: " << portnum << "\n";
    socklen_t myaddr_size;
    myaddr_size = sizeof myStorage;
    mySocket = socket(PF_INET, SOCK_STREAM, 0);
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(port);
    myAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(myAddr.sin_zero, '\0', sizeof(myAddr.sin_zero));
    bind(mySocket, (struct sockaddr *) &myAddr, sizeof(myAddr));

       
    SSL_library_init(); 
    SSL* ssl2;
    SSL_CTX *ctx2;
    ctx2 = InitServerCTX(); 
    LoadCertificates(ctx2, mycert, mykey); // load certs and key


    if(listen(mySocket,10)==0)
        printf("Listening\n");
    else
        printf("Listening error\n");
    
    while(1)
    {
        int peer_fd = -1;
        
        while((peer_fd = accept(mySocket, (struct sockaddr *) &peerInfo, &myaddr_size)) >= 0) 
        {
            cout << "\nConnected by other client.\n";
            // pthread_mutex_lock(&mutex_ssl);
            /// SSL connect
            ssl2 = SSL_new(ctx2);   
            SSL_set_fd(ssl2, peer_fd);
            if (SSL_accept(ssl2)  < 0)
            {
                cout << "SSL connect error!\n";
            }
            else 
            {
                recvFromPeer(ssl2);
                SSL_free(ssl2);
                break;
            }
            // pthread_mutex_unlock(&mutex_ssl);           
        } 
    }

  close(mySocket); //close the socket of server
  printf("Exit thread \n");
  pthread_exit(NULL);
}


void recvFromPeer(SSL* ssl2) //As a payee, talk to server
{
    char peerMessage[2000];
    memset(peerMessage, '\0', sizeof(peerMessage));
    if(SSL_read(ssl2, peerMessage, sizeof(peerMessage)) > 0)
    {
        // payer#amount#payee
        cout << "Receive peer's message: " << peerMessage;      
        string payer, payAmount, payee;
        payer = strtok (peerMessage,"#");
        payAmount = strtok (NULL, "#");
        cout << payer << " gives you " << payAmount <<" dollars.\n";
    }

}
