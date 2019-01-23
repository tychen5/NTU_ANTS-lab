#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <resolv.h>
#include <errno.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


void* p2pServer(void* args);
void* p2pConnect(void* args);
void ShowCerts(SSL * ssl);

/*global variable*/
int sd = 0;
int p2p_sd = 0;
int local_port = 0;
int transfer_money = 0;
int port_num = 0;
char transfer_ip[100] = {};
char receiver[100] = {};
char transfer_port[100] = {};



int main(int argc , char *argv[])
{
    
    SSL_CTX *ctx;
    SSL *ssl;

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(SSLv23_client_method());
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    //construct socket sd
    sd = socket(PF_INET, SOCK_STREAM, 0);
    if (sd == -1){
        printf("Fail to create a socket.");
    }

    //socket connection

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = AF_INET;

    /*localhost test*/
    info.sin_addr.s_addr = inet_addr("127.0.0.1");
    info.sin_port = htons(3698);

    /*temp server*/

    // info.sin_addr.s_addr = inet_addr("140.112.107.194");
    // info.sin_port = htons(33120);
    /*jacob's server*/

    // info.sin_addr.s_addr = inet_addr("140.112.106.45");
    // info.sin_port = htons(33220);
    /*operatable IP and port*/

    // int pnum = 0;
    // char ip[20] = {};
    // info.sin_addr.s_addr = inet_addr(argv[1]);
    // info.sin_port = htons(atoi(argv[2]));


    /*made a connection and check for mistakes*/
    int retcode = connect(sd,(struct sockaddr *)&info,sizeof(info));
    if(retcode==-1){
        printf("%s\n","Connection error");
        return 0;
    }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sd);
    /* construct SSL connection */
    if (SSL_connect(ssl) == -1)
        ERR_print_errors_fp(stderr);
    else
    {
        // printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
        // ShowCerts(ssl);
    }
 


    char id[100] = {};
    char t[100] = {};
    char* delim = "#";
    recv(sd,t,sizeof(t),0);
    // SSL_read(ssl, t,sizeof(t))
    printf("%s",t);
    int login = -1;
    char local_acc[100] = {};
    int local_money = 0;

    
    while(1){
        char receiveMessage[100] = {};
        int option = 0;
        char message[100] = {};
        //if not login
        if(login == -1){
            printf("%s\n", "enter 1 to register or 2 to login");
            scanf("%d", &option);

            //register
            if(option == 1){
                    char money[15] = {};
                    char temp_message[100] = {};
                    printf("%s", "enter your name: ");
                    scanf("%s",id);
                    strcpy(local_acc,id);
                    strcpy(message,"REGISTER#");
                    strcat(message, id);
                    send(sd,message,strlen(message),0);
                    // SSL_write(ssl, message,strlen(message));
                    recv(sd,receiveMessage,sizeof(receiveMessage),0);
                    // SSL_read(ssl, receiveMessage,sizeof(receiveMessage));
                    printf("%s\n",receiveMessage);
                    printf("%s", "enter the amount you want to deposit:");
                    scanf("%d", &local_money);
                    sprintf(money, "%d", local_money);
                    send(sd,money,strlen(money),0);
                    // SSL_write(ssl, money,strlen(money));
                    recv(sd,temp_message,sizeof(temp_message),0);
                    // SSL_read(ssl, temp_message,sizeof(temp_message));
                    printf("%s\n", temp_message);
                }
                else if(option == 2){
                    char temp[100] = {};
                    printf("%s", "enter your name: ");
                    scanf("%s",temp);
                    int port = 0;
                    char num[10] = {};
                    printf("%s", "enter port number: ");
                    scanf("%d", &port);
                    if(port <= 1025 || port >= 69999){
                        printf("%s\n", "port num out of range, please re-assign.");
                    }
                    else{
                        local_port = port;
                        sprintf(num, "%d", port);
                        strcpy(message,temp);
                        strcat(message, "#");
                        strcat(message, num);
                        send(sd,message,strlen(message),0);
                        // SSL_write(ssl, message,strlen(message));
                        recv(sd,receiveMessage,sizeof(receiveMessage),0);
                        // SSL_read(ssl, receiveMessage,sizeof(receiveMessage));
                        printf("%s\n",receiveMessage);
                        //printf("%d", strcmp(receiveMessage, "220 AUTH_FAIL"));
                        if(strcmp(receiveMessage, "220 AUTH_FAIL") == 0){
                            printf("%s\n", "Wrong Account Name! Please re-register or re-login");
                        }
                        else{
                            login = 1;
                            printf("%s\n", "login success");
                            pthread_t server;
                            // 建立子執行緒，create client's server
                            pthread_create(&server, NULL, p2pServer, NULL);
                        }
                    }
                }
        }
        else{
            printf("%s", "enter option code: ");
            scanf("%d", &option);
            if(option == 1){
                strcpy(message,"List");
                send(sd,message,strlen(message),0);
                // SSL_write(ssl, message,strlen(message));
                recv(sd,receiveMessage,sizeof(receiveMessage),0);
                // SSL_read(ssl, receiveMessage,sizeof(receiveMessage));
                printf("%s",receiveMessage);
            }
            else if(option == 2){
                send(sd, "pay", strlen("pay"), 0);
                // SSL_write(ssl, "pay",strlen("pay"));
                //char receiver[100] = {};
                char tempMessage[256] = {};
                // int transfer_money = 0;
                printf("%s", "Please enter the amount of money you want to pay:");
                scanf("%d", &transfer_money);
                printf("%s", "Please enter the user account you want to pay:");
                scanf("%s", receiver);
                strcpy(message,local_acc);
                send(sd,message,strlen(message),0);
                // SSL_write(ssl, message,strlen(message));
                recv(sd,tempMessage,sizeof(tempMessage),0);
                // SSL_read(ssl, tempMessage,sizeof(tempMessage));
                //printf("client receive socket message: %s/n", tempMessage);
                if(strcmp(tempMessage, "transfer failed") == 0){
                    printf("%s\n", "Transfer Failed! User is not log in.");
                }
                else{
                    char *spl;
                    // char transfer_ip[100] = {};
                    // char transfer_port[100] = {};
                    // int port_num = 0;
                    spl = strtok(tempMessage, delim);
                    strcpy(transfer_ip, spl);
                    spl = strtok (NULL, delim);
                    strcpy(transfer_port, spl);
                    port_num = atoi(transfer_port);
                    if(transfer_money <= local_money && strcmp(local_acc, receiver) != 0){
                        pthread_t connection;
                            // 建立子執行緒，傳入 input 進行計算
                        pthread_create(&connection, NULL, p2pConnect, NULL);
                    }
                    else{
                        printf("%s\n", "you don't have enough money to transfer or you transfer money to yourself.");
                    }
                }
                

            }
            else if(option == 8){
                strcpy(message,"Exit");
                send(sd,message,strlen(message),0);
                // SSL_write(ssl, message,strlen(message));
                recv(sd,receiveMessage,sizeof(receiveMessage),0);
                // SSL_read(ssl, receiveMessage,sizeof(receiveMessage));
                printf("%s",receiveMessage);
                break;
            }
            else{
                printf("%s\n", "option number not exist");
            }
        }

    }
    printf("close Socket\n");
    shutdown(sd, 0);
    
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);

    //close(sd);
    return 0;
}


