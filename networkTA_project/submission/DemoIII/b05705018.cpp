#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <assert.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "threadpool.h"

using namespace std;

#define MAXBUF 1024
#define THREAD 32
#define QUEUE  256
#define MAXACCOUNT 512

typedef struct threadpool_t threadpool_t;
typedef struct ClientInfo
{
    struct sockaddr_in addr;
    int clientfd;
    int isConn;
    int index;
    SSL *ssl;
} ClientInfo;

ClientInfo clients[THREAD];
int client_num = 0;
char client_account_name[MAXACCOUNT][1024];
char client_ip_address[MAXACCOUNT][1024];
int client_port_num[MAXACCOUNT];
int client_account_balance[MAXACCOUNT] = {0};
int client_online[MAXACCOUNT] = {0};

SSL_CTX *ctx;
pthread_mutex_t lock1;

void conversation(void* argv);

int main(int argc, char *argv[])
{
	//init ssl
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(SSLv23_server_method());

    char* temp;
    char pwd[100];
    getcwd(pwd,100);
    if (strlen(pwd) == 1) { pwd[0]='\0'; }
    if (SSL_CTX_use_certificate_file(ctx, temp=strcat(pwd,"/ssl/serverCert.pem"), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    getcwd(pwd,100);
    if (strlen(pwd) == 1) { pwd[0]='\0'; }
    if (SSL_CTX_use_PrivateKey_file(ctx, temp=strcat(pwd,"/ssl/serverKey.pem"), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    if (!SSL_CTX_check_private_key(ctx))
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    for(int i = 0; i < THREAD; i++)
       clients[i].isConn = 0;

    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1)
    {
        cout << "Fail to create a socket.\n";
    }

    //socket的連線
    struct sockaddr_in serverInfo;
    bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(8700);
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sockfd,10);

    //init thread pool
    threadpool_t *pool;
    pthread_mutex_init(&lock1, NULL);
    assert((pool = threadpool_create(THREAD, QUEUE, 0)) != NULL);

    //waiting for client connection
    while(1){
        int forClientSockfd = 0;
        struct sockaddr_in clientInfo;
        socklen_t addrlen = sizeof(clientInfo);
        forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);

        if(forClientSockfd == -1)
        {
            cout << "Fail to accept socket\n";
            continue;
        }
        else
        {
            int i = 0;
            while(i < THREAD)
            {
                if(clients[i].isConn == 0)
                    break;
                else
                    i++;
            }
            cout << "Accept socket successfully\n";
            clients[i].clientfd = forClientSockfd;
            clients[i].addr = clientInfo;
            clients[i].isConn = 1;
            clients[i].index = i;

            SSL *ssl;
            ssl = SSL_new(ctx);
            SSL_set_fd(ssl, forClientSockfd);
            if (SSL_accept(ssl) == -1)
            {
                perror("accept");
                close(forClientSockfd);
                break;
            }

            clients[i].ssl = ssl;
            threadpool_add(pool, &conversation, &clients[i], 0);
        }
    }
    assert(threadpool_destroy(pool, 0) == 0);

	return 0;
}

void conversation(void* argv)
{
    cout << "Enter conversation\n";

    pthread_mutex_lock(&lock1);
    ClientInfo *client = (ClientInfo *)(argv);
    int clientfd = client->clientfd;
    struct sockaddr_in addr = client->addr;
    pthread_mutex_unlock(&lock1);
    cout << "ip: " << inet_ntoa(addr.sin_addr) << "\t port: " << ntohs(addr.sin_port) << endl;

    SSL *ssl = client->ssl;

    while (1){
        char name[1024];

        char msg_recieved[1024];
        int handler = 0;
        memset(msg_recieved , 0 , sizeof(msg_recieved));
        handler = SSL_read(ssl, msg_recieved, MAXBUF);
        if(handler > 0)
            printf("Receive Complete !\n");
        else
        {
            printf("Failure to receive message ! Error code is %d，Error messages are '%s'\n", errno, strerror(errno));
            strcpy(msg_recieved, "Exit");
        }
        cout << "Recieve: " << msg_recieved << endl;

        char msg_sent[1024];
        char const delim[2] = "#";
        char *token;
        token = strtok(msg_recieved, delim);

        if (strcmp(token, "REGISTER") == 0)
        {
            token = strtok(NULL, delim);
            char *token2;
            token2 = strtok(NULL, delim);
            int port = atoi(token2);
            int account_exist = 0;
            for (int i = 0; i < client_num; ++i)
            {
                if (strcmp(token, client_account_name[i]) == 0)
                {
                    strcpy(msg_sent, "210 FALL");
                    account_exist = 1;
                    break;
                }
            }
            if (account_exist == 0)
            {
                strcpy(msg_sent, "100 OK");
                strcpy(name, token);

                pthread_mutex_lock(&lock1);
                strcpy(client_account_name[client_num], token);
                strcpy(client_ip_address[client_num], inet_ntoa(addr.sin_addr));
                client_port_num[client_num] = port;
                client_online[client_num] = 1;
                client_num += 1;
                pthread_mutex_unlock(&lock1);
            }
        }
        else if (strcmp(token, "DEPOSIT") == 0)
        {
            token = strtok(NULL, delim);
            int account_exist = 0;
            int account_index = 0;
            for (int i = 0; i < client_num; ++i)
            {
                if (strcmp(token, client_account_name[i]) == 0)
                {
                    account_exist = 1;
                    account_index = i;
                    break;
                }
            }
            if (account_exist == 0)
            {
                strcpy(msg_sent, "DEPOSIT_FALL");
            }
            else
            {
                token = strtok(NULL, delim);
                pthread_mutex_lock(&lock1);
                client_account_balance[account_index] = atoi(token);
                pthread_mutex_unlock(&lock1);
                strcpy(msg_sent, "DEPOSIT_OK");
            }
        }
        else if (strcmp(token, "List") == 0)
        {
            int account_index = 0;
            int client_online_num = 0;
            for (int i = 0; i < client_num; ++i)
            {
                if (client_online[i] != 0)
                {
                    client_online_num += 1;
                }
            }
            pthread_mutex_lock(&lock1);
            char list[2048];
            strcpy(list, "Online Clients: ");
            char client_online_num_char[1024];
            sprintf(client_online_num_char, "%d", client_online_num);
            strcat(list, client_online_num_char);
            strcat(list, "\n");
            strcat(list, "Online Clients Info: ");
            strcat(list, "\n");
            pthread_mutex_unlock(&lock1);
            for (int i = 0; i < client_num; i++)
            {
                if (strcmp(client_account_name[i], name) == 0)
                {
                    account_index = i;
                }
                if (client_online[i] == 1)
                {
                    pthread_mutex_lock(&lock1);
                    strcat(list, client_account_name[i]);
                    strcat(list, "#");
                    strcat(list, client_ip_address[i]);
                    strcat(list, "#");
                    char client_port_num_char[1024];
                    sprintf(client_port_num_char, "%d", client_port_num[i]);
                    strcat(list, client_port_num_char);
                    strcat(list, "\n");
                    pthread_mutex_unlock(&lock1);
                }
            }
            pthread_mutex_lock(&lock1);
            strcpy(msg_sent, "Account Balance: ");
            char client_account_balance_char[1024];
            sprintf(client_account_balance_char, "%d", client_account_balance[account_index]);
            strcat(msg_sent, client_account_balance_char);
            strcat(msg_sent, "\n");
            strcat(msg_sent, list);
            pthread_mutex_unlock(&lock1);
        }
        else if (strcmp(token, "Exit") == 0)
        {
            for (int i = 0; i < MAXACCOUNT; i++)
            {
                if (strcmp(client_account_name[i], name) == 0)
                {
                    pthread_mutex_lock(&lock1);
                    client_online[i] = 0;
                    pthread_mutex_unlock(&lock1);
                    break;
                }
            }
            break;
        }
        else if (strcmp(token, "Pay") == 0)
        {
            token = strtok(NULL, delim);
            int got = 0;
            for (int i = 0; i < client_num; i++)
            {
                if(strcmp(client_account_name[i], token) == 0 && client_online[i] == 1)
                {
                    got = 1;
                    strcpy(msg_sent, "On");
                    strcat(msg_sent, "#");
                    strcat(msg_sent, client_ip_address[i]);
                    strcat(msg_sent, "#");
                    char client_port_num_char[1024];
                    sprintf(client_port_num_char, "%d", client_port_num[i]);
                    strcat(msg_sent, client_port_num_char);
                    break;
                }
            }
            if (got == 0)
            {
                strcpy(msg_sent, "Off");
            }
        }
        else if (strcmp(token, "Paid") == 0)
        {
            char *payer;
            int pay_amount;
            char *payee;
            payer = strtok(NULL, delim);
            pay_amount = atoi(strtok(NULL, delim));
            payee = strtok(NULL, delim);

            int payer_i = -1;
            int payee_i = -1;
            for (int i = 0; i < client_num; i++)
            {
                if(strcmp(client_account_name[i], payer) == 0)
                {
                    payer_i = i;
                }
                if(strcmp(client_account_name[i], payee) == 0)
                {
                    payee_i = i;
                }
            }

            if (payer_i == payee_i || client_account_balance[payer_i] < pay_amount || payer_i == -1 || payee_i == -1)
            {
                strcpy(msg_sent, "Fail");
            }
            else
            {
                client_account_balance[payer_i] -= pay_amount;
                client_account_balance[payee_i] += pay_amount;
                strcpy(msg_sent, "Successful");
            }
        }
        else
        {
            int client_online_num = 0;
            for (int i = 0; i < client_num; i++)
            {
                if (client_online[i] != 0)
                {
                    client_online_num += 1;
                }
            }
            pthread_mutex_lock(&lock1);
            char list[2048];
            strcpy(list, "Online Clients: ");
            char client_online_num_char[1024];
            sprintf(client_online_num_char, "%d", client_online_num+1);
            strcat(list, client_online_num_char);
            strcat(list, "\n");
            strcat(list, "Online Clients Info: ");
            strcat(list, "\n");
            pthread_mutex_unlock(&lock1);

            char *token2;
            token2 = strtok(NULL, delim);
            int port_num = atoi(token2);
            int account_exist = 0;
            int account_index = 0;
            for (int i = 0; i < client_num; ++i)
            {
                if (strcmp(client_account_name[i], token) == 0)
                {
                    client_port_num[i] = port_num;
                    account_index = i;
                    account_exist = 1;
                    strcpy(name, token);
                    pthread_mutex_lock(&lock1);
                    client_online[i] = 1;
                    pthread_mutex_unlock(&lock1);
                }
                if (client_online[i] == 1)
                {
                    pthread_mutex_lock(&lock1);
                    strcat(list, client_account_name[i]);
                    strcat(list, "#");
                    strcat(list, client_ip_address[i]);
                    strcat(list, "#");
                    char client_port_num_char[1024];
                    sprintf(client_port_num_char, "%d", client_port_num[i]);
                    strcat(list, client_port_num_char);
                    strcat(list, "\n");
                    pthread_mutex_unlock(&lock1);
                }
            }

            if (account_exist == 0)
            {
                strcpy(msg_sent, "220 AUTH_FALL");
                cout << "Sent: \n" << msg_sent << endl;
                send(clientfd,&msg_sent,sizeof(msg_sent),0);
                return;
            }
            else
            {
                pthread_mutex_lock(&lock1);
                strcpy(msg_sent, "Account Balance: ");
                char client_account_balance_char[1024];
                sprintf(client_account_balance_char, "%d", client_account_balance[account_index]);
                strcat(msg_sent, client_account_balance_char);
                strcat(msg_sent, "\n");
                strcat(msg_sent, list);
                pthread_mutex_unlock(&lock1);
            }
        }

        cout << "Sent: \n" << msg_sent << endl;
        SSL_write(ssl, msg_sent, strlen(msg_sent));
        memset(msg_sent , 0 , sizeof(msg_sent));
    }

    char msg_sent[1024];
    strcpy(msg_sent, "Bye");
    cout << "Sent: \n" << msg_sent << endl;
    SSL_write(ssl, msg_sent, strlen(msg_sent));
    memset(msg_sent , 0 , sizeof(msg_sent));

    return;
}
