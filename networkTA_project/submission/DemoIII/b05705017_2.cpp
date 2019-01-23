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
#include <arpa/inet.h> 
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <resolv.h>
#include <list>
#include "/opt/local/include/openssl/ssl.h"
#include "/opt/local/include/openssl/err.h"

using namespace std;

#define buffer_size 1024
pthread_mutex_t locker;
list<int> queue;

void* other_sockets(void* data);
int sock;
int portNum;
char mycert[] = "/Users/billhuang/NTU/junior/網路/socket/b05705017_part3/mycert.pem"; 
char mykey[] = "/Users/billhuang/NTU/junior/網路/socket/b05705017_part3/mykey.pem";
SSL* ssl;

SSL_CTX* InitCTX(void){   
    const SSL_METHOD* method;
    SSL_CTX* ctx;

    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = TLSv1_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if(ctx == NULL){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

void ShowCerts(SSL* ssl){   
    X509 *cert;
    char *line;
 
    cert = SSL_get_peer_certificate(ssl); // get the server's certificate
    if ( cert != NULL ){
        printf("---------------------------------------------\n");
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       // free the malloc'ed string 
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);\
        free(line);       // free the malloc'ed string 
        X509_free(cert);     // free the malloc'ed certificate copy */
        printf("---------------------------------------------\n");
    }
    else
        printf("No certificates.\n");
}

SSL_CTX* InitServerCTX(void){   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc. 
    SSL_load_error_strings();   // load all error messages */
    method = TLSv1_server_method();  // create new server-method instance 
    ctx = SSL_CTX_new(method);   // create new context from method
    if(ctx == NULL){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile){
    //set the local certificate from CertFile
    if(SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0){
        ERR_print_errors_fp(stderr);
        abort();
    }
    //set the private key from KeyFile
    if(SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0){
        ERR_print_errors_fp(stderr);
        abort();
    }
    //verify private key 
    if(!SSL_CTX_check_private_key(ctx)){
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

void LoadCertificates_server(SSL_CTX* ctx, char* CertFile, char* KeyFile){
    //New lines
    if(SSL_CTX_load_verify_locations(ctx, CertFile, KeyFile) != 1)
        ERR_print_errors_fp(stderr);
    
    if(SSL_CTX_set_default_verify_paths(ctx) != 1)
        ERR_print_errors_fp(stderr);
    //End new lines
    
    /* set the local certificate from CertFile */
    if(SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0){
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if(SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0){
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if(!SSL_CTX_check_private_key(ctx)){
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
    
    //New lines - Force the client-side have a certificate
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, 4);
    //End new lines
}

int main(int argc, char const* argv[]){
    if(argc != 3){
        cout << "\n error argument\n";
        return -1; 
    }
    struct sockaddr_in server;
    int sock = 0;
    
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cout << "socket create failed\n";
    }
    // need modify

    struct hostent* serv;
    serv = gethostbyname(argv[1]);

    memset(&server, '0', sizeof(server));
    server.sin_family = AF_INET;
    memmove((char*)&server.sin_addr.s_addr, (char*)serv->h_addr, serv->h_length);
    server.sin_port = htons(atoi(argv[2]));

    // end need modify

    if(connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0){
        close(sock);
        abort();
    }

    SSL_CTX* ctx;
    SSL_library_init();
    ctx = InitCTX();
    LoadCertificates(ctx, mycert, mykey);
    char buffer[buffer_size];
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    if(SSL_connect(ssl) == -1) { ERR_print_errors_fp(stderr); }
    else{
        ShowCerts(ssl);
        memset(buffer, 0, buffer_size);
        SSL_read(ssl, buffer, buffer_size);
        
        cout << buffer << endl;
        cout << "Enter 1 for register, 2 for login : ";
        string option;
        //int portNum;
        cin >> option;
        string name, balance;
        //bool login = false;
        
        bool login = false;
        while(!login){
            if(option == "1"){
                cout << "\nEnter your name and initial balance (Please saparate them by a space.) : ";
                cin >> name >> balance;
                string r = "REGISTER#" + name + "#" + balance;
                SSL_write(ssl, r.c_str(), r.length());
                SSL_read(ssl, buffer, buffer_size);
                cout << buffer;
                // if(buffer[0] == '2')
                //     cout << "\nPlease register again.\n";
                //bzero(buffer, 128);
                cout << "Enter 1 for register, 2 for login : ";
                cin >> option;
            }
            else if(option == "2"){
                cout << "\nEnter your name : ";
                cin >> name;
                cout << "Enter port number : ";
                cin >> portNum;
                string r = name + "#" + to_string(portNum);
                SSL_write(ssl, r.c_str(), r.length());
                //cout << r << endl;
                //memset(buffer, 0, buffer_size);
                SSL_read(ssl, buffer, buffer_size);
                cout << buffer;
                if(buffer[0] == '2'){
                    cout << "\nPlease try again.";
                    cout << "\nEnter 1 for register, 2 for login : ";
                    cin >> option;
                }
                else { login = true; }  
            }
            else{
                cout << "Invalid input. Please try again.\n";
                cout << "Enter 1 for register, 2 for login : ";
                cin >> option;
            }
        }
        pthread_t p;
        pthread_create(&p, NULL, &other_sockets, NULL);

        cout << " login successfully.\n";
        sleep(1);
        bool exitt = false;
        while(!exitt){
            cout << "Enter 1 to ask for online users list, 8 to exit, 6 to transfer: ";
            cin >> option;
            if(option == "1"){
                SSL_write(ssl, "list", 4);
                //bzero(buffer, 128); // memset()
                memset(buffer, 0, buffer_size);
                SSL_read(ssl, buffer, buffer_size);
                cout << buffer << endl;
                //bzero(buffer, 128);
            }
            else if(option == "8"){
                SSL_write(ssl, "exit", 4);
                memset(buffer, 0, buffer_size);
                SSL_read(ssl, buffer, buffer_size);
                cout << buffer << endl;
                exitt = true;
            }
            else if(option == "6"){
                int sock_trans;
                struct sockaddr_in server_c;
                char buffer_c[buffer_size];
                string receiver;
                cout << "Please enter the receiver: ";
                cin >> receiver;
                bool b = false;
                while(!b){
                    string send = "TRANSFERTO#" + receiver;
                    SSL_write(ssl, send.c_str(), send.length());
                    memset(buffer_c, 0, buffer_size);
                    SSL_read(ssl, buffer_c, buffer_size);
                    if(buffer_c[1]=='2'&&buffer_c[2]=='0'){ 
                        cout << "user not online. please enter another receiver: "; 
                        cin >> receiver;
                    }
                    else { b = true; } 
                }
                cout << "Please enter the amount: ";
                string a;
                cin >> a;

                string receiver_name(buffer_c);
                portNum = stoi(receiver_name); 
                sock_trans = socket(PF_INET, SOCK_STREAM, 0);
                if(sock_trans < 0){
                    perror("can't opening socket");
                    exit(1);
                }
                bzero((char *) &server_c, sizeof(server_c));
                server_c.sin_family = AF_INET;
                server_c.sin_addr.s_addr = inet_addr("127.0.0.1");
                server_c.sin_port = htons(portNum);
                
                if(connect(sock_trans, (struct sockaddr*)&server_c, sizeof(server_c)) < 0){
                    perror("can't connect");
                    exit(1);
                }
                
                SSL_CTX* c;
                SSL* s;
                c = InitCTX();
                LoadCertificates(c, mycert, mykey);
                s = SSL_new(c);
                SSL_set_fd(s, sock_trans);
                if (SSL_connect(s) == -1)
    			    ERR_print_errors_fp(stderr);
                else{
                    cout<<"Connected to port:"<<portNum; //show the cipher alogrithm
       		        printf(" Connected with %s encryption\n", SSL_get_cipher(s)); //show the cipher alogrithm
                    ShowCerts(s);
                    string send1 = name + "#" + a + "#" + receiver;
                    //cerr << send1 << endl;
                    SSL_write (s, send1.c_str(), send1.length());
                    memset(buffer_c, 0, buffer_size);
                    SSL_read (s, buffer_c, buffer_size);
                    cout << buffer_c << endl;
                    SSL_free(s);
                }
                close(sock_trans);
                SSL_CTX_free(c);
            }
            else { cout << "Invalid input. Please try again.\n"; }
        }
        SSL_free(ssl);
    }
    close(sock);
    SSL_CTX_free(ctx);	
    return 0;
        
}

void* other_sockets(void* data){
    int sock, clientfd, clientLEN;
    struct sockaddr_in serv_addr, cli_addr;
    struct sockaddr_in addr;
    socklen_t addr_len;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock < 0){
        perror("can't opening socket");
        exit(1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;      // ...
    
    serv_addr.sin_port = htons(portNum);
    //cout << portNum << endl;
    if(bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("can't on binding");
    	exit(1);
    }
    SSL_CTX* c2;
    SSL_library_init();
    cout << "Initialize SSL library.\n";;
    SSL* s2;

    int sock1;
    listen(sock, 3);
    //cout << "port number: " << portNum << endl;
    while((sock1 = accept(sock, (struct sockaddr *) &addr, &addr_len))){
        if(sock1 >= 0){
            pthread_mutex_lock(&locker);
            c2 = InitServerCTX();
            LoadCertificates_server(c2, mycert, mykey); 
            s2 = SSL_new(c2);
            SSL_set_fd(s2, sock1);
            SSL_accept(s2);

            char buffer[buffer_size];
            cout << "\n transfer message: ";
            memset(buffer, 0, buffer_size);
            SSL_read(s2, buffer, buffer_size);
            cout << buffer << endl;
            string receive_message(buffer);
            string transfer = "TRANS#" + receive_message;

            SSL_write(ssl, transfer.c_str(), transfer.length());
            bzero(buffer, buffer_size);
            SSL_read(ssl, buffer, buffer_size);
            cout << buffer << endl;
            SSL_write(s2, buffer, buffer_size);
            cerr << "Enter 1 to ask for online users list, 8 to exit, 6 to transfer: ";
        }
    }
    close(sock1);
    SSL_CTX_free(c2); 
}