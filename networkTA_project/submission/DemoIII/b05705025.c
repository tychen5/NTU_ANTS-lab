#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "threadpool.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


#define ser_ip "127.0.0.1"
#define ser_port 3698
#define max_thread_size 20  //initializing maximun size of pools

typedef struct ClientInfo
{
    struct sockaddr_in addr;
    int clientfd;
    int isConn;
    int index;
} ClientInfo;

/*global variable*/
char user[50][100] = {};
char user_port[50][6] = {};
char user_addr[50][20] = {};
char user_dep[50][15] = {};
int giver = 0;
int receiver = 0;
int user_login[50] = {0};
int userCnt = 0;
int onlineCnt = 0;


/*
    server jobs handling
 */
void* serverWork(void* argv)
{
    ClientInfo *client = (ClientInfo *)(argv);
    int clientfd = client->clientfd;
    struct sockaddr_in addr = client->addr;
    char message[] = {"connection successful.\n"};
    char *delim = "#";
    int login = 0;
    int userId = 0;
    char inputBuffer[256] = {};
    send(clientfd,message,strlen(message),0);
    // SSL_write(ssl, p2p_message,strlen(p2p_message));

    while(1){
        bzero(inputBuffer, sizeof(inputBuffer));

        if(recv(clientfd,inputBuffer,sizeof(inputBuffer),0) == 0){
            //printf("this is input buffer1: %s\n", inputBuffer );
            if(user_login[userId] == 1){
                onlineCnt --;
                printf("%s ", user[userId]);
                printf("%s\n", "log out");
                printf("%d\n", userId);
                user_login[userId] = 0;
                close(client->clientfd);
                break;
            }
        }
        else{
        //printf("this is input buffer2: %s\n", inputBuffer );       
            char output[256] = {};
            if(login == 0){
                char *spl;
                spl = strtok(inputBuffer, delim);
                if(strcmp(spl, "REGISTER") == 0){
                    int repeat = 0;
                    spl = strtok (NULL, delim);
                    for(int i = 0; i <= userCnt; i++){
                        if(strcmp(user[i], spl) == 0){
                            send(clientfd, "repeated user number", strlen("repeated user number"), 0);
                            repeat = 1;
                            break;
                        }
                    }
                    if(repeat == 0){
                        char money[15] = {};
                        char dep_message[100] = {};
                        strcpy(user[userCnt], spl);
                        strcpy(output, user[userCnt]);
                        send(clientfd, "100 OK", strlen("100 OK"), 0);
                        userCnt++;
                        if(recv(clientfd,money,sizeof(money),0) != 0){
                            strcpy(dep_message, "your deposit amount is: ");
                            strcat(dep_message, money);
                            send(clientfd, dep_message, strlen(dep_message), 0);
                            strcpy(user_dep[userCnt-1], money); 
                        }
                   }
                }

                else if(spl != NULL){
                    int identify = 0;
                    for(int i = 0; i < userCnt; i++){
                        if(strcmp(spl, user[i]) == 0){
                            identify = 1;
                            userId = i;
                            break;
                        }
                    }
                    if(identify == 0){
                        strcpy(output, "220 AUTH_FAIL");
                        send(clientfd, output, strlen(output), 0);
                        //printf("%s\n", output);
                    }
                    else{
                        spl = strtok(NULL, delim);
                        int port = atoi(spl);
                        if(port > 1024 && port <= 69999){
                            user_login[userId] = 1;
                            onlineCnt++;
                            strcpy(user_port[userId], spl);
                            printf("%s\n", spl);
                            printf("%s\n", user_port[userId]);
                            strcpy(output, user_dep[userId]);
                            send(clientfd, output, strlen(output), 0);
                            //printf("%s\n", output);
                            login = 1;
                            inet_ntop(AF_INET, &(addr.sin_addr), user_addr[userId], INET_ADDRSTRLEN);
                            /*handle list*/
                            char num_of_user[5] = {};
                            char list[200] = {};
                            sprintf(num_of_user, "%d", onlineCnt);
                            strcpy(list, "number of account online: ");
                            strcat(list, num_of_user);
                            strcat(list, "\n");
                            for(int i = 0; i < userCnt; i++){
                                if(user_login[i] == 1){
                                    strcat(list, user[i]);
                                    strcat(list,"#");
                                    strcat(list, user_addr[i]);
                                    strcat(list,"#");
                                    strcat(list,user_port[i]);
                                    strcat(list,"#");
                                    strcat(list,user_dep[i]);
                                    strcat(list, "\n");
                                }
                            }
                            send(clientfd, list, strlen(list), 0);
                        }
                        else{
                            strcpy(output, "port num out of range, please re-assign.\n");
                            send(clientfd, output, strlen(output), 0);
                        }
                    }
                }
            }
            else{
                if(strcmp(inputBuffer, "Exit") == 0){
                    strcpy(output, "Bye\n");
                    send(clientfd, output, strlen(output), 0);
                }
                else if(strcmp(inputBuffer, "List") == 0){
                    char num_of_user[5] = {};
                    char list[200] = {};
                    sprintf(num_of_user, "%d", onlineCnt);
                    strcpy(list, "number of client online: ");
                    strcat(list, num_of_user);
                    strcat(list, "\n");
                    for(int i = 0; i < userCnt; i++){
                        if(user_login[i] == 1){
                            strcat(list, user[i]);
                            strcat(list,"#");
                            strcat(list, user_addr[i]);
                            strcat(list,"#");
                            strcat(list,user_port[i]);
                            strcat(list, "#");
                            strcat(list,user_dep[i]);
                            strcat(list, "\n");
                        }
                    }
                    send(clientfd, list, strlen(list), 0);
                }
                else if(strcmp(inputBuffer, "pay") == 0){
                    giver = userId;
                    char tempBuffer[256] = {};
                    if(recv(clientfd,tempBuffer,sizeof(tempBuffer),0) == 0){
                        if(user_login[userId] == 1){
                            onlineCnt --;
                            printf("%s", user[userId]);
                            printf("%s\n", "log out");
                            printf("%d\n", userId);
                            user_login[userId] = 0;
                            close(client->clientfd);
                            break;
                        }
                    }
                    else{
                        int is_acc = 0;
                        for(int i = 0; i < userCnt; i++){
                            if(user_login[i] == 1 && strcmp(user[i], tempBuffer) == 0){
                                /*receiver = i;
                                printf("receiver = %d\n", receiver);
                                printf("receiver = %s\n", user[i]);*/
                                //printf("tempbuffer = %s\n", tempBuffer);
                                char tempMessage[256] = {};
                                is_acc = 1;
                                strcpy(tempMessage, user_addr[i]);
                                strcat(tempMessage, "#");
                                strcat(tempMessage, user_port[i]);
                                send(clientfd, tempMessage, strlen(tempMessage), 0);
                                break;
                            }
                        }
                        if(is_acc == 0){
                            send(clientfd, "transfer failed", strlen("transfer failed"), 0);
                        }
                    }
                }
                else{
                    //int change = 0;
                    char* cut;
                    cut = strtok(inputBuffer, delim);
                    if(cut == NULL){
                    strcpy(output, "What are u doing?");
                    send(clientfd, output, strlen(output), 0);
                    }
                    else{
                            for(int i = 0; i < userCnt; i++){
                                if(user_login[i]== 1 && strcmp(user[i],cut) == 0){
                                    receiver = i;
                                }
                            }
                            cut = strtok (NULL, delim);
                            int sub_money = 0;
                            int add_money = 0;
                            sub_money = atoi(user_dep[giver]) - atoi(cut);
                            sprintf(user_dep[giver], "%d", sub_money );
                            add_money = atoi(user_dep[receiver]) + atoi(cut);
                            sprintf(user_dep[receiver], "%d", add_money );
                            printf("giver deposit = %s\n", user_dep[giver]);
                            printf("receiver deposit = %s\n", user_dep[receiver]);
                        // }
                    }
                }
            }
        }
    }
    return NULL;
}

