#include <stdio.h>
#include <errno.h>
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
#include <sstream>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <vector>
#include <fstream>
#include <sys/select.h>
#include <map>
#include <list>
#include <resolv.h>
#include "openssl/ssl.h"
#include "openssl/err.h"

using namespace std;

#define threadNum 3
#define msgLen 10240

char mycert[] = "/Users/shihyunchen/Documents/computer networks/assignment/b05705019_part3/cert.pem";
char mykey[] = "/Users/shihyunchen/Documents/computer network/assignment/b05705019_part3/key.pem";

//Init server instance and context
SSL_CTX* InitServerCTX(void)
{   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc. 
    SSL_load_error_strings();   // load all error messages
    method = TLSv1_server_method();  // create new server-method instance 
    ctx = SSL_CTX_new(method);   // create new context from method
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

//Load the certificate 
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
        fprintf(stderr, "The private key does not match.\n");
        abort();
    }
    
    //New lines - Force the client-side have a certificate
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, 4);
    //End new lines
}

// pthread locks
pthread_mutex_t lock_que;
pthread_mutex_t lock_select;
fd_set fd;
fd_set be_processed;

SSL* ssl[msgLen];
SSL_CTX *ctx;

int sockfd = 0; 

// user information: username, ip address, account balance, port number
struct user_info
{
    string username;
    string ip_address;
    string account_balance;
    string portnum;
    bool online; //1: online, 0: not online
};
list<int> client_que; // store client sockets
vector<user_info> users; // store user information into a vector
struct sockaddr_in serverInfo, clientInfo;
map <int, string> store; // store which client is served

// the core of server
void *run_main(void *data);

int main()
{
	SSL_library_init(); //init SSL library
    printf("Initialize SSL library.\n");
    
    ctx = InitServerCTX();  //initialize SSL 
    LoadCertificates(ctx, mycert, mykey); // load certs and key
    // create socket
    // char inputBuffer[256] = {};
    // char message[] = {"Hi, this is server.\n"};
    int structsize = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        printf("Fail to create a socket.\n");
    }

    socklen_t addrlen = sizeof(clientInfo);
    bzero(&serverInfo, sizeof(serverInfo));
    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(12345);
    if(bind(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) == -1)
    {
        printf("Bind failed.\n");
        return 0;
    }

    // listen: the client_que can contain 20 requests at most
    listen(sockfd, 20);

	/*mutex thread initialization*/
    pthread_mutex_init(&lock_select, NULL); 
	pthread_mutex_init(&lock_que, NULL); 


    // p-thread array
    pthread_t p_threads[threadNum];
    // thread pool
	for (int i = 0; i < threadNum; i++) {
		pthread_create(&p_threads[i], NULL, &run_main, NULL);
    } 
    
    // use select() to check which socket is ready to read or write data
    FD_ZERO(&fd);
    FD_SET(sockfd, &fd);
    FD_ZERO(&be_processed);
    be_processed = fd;
    
    int max_socket = sockfd;
    while(1){
    	pthread_mutex_lock(&lock_select);
		// which fd is ready to be read, write
    	select(max_socket + 1, &be_processed, NULL, NULL, NULL);
    	for(int i = 0; i <= max_socket; i++){
    		if(FD_ISSET(i,&be_processed)){
    			int newsockfd;
    			if(i == sockfd){ // new socket
    				if ((newsockfd = accept(sockfd, (struct sockaddr*) &clientInfo, &addrlen))){
    					if (newsockfd < 0)
        					cerr << "error on accepting a client connection.";
        				else{
        					string success = "\nSuccessfully Connected.\nHi, How can I help you?\n";
							ssl[newfd] = SSL_new(ctx);
        					SSL_set_fd(ssl[newfd], newfd);      // set connection socket to SSL state
        					SSL_accept(ssl[newfd]);  
							SSL_read(ssl[newsockfd], success.c_str(), success.length());
							//if successfully connected, push the socket to the queue 
							FD_SET(newsockfd, &fd);
						}
					}
    			} 
				else{ // socket already exist

    				pthread_mutex_lock(&lock_que);
					// push the existing socket to queue and wait for being processed
    				client_que.push_back(i);
    				// cerr << "client_que.back() " << client_que.back() << endl;
    				sleep(1);
    				pthread_mutex_unlock(&lock_que);
    			}
    			if(max_socket < newsockfd)
					max_socket = newsockfd;
    		}
    	}
    	be_processed = fd;
    	pthread_mutex_lock(&lock_que);
		// prevent select the same fd that has been processed.
    	client_que.push_back(-1);
    	// cerr << "client_que.push_back() " << client_que.back() << endl;
    	pthread_mutex_unlock(&lock_que);
	}
	SSL_CTX_free(ctx);
	close(sockfd);
    return 0;
}

