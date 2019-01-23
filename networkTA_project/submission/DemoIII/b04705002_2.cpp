//
//  main.cpp
//  socket_program_hw1
//
//  Created by Apple on 2018/11/30.
//  Copyright © 2018年 Apple. All rights reserved.
//

#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <resolv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <map>
#include <stdlib.h> 

#include <list>

using namespace std;

#define BLEN 102400

SSL_CTX *ctx;
SSL_CTX *sctx;
SSL* c_ssl;
SSL* trade_ssl;

SSL* cs_c_ssl;

void *c_to_c(void *);

static int noThread;
pthread_t threadA[1];
static list<string> login_user_ip;
static list<string> login_user_port;
static string p2p_ip;
static string p2p_port;
static string listen_port;
static int main_sock_fd;
static char str[INET_ADDRSTRLEN];

bool send_msg(int sockfd, string msg , SSL* fssl){	//0 for success
	if (SSL_write(fssl, msg.c_str(), msg.size())<0){ // encrypt & send message 
		cout<<"Error: Sending msg."<<endl;
		return 1;
	}
	return 0;
}

string recv_msg(int sockfd , SSL* fssl){
	char buf[BLEN];
	char *bptr=buf;
	int buflen=BLEN;

	memset(buf, '\0', sizeof(buf)); 

	if (SSL_read(fssl, bptr, buflen)<0){  // get reply & decrypt 
		// cout<<"Recieving Error!"<<endl;
	}
    string ans (buf);
	return ans;
}

void get_ip_list(int sock_FD,SSL* c_ssl){
    send_msg(sock_FD, "List", c_ssl);
    //send(sock_FD, send_mes,strlen(send_mes),0);
    
    string res = recv_msg(sock_FD, c_ssl);
    bool stop = true;
    string temp_list = res;
    while (stop){
        string temp = temp_list;
        int index = temp.find("\n");
        string data = temp.substr(0, index);
        int split = data.find("#");
        string ip_port = data.substr(split+1);
        int second = ip_port.find("#");
        string ip = ip_port.substr(0, second);
        string port = ip_port.substr(second+1);
        login_user_ip.push_front(ip);
        login_user_port.push_front(port);

        if (temp.find("#") ==std::string::npos){
            stop = false;
            break;
        }
        else{
            temp_list = temp_list.substr(index+1);
        }
    }
}


