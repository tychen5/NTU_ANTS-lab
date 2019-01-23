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
#include <openssl/ssl.h>
#include <openssl/err.h>

using namespace std;

#define MAXBUF 1024

int register_request(int argc , char *argv[]);
int login_request(int argc , char *argv[]);
void *c2c_server(void *arg);//payee is server
void *c2c_client(void *arg);//payer is client
void *c2s(void *arg);
void ShowCerts(SSL * ssl);

char name[1024];
char payee_name[1024];
int pay_status = 0;
char c2s_msg[1024];
int change_data = 0;
char *server_ip;
int server_port;
char *ip;
int port;
struct sockaddr_in clientInfo;
int len = 0;
SSL_CTX *ctx;
SSL *ssl;

int main(int argc , char *argv[])
{
    //ssl init
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(SSLv23_client_method());

    cout << "Welcome to the P2P Micropayment System!\n";

    int sockfd = 0;
    while(1)
    {
        cout << "Type 1 to register or 2 to login: ";
        int action = 0;
        cin >> action;
        if(action == 1)
        {
            sockfd = register_request(argc, argv);
            if (sockfd == 0) { return 0; }
            else { break; }
        }
        else if(action == 2)
        {
            sockfd = login_request(argc, argv);
            if (sockfd == 0) { return 0; }
            else { break; }
        }
        else
        {
            cout << "I don't understand...\n";
        }
    }

    pthread_t c2c_s;
    pthread_create(&c2c_s, NULL, &c2c_server, NULL);

    while(1){
        cout << "Type 3 to get list, 4 to pay, or 5 to exit: ";
        int action = 0;
        cin >> action;
        if(action == 3)
        {
            char msg_sent[1024];
            strcpy(msg_sent, "List");
            len = SSL_write(ssl, msg_sent, strlen(msg_sent));
            memset(msg_sent , 0 , sizeof(msg_sent));
            if (len < 0) { cout << "Error" << endl; }

            char msg_recieved[1024];
            memset(msg_recieved , 0 , sizeof(msg_recieved));
            len = SSL_read(ssl, msg_recieved, MAXBUF);
            cout << msg_recieved << endl;
        }
        else if(action == 4)
        {
            //request to pay
            char msg_sent[1024];
            strcpy(msg_sent, "Pay#");
            cout << "Payee account name: ";
            cin >> payee_name;
            strcat(msg_sent, payee_name);
            len = SSL_write(ssl, msg_sent, strlen(msg_sent));
            memset(msg_sent , 0 , sizeof(msg_sent));
            if (len < 0) { cout << "Error" << endl; }

            char msg_recieved[1024];
            memset(msg_recieved , 0 , sizeof(msg_recieved));
            len = SSL_read(ssl, msg_recieved, MAXBUF);
            cout << msg_recieved << endl;

            char const delim[2] = "#";
            char *token;
            token = strtok(msg_recieved, delim);
            if (strcmp(token, "On") == 0)
            {
                char *token2;
                token2 = strtok(NULL, delim);
                strcpy(server_ip, token2);
                char *token3;
                token3 = strtok(NULL, delim);
                server_port = atoi(token3);

                pthread_t c2c_c;
                pthread_create(&c2c_c, NULL, &c2c_client, NULL);
                pthread_join(c2c_c, NULL);
            }
            else
            {
                cout << payee_name << "is not online" << endl;
            }
        }
        else if(action == 5)
        {
            char msg_sent[1024];
            strcpy(msg_sent, "Exit");
            len = SSL_write(ssl, msg_sent, strlen(msg_sent));
            memset(msg_sent , 0 , sizeof(msg_sent));
            if (len < 0) { cout << "Error" << endl; }

            char msg_recieved[1024];
            memset(msg_recieved , 0 , sizeof(msg_recieved));
            len = SSL_read(ssl, msg_recieved, MAXBUF);
            cout << msg_recieved << endl;

            cout << "close Socket\n";
            close(sockfd);
            pthread_cancel(c2c_s);
            break;
        }
        else
        {
            cout << "I don't understand...\n";
            continue;
        }
    }
    pthread_join(c2c_s, NULL);

    return 0;
}

