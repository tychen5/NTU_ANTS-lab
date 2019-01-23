#include <iostream>
#include<stdio.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <assert.h>
#include "threadpool.h"
using namespace std;

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
} ClientInfo;

ClientInfo clients[THREAD];

pthread_mutex_t lock1;

int client_num = 0;
char client_account_name[MAXACCOUNT][1024];
char client_ip_address[MAXACCOUNT][1024];
int client_port_num[MAXACCOUNT];
int client_account_balance[MAXACCOUNT] = {0};
int client_online[MAXACCOUNT] = {0};

void conversation(void* argv);

int main(int argc, char *argv[])
{
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

    threadpool_t *pool;
    pthread_mutex_init(&lock1, NULL);
    assert((pool = threadpool_create(THREAD, QUEUE, 0)) != NULL);

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


    while (1){
        char name[1024];

        char msg_recieved[1024];
        int handler = 0;
        handler = recv(clientfd,&msg_recieved,sizeof(msg_recieved),0);
        if (handler == 0) {
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
                client_port_num[client_num] = ntohs(addr.sin_port);
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
            strcpy(msg_sent, "Payment system is not open yet");
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
                if (strcmp(client_account_name[i], token) == 0 || client_port_num[i] == port_num)
                {
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
        send(clientfd,&msg_sent,sizeof(msg_sent),0);
    }
    char msg_sent[1024];
    strcpy(msg_sent, "Bye");
    cout << "Sent: \n" << msg_sent << endl;
    send(clientfd,&msg_sent,sizeof(msg_sent),0);
    return;
}
