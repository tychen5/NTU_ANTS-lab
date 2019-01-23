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
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BLUE    "\x1b[34m"
#define RESET   "\x1b[0m"
#define MAX 256
#define CLIENT_NUM 5
using namespace std;
SSL_CTX *ctx;
SSL *ssl[MAX];
char pass[MAX];
char SERVER_CERT[MAX] = "servercert.pem";
char SERVER_PRI[MAX] = "serverkey.pem";
char* temp;
int socketServer = 0;
int forClient = 0;
int indivi = 0;

struct User
{
    bool online;
    string money;
    string port;
    string ip;
    string name;
};

struct sockaddr_in serverbind, clientConnect;
socklen_t client_size = sizeof(clientConnect);

vector<User> userlist;
queue<int> client_queue;
pthread_mutex_t mutex_thread = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_userlist = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_online = PTHREAD_MUTEX_INITIALIZER;

void certify_server(SSL_CTX* ctx)
{
    // if (SSL_CTX_load_verify_locations(ctx, SERVER_CERT, SERVER_PRI) != 1)
    //     ERR_print_errors_fp(stderr);
    
    // if (SSL_CTX_set_default_verify_paths(ctx) != 1)
    //     ERR_print_errors_fp(stderr);
    
    if (SSL_CTX_use_certificate_file(ctx, SERVER_CERT, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx, SERVER_PRI, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    
    if (!SSL_CTX_check_private_key(ctx))
    {
        ERR_print_errors_fp(stderr);
        cout << "--> Private key does not match the public certification." << endl;
        abort();
    }
    
    // SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    // SSL_CTX_set_verify_depth(ctx, 4);
}

SSL_CTX* initCTXServer(void)
{   
    const SSL_METHOD *ssl_method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ssl_method = TLSv1_server_method();
    ctx = SSL_CTX_new(ssl_method);
    if(ctx == NULL)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

string getOnline()
{
    char list[MAX];
    indivi = 0;
    string indiviString;

    pthread_mutex_lock(&mutex_online);
    strcpy(list, "\n");
    for(int i = 0; i < userlist.size(); i++)
    {
        if(!userlist.empty() && userlist[i].online == true)
        {
            indivi += 1;
            strcat(list, userlist[i].name.c_str());
            strcat(list, "#");
            strcat(list, userlist[i].ip.c_str());
            strcat(list, "#");
            strcat(list, userlist[i].port.c_str());
            strcat(list, "\n");
        }
    }
    string listDetail(list);
    indiviString = to_string(indivi);
    listDetail = "\nnumber of accounts online: " + indiviString + listDetail;
    pthread_mutex_unlock(&mutex_online);

    return listDetail;
}

void chatting(int sock_client, char *clientIP, SSL *ssl)
{
    bool transComplete = false;
    char buff[MAX];
    char sent[MAX];
    int status = 0;
    int memo = 0;
    int n = 0;
    string online;
    string transAmount;
    string objective;
    string source;
    string tempUser = "";
    string currentUser = "";
    string onlineString = "";
    
    while(true)
    {
        bzero(buff, MAX);
        bzero(sent, MAX);
        n = SSL_read(ssl, buff, sizeof(buff));
        if(n == 0) // offline
            break;
        string temp(buff);
        
        if(temp.find("REGISTER#") != string::npos) // Register
            status = 1;
        else if(temp.find("TRANSFER#") != string::npos) // Update balance
            status = 5;
        else if(temp.find("SEARCH#") != string::npos) // Request name
            status = 4;
        else
        {
            if(temp.find("#") != string::npos && temp.find("REGISTER#") == string::npos) // Login
                status = 2;
            else if(temp.find("Exit") != string::npos) // Exit
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
                    User u;
                    pthread_mutex_lock(&mutex_userlist);
                    u.online = false;
                    u.name = temp.substr(9, i-9);
                    u.money = temp.substr(i+1, temp.length()-i-1);
                    u.ip = "";
                    u.port = "";
                    userlist.push_back(u);
                    pthread_mutex_unlock(&mutex_userlist);
                    break;
                }
            }

            strcpy(sent, "100 OK");
            SSL_write(ssl, sent, strlen(sent));
            
            if(temp[9] == '\0')
            {
                bzero(sent, MAX);
                strcpy(sent, "210 FAIL");
                SSL_write(ssl, sent, strlen(sent));
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
            for(int i = 0; i < userlist.size(); i++)
            {
                if(tempUser == userlist[i].name)
                {
                    a = 1;
                    pthread_mutex_lock(&mutex_userlist);
                    userlist[i].port = temp.substr(memo+1, temp.length()-memo-1);
                    userlist[i].ip = clientIP;
                    userlist[i].online = true;
                    pthread_mutex_unlock(&mutex_userlist);

                    bzero(sent, MAX);
                    strcpy(sent, userlist[i].money.c_str());
                    online = getOnline();
                    strcat(sent, online.c_str());
                    SSL_write(ssl, sent, strlen(sent));
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
                SSL_write(ssl, sent, strlen(sent));
                bzero(sent, MAX);
                temp = "";
                continue;
            }
        }
        else if(status == 4) // Find
        {
            bool find = false;
            objective = temp.substr(7, temp.length()-7);

            for(int i = 0; i < userlist.size(); i++)
            {
                if(objective == userlist[i].name && userlist[i].online == true)
                {
                    find = true;
                    string findMsg = userlist[i].ip + "#" + userlist[i].port;
                    SSL_write(ssl, findMsg.c_str(), findMsg.length());
                    break;
                }
            }

            if(find == false)
            {
                string findMsg = "404";
                SSL_write(ssl, findMsg.c_str(), findMsg.length());
            }
        }
        else if(status == 5) // Transfer
        {
            objective = tempUser;
            for(int i = 9; i < temp.length(); i++)
            {
                if(temp[i] == '#')
                {
                    source = temp.substr(9, i-9);
                    transAmount = temp.substr(i+1, temp.length()-i-1);
                    break;
                }
            }

            for(int i = 0; i < userlist.size(); i++)
            {
                if(source == userlist[i].name)
                {
                    pthread_mutex_lock(&mutex_userlist);
                    if(stoi(userlist[i].money) < stoi(transAmount))
                    {
                        cout << "--> Transfer failed." << endl 
                             << "--> " << source << " has not enough balance to transfer." << endl;
                        transComplete = false;

                        bzero(sent, MAX);
                        strcpy(sent, "Transfer failed. ");
                        SSL_write(ssl, sent, strlen(sent));

                        pthread_mutex_unlock(&mutex_userlist);
                        break;
                    }
                    else
                    {
                        transComplete = true;
                        int calcu = stoi(userlist[i].money) - stoi(transAmount);
                        userlist[i].money = to_string(calcu);
                        pthread_mutex_unlock(&mutex_userlist);

                        for(int t = 0; t < userlist.size(); t++)
                        {
                            if(objective == userlist[t].name)
                            {
                                pthread_mutex_lock(&mutex_userlist);
                                calcu = 0;
                                calcu = stoi(userlist[t].money) + stoi(transAmount);
                                userlist[t].money = to_string(calcu);
                                pthread_mutex_unlock(&mutex_userlist);
                                break;
                            }
                        }
                    }
                }
            }

            if(transComplete == false)
                continue;
            else
            {
                bzero(sent, MAX);
                strcpy(sent, "Transfer success.");
                SSL_write(ssl, sent, strlen(sent));
                temp = "";
            }
        }
        else if(status == 7) // List
        {
            for(int i = 0; i < userlist.size(); i++)
            {
                if(tempUser == userlist[i].name)
                {
                    bzero(sent, MAX);
                    strcpy(sent, userlist[i].money.c_str());
                    online = getOnline();
                    strcat(sent, online.c_str());
                    SSL_write(ssl, sent, strlen(sent));
                    bzero(sent, MAX);
                    break;
                }
            }
            temp = "";
        }
        else if(status == 8) // Exit
        {
            for(int i = 0; i < userlist.size(); i++)
            {
                if(tempUser == userlist[i].name)
                {
                    cout << "--> The client named " << tempUser << " has exited." << endl;
                    pthread_mutex_lock(&mutex_userlist);
                    userlist[i].online = false;
                    pthread_mutex_unlock(&mutex_userlist);

                    bzero(sent, MAX);
                    strcpy(sent, "Bye");
                    SSL_write(ssl, sent, strlen(sent));
                    bzero(sent, MAX);
                    tempUser = "";
                    temp = "";
                    break;
                }
            }
            
        }
    
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock_client);
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
            cout << "--> Acception fail." << endl;
            continue;
        }
        else if(client_queue_id == 0)
        {
            cout << "--> There is no client online." << endl;
            continue;
        }
        
        inet_ntop(AF_INET, &clientConnect.sin_addr.s_addr, clientIP, sizeof(clientIP));
        bzero(sen, MAX);
        chatting(client_queue_id, clientIP, ssl[client_queue_id]);

    }
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        cout << "--> Not enough information.";
        return -1;
    }
    int portNum = stoi(argv[1]);

    // thread create
    pthread_t thread_iden[CLIENT_NUM]; // thread identifier
    pthread_attr_t attr; // set attribute of thread

    // initialize SSL
    SSL_library_init();
    ctx = initCTXServer();
    certify_server(ctx);

    // build socket
    socketServer = socket(AF_INET, SOCK_STREAM, 0);
    if(socketServer < 0)
    {
        cout << "--> Fail to create socket.";
        return -1;
    }

    // bind port address
    bzero(&serverbind, sizeof(serverbind));
    serverbind.sin_family = AF_INET;
    serverbind.sin_addr.s_addr = INADDR_ANY;
    //inet_aton("127.0.0.1", &serverbind.sin_addr);
    serverbind.sin_port = htons(portNum);

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

        ssl[forClient] = SSL_new(ctx);
        SSL_set_fd(ssl[forClient], forClient);
        int sslAcceptRes = SSL_accept(ssl[forClient]);
        if(sslAcceptRes < 0)
        {
            cout << "--> No SSL acception at server." << endl;
            continue;
        }
        string msg = "--> Connection accepted.";
        SSL_write(ssl[forClient], msg.c_str(), msg.length());

        client_queue.push(forClient);
    }

    close(socketServer);
    SSL_CTX_free(ctx);

    return 0;
}