void *c2c_server(void *arg)//payee is a server, sould always open
{
    SSL_CTX *ctx5;
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx5 = SSL_CTX_new(SSLv23_server_method());

    char* temp;
    char pwd[100];
    getcwd(pwd,100);
    if(strlen(pwd)==1) { pwd[0]='\0'; }
    if (SSL_CTX_use_certificate_file(ctx5, temp=strcat(pwd,"/ssl/clientCert.pem"), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    getcwd(pwd,100);
    if(strlen(pwd)==1) { pwd[0]='\0'; }
    if (SSL_CTX_use_PrivateKey_file(ctx5, temp=strcat(pwd,"/ssl/clientKey.pem"), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    if (!SSL_CTX_check_private_key(ctx5))
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1) { cout << "Fail to create a socket.\n"; }

    struct sockaddr_in serverInfo,clientInfo;
    socklen_t addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverInfo.sin_port = htons(port);
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sockfd,3);

    while(1)
    {
        int clientfd = 0;
        clientfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);

        SSL *ssl5;
        ssl5 = SSL_new(ctx5);
        SSL_set_fd(ssl5, clientfd);
        if (SSL_accept(ssl5) == -1)
        {
            perror("accept");
            close(clientfd);
            break;
        }

        char msg_recieved[1024];
        memset(msg_recieved , 0 , sizeof(msg_recieved));
        SSL_read(ssl5, msg_recieved, MAXBUF);
        cout << "\nRecieve: " << msg_recieved << endl;

        while(1)
        {
            int reply = 6;
            if (reply == 6)
            {
                char msg_sent[1024];
                strcpy(msg_sent, "OK");
                SSL_write(ssl5, msg_sent, strlen(msg_sent));
                memset(msg_sent , 0 , sizeof(msg_sent));

                cout << "Please wait for the result..." << endl;
                char msg_recieved[1024];
                memset(msg_recieved , 0 , sizeof(msg_recieved));
                SSL_read(ssl5, msg_recieved, MAXBUF);
                cout << msg_recieved;
                break;
            }
            else if (reply == 7)
            {
                char msg_sent[1024];
                strcpy(msg_sent, "NO");
                SSL_write(ssl5, msg_sent, strlen(msg_sent));
                memset(msg_sent , 0 , sizeof(msg_sent));
            }
            else
            {
                cout << "I don't understand." << endl;
            }
        }
    }
    pthread_exit(NULL);
}

void *c2c_client(void *arg)//payer is a client
{
    int sockfd2 = 0;
    sockfd2 = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd2 == -1) { cout << "Fail to create a socket."; }

    struct sockaddr_in info;
    bzero(&info,sizeof(info));

    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr("127.0.0.1");
    info.sin_port = htons(server_port);

    //socket的連線
    int err = connect(sockfd2,(struct sockaddr *)&info,sizeof(info));
    if(err == -1) { cout << "Connection error" << endl; }
    else { cout << "Connect to server successfully" << endl; }

    //ssl init
    SSL_CTX *ctx2;
    SSL *ssl2;
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx2 = SSL_CTX_new(SSLv23_client_method());

    ssl2 = SSL_new(ctx2);
    if (ctx2 == NULL)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    SSL_set_fd(ssl2, sockfd2);
    if (SSL_connect(ssl2) == -1)
    {
        ERR_print_errors_fp(stderr);
    }
    else
    {
        printf("Connected with %s encryption\n", SSL_get_cipher(ssl2));
        ShowCerts(ssl2);
    }


    cout << "Pay amount: ";
    int money = 0;
    cin >> money;
    char money_char[1024];
    sprintf(money_char,"%d", money);

    char msg_sent[1024];
    strcpy(msg_sent, name);
    strcat(msg_sent, "#");
    strcat(msg_sent, money_char);
    strcat(msg_sent, "#");
    strcat(msg_sent, payee_name);
    len = SSL_write(ssl2, msg_sent, strlen(msg_sent));
    strcpy(c2s_msg, msg_sent);
    memset(msg_sent , 0 , sizeof(msg_sent));

    char msg_recieved[1024];
    memset(msg_recieved , 0 , sizeof(msg_recieved));
    len = SSL_read(ssl2, msg_recieved, MAXBUF);

    if (strcmp(msg_recieved, "OK") == 0)
    {
        cout << payee_name << " said OK!" << endl;

        pthread_t c2s_thread;
        pthread_create(&c2s_thread, NULL, &c2s, NULL);
        pthread_join(c2s_thread, NULL);

        if (change_data == 1) 
        {
            char msg_sent[1024];
            strcpy(msg_sent, "Successful\n");
            len = SSL_write(ssl2, msg_sent, strlen(msg_sent));
            memset(msg_sent , 0 , sizeof(msg_sent));

            change_data = 0;
        }
        else if (change_data == -1)
        {
            char msg_sent[1024];
            strcpy(msg_sent, "Fail\n");
            len = SSL_write(ssl2, msg_sent, strlen(msg_sent));
            memset(msg_sent , 0 , sizeof(msg_sent));
        }
        else
        {
            cout << "Wrong when changing data..." << endl;
        }
    }
    else
    {
        cout << payee_name << " does not want to be paid." << endl;
    }

    pthread_exit(NULL);
    return NULL;
}

