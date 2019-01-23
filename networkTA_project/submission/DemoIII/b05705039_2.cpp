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
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#include <openssl/ssl.h>
#include <openssl/err.h>
int online = 0;
SSL *ssl;
pthread_t thread;
pthread_attr_t attr;
SSL_CTX* InitCTX(void){   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = SSLv23_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
SSL_CTX* InitServerCTX(void){   
    const SSL_METHOD *method;
    SSL_CTX *context;
 
    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc. 
    SSL_load_error_strings();   // load all error messages */
    method = SSLv23_server_method();  // create new server-method instance 
    context = SSL_CTX_new(method);   // create new context from method
    if ( context == NULL ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return context;
}
void LoadCertificates(SSL_CTX* context, char* CertFile, char* KeyFile)
{
    //set the local certificate from CertFile
    if ( SSL_CTX_use_certificate_file(context, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //set the private key from KeyFile
    if ( SSL_CTX_use_PrivateKey_file(context, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //verify private key 
    if ( !SSL_CTX_check_private_key(context) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}
void* becomeServer(void* port){
    SSL_CTX *context3;
    context3 = InitServerCTX();
    SSL *ssl3;
    char* myport = (char*)port;
    char receiveMessage[2000] = {0};
    LoadCertificates(context3, "mycert.pem", "mykey.pem"); // load certs and key
    struct sockaddr_in dest;
    dest.sin_port = htons(atoi(myport));
    dest.sin_addr.s_addr = INADDR_ANY;
    dest.sin_family = AF_INET;

    int sockPeer = socket(AF_INET, SOCK_STREAM, 0);

    int err = -1;
    err = bind(sockPeer, (struct sockaddr*)&dest, sizeof(dest));

    if(err < 0){
        printf("\nunable to bind the port\n");
    }

    listen(sockPeer, 10); //listen to the port

    while(online == 1){  
        int clientsock; //client descriptor
        struct sockaddr_in peerclient;
        int c = sizeof(peerclient);
        clientsock = accept(sockPeer, (struct sockaddr*)&peerclient, (socklen_t *)&c);
        ssl3 = SSL_new(context3);              // get new SSL state with context 
        SSL_set_fd(ssl3, clientsock);      // set connection socket to SSL state

        if ( SSL_accept(ssl3) == -1 ){
            ERR_print_errors_fp(stderr);
            printf("get here2");
        } 
        else{
            memset(receiveMessage,'\0',sizeof(receiveMessage));
            SSL_read(ssl3, receiveMessage, sizeof(receiveMessage)); // get reply & decrypt
            char message[20];
            strcpy(message,"T#") ;
            strcat(message,receiveMessage);
            SSL_write(ssl, message, strlen(message)); //send to server
            break;
        }
    }
    // pthread_join(thread, NULL);
    SSL_CTX_free(context3);      // release context
}
int main(int argc , char *argv[]){

    SSL_library_init();
    SSL_load_error_strings();
    SSL_CTX *context;
    context = InitCTX();
    ssl = SSL_new(context);      // create new SSL connection state

//-----socket connection---------
    int sock = 0;
    sock = socket(AF_INET , SOCK_STREAM , 0);//create socket

    if (sock == -1){//check socket performance
        printf("Fail to create a socket.");
    }

    char* serverIP = argv[1];
    char* serverPort = argv[2];
    int severPort_i = atoi(serverPort);
//------server ip----

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr("127.0.1");
    info.sin_port = htons(severPort_i);
    int err = connect(sock,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        printf("Connection error");
    }
    printf("connect commit");


    SSL_set_fd(ssl, sock);    // attach the socket descriptor 

    int myPort, n;
    struct sockaddr_in my_addr;
    char myPortC[200] = {};
    char receiveMessage[2000] = {0};

    // bzero(&my_addr, sizeof(my_addr));
    // unsigned int len = sizeof(my_addr);
    // getsockname(sock, (struct sockaddr *) &my_addr, &len);
    // myPort = ntohs(my_addr.sin_port);
    // n = sprintf (myPortC, "%d",myPort);
// 

    int tmp = 0;
    int length = 0;
    char tag[2] = {"#"};
    int action = 0;
    if ( SSL_connect(ssl) == -1 )   // perform the connection
        ERR_print_errors_fp(stderr);
    else
    {   
        while (tmp == 0){
            printf("Enter 1 for register, Enter 2 for login : ");
            std::cin >> tmp;
            if(tmp == 1){
                //-----register----
                char name[20] = {};
                char message1[100] = {"REGISTER#"};
                printf("register\n");
                // recv(sock,receiveMessage,sizeof(receiveMessage),0);
                printf("%s",receiveMessage);
                printf("Enter the name you want to register :");
                std::cin >> name;
                strcat(message1,name);
                //send register message

                SSL_write(ssl, message1, strlen(message1));
                length = 0;
                length = SSL_read(ssl, receiveMessage, sizeof(receiveMessage));
                printf("%s",receiveMessage);
                memset(receiveMessage,'\0',sizeof(receiveMessage));
                memset(message1,'\0',sizeof(message1));
                //first deposit
                printf("%s\n","Enter the amount you want to deposit:");
                std::cin >> message1;
                SSL_write(ssl, message1, strlen(message1));
                printf("%s",receiveMessage);
                tmp = 0;
            }
            else if(tmp == 2){
                //-----login------
                char portname[200];
                char name[20] = {};
                memset(portname, '\0', sizeof(portname));
                memset(name, '\0', sizeof(name));
                online = 1;
                char message2[200] = {};
                
                // char myPortC[20] = {};
                printf("login\n");
                printf("Enter your name:");
                std::cin >> message2;
                strcpy(name,message2);
                strcat(message2,"#");
                printf("Enter your port name:\n");
                int portname_i = 0;
                std::cin >> portname;
                portname_i = atoi(portname);
                while(portname_i < 1024 || portname_i > 655355){
                    printf("wrong port number\n" );
                    printf("Enter your port name again:\n");
                    memset(portname, '\0', sizeof(portname));
                    std::cin >> portname;
                    portname_i = atoi(portname);
                }
                printf("%s\n", portname);
                strcat(message2,portname);

                //send login message

                SSL_write(ssl, message2, strlen(message2));
                length = 0;
                memset(receiveMessage,'\0',sizeof(receiveMessage));
                length = SSL_read(ssl, receiveMessage, sizeof(receiveMessage));
                printf("%s\n",receiveMessage);

                pthread_attr_init(&attr);
                pthread_create(&thread, &attr, becomeServer, (void*) (portname));
                while(online == 1 && tmp != 0){
                    //other operation
                    printf("Enter 3 to get onlinelist, 4 to exit, 5 to make transaction:\n");
                    std::cin>>tmp;
                    if (tmp == 3){
                        char list[200] = {0};
                        strcpy(list,"List");
                        SSL_write(ssl, list, strlen(list));
                        length = 0;
                        memset(receiveMessage,'\0',sizeof(receiveMessage));
                        length = SSL_read(ssl, receiveMessage, sizeof(receiveMessage));
                        printf("%s\n",receiveMessage);
                        tmp = 2;
                    }
                    else if (tmp == 4){
                        char list[200] = {0};
                        strcpy(list,"Exit");

                        SSL_write(ssl, list, strlen(list));
                        length = 0;
                        memset(receiveMessage,'\0',sizeof(receiveMessage));
                        length = SSL_read(ssl, receiveMessage, sizeof(receiveMessage));
                        printf("%s\n",receiveMessage);
                        close(sock);
                        tmp = 2;
                        break;
                    }
                    else if(tmp == 5){//transaction
                        char peerNum[200];
                        char peerName[200];
                        char ip[200] = "127.0.0.1";
                        char amount[200];
                        //another ssl connection
                        SSL_CTX *context2;
                        context2 = InitCTX();
                        SSL *ssl2;

                        printf("Please enter the port number of the peer: ");
                        std::cin >> peerNum;
                        printf("Please enter the name of peer: ");
                        std::cin >> peerName;
                        printf("How much you want to transfer ?");
                        std::cin >> amount;

                        
                        struct sockaddr_in dest;
                        int peerSock; //socket descripter

                        bzero(&info,sizeof(dest));
                        dest.sin_family = PF_INET;
                        dest.sin_addr.s_addr = inet_addr("127.0.1");
                        dest.sin_port = htons(atoi(peerNum));

                        peerSock = socket(AF_INET, SOCK_STREAM, 0); //allocate socket
                        int err = connect(peerSock,(struct sockaddr *)&dest,sizeof(dest));
                        if(err==-1){
                            printf("transaction fail to connected");
                        }


                        ssl2 = SSL_new(context2);      // create new SSL connection state
                        SSL_set_fd(ssl2, peerSock);    // attach the socket descriptor


                        if ( SSL_connect(ssl2) == -1 )   // perform the connection
                            ERR_print_errors_fp(stderr);
                        else{
                            //send transaction message to peer
                            char mesasge[1024];
                            memset(&mesasge, 0, sizeof(mesasge));
                            snprintf(mesasge, 1024, "%s#%s#%s\r\n", name, amount, peerName);
                            SSL_write(ssl2, mesasge, strlen(mesasge));
                        }
                        SSL_free(ssl2);        // release connection state 
                        SSL_CTX_free(context2);       // release context
                        tmp = 2;

                    }


                }
                 
            }
            else if(tmp == 6){
                printf("close Socket\n");
                // close(sock);
                break;
            }
            else{
                printf("wrong number");
                tmp = 0;
            }
        }
        SSL_free(ssl);
    }
    SSL_CTX_free(context); 
    return 0;
}
