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

#define MAX_BUF 10240
#define thread_handler 5
#define FAIL    -1
void error(string message){
	cout << message << endl;
} // if error appears, show the message


struct user {
	string username;
	string ip; //store the ip address
	int port_num;
	int balance;
	bool online; //1:yes, 0:no
};


//deal with server certificate
SSL_CTX* InitServerCTX(void)
{   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc. 
    SSL_load_error_strings();   // load all error messages */
    method = TLSv1_server_method();  // create new server-method instance 
    ctx = SSL_CTX_new(method);   // create new context from method
    if (ctx == NULL)
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
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
    
    //New lines - Force the client-side have a certificate
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, 4);
    //End new lines
}


/*pthread lock*/
pthread_mutex_t lock_que;
pthread_mutex_t for_select;

vector <user> table; // table: the vector of the users
int is_online = 0; //count how many client there it is

/*use a vector to implement list of request client*/
list<int>Queue;
/*dest:server client_addr: client address*/
struct sockaddr_in dest, client_addr; 
map <int, string> store;
fd_set fd;
fd_set be_action;
SSL* ssl[MAX_BUF];
SSL_CTX *ctx;
void* worker (void * data){ // worker function is what the thread take the action in the program
    while(true) {
        pthread_mutex_lock(&lock_que);
        /* lock there because the queue size may be changed by other thread*/
        if(Queue.size() > 0) {
            int socketfd = Queue.front();
        	 //eliminate the first element
            if(socketfd >= 0){
            	Queue.pop_front();
            	pthread_mutex_unlock(&lock_que);
            	char local_buffer[MAX_BUF]; 
	            string the_user; /*the_user is the variable to store which user is being served*/
	            memset(local_buffer, 0, MAX_BUF);
	            if(SSL_read(ssl[socketfd], local_buffer, MAX_BUF)==0){
	            	error("no action and leave");
				    pthread_mutex_lock(&lock_que);
	       			the_user = store[socketfd];
	            	pthread_mutex_unlock(&lock_que);
				    for (int i=0; i<table.size(); i++){
				        if (the_user == table[i].username)
				       	    table[i].online = 0;
				    }
	            	FD_CLR(socketfd,&fd);
	            	close(socketfd);
	            }else{
	            	/*switch the character array to a string to take actions below*/
	       			string recv_message(local_buffer); 
	       			bool exist = false;
	       			if(strncmp(local_buffer, "REGISTER", 8) == 0){
       					cerr << recv_message << endl;
       					string compare; /*to compare the user name for register have been used or not.*/
       					bool newuser=true; /*to make sure if it register successfully*/
       					size_t name_pos = recv_message.find("#");
       					recv_message.erase(0,name_pos+1);
       					name_pos = recv_message.find("#");
       					compare = recv_message.substr(0,name_pos); 
       					for(int i=0;i<table.size();i++){
       						if(table[i].username == compare){ //same account name has been registered
       							string back = "210 FAIL\n";
								cerr << back << endl;
        		    	    	cerr << "The account has been registered."<<endl;
								SSL_write(ssl[socketfd], back.c_str(), back.length());
								newuser = false;
								break;
       						}
       					}
       					string user_balance_line = recv_message.substr(name_pos+1);
       					if(newuser){
       						user u;
       						pthread_mutex_lock(&lock_que);
       						u.username = compare;
        		    		u.balance = stoi(user_balance_line, nullptr, 10); //new_change
        		    		table.push_back(u);
							u.online = 0;
							pthread_mutex_unlock(&lock_que);
							string back = "100 OK \n";
							cerr << back;
                			cerr << "Registeration complete."<<endl;
							SSL_write (ssl[socketfd], back.c_str(), back.length());
       					}
       				}else if(recv_message.find("#", 0) != -1){
       					cerr << recv_message << endl;
       					if(strncmp(local_buffer, "MONEY_TRANSFER", 14) == 0){
       						size_t name_pos = recv_message.find("#");
       						string port_test = recv_message.substr(name_pos+1);
       						int port_use = stoi(port_test, nullptr, 10);
       						bool canuse = false;
       						for(int i=0;i<table.size();i++){
								if(table[i].port_num == port_use && table[i].online==1){
									string sendback = table[i].username;
									SSL_write (ssl[socketfd], sendback.c_str(), sendback.length());
									canuse = true;
									break;
								}
							}
							if(!canuse){
								string err = "220 AUTH_FAIL";
								SSL_write (ssl[socketfd], err.c_str(), err.length());
							}
       					}else if(strncmp(local_buffer, "DO_transfer", 11) == 0){
       						recv_message.erase(0,12);
       						cerr<<recv_message<<endl;
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
       						for(int i=0;i<table.size();i++){
								if((table[i].username == user_give || table[i].username == user_recv) && table[i].online == 1){
									for(int j=i+1;j<table.size();j++){
										if((table[j].username == user_give || table[j].username == user_recv)&& table[j].online == 1){
											if(table[i].username == user_give){
												if(table[i].balance >= transfer_amount){
													table[i].balance -= transfer_amount;
													table[j].balance += transfer_amount;
													do_success = true;
													string do_good = "Transfer_success";
													SSL_write (ssl[socketfd], do_good.c_str(), do_good.length());
												}else{
													do_success = true;
													string do_bad = "Transfer_fail";
													SSL_write (ssl[socketfd], do_bad.c_str(), do_bad.length());
												}
												
											}else{
												if(table[j].balance >= transfer_amount){
													table[i].balance +=transfer_amount;
													table[j].balance -=transfer_amount;
													do_success = true;
													string do_good = "Transfer_success";
													SSL_write (ssl[socketfd], do_good.c_str(), do_good.length());
												}else{
													do_success = true;
													string do_bad = "Transfer_fail";
													SSL_write (ssl[socketfd], do_bad.c_str(), do_bad.length());
												}
											}
											break;
										}
									}
									break;
								}
							}
							if(!do_success){
								string err = "220 AUTH_FAIL";
								SSL_write (ssl[socketfd], err.c_str(), err.length());
							}

       					}else{
       						size_t name_pos = recv_message.find("#");
       						string user_name = recv_message.substr(0, name_pos);
                    		string porttonum = recv_message.substr(name_pos+1);
                    		string client_ip(inet_ntoa(client_addr.sin_addr));
							the_user = user_name;
							bool sameport = false; // true is occupied false is not occupied
							/*check port*/
							int portnum = stoi(porttonum, nullptr, 10);
							for(int i=0;i<table.size();i++){
								if(table[i].port_num == portnum && table[i].online==1){
									sameport = true; //fail
									break;
								}
							}
							for(int i=0;i<table.size();i++){
								if(table[i].username == the_user && !sameport && (table[i].online == 0)){
									table[i].ip = client_ip;
									table[i].online = 1;
									table[i].port_num = portnum; //new change
									table[i].ip = client_ip;
									pthread_mutex_lock(&lock_que);
	            					store[socketfd] = the_user;
	            					pthread_mutex_unlock(&lock_que); 
									exist = true;
								}
							}
						
							if(exist){
								string back;
								for(int i=0;i<table.size();i++){
									if(the_user == table[i].username){
										back = "Account Balance:" + to_string(table[i].balance) + "\n";
									}
								}
								is_online=0;
								for(int i=0;i<table.size();i++){
									if(table[i].online == 1){
										is_online++;
									}
								}
								back += "Number of users:" + to_string(is_online) + "\n";
								for (int j=0; j<table.size(); j++){
									if (table[j].online==1){
										back += table[j].username + "#" + table[j].ip + "#" + to_string(table[j].port_num)+ "\n";
									}
								}
								SSL_write (ssl[socketfd], back.c_str(), back.length());
							}else{
							
							}
						}	
	       			}else if(strncmp(local_buffer, "List", 4) == 0){
	       				char buff[MAX_BUF];
						memset(buff, 0, MAX_BUF);
	       				string recv_message(buff);
	       				cerr << recv_message;	string err = "220 AUTH_FAIL";
								SSL_write(ssl[socketfd], err.c_str(), err.length());
	       				string list_now;
	       				list_now="";
	       				pthread_mutex_lock(&lock_que);
	       				the_user = store[socketfd];
	       				cerr << the_user;
	            		pthread_mutex_unlock(&lock_que);
	                    for(int i=0;i<table.size();i++){
							if(the_user == table[i].username){
								list_now = "Account Balance:" + to_string(table[i].balance) + "\n";
							}
						}
						is_online=0;
						for(int i=0;i<table.size();i++){
							if(table[i].online == 1){
								is_online++;
							}
						}
						list_now += "Number of users:" + to_string(is_online) + "\n";
						for (int j=0; j<table.size(); j++){
							if (table[j].online == 1){
								list_now += table[j].username + "#" + table[j].ip + "#" + to_string(table[j].port_num)+"\n";
							}
						}
						SSL_write (ssl[socketfd], list_now.c_str(), list_now.length());
	       			}else if(strncmp(local_buffer, "Exit", 4) == 0){
	       				string bye = "Bye";
						SSL_write(ssl[socketfd], bye.c_str(), bye.length());
				        pthread_mutex_lock(&lock_que);
	       				the_user = store[socketfd];
	            		pthread_mutex_unlock(&lock_que);
				       	for (int i=0; i<table.size(); i++){
				           	if (the_user == table[i].username)
				                table[i].online = 0;
				        }
				        cerr << the_user << " leave."<<endl;
				        pthread_mutex_lock(&lock_que);
	       				store.erase(socketfd);
	            		pthread_mutex_unlock(&lock_que);
	            		FD_CLR(socketfd,&fd);
	       			}
	            }
	            
	            
        	}else{
        		Queue.pop_front();
        		pthread_mutex_unlock(&for_select);
        		pthread_mutex_unlock(&lock_que);
        	}
            
		}else {
        	pthread_mutex_unlock(&lock_que);
        }
	}
}

  
int main(int argc, char *argv[]){
	SSL_library_init(); //init SSL library
	if (argc != 2){
		printf("Usage: %s <portnum>\n", argv[0]);
        exit(0);
	}
    int portnum = 8899; // portnum is the port number that we give to the 執行檔
    
 	
    printf("Initialize SSL library.\n");
    std::string str1 = "mycert.pem";
    std::string str2 = "mykey.pem";
    char *a = new char[str1.length()+1];
    char *b = new char[str2.length()+1];
    strcpy(a,str1.c_str());
    strcpy(b,str2.c_str());
    ctx = InitServerCTX();  //initialize SSL 
    LoadCertificates(ctx, a, b); // load certs and key

    /*create a socket*/
	int socketfd = socket(AF_INET, SOCK_STREAM, 0); // create a descriptor
	if(socketfd < 0)
		error("Fail to creaate a socket."); // if socket's descriptor doesn't open successfully, print out the message

	/*setup a server*/
	memset((char *) &dest, 0, sizeof(dest));
	dest.sin_family = AF_INET; //address family
	dest.sin_port = htons(portnum); //port number of server
	dest.sin_addr.s_addr = INADDR_ANY;
    bind(socketfd, (struct sockaddr*) &dest, sizeof(dest)); 
    //binding 
    listen(socketfd, 20);//listen
    cout << "Listening on port number:"<<portnum<<endl; // we listen to a port
    
    /*accept the connection*/
	socklen_t addrlen = sizeof(client_addr); 

	/*thread declaration*/
	pthread_mutex_init(&lock_que, NULL); 
	// pthread lock initialization. 
    pthread_t p_threads[thread_handler]; // create the p-thread array

    /* create thread pool*/
	for (int i=0; i<thread_handler; i++) {
		pthread_create(&p_threads[i], NULL, &worker, NULL);
    }
    FD_ZERO(&fd);
    FD_SET(socketfd, &fd);

    FD_ZERO(&be_action);
    be_action = fd;

    int max_socket = socketfd;

    while(true){
    	pthread_mutex_lock(&for_select);
    	select(max_socket+1,&be_action, NULL, NULL, NULL);
    	for(int i=0; i <= max_socket;i++){
    		if(FD_ISSET(i,&be_action)){
    			int newfd;
    			if(i == socketfd){
    				if ((newfd = accept(socketfd, (struct sockaddr*) &client_addr, &addrlen))){
    					if (newfd < 0)
        					error("ERROR on accept a client connection.");
        				else{
        					ssl[newfd] = SSL_new(ctx);
        					SSL_set_fd(ssl[newfd], newfd);      // set connection socket to SSL state
        					SSL_accept(ssl[newfd]);  
        					string success="Successful Connection.";
							//send the message to client that it connects successfully
							SSL_write (ssl[newfd], success.c_str(), success.length());
							//push the socket to the queue 
							FD_SET(newfd, &fd);           
						}
					}
    			} else {
    				pthread_mutex_lock(&lock_que);
    				Queue.push_back(i);
    				cerr << Queue.back()<<endl;
    				sleep(1);
    				pthread_mutex_unlock(&lock_que);
    			}
    			if(max_socket < newfd)
					max_socket = newfd;
    		}
    	}
    	be_action = fd;
    	pthread_mutex_lock(&lock_que);
    	Queue.push_back(-1);
    	cerr << Queue.back()<<endl;
    	pthread_mutex_unlock(&lock_que);
	}
    

    
	close(socketfd); //close the socket of server
    SSL_CTX_free(ctx);         // release context
    return 0;
} 
