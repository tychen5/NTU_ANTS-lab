#include <iostream>
#include <cstdio>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <queue>

#define BLUE    "\x1b[34m"
#define RESET   "\x1b[0m"
#define MAX 256
#define CLIENT_NUM 5
using namespace std;
int socketServer = 0;
int forClient = 0;

struct sockaddr_in serverbind, clientConnect;
socklen_t client_size = sizeof(clientConnect);;

queue<int> client_queue;
pthread_mutex_t mutex_thread = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_username = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_userip = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_userport = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_usermoney = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_online = PTHREAD_MUTEX_INITIALIZER;

vector<string> username;
vector<string> userip;
vector<string> userport;
vector<string> usermoney;

string getOnline()
{
    char list[MAX];
    int indivi = 0;
    string indiviString;
    strcpy(list, "\nnumber of accounts online: ");

    pthread_mutex_lock(&mutex_online);
    indivi = username.size();
    indiviString = to_string(indivi);
    strcat(list, indiviString.c_str());
    strcat(list, "\n");
    for(int i = 0; i < username.size(); i++)
    {
        if(!username.empty())
        {
            strcat(list, username[i].c_str());
            strcat(list, "#");
            strcat(list, userip[i].c_str());
            strcat(list, "#");
            strcat(list, userport[i].c_str());
            strcat(list, "\n");
        }
    }
    string result(list);
    pthread_mutex_unlock(&mutex_online);
    return result;
}

void chatting(int sock_client, char *clientIP)
{
    char buff[MAX];
    char sent[MAX];
    int status = 0;
    int memo = 0;
    int n = 0;
    string online;
    string tempUser = "";
    string currentUser = "";
    string onlineString = "";
    
    while(true)
    {
        bzero(buff, MAX);
        bzero(sent, MAX);
        n = recv(sock_client, buff, sizeof(buff), 0);
        if(n == 0) // offline
            break;
        string temp(buff);
        
        if(temp.find("REGISTER#") != string::npos) // Register
            status = 1;
        else if(temp.find("#") != string::npos && temp.find("REGISTER#") == string::npos) // Login
            status = 2;
        else
        {
            if(temp.find("Exit") != string::npos) // Exit
                status = 8;
            else // List
                status = 7;
        }
        bzero(buff, MAX);
        bzero(sent, MAX);

        if(status == 1) // Register
        {
            for(int i = 9; i < temp.length(); i++)
            {
                if(temp[i] == '#')
                {
                    pthread_mutex_lock(&mutex_username);
                    pthread_mutex_lock(&mutex_usermoney);
                    username.push_back(temp.substr(9, i-9));
                    usermoney.push_back(temp.substr(i+1, temp.length()-i-1));
                    pthread_mutex_unlock(&mutex_usermoney);
                    pthread_mutex_unlock(&mutex_username);
                    break;
                }
            }
            pthread_mutex_lock(&mutex_userip);
            pthread_mutex_lock(&mutex_userport);

            userip.push_back("");
            userport.push_back("");

            pthread_mutex_unlock(&mutex_userport);
            pthread_mutex_unlock(&mutex_userip);
            strcpy(sent, "100 OK");
            send(sock_client, sent, strlen(sent), 0);
            
            if(temp[9] == '\0')
            {
                bzero(sent, MAX);
                strcpy(sent, "210 FAIL");
                send(sock_client, sent, strlen(sent), 0);
            }
            temp = "";
            bzero(sent, MAX);
        }
        else if(status == 2) // Login
        {
            int a = 0;
            for(int i = 0; i < temp.length(); i++)
            {
                if(temp[i] == '#')
                {
                    tempUser = temp.substr(0, i);
                    memo = i;
                    break;
                }
            }
            for(int i = 0; i < username.size(); i++)
            {
                if(tempUser == username[i])
                {
                    a = 1;
                    pthread_mutex_lock(&mutex_userip);
                    pthread_mutex_lock(&mutex_userport);

                    userport[i] = temp.substr(memo+1, temp.length()-memo-1);
                    userip[i] = clientIP;

                    pthread_mutex_unlock(&mutex_userport);
                    pthread_mutex_unlock(&mutex_userip);

                    bzero(sent, MAX);
                    strcpy(sent, usermoney[i].c_str());
                    online = getOnline();
                    strcat(sent, online.c_str());
                    send(sock_client, sent, strlen(sent), 0);
                    bzero(sent, MAX);
                    temp = "";
                    memo = 0;
                    break;
                }
            }
            if(a == 0)
            {
                bzero(sent, MAX);
                strcpy(sent, "220 AUTH_FAIL");
                send(sock_client, sent, strlen(sent), 0);
                bzero(sent, MAX);
                temp = "";
                continue;
            }
        }
        else if(status == 7) // List
        {
            for(int i = 0; i < username.size(); i++)
            {
                if(tempUser == username[i])
                {
                    bzero(sent, MAX);
                    strcpy(sent, usermoney[i].c_str());
                    online = getOnline();
                    strcat(sent, online.c_str());
                    send(sock_client, sent, strlen(sent), 0);
                    bzero(sent, MAX);
                    break;
                }
            }
            temp = "";
        }
        else if(status == 8) // Exit
        {
            for(int i = 0; i < username.size(); i++)
            {
                if(tempUser == username[i])
                {
                    cout << "The client named " << tempUser << " has exited." << endl;
                    pthread_mutex_lock(&mutex_username);
                    pthread_mutex_lock(&mutex_usermoney);
                    pthread_mutex_lock(&mutex_userip);
                    pthread_mutex_lock(&mutex_userport);

                    username.erase(username.begin()+i);
                    userport.erase(userport.begin()+i);
                    userip.erase(userip.begin()+i);
                    usermoney.erase(usermoney.begin()+i);

                    pthread_mutex_unlock(&mutex_userport);
                    pthread_mutex_unlock(&mutex_userip);
                    pthread_mutex_unlock(&mutex_usermoney);
                    pthread_mutex_unlock(&mutex_username);

                    bzero(sent, MAX);
                    strcpy(sent, "Bye");
                    send(sock_client, sent, strlen(sent), 0);
                    bzero(sent, MAX);
                    tempUser = "";
                    temp = "";
                    break;
                }
            }
            
        }
    
    }

}