void* p2pServer(void* args){
    SSL_CTX *ctx2;
    char pwd[100];
    char* temp;

    /* SSL initialize */
    SSL_library_init();
    /* load SSL algorithm */
    OpenSSL_add_all_algorithms();
    /* load SSL error message */
    SSL_load_error_strings();
    /* use SSL V2 & V3 create SSL_CTX aka SSL Content Text */
    ctx2 = SSL_CTX_new(SSLv23_server_method());
    /* nor using SSLv2_server_method() or SSLv3_server_method() along with V2 or V3 procedure */
    if (ctx2 == NULL)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* get public key of client */
    getcwd(pwd,100);
    if(strlen(pwd)==1)
        pwd[0]='\0';
    if (SSL_CTX_use_certificate_file(ctx2, temp=strcat(pwd,"/servercert.pem"), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* get private key */
    getcwd(pwd,100);
    if(strlen(pwd)==1)
        pwd[0]='\0';
    if (SSL_CTX_use_PrivateKey_file(ctx2, temp=strcat(pwd,"/serverkey.pem"), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* check whether the keys are correct */
    if (!SSL_CTX_check_private_key(ctx2))
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }


    char inputBuffer[256] = {};
    int sockfd = 0,forClientSockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1){
        printf("Fail to create a socket.");
    }
    /*socket connection*/
    struct sockaddr_in serverInfo,clientInfo;
    socklen_t addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(local_port);
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sockfd,5);
   
    // printf("successfully create p2p server\n" );

    char *delim = "#";

    while(1){
        forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);
        //printf("successfully connect to p2p client\n");
        bzero(inputBuffer, sizeof(inputBuffer));
        SSL *ssl2;
        /* based on ctx create SSL */
        ssl2 = SSL_new(ctx2);
        /* add user connected to socket adding to SSL */
        SSL_set_fd(ssl2, forClientSockfd);
        printf("successfully add socket into ssl \n");
        /* construct SSL connection */
        if (SSL_accept(ssl2) == -1)
        {
            perror("accept\n");
            close(forClientSockfd);
            break;
        }
        printf("successfully accept ssl connection \n");
        recv(forClientSockfd,inputBuffer,sizeof(inputBuffer),0);
        // SSL_read(ssl2, inputBuffer,sizeof(inputBuffer));
        char *spl;
        spl = strtok(inputBuffer, delim);
        //printf("spl = %s\n", spl);
        if(strcmp(spl, "success") == 0){
            char tempMessage2[256] = {};
            char sender[100] = {};
            char receive_money[100] = {};
            spl = strtok (NULL, delim);
            //printf("spl2 = %s\n", spl);
            strcpy(sender, spl);
            spl = strtok (NULL, delim);
            //printf("spl3 = %s\n", spl);
            strcpy(receive_money, spl);
            strcpy(tempMessage2, sender);
            strcat(tempMessage2, "#");
            strcat(tempMessage2, receive_money);
            //printf("sending success giver info %s\n", spl);
            send(sd, tempMessage2,strlen(tempMessage2),0);
            // SSL_write(ssl2, tempMessage2,strlen(tempMessage2));
        }
        /* shutdown SSL connection */
        SSL_shutdown(ssl2);
        /* release SSL */
        SSL_free(ssl2);
        //printf("ssl2 set free \n");
    }
    SSL_CTX_free(ctx2);
    return 0;
}