void *c2s(void *arg)
{
    int sockfd3 = 0;
    sockfd3 = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd3 == -1) { cout << "Fail to create a socket."; }

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr("127.0.0.1");
    info.sin_port = htons(8700);

    int err = connect(sockfd3,(struct sockaddr *)&info,sizeof(info));
    if(err==-1) { cout << "Connection error" << endl; }
    else { cout << "Connect to server successfully" << endl; }

    SSL_CTX *ctx3;
    SSL *ssl3;
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx3 = SSL_CTX_new(SSLv23_client_method());
    ssl3 = SSL_new(ctx3);
    if (ctx3 == NULL)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    SSL_set_fd(ssl3, sockfd3);
    if (SSL_connect(ssl3) == -1)
    {
        ERR_print_errors_fp(stderr);
    }    
    else
    {
        printf("Connected with %s encryption\n", SSL_get_cipher(ssl3));
        ShowCerts(ssl3);
    }

    char msg_sent[1024];
    strcpy(msg_sent, "Paid#");
    strcat(msg_sent, c2s_msg);
    len = SSL_write(ssl3, msg_sent, strlen(msg_sent));
    memset(msg_sent , 0 , sizeof(msg_sent));

    char msg_recieved[1024];
    memset(msg_recieved , 0 , sizeof(msg_recieved));
    len = SSL_read(ssl3, msg_recieved, MAXBUF);
    cout << msg_recieved << endl;

    if (strcmp(msg_recieved, "Successful") == 0) { change_data = 1; }
    else if (strcmp(msg_recieved, "Fail") == 0) { change_data = -1; }
    else { cout << "something wrong happen..." << endl; }

    pthread_exit(NULL);
    return NULL;
}

