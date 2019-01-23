#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <vector>
#include <fstream>
#include <sys/select.h>
#include <map>
#include <list>
#include <resolv.h>
#include "/usr/local/opt/openssl/include/openssl/ssl.h"
#include "/usr/local/opt/openssl/include/openssl/err.h"


using namespace std;

#define threadNum 2
#define buffer_size 1024

char mycert[] = "/Users/billhuang/NTU/junior/網路/socket/b05705017_part3/mycert.pem"; 
char mykey[] = "/Users/billhuang/NTU/junior/網路/socket/b05705017_part3/mykey.pem";

SSL_CTX* InitServerCTX(void)
{   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc. 
    SSL_load_error_strings();   // load all error messages */
    method = TLSv1_server_method();  // create new server-method instance 
    ctx = SSL_CTX_new(method);   // create new context from method
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile){
    //New lines
    if (SSL_CTX_load_verify_locations(ctx, CertFile, KeyFile) != 1)
        ERR_print_errors_fp(stderr);
    
    if (SSL_CTX_set_default_verify_paths(ctx) != 1)
        ERR_print_errors_fp(stderr);
    //End new lines
    
    /* set the local certificate from CertFile */
    if (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if (SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if (!SSL_CTX_check_private_key(ctx))
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
    
    //New lines - Force the client-side have a certificate
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, 4);
    //End new lines
}

struct user{
    string ip;
    string name;
    string portNum;
    int balance;
    bool isOnline;
};

pthread_mutex_t locker;
pthread_mutex_t Select;

struct sockaddr_in server, client;
fd_set fd;
fd_set active;
list<int> queue;
vector<user> userList;
map<int, string> userSocket;

SSL* ssl[buffer_size];
SSL_CTX *ctx;

void* worker(void* data){
    while(true){
        pthread_mutex_lock(&locker);
        if(queue.size() > 0){
            int sock = queue.front();
            if(sock >= 0){
                queue.pop_front();
                pthread_mutex_unlock(&locker);
                char buffer[buffer_size];
                memset(buffer, 0, buffer_size);
                string nowUser;
                if(SSL_read(ssl[sock], buffer, buffer_size) == 0){ //ctrl-c
                    pthread_mutex_lock(&locker);
                    nowUser = userSocket[sock];
                    pthread_mutex_lock(&locker);
                    for(auto& i : userList){
                        if(nowUser == i.name)
                            i.isOnline = 0;
                    }
                    FD_CLR(sock, &fd);
                    close(sock);
                }
                else{
                    string message(buffer);
                    bool b = false;
                    if(strncmp(buffer, "REGISTER", 8) == 0){
                        cerr << message << endl;
                        bool isNew = true;
                        size_t begin = message.find("#");
                        message.erase(0, begin+1);
                        begin = message.find("#");
                        string newUser = message.substr(0, begin);
                        cerr << newUser << endl;
                        for(auto& i : userList){
                            if(i.name == newUser){
                                string sends = "210 FAIL\nName already been used. Please Register Again.\n";
                                SSL_write(ssl[sock], sends.c_str(), sends.length());
                                isNew = false;
                                break;
                            }
                        }
                        if(isNew){
                            user u;
                            pthread_mutex_lock(&locker);
                            u.name = newUser;
                            u.balance = stoi(message.substr(begin+1));
                            u.isOnline = 0;
                            userList.push_back(u);
                            pthread_mutex_unlock(&locker);
                            string sends = "100 OK\nNow you can login.\n";
                            cerr << sends;
                            SSL_write(ssl[sock], sends.c_str(), sends.length());  
                        }
                    }
                    else if(message.find_first_of("#") != string::npos){
                        cerr << message << endl;
                        if(strncmp(buffer, "TRANSFERTO", 10) == 0){
                           size_t nameIdx = message.find("#");
                           string receiver = message.substr(nameIdx+1);
                           bool transable = false;
                           for(auto& i : userList){
                              if(i.name == receiver && i.isOnline){
                                 string s = i.portNum;
                                 SSL_write(ssl[sock], s.c_str(), s.length());
                                 transable = true;
                                 break;
                              }
                           }
                           if(!transable){
                              string e = "220 AUTH_FAIL";
                              SSL_write(ssl[sock], e.c_str(), e.length());
                           }
                        }
                        else if(strncmp(buffer, "TRANS", 5) == 0){
                           message.erase(0, 6);
                           cerr << message << endl;
                           size_t idx = message.find("#");
                           string payer = message.substr(0, idx);
                           cerr << payer;
                           message.erase(0, idx+1);
                           idx = message.find("#");
                           string amount = message.substr(0, idx);
                           cerr << amount;
                           string receiver = message.substr(idx+1);
                           cerr << receiver;

                           bool suc = false;
                           int num = stoi(amount);
                           for(int i = 0; i < userList.size(); i++){
                              if((userList[i].name == payer || userList[i].name == receiver) && userList[i].isOnline){
                                 for(int j = i+1; j < userList.size(); j++){
                                    if((userList[j].name == payer || userList[j].name == receiver)&& userList[j].isOnline){
                                       if(userList[i].name == payer){
                                          if(userList[i].balance >= num){
                                             userList[i].balance -= num;
                                             userList[j].balance += num;
                                             suc = true;
                                             string s = "success";
                                             SSL_write (ssl[sock], s.c_str(), s.length());
                                          }
                                          else{
                                             suc = true;
                                             string s = "fail";
                                             SSL_write (ssl[sock], s.c_str(), s.length());
                                          } 
                                       }
                                       else{
                                          if(userList[j].balance >= num){
                                             userList[i].balance += num;
                                             userList[j].balance -= num;
                                             suc = true;
                                             string s = "success";
                                             SSL_write (ssl[sock], s.c_str(), s.length());
                                          }
                                          else{
                                             suc = true;
                                             string s = "fail";
                                             SSL_write (ssl[sock], s.c_str(), s.length());
                                          }
                                       }
                                       break;
                                    }
                                 }
                                 break;
                              }
                           }
                           if(!suc){
                              string e = "220 AUTH_FAIL";
                              SSL_write (ssl[sock], e.c_str(), e.length());
                           }
                        }
                        else{
                           size_t begin = message.find("#");
                           string nowUser = message.substr(0, begin);
                           string portNumber = message.substr(begin+1);
                           cerr << portNumber << endl;
                           cerr << nowUser << endl;
                           string IP(inet_ntoa(client.sin_addr));
                           bool isUsed;
                           for(auto& i : userList){
                              if(i.portNum == portNumber && (i.isOnline == 1)){
                                 isUsed = true;
                                 break;
                              }
                           }
                           for(auto& i : userList){
                              cerr << i.name << endl;
                              if(i.name == nowUser && !isUsed && (i.isOnline == 0)){
                                 i.ip = IP;
                                 i.portNum = portNumber;
                                 i.isOnline = 1;
                                 pthread_mutex_lock(&locker);
                                 userSocket[sock] = nowUser;
                                 pthread_mutex_unlock(&locker);
                                 b = true;
                              }
                           }
                           string sends;
                           if(b){
                              for(auto& i : userList){
                                 if(nowUser == i.name)
                                       sends = "Account balance : " + to_string(i.balance) + "\n";
                              }
                              int isOnline = 0;
                              for(auto& i : userList){
                                 if(i.isOnline){
                                       sends += i.name + "#" + i.ip + "#" + i.portNum + "\n";
                                       isOnline++;
                                 }        
                              }
                              sends += "online user : " + to_string(isOnline) + "\n";
                              SSL_write(ssl[sock], sends.c_str(), sends.length());
                           }
                           else{
                              sends.clear();
                              sends = "220 AUTH_FAIL";
                              SSL_write(ssl[sock], sends.c_str(), sends.length());
                           }
                        }
                    }
                    else if(strncmp(buffer, "list", 4) == 0){
                        char buffer2[buffer_size];
                        memset(buffer2, 0, buffer_size);
                        string message(buffer);
                        string list = "";
                        pthread_mutex_lock(&locker);
                        nowUser = userSocket[sock];
                        pthread_mutex_unlock(&locker);
                        for(auto& i : userList){
                            if(nowUser == i.name)
                                list = "Account balance : " + to_string(i.balance) + "\n";

                        }
                        int isOnline = 0;
                        for(auto& i : userList){
                            if(i.isOnline){
                                list += i.name + "#" + i.ip + "#" + i.portNum + "\n";
                                isOnline++;  
                            }   
                        }
                        list += "online user :" + to_string(isOnline) + "\n";
                        SSL_write(ssl[sock], list.c_str(), list.length());
                    }
                    else if(strncmp(buffer, "exit", 4) == 0){
                        string send1 = "Goodbye";
                        SSL_write(ssl[sock], send1.c_str(), send1.length());
                        pthread_mutex_lock(&locker);
                        nowUser = userSocket[sock];
                        pthread_mutex_unlock(&locker);
                        for(auto& i : userList){
                            if(nowUser == i.name)
                                i.isOnline = 0;
                        }
                        cerr << nowUser << " is leave."<< endl;
                        pthread_mutex_lock(&locker);
                        userSocket.erase(sock);
                        pthread_mutex_unlock(&locker);
                        FD_CLR(sock, &fd);
                    }
                }      
            }
            else{
                queue.pop_front();
                pthread_mutex_unlock(&Select);
                pthread_mutex_unlock(&locker);
            }
        }
        else{
            pthread_mutex_unlock(&locker);
        }
    }
}

int main(int argc, char const* argv[]){
    if(argc != 2)
        cout << "error argument\n";
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        cout << "socket create failed\n";
    
    SSL_library_init();
    cout << "Initialize SSL library.\n";
    
    ctx = InitServerCTX();
    LoadCertificates(ctx, mycert, mykey);

    memset((char*)&server, 0, sizeof(server));
    cerr << "!" << endl;
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[1]));
    server.sin_addr.s_addr = INADDR_ANY;
    if(bind(sock, (struct sockaddr*)& server, sizeof(server)) < 0)
        cout << "bind failed\n";
    listen(sock, 20);
    socklen_t addrLens = sizeof(client);

    pthread_mutex_init(&locker, NULL);
    pthread_mutex_init(&Select, NULL);
    pthread_t p_threads[threadNum];
    for (size_t i = 0; i < threadNum; ++i)
		pthread_create(&p_threads[i], NULL, &worker, NULL);
    FD_ZERO(&fd);
    FD_SET(sock, &fd);
    FD_ZERO(&active);
    active = fd;
    int maxSocket = sock;
    cerr << sock << endl;
    while(true){
        pthread_mutex_lock(&Select);
        select(maxSocket+1, &active, NULL, NULL, NULL);
        for(int i = 0; i <= maxSocket; ++i){
            if(FD_ISSET(i, &active)){
                int newfd = 0;
                if(i == sock){
                    if((newfd = accept(sock, (struct sockaddr*)& client, &addrLens))){
                        if(newfd < 0)
                            cout << "Can't accept connection.\n";
                        else{
                            ssl[newfd] = SSL_new(ctx);
        					SSL_set_fd(ssl[newfd], newfd);      // set connection socket to SSL state
                            SSL_accept(ssl[newfd]); 
                            string s = "successful connected.";
                            cout << s << endl;
                            SSL_write(ssl[newfd], s.c_str(), s.length());
                            FD_SET(newfd, &fd);   
                        }
                    }
                }
                else{
                    pthread_mutex_lock(&locker);
                    queue.push_back(i);
                    cerr << queue.back() << endl;
                    sleep(1);
                    pthread_mutex_unlock(&locker);
                }
                if(newfd >= maxSocket) { maxSocket = newfd; }
            }
        }
        active = fd;
        pthread_mutex_lock(&locker);
        queue.push_back(-1);
        cerr << queue.back() << endl;
        pthread_mutex_unlock(&locker);
    }
    close(sock);
    SSL_CTX_free(ctx);
    return 0;
}