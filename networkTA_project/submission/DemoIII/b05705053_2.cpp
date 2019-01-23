// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <iostream>
//Self-defined function
#include "color.h"
using namespace std;

#define PORT 8080
#define _GNU_SOURCE
#define MAXBUF 1024
const string cert = "./PKI/ca.crt";
const string key = "./PKI/ca.key";
//Global Function for
void ShowCerts(SSL *ssl);
void Prompt(void);
void *Transaction_handler(void *);
SSL_CTX *InitServerCTX(void);
SSL_CTX *InitClientCTX(void);
void LoadCertificates(SSL_CTX *ctx, string cert, string key);

//Global variable Sever SSL
SSL *server_ssl;
int sock;
//Utility Function for communicate with the sever

int main(int argc, char const *argv[])
{
    struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char ip_address[24] = {0};
    int port_num = 9999, trade_port = 8080;
    string msg;
    char command[1024];
    string client_name;
    SSL_CTX *ctx;

    //set ip and portNum
    if (argc >= 2)
    {
        sscanf(argv[1], "%s", ip_address);
    }
    if (argc >= 3)
    {
        sscanf(argv[2], "%d", &port_num);
    }
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cout << "\n Socket creation error \n";
        return -1;
    }

    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_num);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip_address, &serv_addr.sin_addr) <= 0)
    {
        cout << "\nInvalid address/ Address not supported \n";
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cout << "\nConnection Failed \n";
        return -1;
    }

    SSL_library_init();
    ctx = InitClientCTX();
    server_ssl = SSL_new(ctx);
    SSL_set_fd(server_ssl, sock);

    if (SSL_connect(server_ssl) == -1)
    {
        ERR_print_errors_fp(stderr);
    }
    else
    {
        cout << "Connected with" << SSL_get_cipher(server_ssl)
                << "encryption\n";
            ShowCerts(server_ssl);
    }

    Prompt();
    cout << ANSI_COLOR_RESET;

    //Accept the connection of the server
    SSL_read(server_ssl, buffer, sizeof(buffer));
    if (strcmp(buffer, "connection accepted!") < 0)
    {
        cout << buffer;
        cout << ANSI_COLOR_RED << "Server Connection Fail";
        return -1;
    }
    cout << endl << buffer;

    //Listen the command of the client Register & Login
    while (true)
    {
        memset(buffer, '\0', sizeof(buffer));
        msg = "";
        cout << ANSI_COLOR_RESET << endl << "client>";
        cin >> command;
        if (strstr(":h", command) != NULL)
        {
            cout << ANSI_COLOR_YELLOW;
            Prompt();
        }
        else if (strstr(":r", command) != NULL)
        {
            char account[48] = {0};
            char password[48] = {0};
            char verified[48] = {0};
            int balance = 10000;

            cout << ANSI_COLOR_CYAN <<  
                "Enter your user account to register:"
                << ANSI_COLOR_RESET;

            cin >> account;
            do
            {
                if (strlen(password) != 0)
                {
                    cout << ANSI_COLOR_MAGENTA << 
                        "inconsistent between passwords!\n"
                        << ANSI_COLOR_RESET;
                }
                bzero(password, sizeof(password));
                bzero(verified, sizeof(verified));
                cout << ANSI_COLOR_CYAN 
                    << "Enter your password to register:" 
                    << ANSI_COLOR_RESET;
                cin >> password;
                cout << ANSI_COLOR_CYAN 
                    << "Enter your verified password:"
                    << ANSI_COLOR_RESET;
                cin >> verified;

            } while (strcmp(password, verified) != 0);

            cout << ANSI_COLOR_CYAN 
                <<  "Enter your initial balance:"
                << ANSI_COLOR_RESET;

            cin >> balance;
            msg += "REGISTER#" + string(account) +
                "#" + string(password) + "#" + to_string(balance);

            SSL_write(server_ssl, msg.c_str(), msg.length());
            SSL_read(server_ssl, buffer, sizeof(buffer));
            cout <<  buffer << endl;
        }
        else if (strstr(":l", command) != NULL)
        {
            char account[48] = {0};
            char password[48] = {0};
            char port[10] = {0};
            cout << ANSI_COLOR_GREEN <<
                "Enter your user account to login:"
                << ANSI_COLOR_RESET;
            cin >> account;
            cout << ANSI_COLOR_GREEN
                << "Enter your user password to login:"
                << ANSI_COLOR_RESET;

            cin >> password;
            cout << ANSI_COLOR_GREEN
                "Please enter your port:"
                << ANSI_COLOR_RESET;

            cin >> port;
            while (((atoi(port) <= 1024) || (atoi(port) > 65535)))
            {
                bzero(port, sizeof(port));
                cout << ANSI_COLOR_MAGENTA <<
                    "Please enter your port between 1024 and 65535: "
                    << ANSI_COLOR_RESET;
                cin >> port;
            }
            msg += string(account) + "#" + string(password) + "#" + string(port);
            SSL_write(server_ssl, msg.c_str(), msg.length());
            SSL_read(server_ssl, buffer, sizeof(buffer));
            cout << buffer << endl;
            trade_port = atoi(port);
            if(string(buffer).find("Account") != string::npos){
                client_name = string(account);
                break;
            }
        }
        else
        {
            cout << ANSI_COLOR_MAGENTA << "Please input :h to help\n";
            continue;
        }
    };

    //Create an thread to connect the trader
    pthread_t trader;
    if (pthread_create(&trader, NULL, &Transaction_handler, &trade_port) < 0)
    {
        return -1;
    }

    //Listen the command of the client : List & Trade & Exit
    while (true){
        memset(buffer, '\0', sizeof(buffer));
        msg = "";
        cerr << ANSI_COLOR_RESET << endl << "client>";
        cin >> command;
        if (strstr(":L", command) != NULL)
        {
            msg = "List";
            SSL_write(server_ssl, msg.c_str(), msg.length());
            SSL_read(server_ssl, buffer, sizeof(buffer));
            cout << buffer << endl;
        }
        else if(strstr(":T", command) != NULL){
            char name[48];
            cout << ANSI_COLOR_CYAN 
                << "Enter the name you want to trade with:"
                << ANSI_COLOR_RESET ;

            cin >> name;
            msg = "Trade#";
            msg += string(name);
            SSL_write(server_ssl, msg.c_str(), msg.length());
            
            bzero(buffer, sizeof(buffer));
            SSL_read(server_ssl, buffer, sizeof(buffer));
            if (strstr(buffer, "230 NO_MATCH") != NULL)
            {
                cout << buffer << endl;
            }
            else
            {
                cout << "Find the peers" << endl;
                //Initialize the the address
                const char* trade_name;
                const char* trade_ip;
                const char* port;
                trade_name = strtok(buffer, "#");
                trade_ip = strtok(NULL, "#");
                port = strtok(NULL, "#");
                    
                // //Initialization of the trade port
                struct sockaddr_in trade_addr;
                int trade_fd = socket(PF_INET, SOCK_STREAM, 0);

                bzero((char *)&trade_addr, sizeof(trade_addr));
                trade_addr.sin_family = AF_INET;
                trade_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
                trade_addr.sin_port = htons(atoi(port));

                // // Convert IPv4 and IPv6 addresses from text to binary form
                if (inet_pton(AF_INET, trade_ip , &trade_addr.sin_addr) <= 0)
                {
                    cout << "\nInvalid address/ Address not supported \n";
                    return -1;
                }

                //Connect with the trade socket
                if(connect(trade_fd, (struct sockaddr *)&trade_addr, sizeof(trade_addr)) < 0)
                {
                    cerr << "Fail to connect with the peers !!";
                    return -1;
                }
                
                SSL_CTX *ctx_to_trade;
                SSL_library_init();
                SSL *ssl_to_trade;
                ctx_to_trade = InitClientCTX();
                LoadCertificates(ctx_to_trade, cert, key);
                ssl_to_trade = SSL_new(ctx_to_trade);
                SSL_set_fd(ssl_to_trade, trade_fd);
                if (SSL_connect(ssl_to_trade) == -1)
                {
                    ERR_print_errors_fp(stderr);
                }
                char pay[48] = {0};
                cout << ANSI_COLOR_CYAN << "Enter the amount you want to pay: "
                        << ANSI_COLOR_RESET;
                cin >> pay;
                SSL_write(ssl_to_trade, pay, strlen(pay));
                
                //Sent to the server
                string money = "#-" + string(pay);
                SSL_write(server_ssl, money.c_str(), money.length());
                bzero(buffer, sizeof(buffer));
                SSL_read(server_ssl, buffer , sizeof(buffer));
                cout << buffer << endl;
                SSL_free(ssl_to_trade);
                SSL_CTX_free(ctx_to_trade);
            }
        }
        else if (strstr(command, ":e") != NULL)
        {
            msg = "Exit";
            SSL_write(server_ssl, msg.c_str(), msg.length());
            SSL_read(server_ssl, buffer, sizeof(buffer));
            cout <<  buffer << endl;
            return 0;
        }
        else if (strstr(command, ":o") != NULL)
        {
            msg = "LogOut";
            SSL_write(server_ssl, msg.c_str(), msg.length());
            SSL_read(server_ssl, buffer, sizeof(buffer));
            cout << buffer << endl;
        }
        else
        {
            cout << ANSI_COLOR_MAGENTA <<  "Please input :h to help\n";
            continue;
        }
    }
    return 0;
}