int register_request(int argc , char *argv[])
{
    while(1)
    {
      cout << "Please type in your account name: ";
      cin >> name;
      cout << "Please type in a number(from 1024 to 65535) to be your port number: ";
      cin >> port;

      if(port < 1024 || port > 65535)
      {
        port = 0;
        cout << "You type a invalid number, idiot.\n";
        continue;
      }
      else
      {
        cout << "Please wait for connection...\n";
        break;
      }
    }

    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        cout << "Fail to create a socket.";
        return 0;
    }

    //server setting
    server_ip = argv[1];
    server_port = atoi(argv[2]);

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(server_ip);
    info.sin_port = htons(server_port);

    //socket的連線
    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        cout << "Connection error" << endl;
        return 0;
    }
    else{
        cout << "Connect to server successfully" << endl;
    }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    if (SSL_connect(ssl) == -1)
    {
      cout << "error" << endl;
    }
    else
    {
      cout << "Connected with " << SSL_get_cipher(ssl) << "encryption\n";
      ShowCerts(ssl);
    }

    //register
    cout << "Please wait for register..." << endl;
    char msg_sent[1024];
    strcpy(msg_sent, "REGISTER#");
    strcat(msg_sent, name);
    strcat(msg_sent, "#");
    char port_char[1024];
    sprintf(port_char, "%d", port);
    strcat(msg_sent, port_char);
    len = SSL_write(ssl, msg_sent, MAXBUF);
    memset(msg_sent , 0 , sizeof(msg_sent));


    char msg_recieved[1024];
    memset(msg_recieved , 0 , sizeof(msg_recieved));
    len = SSL_read(ssl, msg_recieved, MAXBUF);
    cout << msg_recieved << endl;

    if(strcmp(msg_recieved, "210 FALL") == 0)
    {
        return 0;
    }
    else
    {
        char money[100];
        while (1) {
            cout << "Please type in the number of money you want to deposit: ";
            cin >> money;

            if (atoi(money) > 1000000) {
                cout << "You don't have so much money...\n";
                memset(money, 0, sizeof(money));
            }
            else {
                break;
            }
        }

        char msg_sent[1024];
        strcpy(msg_sent, "DEPOSIT#");
        strcat(msg_sent, name);
        strcat(msg_sent, "#");
        strcat(msg_sent, money);
        len = SSL_write(ssl, msg_sent, strlen(msg_sent));
        memset(msg_sent , 0 , sizeof(msg_sent));
        if (len < 0)
        {
            cout << "Error" << endl;
        }

        char msg_recieved[1024];
        memset(msg_recieved , 0 , sizeof(msg_recieved));
        len = SSL_read(ssl, msg_recieved, MAXBUF);
        cout << msg_recieved << endl;
        if(strcmp(msg_recieved, "DEPOSIT_FALL") == 0)
        {
            return 0;
        }
    }
    return sockfd;
}

int login_request(int argc , char *argv[])
{
    while(1)
    {
      cout << "Please type in your account name: ";
      cin >> name;
      cout << "Please type your port number: ";
      cin >> port;

      if(port < 1024 || port > 65535)
      {
        port = 0;
        cout << "You type a invalid number, idiot.\n";
        continue;
      }
      else
      {
        cout << "Please wait for connection...\n";
        break;
      }
    }

    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        cout << "Fail to create a socket.";
        return 0;
    }

    //server setting
    server_ip = argv[1];
    server_port = atoi(argv[2]);

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(server_ip);
    info.sin_port = htons(server_port);

    //socket的連線
    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        cout << "Connection error" << endl;
        return 0;
    }
    else{
        cout << "Connect to server successfully" << endl;
    }


    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    if (SSL_connect(ssl) == -1)
        cout << "error" << endl;
    else
    {
        cout << "Connected with " << SSL_get_cipher(ssl) << "encryption\n";
        ShowCerts(ssl);
    }

    //login
    cout << "Please wait for login..." << endl;
    char msg_sent[1024];
    strcpy(msg_sent, name);
    strcat(msg_sent, "#");
    char port_char[1024];
    sprintf(port_char,"%d",port);
    strcat(msg_sent, port_char);
    len = SSL_write(ssl, msg_sent, strlen(msg_sent));
    memset(msg_sent , 0 , sizeof(msg_sent));

    char msg_recieved[1024];
    memset(msg_recieved , 0 , sizeof(msg_recieved));
    len = SSL_read(ssl, msg_recieved, MAXBUF);
    cout << msg_recieved << endl;
    if(strcmp(msg_recieved, "220 AUTH_FALL") == 0) { return 0; }

    return sockfd;
}

void ShowCerts(SSL * ssl)
{
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl);
    if (cert != NULL)
    {
        printf("Digital certificate information:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Certificate: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("No certificate information！\n");
}