void *run_main(void *data){
    // while true, keep receiving message from client.
    bool flag_bye = 0;
    while(1)
    {
		// the message that received
    	vector<string> msgarr;
		int index_reg = 0;

        string user_serving;
        pthread_mutex_lock(&lock_que);
        if(client_que.size() > 0)
        {
            int sockfd = client_que.front();
            if(sockfd >= 0)
            {
                client_que.pop_front();
                pthread_mutex_unlock(&lock_que);

                // register(REGISTER#12345), login(shih#7777), list(List), exit(Exit)
                char receiveMessage[1024] = {};
                
				if(SSL_read(ssl[sockfd], receiveMessage, sizeof(receiveMessage)) == 0)
				{
					pthread_mutex_lock(&lock_que);
					user_serving = store[sockfd];
					// cerr << user_serving << " has leaved\n";
					pthread_mutex_unlock(&lock_que);
					for(int i = 0; i < users.size(); i++)
					{
						if(user_serving == users[i].username)
							users[i].online = 0;
					}
					FD_CLR(sockfd, &fd);
					close(sockfd);
				}
				// index of the client
				// int index_reg = 0;

				// list
				if(strcmp(receiveMessage, "List\n") == 0) // receiveMessage is only one string
                {
					pthread_mutex_lock(&lock_que);
					user_serving = store[sockfd];
					// cerr << "list user serving " << user_serving << "\n";
					pthread_mutex_unlock(&lock_que);

					int online_num = 0;
					for(int i = 0; i < users.size(); i++)
					{
						if(users[i].online == 1)
							online_num++;
					}
					string listMessage = users[index_reg].account_balance + "Number of accounts online: " + to_string(online_num) + "\n";
					for(int i = 0; i < users.size(); i++)
					{
						if(users[i].online == 1)
							listMessage += users[i].username + "#" + users[i].ip_address + "#" + users[i].portnum;
					}
					SSL_write(ssl[sockfd], listMessage.c_str(), listMessage.length());
                }
				// exit
				if(strcmp(receiveMessage ,"Exit\n") == 0)
				{
					flag_bye = 1;
					char bye[1024] = "Bye\n";
					SSL_write(ssl[sockfd], bye, strlen(bye), 0);
					pthread_mutex_lock(&lock_que);
					user_serving = store[sockfd];
					pthread_mutex_unlock(&lock_que);
					for (int i = 0; i < users.size(); i++){
						if (user_serving == users[i].username)
							users[i].online = 0;
					}
					cout << user_serving << " has leaved." << endl;
					pthread_mutex_lock(&lock_que);
					store.erase(sockfd);
					pthread_mutex_unlock(&lock_que);
					// say bye and clear the fd
					FD_CLR(sockfd, &fd);
				}
				else
				{
					char *message = strtok(receiveMessage, "#");
					int token_num = 0;
					// the first variable stores the first message
					// and the second one stores the second message.
					while (message != NULL) {
						// printf("%s\n", message);
						msgarr.push_back(message);
						message = strtok(NULL, "#");
						token_num++;
					}

					// debug: members of array
					// for(int i = 0; i < token_num; i++)
					// 	cerr << "i " << i << " msgarr[i] " << msgarr[i] << ";";

					if(token_num >= 2) // receiveMessage is seperate into to strings
					{
						// register
						if(msgarr[0] == "REGISTER")
						{
							// had registered before: flag = 1
							// hadn't registered before: flag = 0
							// index_reg record the registered user
							bool flag = 0;
							for(int i = 0; i < users.size(); i++)
							{
								if(users[i].username == msgarr[1])
								{
									flag = 1;
									index_reg = i;
									break;
								}
							}
							if(flag == 0)
							{
								user_info new_user;
								pthread_mutex_lock(&lock_que);
								new_user.username = msgarr[1];
								new_user.online = 0;
								new_user.account_balance = msgarr[2];
								users.push_back(new_user);
								pthread_mutex_unlock(&lock_que);
								char regMessage[1024] = "100 OK\n";
								SSL_write(ssl[sockfd], regMessage, strlen(regMessage));
							}
							else
							{
								cerr << "This account has been registered." << endl;
								char regMessage[1024] = "210 FAIL\n";
								SSL_write(ssl[sockfd], regMessage, strlen(regMessage));
							}
						}
						// to transfer money, server will find whether the port is listened by a client
						else if(msgarr[0] == "MONEY_TRANSFER")
						{
							string payee = atoi(msgarr[2]);
							bool canuse = false;
							for(int i=0;i<users.size();i++){
								if(users[i].username == payee && users[i].online==1){
									string sendback = to_string(users[i].port_num);
									SSL_write (ssl[socketfd], sendback.c_str(), sendback.length());
									canuse = true;
									break;
								}
							}
							if(!canuse){
								string err = "220 AUTH_FAIL";
								SSL_write (ssl[socketfd], err.c_str(), err.length());
							}
						}
						// if can find the exact client for transfering, then can transfer
						else if(msgarr[0] == "transfer")
						{
							recv_message.erase(0,12);
							cerr << recv_message;
							size_t name_pos = recv_message.find("#");
							string user_give =recv_message.substr(0,name_pos);
							recv_message.erase(0,name_pos+1);
							cerr << user_give;
							name_pos = recv_message.find("#");
							string transfer_amou = recv_message.substr(0,name_pos);
							cerr << transfer_amou;
							string user_recv = recv_message.substr(name_pos+1);
							cerr << user_recv;
							bool do_success = false;
							int transfer_amount = stoi(transfer_amou, nullptr, 10);
							for(int i=0;i<users.size();i++)
							{
								if((users[i].username == user_give || users[i].username == user_recv) && users[i].online == 1)
								{
									for(int j=i+1;j<users.size();j++)
									{
										if((users[j].username == user_give || users[j].username == user_recv)&& users[j].online == 1)
										{
											if(users[i].username == user_give)
											{
												if(users[i].balance >= transfer_amount)
												{
													users[i].balance -=transfer_amount;
													users[j].balance +=transfer_amount;
													do_success = true;
													string do_good = "Successfully Transfered";
													SSL_write(ssl[socketfd], do_good.c_str(), do_good.length());
												}
												else
												{
													do_success = true;
													string do_bad = "Transfer failed";
													SSL_write(ssl[socketfd], do_bad.c_str(), do_bad.length());
												}
												
											}
											else
											{
												if(users[j].balance >= transfer_amount)
												{
													users[i].balance +=transfer_amount;
													users[j].balance -=transfer_amount;
													do_success = true;
													string do_good = "Successfully Transfered";
													SSL_write(ssl[socketfd], do_good.c_str(), do_good.length());
												}
												else
												{
													do_success = true;
													string do_bad = "Transfer failed";
													SSL_write(ssl[socketfd], do_bad.c_str(), do_bad.length());
												}
											}
											break;
										}
									}
									break;
								}
							}
							if(!do_success)
							{
								string err = "220 AUTH_FAIL";
								SSL_write(ssl[socketfd], err.c_str(), err.length());
							}
						}
						// login
						else
						{
							// client ip address
							string client_ip(inet_ntoa(clientInfo.sin_addr));
							// had registered before: flag = 1
							// hadn't registered before: flag = 0
							// index_reg record the registered user
							bool flag = 0;
							for(int i = 0; i < users.size(); i++)
							{
								if(users[i].username == msgarr[0])
								{
									flag = 1;
									index_reg = i;
									break;
								}
							}
							// examine whether the port number is used
							bool flag2 = 0;
							for(int i = 0; i < users.size(); i++)
							{
								if(users[i].portnum == msgarr[1] && users[i].online == 1)
								{
									flag2 = 1;
									break;
								}
							}
							if(flag == 1 && flag2 == 0)
							{
								users[index_reg].ip_address = client_ip;
								users[index_reg].portnum = msgarr[1];
								users[index_reg].online = 1;

								pthread_mutex_lock(&lock_que);
								store[sockfd] = msgarr[0]; // msgarr[0] = the name of the client served now 
								pthread_mutex_unlock(&lock_que); 
								string loginMessage;
								// 1. send the account balance & "number of account online\n"
								int online_num = 0;
								for(int i = 0; i < users.size(); i++)
								{
									if(users[i].online == 1)
										online_num++;
								}
								loginMessage = users[index_reg].account_balance + "Number of accounts online: " + to_string(online_num) + "\n";
								// 2. send the list online
								for(int i = 0; i < users.size(); i++)
									loginMessage += users[i].username + "#" + users[i].ip_address + "#" + users[i].portnum;
								SSL_write(ssl[sockfd], loginMessage.c_str(), loginMessage.length());

							}
							else
							{
								char loginMessage[1024] = "220 AUTH_FAIL\n";
								SSL_write(sockfd, loginMessage, strlen(loginMessage));
							}
						}
					}
				}
            }
            else
            {
				// get -1 from client_queue, so that can continue to process the next one.
        		client_que.pop_front();
        		pthread_mutex_unlock(&lock_select);
        		pthread_mutex_unlock(&lock_que);
            }
        }
        else
        {
            pthread_mutex_unlock(&lock_que);
        }
    }
}