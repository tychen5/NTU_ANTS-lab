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
#include <unistd.h>
#include <vector>
#include <openssl/ssl.h>
#include <openssl/err.h>
using namespace std;

#define BLUE    "\x1b[34m"
#define RESET   "\x1b[0m"
#define MAX 256

pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in c2c;
void* anotherClient(void* data);
char CLIENT_CERT[MAX] = "client1cert.pem";
char CLIENT_PRI[MAX] = "client1key.pem";
char CLIENT_CERT2[MAX] = "client2cert.pem";
char CLIENT_PRI2[MAX] = "client2key.pem";
char portNum[MAX];
SSL *ssl;

SSL_CTX* initCTX(void)
{
    const SSL_METHOD *ssl_method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ssl_method = TLSv1_client_method();
    ctx = SSL_CTX_new(ssl_method);
    if(ctx == NULL){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
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

void showCerts(SSL *ssl)
{
    X509 *certification;
    char *certResult;
    
    certification = SSL_get_peer_certificate(ssl);
    if (certification != NULL)
    {
        cout << "Digital certificate information: " << endl;
        certResult = X509_NAME_oneline(X509_get_subject_name(certification), 0, 0);
        cout << "Certification: " << certResult << endl;
        free(certResult);
        certResult = X509_NAME_oneline(X509_get_issuer_name(certification), 0, 0);
        cout << "Issuer: " << certResult << endl;
        free(certResult);
        X509_free(certification);
    }
    else
        cout << "--> No certification!" << endl;
}

void certify_client(SSL_CTX* ctx)
{
    if(SSL_CTX_use_certificate_file(ctx, CLIENT_CERT, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, CLIENT_PRI, SSL_FILETYPE_PEM) <= 0)
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
}

void certify_server(SSL_CTX* ctx)
{
    // if (SSL_CTX_load_verify_locations(ctx, CLIENT_CERT, CLIENT_PRI) != 1)
    //     ERR_print_errors_fp(stderr);
    
    // if (SSL_CTX_set_default_verify_paths(ctx) != 1)
    //     ERR_print_errors_fp(stderr);
    
    if (SSL_CTX_use_certificate_file(ctx, CLIENT_CERT, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx, CLIENT_PRI, SSL_FILETYPE_PEM) <= 0)
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

int main(int argc, char *argv[])
{
    char buff[MAX];
    char command[MAX];
    char money[MAX];
    char regisCommand[MAX] = "REGISTER#";
    char logCommand[MAX] = "#";
    int option = 0;
    int status = 0;
    int cnt = 0;

    if(argc < 3)
    {
        cout << "--> Not enough information.";
        return -1;
    }

    // build socket
    int socketback = 0;
    socketback = socket(AF_INET, SOCK_STREAM, 0);
    if(socketback < 0)
    {
        cout << "--> Fail to create socket in client.";
        return -1;
    }

    // socket connect
    struct sockaddr_in connection;
    bzero(&connection, sizeof(connection));
    connection.sin_family = AF_INET;
    // connection.sin_port = htons(8080);
    // inet_aton("127.0.0.1", &connection.sin_addr);
    connection.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &connection.sin_addr);

    int connectback = connect(socketback, (struct sockaddr *)&connection, sizeof(connection));
    if(connectback < 0)
    {
        cout << "--> Fail to connect in client.";
        return -1;
    }

    // initialize SSL
    SSL_CTX *ctx;
    SSL_library_init();
    ctx = initCTX();
    certify_client(ctx);

    // based on CTX and generate new SSL and connect it
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, socketback);
    if(SSL_connect(ssl) <= 0)
        ERR_print_errors_fp(stderr);
    else
    {
        bzero(buff, MAX);
        showCerts(ssl);

        // receive register
        int reciback = SSL_read(ssl, buff, sizeof(buff));
        if(reciback < 0)
        {
            cout << "--> Fail to receive.";
            return -1;
        }
        cout << buff << endl;

        while(status == 0)
        {
            cout << BLUE << "1 for register, 2 for login, 3 for exit: " << RESET;
            cin >> option;
            
            if(option == 1)
            {
                cnt += 1;
                if(cnt > 1)
                {
                    cout << "--> You have registered before." << endl;
                    continue;
                }
                cout << BLUE << "Please enter your username for register: " << RESET;

                // register
                cin >> command;
                strcat(regisCommand, command);

                cout << BLUE << "Please enter amount: " << RESET;
                cin >> money;
                strcat(regisCommand, "#");
                strcat(regisCommand, money);
                SSL_write(ssl, regisCommand, strlen(regisCommand));

                // receive register message
                bzero(buff, MAX);
                SSL_read(ssl, buff, sizeof(buff));
                cout << buff << endl;
            }
            else if(option == 2)
            {
                // log in
                cout << BLUE << "Please enter your username for login: " << RESET;
                bzero(command, MAX);
                cin >> command;
                cout << BLUE << "Please enter your port number: " << RESET;
                cin >> portNum;
                while(stoi(portNum) < 1024 || stoi(portNum) > 65535)
                {
                    cout << "--> The port number is out of range. Please reenter the port number: ";
                    cin >> portNum;
                }
                strcat(command, logCommand);
                strcat(command, portNum);

                // send log message
                SSL_write(ssl, command, strlen(command));

                // receive log message
                bzero(buff, MAX);
                SSL_read(ssl, buff, sizeof(buff));
                cout << buff << endl;

                string authen(buff);
                if(authen.find("220 AUTH_FAIL") != string::npos)
                {
                    cout << "--> Username or password error." << endl;
                    status = 0;
                }
                else
                {
                    status = 2;
                }
            }
            else if(option == 3)
            {
                return -1;
            }
            else
            {
                cout << "--> Wrong option. Try Again." << endl;
            }
        }

        // create socket for another client
        pthread_t tid;
        pthread_create(&tid, NULL, &anotherClient, NULL);

        while(status == 2)
        {
            cout << BLUE << "5 for transfer, 7 for latest list, 8 for exit: " << RESET;
            cin >> option;
            if(option == 7)
            {
                bzero(command, MAX);
                strcpy(command, "List");

                // send
                SSL_write(ssl, command, strlen(command));

                // receive
                bzero(buff, MAX);
                SSL_read(ssl, buff, sizeof(buff));
                cout << buff << endl;
            }
            else if(option == 8)
            {
                bzero(command, MAX);
                strcpy(command, "Exit");

                // send
                SSL_write(ssl, command, strlen(command));

                // receive
                bzero(buff, MAX);
                SSL_read(ssl, buff, sizeof(buff));
                cout << buff << endl;

                break;
            }
            else if(option == 5)
            {
                int socket_transfer = 0;
                bool requestSuccess = false;
                struct sockaddr_in conn_addr;
                char connbuff[MAX];
                string payername;
                string payeename;
                string payment;
                string tellname;
                cout << "--> Requesting for payee via server." << endl;
                cout << BLUE << "Enter payee's name: " << RESET << endl;
                cin >> payeename;

                while(!requestSuccess)
                {
                    string requestMsg = "SEARCH#";
                    requestMsg += payeename;
                    SSL_write(ssl, requestMsg.c_str(), requestMsg.length());

                    bzero(buff, MAX);
                    SSL_read(ssl, buff, sizeof(buff));
                    if(buff[0] == '4' && buff[1] == '0' && buff[2] == '4')
                    {
                        cout << "--> The client doesn't exist." << endl;
                        cout << BLUE << "Enter payee's name again: " << RESET;
                        cin >> payeename;
                    }
                    else
                        requestSuccess = true;
                }

                string viaServer(buff);
                string anotherport;
                for(int i = 0; i < viaServer.length(); i++)
                {
                    if(viaServer[i] == '#')
                    {
                        anotherport = viaServer.substr(i+1, viaServer.length()-i-1);
                        break;
                    }
                }

                socket_transfer = socket(PF_INET, SOCK_STREAM, 0);
                if(socket_transfer < 0)
                {
                    cout << "--> Error in subjectively connectng socket.";
                    continue;
                }

                bzero((char *)&conn_addr, sizeof(conn_addr));
                conn_addr.sin_family = AF_INET;
                conn_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
                conn_addr.sin_port = htons(stoi(anotherport));

                // Connectng
                if(connect(socket_transfer, (struct sockaddr*)&conn_addr, sizeof(conn_addr)) < 0)
                {
                    cout << "--> Error when connecting another client." << endl;
                    continue;
                }

                SSL_CTX *ctx1;
                SSL *ssl1;
                ctx1 = initCTX();
                certify_client(ctx1);
                ssl1 = SSL_new(ctx1);
                SSL_set_fd(ssl1, socket_transfer);
                if(SSL_connect(ssl1) <= 0)
                    ERR_print_errors_fp(stderr);
                else
                {
                    cout << "--> Now connecting to port: " << anotherport << endl;
                    bzero(connbuff, MAX);
                    SSL_read(ssl1, connbuff, sizeof(connbuff));
                    cout << connbuff << endl;
                    cout << BLUE << "Please enter your name: " << RESET;
                    cin >> tellname;
                    SSL_write(ssl1, tellname.c_str(), tellname.length());

                    cout << "--> How much would you want to pay ?" << endl;
                    cin >> payment;

                    SSL_write(ssl1, payment.c_str(), payment.length());

                    bzero(connbuff, MAX);
                    SSL_read(ssl1, connbuff, sizeof(connbuff));
                    cout << connbuff << endl;
                }

                SSL_free(ssl1);
            }
            else
            {
                cout << "--> Wrong option. Try Again." << endl;
            }

        }
    }
    
    close(socketback);
    SSL_free(ssl);
    SSL_CTX_free(ctx);

    return 0;
}

void* anotherClient(void* data)
{
  	int anotherSocket, clientSocket, clientlen, newSocket = 0;
    struct sockaddr_in server_addr, client_addr;
    char serverbuff[MAX];
    char serversend[MAX];
    string portnum(portNum);
    socklen_t client_len = sizeof(client_addr);
   	
    // Create a server socket
    anotherSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(anotherSocket < 0)
    {
        cout << "--> Error in another socket.";
        exit(1);
    } 

    // Initialize socket structure
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(stoi(portnum));

    // Assign a port number to server
    int bindback = bind(anotherSocket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(bindback < 0)
    {
        cout << "--> Error when binding.";
    	exit(1);
    }

    listen(anotherSocket, 3);
    // cout << "Now listening on port: " << portno << endl;

    SSL_CTX *ctx2;
    SSL_library_init();
    SSL *ssl2;

    while((newSocket = accept(anotherSocket, (struct sockaddr *)&client_addr, &client_len)))
    {
        if(newSocket >= 0)
        {
            pthread_mutex_lock(&mutex_lock);

            ctx2 = initCTXServer();
            certify_server(ctx2);
            ssl2 = SSL_new(ctx2);
            SSL_set_fd(ssl2, newSocket);
            SSL_accept(ssl2);
            
            bzero(serversend, MAX);
            strcpy(serversend, "--> Connection permitted.");
            SSL_write(ssl2, serversend, strlen(serversend));

            pthread_mutex_unlock(&mutex_lock);

            char anotherIP[MAX];
            char anotherName[MAX];
            inet_ntop(AF_INET, &client_addr.sin_addr, anotherIP, INET_ADDRSTRLEN);
            bzero(serverbuff, MAX);
            SSL_read(ssl2, serverbuff, sizeof(serverbuff));
            strcpy(anotherName, serverbuff);

            // cout << "\nNew connection." << endl;
            cout << "\n--> New connection from " << anotherIP << ":" 
                 << client_addr.sin_port << ", named " << anotherName << "." << endl;

            string notifyServer(anotherName);
            bzero(serverbuff, MAX);
            SSL_read(ssl2, serverbuff, sizeof(serverbuff));
            cout << "--> " << anotherName << " pays you: " << serverbuff << " dollars." << endl;

            string buf(serverbuff); 
            notifyServer = "TRANSFER#" + notifyServer + "#" + buf;
            SSL_write(ssl, notifyServer.c_str(), notifyServer.length());

            bzero(serverbuff, MAX);
            SSL_read(ssl, serverbuff, MAX);
            cout << serverbuff << endl;
            SSL_write(ssl2, serverbuff, MAX);
            
            SSL_free(ssl2);
        } 
    }

    // close server socket 
    close(newSocket);
    SSL_CTX_free(ctx2);     
  
    return 0;
}