void ShowCerts(SSL *ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl);

    if (cert) {
        cout << "==============================================\n";
        cout << "Server certificates:\n";
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        cout << "Subject: " << line << endl;
        free(line); 
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        cout << "Issuer: " << line << endl;

        //free the line pointer
        free(line); 
        X509_free(cert); 
        cout << "==============================================\n";
    }
    else{
        cout << "No certificates.\n";
    }
}

void Prompt(void)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    fp = fopen("prompt.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    while ((read = getline(&line, &len, fp)) != -1)
    {
        cout << ANSI_COLOR_YELLOW;
        cout << line;
    }
    fclose(fp);
    if (line)
        free(line);
}
void *Transaction_handler(void* port){
    struct sockaddr_in addr;
    int client_port = *(int *)port;
    memset(&addr, '0', sizeof(addr));
    
    string msg;
    char buffer[1024] = {0};

    //Use the ca and key of the client
    string cert = "./PKI/client.crt";
    string key =  "./PKI/client.key";

    SSL_CTX *ctx_trade;
    SSL_library_init();
    ctx_trade = InitServerCTX();
    LoadCertificates(ctx_trade, cert, key);

    //Initialize the listen port
    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(client_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    //Create the socket for listen the incoming msg
    int sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        puts("Error in listening socket");
        exit(1);
    }
    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        puts("Fail to bind the trade port address!");
        exit(1);
    }
    listen(sock_fd, 5);

    int peer_sock = -1;
    struct sockaddr_in peer;
    int p = sizeof(peer);
    SSL *ssl_trade;
    while (peer_sock = accept(sock_fd, (struct sockaddr *)&peer, (socklen_t *)&p))
    {
        char buffer[1024] = {0};
        if (peer_sock < 0)
        {
            continue;
        }
        cout << "Trade!!" << endl;
        ssl_trade = SSL_new(ctx_trade);
        SSL_set_fd(ssl_trade, peer_sock);
        if (SSL_accept(ssl_trade) < 0)
        {
            perror("Fail to accept");
            close(peer_sock);
            break;
        }
        SSL_read(ssl_trade, buffer, sizeof(buffer));
        cout << "GET " << buffer << " dollars\n";
        string money = "#" + string(buffer);
        SSL_write(server_ssl, money.c_str(), money.length());
        bzero(buffer, sizeof(buffer));
        SSL_read(server_ssl, buffer, sizeof(buffer));
        cout << buffer<< endl << "client>";
        close(peer_sock);
        SSL_free(ssl_trade);
    }
    close(sock_fd);
    cout << "Exit thread \n";
    pthread_exit(NULL);
    return 0;
}
SSL_CTX *InitServerCTX(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();   
    method = TLSv1_server_method();
    ctx = SSL_CTX_new(method);
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
SSL_CTX *InitClientCTX(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    method = TLSv1_client_method();
    ctx = SSL_CTX_new(method);
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
void LoadCertificates(SSL_CTX *ctx, string cert, string key)
{
    // set the local certificate from CertFile
    if (SSL_CTX_use_certificate_file(ctx, cert.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //set the private key from KeyFile
    if (SSL_CTX_use_PrivateKey_file(ctx, key.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //verify private key
    if (!SSL_CTX_check_private_key(ctx))
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}
