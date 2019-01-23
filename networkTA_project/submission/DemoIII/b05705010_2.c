#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h> 
#include "openssl/ssl.h"
#include "openssl/err.h"
#include <errno.h>
#define len 1024 //max buffer size

//========== to store the user name logged in now ================================
struct parameter {
    int server;
    int port;
};

//=========== to initialize the SSL_CTX ==========================================
SSL_CTX* InitCTX(void){   
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();       // Load cryptos, et.al. 
    SSL_load_error_strings();           // Bring in and register error messages 
    method = TLSv1_client_method();     // Create new client-method instance 
    ctx = SSL_CTX_new(method);          // Create new context 
    if ( ctx == NULL ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

SSL_CTX* InitServerCTX(void){
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();          //load & register all cryptos, etc.
    SSL_load_error_strings();              // load all error messages 
    method = TLSv1_server_method();        // create new server-method instance
    ctx = SSL_CTX_new(method);             // create new context from method
    if ( ctx == NULL ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
//=========== Load the certificate ====================================================
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
        fprintf(stderr, "Error: Private key failure exists\n");
        abort();
    }
}

void ShowCerts(SSL* ssl){
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl); // get the server's certificate
    if ( cert != NULL ){
        printf("=============================================\n");
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       // free the malloc'ed string
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);\
        free(line);       // free the malloc'ed string
        X509_free(cert);     // free the malloc'ed certificate copy */
        printf("=============================================\n");
    }
    else
        printf("No certificate exists.\n");
}
//====================== open connection ========================================================
int OpenConnection(const char *hostname, int port){
    int sd;
    struct hostent *host;
    struct sockaddr_in addr;

    if((host = gethostbyname(hostname))==NULL){
        perror(hostname);
        abort();
    }

    sd = socket(PF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = *(long*)(host->h_addr);
    if(connect(sd,(struct sockaddr*)&addr, sizeof(addr))!=0){
        close(sd);
        perror(hostname);
        abort();
    }

    return sd;
}

int OpenListener(int port){
    int sd;
    struct sockaddr_in addr;

    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if ( bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 ) {//bind the IP and port number
        perror("can't bind port");
        abort();
    }
    if ( listen(sd, 10) != 0 ){
        perror("Can't configure listening port");
        abort();
    }
    return sd;
}

void * ClientListner(void * param){
    //SSL var
    SSL_CTX *ctx;
    SSL_library_init(); //init SSL library
    ctx = InitServerCTX();  //initialize SSL
    LoadCertificates(ctx, "mycert.pem", "mykey.pem"); // load certs and key
    SSL *ssl;
    //listening interaction msg from another clients
    int port = ((struct parameter*)param)->port;
    int server = ((struct parameter*)param)->server;
    int peer = OpenListener(port); //set listener for clients
    int my_server;
    struct sockaddr_in addr;
    socklen_t leng = sizeof(addr);
    char buff[leng];
    while (1){
        //GetLocal(my_server);
        //GetRemote(my_server);
        //printf("Connection from: %s:%d\n",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        my_server = accept(peer, (struct sockaddr*)&addr, &leng);
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, my_server);
        if ( SSL_accept(ssl) == -1 ){ //do SSL-protocol accept
            ERR_print_errors_fp(stderr);
        }
        else{
            int bytes;
            while((bytes = SSL_read(ssl,buff,1000))){
                if(bytes > 0){
                    printf("\n%s\n", buff);
                    send(server, buff, strlen(buff), 0);
                    //ReceiveMsg(server);
                    bzero(&buff,sizeof(buff));
                    recv(server,buff,sizeof(buff),0);
                    printf("%s",buff);
                    bzero(&buff,sizeof(buff));
                }
            }
        }
    }
    SSL_CTX_free(ctx);
    close(my_server);
    close(peer);
    pthread_exit(0);
    return NULL;
}



