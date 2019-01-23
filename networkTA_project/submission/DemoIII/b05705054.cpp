#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <queue>
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>


using namespace std;

#define MAX_BUF 1024

int sockfd = 0,forClientSockfd = 0;
char clientMessage[10000]={0}; 
char clientName[200][200]={0};
char clientIP[200][200]={0};
int clientNum = 0;
char clientPort[200][200]={0};
int clientBalance[200]={0};
int clientOnline[200] = {0};
string list();

SSL_CTX *ctx;
SSL *ssl[MAX_BUF];

char mycert[] = "./cacert.pem"; 
char mykey[] = "./cakey.pem";

queue<int> client_sock_queue;
pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_thread = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_clientName = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_clientIP = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_clientPort = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_clientBalance = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_clientNum = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_online = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cl = PTHREAD_MUTEX_INITIALIZER;

void certify_server(SSL_CTX* ctx){
    if (SSL_CTX_load_verify_locations(ctx, mycert, mykey) != 1){
        ERR_print_errors_fp(stderr);
    }
    if (SSL_CTX_set_default_verify_paths(ctx) != 1){
        ERR_print_errors_fp(stderr);
    }
    if (SSL_CTX_use_certificate_file(ctx, mycert, SSL_FILETYPE_PEM) <= 0){
        ERR_print_errors_fp(stderr);
        abort();
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, mykey, SSL_FILETYPE_PEM) <= 0){
        ERR_print_errors_fp(stderr);
        abort();
    }
    
    if (!SSL_CTX_check_private_key(ctx)){
        ERR_print_errors_fp(stderr);
        printf("Private key does not match the public certification\n");
        abort();
    }
    
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, 4);
}
//Init server instance and context
SSL_CTX* InitServerCTX(void){   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc. 
    SSL_load_error_strings();   // load all error messages */
    method = TLSv1_server_method();  // create new server-method instance 
    ctx = SSL_CTX_new(method);   // create new context from method
    if ( ctx == NULL ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}


string list(){
    string msg;
    int onlineNum = 0;
    pthread_mutex_lock(&mutex_online);
    for (int i = 0; i < 200; i++){
        if(clientOnline[i] == 1){
            msg += clientName[i];
            msg += "#";
            msg += clientIP[i];
            msg += "#";
            msg += clientPort[i];
            msg += "\n";
        }
    }
    pthread_mutex_unlock(&mutex_online);
    return msg;
}



void threadDealer(int forClientSockfd,SSL *ssl){
    bool transComplete = false;

    string source;
    char objective[20];
    char tempUser[20];
    while(true){
        char useronline[200] = {0};
        char inputBuffer[10000] = {0};
        
        while(SSL_read(ssl,inputBuffer,sizeof(inputBuffer)) > 0){
            puts("recv message from client");
            puts(inputBuffer);

            const char delim[2] = "#";
            char *token;
            token = strtok(inputBuffer, delim);

            struct sockaddr_in news;
            socklen_t newslen = sizeof(news);
            getpeername(forClientSockfd, (struct sockaddr*) &news, &newslen);

            if(strcmp(token, "REGISTER") == 0){
                token = strtok(NULL, delim);
                int accountExist = 0;
                for(int i = 0; i < 200; i ++){
                    pthread_mutex_lock(&mutex_clientName);
                    if(strcmp(token, clientName[i]) == 0){
                        pthread_mutex_unlock(&mutex_clientName);
                        accountExist = 1;
                        break;
                    }
                    pthread_mutex_unlock(&mutex_clientName);
                }
                if(accountExist == 1){
                    char msg[10] = "210 FAIL";
                    SSL_write(ssl,msg,strlen(msg));
                }
                else{
                    pthread_mutex_lock(&mutex_clientName);
                    pthread_mutex_lock(&mutex_clientNum);
                    strcpy(clientName[clientNum], token);
                    pthread_mutex_unlock(&mutex_clientName);
                    pthread_mutex_unlock(&mutex_clientNum);

                    int ba = atoi(strtok(NULL, delim));
                    pthread_mutex_lock(&mutex_clientBalance);
                    pthread_mutex_lock(&mutex_clientNum);
                    clientBalance[clientNum] = ba;
                    pthread_mutex_unlock(&mutex_clientBalance);
                    pthread_mutex_unlock(&mutex_clientNum);

                    char msg[10] = "100 OK\n";
                    SSL_write(ssl,msg,strlen(msg));
                    pthread_mutex_lock(&mutex_clientNum);
                    clientNum += 1;
                    pthread_mutex_unlock(&mutex_clientNum);
                }
                
                
            }
            else if(strcmp(token, "List") == 0){
                string clientlist;
                clientlist = list();
                SSL_write(ssl,clientlist.c_str(),clientlist.length());
            }
            else if(strcmp(token, "Exit") == 0){
                for(int i = 0; i < 200; i++){
                    pthread_mutex_lock(&mutex_clientName);
                    pthread_mutex_lock(&mutex_online);
                    if(strcmp(useronline, clientName[i]) == 0){
                        clientOnline[i] = 0;
                    }
                    pthread_mutex_unlock(&mutex_online);
                    pthread_mutex_unlock(&mutex_clientName);
                    
                }

                char msg[] = "Bye\n";
                SSL_write(ssl,msg,strlen(msg));
                forClientSockfd = -1;
                break;
            }
            else if(strcmp(token, "TRANSFER") == 0){
                strcpy(objective, tempUser);
                // puts(objective);
                token = strtok(NULL, delim);
                // puts(token);
                source = token;
                token = strtok(NULL, delim);
                // puts(token);

                for(int i = 0; i < 200; i++){
                    if(source == clientName[i]){
                        pthread_mutex_lock(&mutex_clientBalance);
                        int calcu = clientBalance[i];
                        if(calcu < stoi(token)){
                            cout << "Transfer failed" << endl 
                                 << source << " has not enough balance to transfer" << endl;
                            transComplete = false;

                            char sent[100];
                            strcpy(sent, "Transfer failed. ");
                            SSL_write(ssl, sent, sizeof(sent));

                            pthread_mutex_unlock(&mutex_clientBalance);
                            break;
                        }
                        else{
                            transComplete = true;
                            calcu -= stoi(token);
                            clientBalance[i] = calcu;
                            pthread_mutex_unlock(&mutex_clientBalance);

                            // puts("this objective\n");
                            // puts(objective);

                            for(int t = 0; t < 200; t++){
                                if(strcmp(objective,clientName[t]) == 0){
                                    pthread_mutex_lock(&mutex_clientBalance);
                                    int a = stoi(token);
                                    clientBalance[t] += stoi(token);
                                    pthread_mutex_unlock(&mutex_clientBalance);
                                    break;
                                }
                            }
                        }
                    }
                }

                if(transComplete == false){
                    continue;
                }
                    
                else{
                    char sent[200];
                    strcpy(sent, "Transfer success.");
                    SSL_write(ssl, sent, sizeof(sent));
                }
            }
            else if(strcmp(token, "SEARCH") == 0){
                puts(token);
                token = strtok(NULL, delim);
                bool find = false;
                
                puts(token);
                
                for(int i = 0; i < 200; i++){
                    if(strcmp(clientName[i],token) == 0){
                        find = true;
                        cout << token << "jijij" << clientName[i];
                        char findMsg[200];
                        strcpy(findMsg, clientIP[i]);
                        strcat(findMsg, "#");
                        strcat(findMsg, clientPort[i]);
                        
                        SSL_write(ssl, findMsg, strlen(findMsg));
                        break;
                    }
                }

                if(find == false){
                    string findMsg = "404";
                    SSL_write(ssl, findMsg.c_str(), findMsg.length());
                }
            }
            else{
                puts("i receive login");
                int clientFound = 0;
                for(int i = 0; i <= 200; i++){
                    pthread_mutex_lock(&mutex_clientName);
                    if(strcmp(token, clientName[i]) == 0){
                        strcpy(tempUser, clientName[i]);
                        clientFound = 1;
                    }
                    pthread_mutex_unlock(&mutex_clientName);
                }
                if(clientFound == 0){
                    char msg[100] = "220 AUTH_FAIL\n";
                    SSL_write(ssl,msg,strlen(msg));
                }
                else{
                    for(int i = 0; i <= 200; i++){
                        pthread_mutex_lock(&mutex_clientName);
                        bzero(useronline,sizeof(useronline));
                        strcpy(useronline, clientName[i]);
                        pthread_mutex_unlock(&mutex_clientName);

                        if(strcmp(token, useronline) == 0){
                            puts("find the user");
                            pthread_mutex_lock(&mutex_online);
                            clientOnline[i] = 1;
                            pthread_mutex_unlock(&mutex_online);
                            
                            char balancemsg[10000];
                            pthread_mutex_lock(&mutex_clientBalance);
                            sprintf(balancemsg, "%d", clientBalance[i]); 
                            pthread_mutex_unlock(&mutex_clientBalance);

                            strcat(balancemsg, "\n");
                            pthread_mutex_lock(&mutex_clientIP);
                            strcpy(clientIP[i], inet_ntoa(news.sin_addr));
                            pthread_mutex_unlock(&mutex_clientIP);

                            token = strtok(NULL, delim);
                            pthread_mutex_lock(&mutex_clientPort);
                            strcpy(clientPort[i],token);
                            pthread_mutex_unlock(&mutex_clientPort);


                            string clientlist (balancemsg);
                            clientlist += list();
                            SSL_write(ssl,clientlist.c_str(),clientlist.length());
                            break;
                        }
                    }                
                }  
            }
            bzero(inputBuffer, sizeof(inputBuffer));  
        }
        
    }
}


void* clientDealer(void* socket_desc){
    int thread_id = -1;
    char sen[1024];
    
    while(true){
        int client_queue_id = -1;

        char clientIP[1024];
        pthread_mutex_lock(&mutex_cl);
        if(client_sock_queue.size() <= 0){
            pthread_mutex_unlock(&mutex_cl);
            continue;
        }
        client_queue_id = client_sock_queue.front();
        client_sock_queue.pop();
        pthread_mutex_unlock(&mutex_cl);

        if(client_queue_id < 0){   
            puts("Acception fail.\n");
            continue;
        }
        else if(client_queue_id == 0){
            puts("There is no client online.\n");
            continue;
        }
        
        threadDealer(client_queue_id,ssl[client_queue_id]);

    }
}

int main(int argc , char *argv[]){

    //socket的建立

    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket的連線
    struct sockaddr_in serverInfo,clientInfo;
    socklen_t addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(8700);
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sockfd,5);


    SSL_library_init(); //init SSL library
    ctx = InitServerCTX();  //initialize SSL 
    certify_server(ctx); // load certs and key


    pthread_t *client_thread;
    client_thread = new pthread_t[5];
    for (int i = 0; i < 5; i++){
        if (pthread_create(&client_thread[i], NULL, clientDealer, nullptr) < 0){
            return -1;
        }
    }
    while(true){
        forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);
        // pthread_mutex_lock(&mutex_queue);

        ssl[forClientSockfd] = SSL_new(ctx);
        SSL_set_fd(ssl[forClientSockfd], forClientSockfd);      // set connection socket to SSL state

        int accept = SSL_accept(ssl[forClientSockfd]);  
        if(accept < 0){
            continue;
        }
        string success="Successful Connection\n";
        //send the message to client that it connects successfully
        SSL_write (ssl[forClientSockfd], success.c_str(), success.length());

        client_sock_queue.push(forClientSockfd);
        // pthread_mutex_unlock(&mutex_queue);
    }
    
    
    
    return 0;
}