int main()
{
    struct sockaddr_in ser_addr,client_addr;
    int serv_sock,client_sock;
    socklen_t client_sz; 
    thread_pool * pool;
    struct ClientInfo clients[256];
    int i = 0;
    char pwd[100];
    char* temp;

    SSL_CTX *ctx;
    /* SSL inicialize */
    SSL_library_init();
    /* loading SSL algorithm */
    OpenSSL_add_all_algorithms();
    /* loading SSL error message */
    SSL_load_error_strings();
    /*  SSL V2 & V3 standard to create SSL_CTX aka SSL Content Text */
    ctx = SSL_CTX_new(SSLv23_server_method());
    /* also SSLv2_server_method() or SSLv3_server_method() represent V2 or V3standard */
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* get client public key */
    getcwd(pwd,100);
    if(strlen(pwd)==1)
        pwd[0]='\0';
    if (SSL_CTX_use_certificate_file(ctx, temp=strcat(pwd,"/servercert.pem"), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* getprivate key */
    getcwd(pwd,100);
    if(strlen(pwd)==1)
        pwd[0]='\0';
    if (SSL_CTX_use_PrivateKey_file(ctx, temp=strcat(pwd,"/serverkey.pem"), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* check key correction */
    if (!SSL_CTX_check_private_key(ctx))
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
 
    pool = (thread_pool*)malloc(max_thread_size*sizeof(thread_pool));

    serv_sock = socket(AF_INET,SOCK_STREAM,0);
    bzero(&ser_addr,sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ser_addr.sin_port = htons(ser_port);

    thread_pool_init(pool,max_thread_size);
    
    if(bind(serv_sock,(struct sockaddr*)& ser_addr,sizeof(ser_addr)) == -1)
    {
        perror("bind() ");
        exit(1);
    }

    if(listen(serv_sock,20) == -1)
    {
        perror("listen() ");
        exit(1);
    }

    //while(int j = 0;j<max_thread_size;j++)
    while(1)
    {
        client_sz = sizeof(client_addr);
        client_sock = accept(serv_sock,(struct sockaddr*)& client_addr,&client_sz);
        clients[i].clientfd = client_sock;
        clients[i].addr = client_addr;

        /* base on ctx create SSL */
        SSL *ssl;
        ssl = SSL_new(ctx);
        /* connect socket to SSL */
        SSL_set_fd(ssl, client_sock);
        /* construct SSL connection */
        if (SSL_accept(ssl) == -1)
        {
            perror("accept");
            close(client_sock);
            break;
        }

        //deal with server's work and assign to each pool
        threadpool_add_runner(pool,serverWork,(void*)&clients[i]);
        i++;

        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    return 0;
 
    SSL_CTX_free(ctx);
}