void* dealClient(void* thread_iden) // thread
{
    int thread_id = -1;
    char sen[MAX];
    while(true)
    {
        int client_queue_id = -1;
        char clientIP[MAX];
        pthread_mutex_lock(&mutex_thread);
        if(client_queue.size() <= 0)
        {
            pthread_mutex_unlock(&mutex_thread);
            continue;
        }
        client_queue_id = client_queue.front();
        client_queue.pop();
        pthread_mutex_unlock(&mutex_thread);

        if(client_queue_id < 0)
        {
            cout << "Acception fail." << endl;
            continue;
        }
        
        inet_ntop(AF_INET, &clientConnect.sin_addr.s_addr, clientIP, sizeof(clientIP));
        
        bzero(sen, MAX);
        strcpy(sen, "Connection accepted.");
        send(forClient, sen, strlen(sen), 0);
        bzero(sen, MAX);

        chatting(client_queue_id, clientIP);
    }
}

int main(int argc, char *argv[])
{
    // thread create
    pthread_t thread_iden[CLIENT_NUM]; // thread identifier
    pthread_attr_t attr; // set attribute of thread

    // build socket
    socketServer = socket(AF_INET, SOCK_STREAM, 0);
    if(socketServer < 0)
    {
        cout << "Fail to create socket.";
        return -1;
    }

    // bind port address
    bzero(&serverbind, sizeof(serverbind));
    serverbind.sin_family = AF_INET;
    serverbind.sin_addr.s_addr = INADDR_ANY;
    //inet_aton("127.0.0.1", &serverbind.sin_addr);
    serverbind.sin_port = htons(8080);

    // bind
    bind(socketServer, (struct sockaddr *)&serverbind, sizeof(serverbind));

    // listen
    listen(socketServer, CLIENT_NUM);

    for(int i = 0; i < CLIENT_NUM; i++) // create thread
    {
        pthread_attr_init(&attr);
        pthread_create(&thread_iden[i], &attr, dealClient, (void*)(thread_iden));
    }
    
    while(true)
    {
        forClient = accept(socketServer, (struct sockaddr *)&clientConnect, &client_size);
        client_queue.push(forClient);
    }

    return 0;
}
