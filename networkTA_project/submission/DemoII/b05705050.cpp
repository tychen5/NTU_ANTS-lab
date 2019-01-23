#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <assert.h>
using namespace std;

#define MAXCONN 10
#define MAXACCOUNT 10

typedef struct ClientInfo
{
    struct sockaddr_in addr;
    int clientfd;
    int isConn;
    int index;
} ClientInfo;

pthread_t threadID[MAXCONN];
ClientInfo clients[MAXCONN];

int tasks = 0, done = 0;


int client_num = 0;
char client_account_name[MAXACCOUNT][256];
char client_ip_address[MAXACCOUNT][256];
int client_port_num[MAXACCOUNT];
int client_account_balance[MAXACCOUNT] = {0};
int client_online[MAXACCOUNT] = {0};

void *conversation(void* argv);

int main(int argc, char *argv[])
{
    for(int i = 0; i < MAXCONN; i++)
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
    serverInfo.sin_port = htons(8701);
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sockfd,10);

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
            while(i < MAXCONN)
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
            pthread_create(&threadID[i], NULL, conversation, &clients[i]);
            
        }
    }

    while((tasks / 2) > done) {
        usleep(10000);
    }


	return 0;
}

void *conversation(void* argv)
{

    ClientInfo *client = (ClientInfo *)(argv);
    int clientfd = client->clientfd;
    struct sockaddr_in addr = client->addr;
    int isConn = client->isConn;
    int clientIndex = client->index;
    cout << "ip: " << inet_ntoa(addr.sin_addr) << "\n port: " << ntohs(addr.sin_port) << endl;


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

        char const delim[2] = "#";    //用＃來切
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

                strcpy(client_account_name[client_num], token);
                strcpy(client_ip_address[client_num], inet_ntoa(addr.sin_addr));
                client_port_num[client_num] = ntohs(addr.sin_port);
                client_online[client_num] = 1;
                client_num += 1;
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
                client_account_balance[account_index] = atoi(token);
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
            char list[2048];
            strcpy(list, "Clients Online: ");
            char client_online_num_char[1024];
            sprintf(client_online_num_char, "%d", client_online_num);
            strcat(list, client_online_num_char);
            strcat(list, "\n");
            for (int i = 0; i < client_num; i++)
            {
                if (strcmp(client_account_name[i], name) == 0)
                {
                    account_index = i;
                }
                if (client_online[i] == 1)
                {
                    strcat(list, client_account_name[i]);
                    strcat(list, "#");
                    strcat(list, client_ip_address[i]);
                    strcat(list, "#");
                    char client_port_num_char[1024];
                    sprintf(client_port_num_char, "%d", client_port_num[i]);
                    strcat(list, client_port_num_char);
                    strcat(list, "\n");
                }
            }
            strcpy(msg_sent, "Account Balance: ");
            char client_account_balance_char[1024];
            sprintf(client_account_balance_char, "%d", client_account_balance[account_index]);
            strcat(msg_sent, client_account_balance_char);
            strcat(msg_sent, "\n");
            strcat(msg_sent, list);
        }
        else if (strcmp(token, "Exit") == 0)
        {
            for (int i = 0; i < MAXACCOUNT; i++)
            {
                if (strcmp(client_account_name[i], name) == 0)
                {
                    client_online[i] = 0;
                    break;
                }
            }
            break;
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
            char list[2048];
            strcpy(list, "Clients Online: ");
            char client_online_num_char[1024];
            sprintf(client_online_num_char, "%d", client_online_num+1);
            strcat(list, client_online_num_char);
            strcat(list, "\n");

            int account_exist = 0;
            int account_index = 0;
            for (int i = 0; i < client_num; ++i)
            {
                if (strcmp(client_account_name[i], token) == 0)
                {
                    account_index = i;
                    account_exist = 1;
                    strcpy(name, token);
                    client_online[i] = 1;
                }
                if (client_online[i] == 1)
                {
                    strcat(list, client_account_name[i]);
                    strcat(list, "#");
                    strcat(list, client_ip_address[i]);
                    strcat(list, "#");
                    char client_port_num_char[1024];
                    sprintf(client_port_num_char, "%d", client_port_num[i]);
                    strcat(list, client_port_num_char);
                    strcat(list, "\n");
                }
            }

            if (account_exist == 0)
            {
                strcpy(msg_sent, "220 AUTH_FALL");
                cout << "Sent: \n" << msg_sent << endl;
                send(clientfd,&msg_sent,sizeof(msg_sent),0);
                return 0;
            }
            else
            {
                strcpy(msg_sent, "Money Balance: ");
                char client_account_balance_char[1024];
                sprintf(client_account_balance_char, "%d", client_account_balance[account_index]);
                strcat(msg_sent, client_account_balance_char);
                strcat(msg_sent, "\n");
                strcat(msg_sent, list);
            }
        }

        cout << "Sent: \n" << msg_sent << endl;
        send(clientfd,&msg_sent,sizeof(msg_sent),0);
    }
    char msg_sent[1024];
    strcpy(msg_sent, "Bye");
    cout << "Sent: \n" << msg_sent << endl;
    send(clientfd,&msg_sent,sizeof(msg_sent),0);
    return 0;
}