int main(int argc, char *argv[])
{
    int server;
    char *hostname, *portnum;
    if(argc !=3){
        printf("usage: %s <hostname> <portnum>\n", argv[0]);
        exit(0);
    }

    hostname = argv[1];
    portnum = argv[2];
    server = OpenConnection(hostname, atoi(portnum));
    printf("Connected to %s:%d\n", hostname, atoi(portnum));

//============== Set up some global function =========================================
    char message[100] = {};
    char receiveMessage[100] = {};
    int command = 0;
    char account[100] = {};
    char regis_msg[20] = {"REGISTER#"};
    char port[] = {};
    char bal[] = {};
    char ack[] = {"ack"};
    char list[] = {"List"};
    
    recv(server,receiveMessage,sizeof(receiveMessage),0);//connection message
    printf("%s",receiveMessage);
    printf("\n");

    int online = 0;//whether the client is online
    while(1){
        command = 0;
        bzero(&port,sizeof(port));
        bzero(&message,sizeof(message));
        bzero(&receiveMessage,sizeof(receiveMessage));
        bzero(&account,sizeof(account));

        printf("%s", "Enter 1 for register, 2 for log in:\n");
        scanf("%d", &command);
//============= CASE: REGISTER =======================================================
        if(command == 1){
            printf("%s","Enter the name you want to regist:\n");
            scanf("%s", account);
            printf("%s","Enter your beginning balance:\n");
            scanf("%s", bal);
            bzero(&message,sizeof(message));
            
            strcat(message, regis_msg);
            strcat(message, account);
           
            //printf("%s\n",regis_msg);
            //printf("%s\n", message);
            send(server,message,strlen(message),0);//send account Id
            recv(server,ack,sizeof(ack),0);        //get ACK
            bzero(&message,sizeof(message));
            strcat(message, bal);
            send(server,message,strlen(bal),0);    //send balance
            bzero(&message,sizeof(message));
            recv(server,receiveMessage,sizeof(receiveMessage),0);//get accept message
            printf("%s",receiveMessage);
            printf("\n");
        }
//============= CASE: LOG IN ==========================================================
        else if(command == 2){
            printf("%s","Enter your name:\n");
            scanf("%s", account);
            printf("%s","Enter your port number:\n");
            scanf("%s", port);

            int port_int = 0;
            port_int = atoi(port);
            if(port_int <= 65535 && port_int >= 1024){
                bzero(&message,sizeof(message));
                strcat(message, account);
                char aaa[] = {"#"};
                strcat(message, aaa);
                strcat(message, port);
                send(server,message,strlen(message),0);


                /*printf("%s\n", account);
                printf("%s\n", aaa);
                printf("%s\n", port);
                printf("%s\n", message);*/

                recv(server,receiveMessage,sizeof(receiveMessage),0);
                printf("%s",receiveMessage);
                printf("\n");

//============ CASE: WHEN LOGGED IN ===================================================
                int client = 0;
                struct parameter p;
                p.server = server;
                p.port = atoi(port);
                void *ptr = &p;
                pthread_t tid;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_create(&tid, &attr, ClientListner, ptr);

                while(1){
                    bzero(&receiveMessage,sizeof(receiveMessage));
                    /*recv(server,receiveMessage,sizeof(receiveMessage),0);
                    printf("%s",receiveMessage);*/
                    bzero(&message,sizeof(message));

                    command = 0;
                    printf("%s", "Enter 3 for the latest list,4 for transection, 5 for exit:\n");
                    printf("\n");
                    scanf("%d", &command);
//============ CASE: WHEN LOGGED IN : LIST ===================================================
                    if(command == 3){
                        strcpy(message, list);
                        send(server,message,strlen(message),0);
                        recv(server,receiveMessage,sizeof(receiveMessage),0);
                        printf("%s",receiveMessage);
                    }
//============ CASE: WHEN LOGGED IN : TRANSECTION ===================================================
                    else if(command == 4){
                        SSL_CTX *ctx;
                        SSL *ssl;
                        SSL_library_init();
                        ctx = InitCTX();
                        ssl = SSL_new(ctx);
                        //
                        int client;
                        char buff1[len];
                        char msg[len];
                        char payAmout[10];
                        char payee[20];
                        int numBytes;
                        memset(buff1, '\0', len); //init
        //======================print LIST ========================================
                        bzero(&message,sizeof(message));
                        strcpy(message, list);
                        send(server,message,strlen(message),0);
                        bzero(&message,sizeof(message));
                        bzero(&receiveMessage,sizeof(receiveMessage));
                        /*recv(server,receiveMessage,sizeof(receiveMessage),0);
                        printf("%s",receiveMessage);
                        bzero(&receiveMessage,sizeof(receiveMessage));*/
        //=========================================================================
                        if ( (numBytes = recv(server,buff1,len,0)) == -1 ){
                            perror("recv");
                            exit(1);
                        }
                        else{
                            //printf("Server:\n%s",buff);
                            memset(msg, '\0', len);
                            printf("Please enter the payee account(within 9 char):\n");
                            scanf("%s", payee);
                            printf("Please enter theamount of transaction:\n");
                            scanf("%s", payAmout);
                            strcat(msg,account);
                            strcat(msg,"#");
                            strcat(msg,payAmout);
                            strcat(msg,"#");
                            strcat(msg,payee);                  //the format of msg == userID#money#receiverID
                            


                            //printf("%s\n",msg);


                            char *h;
                            char *p;                            //find user ip and port from buffer of list recv from server
                            char *par;
                            par = strtok(buff1, "\n");          //token off balance
                            par = strtok(NULL, "\n");           //token off online user number
                            par = strtok(NULL, "\n");           //par is now the users online
                            int found = 0;
                            while(par != NULL){
                                int pos = strlen(par) - strlen(strchr(par,'#'));
                                char parAcc [10];
                                memset(parAcc,0,10);
                                strncpy(parAcc, par, pos);
                                //printf("%s\n", par);
                                if(strcmp(payee,parAcc)==0){
                                    strtok(par,"#");            //token off account name
                                    h = strtok(NULL,"#");       //store the IP address
                                    p = strtok(NULL,"\n");      //store the amount
                                    found = 1;
                                    break;
                                }
                                //printf("%s\n",par);
                                par = strtok(NULL, "\n");
                            }
                            if(found == 0){
                                printf("Transfer failure: user offline or does not exist\n");
                                return -1;
                            }
                            client = OpenConnection(h, atoi(p));
                            SSL_set_fd(ssl, client);
                            if ( SSL_connect(ssl) == -1 ){   // perform the connection
                                ERR_print_errors_fp(stderr);
                            }
                            else{
                                printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
                                ShowCerts(ssl);
                                SSL_write(ssl, msg, strlen(msg));
                                SSL_free(ssl);
                            }
                        }
                        SSL_CTX_free(ctx);
                        close(client);

                        bzero(&receiveMessage,sizeof(receiveMessage));
                        recv(server,receiveMessage,sizeof(receiveMessage),0);
                        printf("%s",receiveMessage);
                    }
//============ CASE: WHEN LOGGED IN : EXIT ===================================================
                    else if(command == 5){
                        char Exit[] = {"Exit"};
                        strcpy(message, Exit);
                        send(server,message,strlen(message),0);
                        recv(server,receiveMessage,sizeof(receiveMessage),0);
                        printf("%s",receiveMessage);
                        break;
                    }
                    else{
                        printf("%s","the command is not available.\n");
                    }
                }
                break;
            }
            else{
                printf("%s","the port number is not available.\n");
            }
        }
        else{
             printf("%s","the command is not available.\n");
        }
    }
    
    printf("close Socket\n");
    close(server);
    return 0;
}