SSL_CTX* InitCTX(void){   
    
    const SSL_METHOD *method;
    SSL_CTX *fctx;
 
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = TLSv1_client_method();  /* Create new method instance */
    fctx = SSL_CTX_new(method);   /* Create new context */
    if ( fctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return fctx;
}

SSL_CTX* InitServerCTX(void)
{   
    const SSL_METHOD *method;
    SSL_CTX *fctx;
 
    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc. 
    SSL_load_error_strings();   // load all error messages */
    method = TLSv1_server_method();  // create new server-method instance 
    fctx = SSL_CTX_new(method);   // create new context from method
    if ( fctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }

    return fctx;
}

void LoadCertificates(SSL_CTX* fctx, string CertFile, string KeyFile)
{
    //set the local certificate from CertFile
    if ( SSL_CTX_use_certificate_file(fctx, CertFile.c_str(), SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //set the private key from KeyFile
    if ( SSL_CTX_use_PrivateKey_file(fctx, KeyFile.c_str(), SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //verify private key 
    if ( !SSL_CTX_check_private_key(fctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

int main(int argc, const char * argv[]) {

    
    
    
    char addr [100];
    strcpy(addr, argv[1]);
    int port = atoi(argv[2]);
    // insert code here...
    int sock_FD = 0;

    //ssl initailize
    SSL_library_init();
	ctx = InitCTX();
	sctx = InitServerCTX();
	c_ssl = SSL_new(ctx);



    sock_FD = socket(PF_INET , SOCK_STREAM , 0); //AF_INET : IPv4 , SOCK_STREAM: TCP connection
    
    if (sock_FD == -1){
        printf("Fail to create a socket.");
    }
    else{
        string res = "Client: socket created" + to_string(sock_FD);
        cout<<res<<endl;
    }
    main_sock_fd = sock_FD;
    struct sockaddr_in connect_info;
    bzero(&connect_info,sizeof(connect_info)); //initailize
    connect_info.sin_family = AF_INET;  //AF = Address Family for address ,PF = Protocol Family for socket initialize ,AF_INET = PF_INET
    connect_info.sin_addr.s_addr =  inet_addr(addr); // convert ip address to integer format
    connect_info.sin_port = htons(port);
    int err = connect(sock_FD, (struct sockaddr *) &connect_info,sizeof(connect_info));
    if(err==-1){
        printf("Connection error");
        cout<<endl;
        return 0;
    }
    else{
        SSL_set_fd(c_ssl, sock_FD);      // set connection socket to SSL state
    	if ( SSL_connect(c_ssl) < 0 ){
    		cout<<"SSL connection to host error!!!"<<endl;
    		return 1;
    	}
        cout<< "SSL Successfully connected" <<endl;
    	//finish connection
    }

    // char str[INET_ADDRSTRLEN];
    // inet_ntop(AF_INET, &(connect_info.sin_addr), str, INET_ADDRSTRLEN);
    // printf("%s\n", str); 


    
    //recv(sock_FD,first,sizeof(first),0);
    cout<< recv_msg(sock_FD, c_ssl);
    
    cout<<"Enter 1 for register, 2 for Login: ";
    char decision[10];
    cin>> decision;
    char receiveMessage[2000] ={};
    string user_name ;
    string member_list;
    bool flag = 0;
    bool flag_decision = 0;
    bool login = 0;
    bool deposit = 0;
    noThread = 0;

    



    while(cin){
        if(strcmp(decision,"1") == 0){
            cout<<"Enter the name you want to Register: ";
        }
        else if(strcmp(decision,"2") ==0 ){
            cout<<"Enter your name: ";
        }
        else if (strcmp(decision,"3") ==0 && login){
            cout<<"Enter the number of actions you want to take: \n";
            cout<<"1 for ask the latest list \n";
            cout<<"2 for checking account \n";
            cout<< "3 for deposit \n";
            cout<< "4 for trade mode\n";
            cout<< "5 for trade start\n";
            cout<<"8 for Exit : \n";

        }
        else{
            cout<<"please enter 1 for register, 2 for login: ";
            cin>> decision;
            continue;
        }
        // int start = 0, end =0;
        
        char message[200] = {};
        bzero(&message,sizeof(message)); //initailize
        cin>>message;
        char * send_mes ;
        bzero(&send_mes,sizeof(send_mes));
        if (strcmp(decision,"1") ==0 ){
            char pre[] = "REGISTER#";
            send_mes = (char *) malloc(strlen(pre)+ strlen(message) );
            strcpy(send_mes, pre);
            strcat(send_mes, message);
            strcpy(decision, "100");
        }
        else if(strcmp(decision,"2") ==0 ){
            char port[20] = {};
            cout<< "Enter your port number: ";
            cin>> port;
            string a(port);
            listen_port = a;
            bool is_digit = 0;
            for(int i=0;i<strlen(port);i++){
                if (isdigit(port[i])==0){
                    is_digit =1;
                }
            }
            while (is_digit){
                cout<<"port number should be number!!! ";
                cout<< "Please enter your port number again: ";
                cin>> port;
                is_digit = 0;
                for(int i=0;i<strlen(port);i++){
                    if (isdigit(port[i])==0){
                        is_digit =1;
                    }
                }
            }
            send_mes = (char *) malloc(2+strlen(message) + strlen(port) );
            strcpy(send_mes, message);
            strcat(send_mes, "#");
            strcat(send_mes, port);
        }
        else if(strcmp(decision,"3") ==0 ){
            send_mes = (char *) malloc(1000);
            if (strcmp(message,"1") ==0){
                strcpy(send_mes, "List");
            }
            else if (strcmp(message,"8") ==0){
                strcpy(send_mes, "Exit");
            }
            else if (strcmp(message, "2") == 0){
                strcpy(send_mes, "Check_account");
            }
            else if (strcmp(message, "3") == 0){
                strcpy(send_mes, "Depo");
                deposit = true;
            }
            else if (strcmp(message,"4") ==0 ){
                
                SSL* cs_ssl;
                SSL_CTX* cs_ctx;
                // SSL_library_init(); //init SSL library
                cs_ctx = InitServerCTX();  //initialize SSL 
                LoadCertificates(cs_ctx, "mycert.pem", "mykey.pem"); // load certs and key


                int c_server_FD = 0 ;
                c_server_FD = socket(PF_INET , SOCK_STREAM , 0);
                if (c_server_FD == -1){
                    printf("Fail to create a socket.");
                }
                else{
                    string res = "Server: socket created for p2p " + to_string(c_server_FD);
                    cout<< res << endl;
                }
                //socket的連線
                struct sockaddr_in serverInfo,clientInfo;
                socklen_t addrlen = sizeof(clientInfo);
                serverInfo.sin_family = AF_INET;
                serverInfo.sin_addr.s_addr = INADDR_ANY;
                serverInfo.sin_port = htons(atoi(  (listen_port.c_str()) ));
                bind(c_server_FD,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
                listen(c_server_FD, 10);

                cs_ssl = SSL_new(cs_ctx);              // get new SSL state with context 
                SSL_set_fd(cs_ssl, c_server_FD); 

                int conn_FD = -100;
                
            
                cout<<noThread<<endl;
                cout << "Listening No."<< noThread << endl;
                //this is where client connects. svr will hang in this mode until client conn
                
                bool flag = false;
                
                
                conn_FD = accept(c_server_FD ,(struct sockaddr*) &clientInfo, &addrlen);
                
                if (conn_FD < 0)
                {
                    cerr << "Cannot accept connection" << endl;
                    return 0;
                }
                else
                {   
                    //SSL_set_fd(c_ssl, conn_FD);      // set connection socket to SSL state
                    //ssl initailize
                    cout << "Connection successful" << endl;
                    
                    
                }
            
                recv(conn_FD,receiveMessage,sizeof(receiveMessage),0);
                string temp_rec(receiveMessage);
                //cout<<temp_rec<<endl;
                //temp_rec = recv_msg(conn_FD, c_ssl);
                //cout<<temp_rec;
                send_msg(main_sock_fd, "Depo", c_ssl);
                string res = recv_msg(main_sock_fd, c_ssl);
                cout<<res;
                send_msg(main_sock_fd, temp_rec.c_str(), c_ssl);
                //close(conn_FD);
                    
                
                    
            
            }
            

            else if(strcmp(message,"5") ==0 && noThread <= 1){
                get_ip_list(sock_FD, c_ssl);
                cout<<"who do you want to trade with?";
                list<string>::iterator it = login_user_ip.begin();
                list<string>::iterator it_two = login_user_port.begin();
                for(int i=0;i<login_user_ip.size();i++){
                    cout<<*it;
                    cout<<" port: ";
                    cout<<*it_two<<endl;
                    advance(it_two, 1);
                    advance(it, 1);
                }
                string cin_ip;
                cin>>cin_ip;
                p2p_ip = cin_ip;
                string cin_port;
                cin>>cin_port;
                p2p_port = cin_port;
                pthread_create(&threadA[noThread], NULL, c_to_c, NULL); 
                noThread ++;
                continue;
            }
            else{
                continue;
            }
        }
        
        else{
            cout<<"Please enter once (1 for register, 2 for Login: )";
            continue;
        }
        send_msg(sock_FD, send_mes, c_ssl);
        //send(sock_FD, send_mes,strlen(send_mes),0);
        bzero(&receiveMessage,sizeof(receiveMessage)); //initailize
        
        
        //recv(sock_FD,receiveMessage,sizeof(receiveMessage),0);
        //cout<<receiveMessage;
        string res = recv_msg(sock_FD, c_ssl);
        cout<<res;
        if(res == "closing"){
            close(sock_FD);
            break;
        }
        if (deposit == true){
            cout<<"How much do you want to deposit? :";
            char amount[200] = {};
            cin>>amount;
            send_msg(sock_FD, amount, c_ssl);
            //send(sock_FD, amount,sizeof(amount),0);
            deposit = false;
        }
        
        
        string temp = send_mes;

        if(res.substr(0, 6) == "100 OK"){
            continue;
        }
        else if (res.substr(0,3) == "210"){
            cout<<"error while register"<<endl;
            continue;
        }
        else if (res.substr(0,3) == "220"){
            cout<<"error happened while log in, please make sure you registed and try again"<<endl;
            cout<<"Enter 1 for register, 2 for Login: ";
            cin>>decision;
            continue;
        }
        else{
            if (login ==0){
            bzero(&receiveMessage,sizeof(receiveMessage)); //initailize
            //recv(sock_FD,receiveMessage,sizeof(receiveMessage),0);
            cout<<recv_msg(sock_FD, c_ssl);
            bzero(&receiveMessage,sizeof(receiveMessage)); //initailize
            strcpy(decision, "3");
            flag =0;
            login = 1;

            
            }
        }
        
    }
    
    
    printf("close Socket\n");
    
    return 0;
}


void *c_to_c (void *dummyPt)
{   
    cout<<p2p_ip<<" "<<p2p_port;
    struct sockaddr_in connect_info;
    bzero(&connect_info,sizeof(connect_info)); //initailize
    connect_info.sin_family = AF_INET;  //AF = Address Family for address ,PF = Protocol Family for socket initialize ,AF_INET = PF_INET
    connect_info.sin_addr.s_addr =  inet_addr(p2p_ip.c_str()); // convert ip address to integer format
    connect_info.sin_port = htons(atoi(p2p_port.c_str()));
    
    int sock_FD=0;
    sock_FD = socket(PF_INET , SOCK_STREAM , 0); //AF_INET : IPv4 , SOCK_STREAM: TCP connection
    
    SSL_CTX * trade_sctx;
    SSL_CTX * trade_ctx;
    // //ssl initailize
    // SSL_library_init();
	trade_ctx = InitCTX();
	trade_sctx = InitServerCTX();
	trade_ssl = SSL_new(trade_ctx);

    int err = connect(sock_FD, (struct sockaddr *) &connect_info,sizeof(connect_info));
    if(err==-1){
        printf("Connection error");
        cout<<endl;
    }
    else{
        // SSL_set_fd(trade_ssl  , sock_FD);      // set connection socket to SSL state

        // if ( SSL_accept(trade_ssl ) < 0){ //do SSL-protocol accept
        //     cout<<"SSL Acception Failed!"<<endl;
        //     SSL_free(trade_ssl  );
            
        // }
        cout<< "SSL Successfully connected" <<endl;
    	//finish connection
    }
    cout<<"How much do you want to trade ?\n";
    char  amount[2000] ={};
    char pay [2000]="-" ;
    cin>> amount;
    strcpy(amount, "1000");
    strcat(pay, amount);
    send(sock_FD, amount,strlen(amount),0);
    // //send_msg(sock_FD, "1000",trade_ssl  );
    // strcpy(pay ,to_string(-1 * atoi(amount)).c_str());
    // cout<<pay;
    send_msg(main_sock_fd, "Depo", c_ssl );
    string res = recv_msg(main_sock_fd, c_ssl );
    send_msg(main_sock_fd, pay, c_ssl  );
    
    cout << "\nClosing thread and connection" << endl;
    close(sock_FD);
    noThread --;
    pthread_exit(0);
    return 0;
}