void* p2pConnect(void* args){
    

    p2p_sd = socket(PF_INET, SOCK_STREAM, 0);
    if (p2p_sd == -1){
        printf("Fail to create a socket.");
    }
    //socket connection
    struct sockaddr_in info2;
    bzero(&info2,sizeof(info2));
    info2.sin_family = AF_INET;

    /*localhost test*/
    // printf("%s\n", "IP address & port = ");
    // printf("%s\n", transfer_ip);
    // printf("%d\n", port_num);
    info2.sin_addr.s_addr = inet_addr(transfer_ip);
    info2.sin_port = htons(port_num);
    
    //made a connection and check for mistakes
    int retcode = connect(p2p_sd,(struct sockaddr *)&info2,sizeof(info2));
    if(retcode==-1){
        printf("%s\n","P2P local client connection error");
        return 0;
    }
    SSL_CTX *ctx3;
    SSL *ssl3;
    /* SSL inicialize */
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx3 = SSL_CTX_new(SSLv23_client_method());
    if (ctx3 == NULL)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    ssl3 = SSL_new(ctx3);
    SSL_set_fd(ssl3, p2p_sd);
    /* construct SSL connection */
    if (SSL_connect(ssl3) == -1){
        ERR_print_errors_fp(stderr);
        printf("ssl client connection error;\n");
    }
    else
    {
        // printf("Connected with %s encryption\n", SSL_get_cipher(ssl3));
        // ShowCerts(ssl3);
    }
    char p2p_message[256] = {};
    char temp_money[10] = {};
    sprintf(temp_money, "%d", transfer_money);
    strcpy(p2p_message, "success");
    strcat(p2p_message, "#");
    strcat(p2p_message, receiver);
    strcat(p2p_message, "#");
    strcat(p2p_message, temp_money);
    send(p2p_sd,p2p_message,strlen(p2p_message),0);
    // SSL_write(ssl3, p2p_message,strlen(p2p_message));

    //close(p2p_sd);
    pthread_exit(NULL);
}


void ShowCerts(SSL * ssl)
{
  X509 *cert;
  char *line;
 
  cert = SSL_get_peer_certificate(ssl);
  if (cert != NULL) {
    printf("Digital certificate information:\n");
    line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
    printf("Certificate: %s\n", line);
    free(line);
    line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
    printf("Issuer: %s\n", line);
    free(line);
    X509_free(cert);
  }
  else
    printf("No certificate information！\n");

}