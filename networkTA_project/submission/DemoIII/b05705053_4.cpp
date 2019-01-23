#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "sockQueue.h"
#include "user.h"
using namespace std;

//The thread function
void *connection_handler(void *);

//Global variable
const int namebuffer = 128;

//Message from client
const int buffer_size = 1024;
UserOnlineList user_online_list;
UserList user_list;

//Thread pool
SocketQueue sock_queue;
const int max_active_thread = 5;
pthread_mutex_t mutex_thread = PTHREAD_MUTEX_INITIALIZER;

SSL_CTX *ctx;
pthread_mutex_t ctx_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[])
{

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(TLSv1_server_method());
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    if (SSL_CTX_use_certificate_file(ctx, "./PKI/ca.crt", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "./PKI/ca.key", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    if (!SSL_CTX_check_private_key(ctx))
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    int socket_desc, client_sock, c, *new_sock;
    struct sockaddr_in server, client;

    //Create socket
    socket_desc = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("[server ]Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = PF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

    int serv_port;
    if (argc >= 2)
    {
        serv_port = atoi(argv[1]);
    }
    else
    {
        serv_port = 9090;
    }

    printf("[server msg] Server Port Number is %d\n", serv_port);
    server.sin_port = htons(serv_port);

    //Bind
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        //Print the error message
        perror("[server msg] Bind failed. Error");
        return 1;
    }
    puts("[server msg] Bind successfully!");

    //Listen
    listen(socket_desc, max_active_thread);

    //Accept and incoming connection
    puts("[server msg] Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    pthread_t *client_thread;
    client_thread = new pthread_t[max_active_thread];
    for (int i = 0; i < max_active_thread; i++)
    {
        if (pthread_create(&client_thread[i], NULL, connection_handler, (void *)new_sock) < 0)
        {
            return -1;
        }
    }
    while (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c))
    {
        new_sock = new int[1];
        *new_sock = client_sock;
        sock_queue.push(new_sock);
    }
    if (client_sock < 0)
    {
        perror("Accept failed");
        return 1;
    }
    pthread_exit(NULL);
}

// This will handle connection for each client
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    while (true)
    {
        int *new_sock = NULL;
        SSL *ssl = NULL;
        pthread_mutex_lock(&mutex_thread);
        if (sock_queue.size() <= 0)
        {
            pthread_mutex_unlock(&mutex_thread);
            continue;
        }
        new_sock = sock_queue.pop();
        pthread_mutex_unlock(&mutex_thread);
        
        //Use certificate 
        pthread_mutex_lock(&ctx_mutex);
        ssl = SSL_new(ctx);
        pthread_mutex_unlock(&ctx_mutex);

        SSL_set_fd(ssl, *new_sock);
        if (SSL_accept(ssl) == -1)
        {
            perror("accept");
            close(*new_sock);
            break;
        }
        int sock = *(int *)new_sock;
        int n;
        char client_msg[buffer_size];

        //Sent to the client
        string msg;
        //After client register successfully, the client name
        string client_name;

        struct sockaddr_in c;
        socklen_t cLen = sizeof(c);
        getpeername(sock, (struct sockaddr *)&c, &cLen); 
        char *c_ip = inet_ntoa(c.sin_addr);

        // Client connection success!
        msg = "connection accepted!";
        SSL_write(ssl, msg.c_str(), msg.length());

        while ((n = SSL_read(ssl, client_msg, sizeof(client_msg))) > 0)
        {
            // Output the client msg
            puts(("[client msg] " + string(client_msg)).c_str());
            if (strstr(client_msg, "REGISTER#") != NULL)
            {
                char *account = strtok(client_msg, "REGISTER#");
                string *user = new string(account);
                puts(account);
                char *password = strtok(NULL, "#");
                string *pass = new string(password);
                puts(password);
                char *balance = strtok(NULL, "#");

                if (user_list.registered(*user))
                {
                    msg = "The account have been registered!";
                }
                else
                {
                    user_list.update_user(*user, *pass, atoi(balance));
                    msg = "100 OK";
                }
            }
            else if (strstr(client_msg, "List") != NULL)
            {
                if (user_online_list.is_online(client_name))
                {
                    msg = "Account Balance: ";
                    msg += to_string(user_list.get_balance(client_name));
                    msg += "\n";
                    msg += user_online_list.list();
                }
                else
                {
                    msg = "401 UNAUTHORIZED";
                }
            }
            else if (strstr(client_msg, "Trade") != NULL){
                char *account = strtok(client_msg, "#");
                account = strtok(NULL, "#");
                string user(account);

                // Check whether the user is online
                if (!user_online_list.is_online(user))
                {
                    msg = "230 NO_MATCH";
                }
                else
                {
                    msg = user_online_list.get_user_info(user);
                }
            }
            else if (client_msg[0] == '#')
            {
                puts("transaction");
                int deltaNum;
                char *num = strtok(client_msg, "#");
                deltaNum = atoi(strtok(client_msg, "#"));

                if (user_list.modify_money(client_name, deltaNum))
                {
                    msg = "Trade Done";
                }
                else
                {
                    msg = "404 NOT_FOUND";
                }
            }
            else if (strstr(client_msg, "#") != NULL)
            {
                char *username = (strtok(client_msg, "#"));
                char *password = (strtok(NULL, "#"));
                char *port = (strtok(NULL, "#"));
                client_name = *new string(username);
                if (user_list.login(client_name, *new string(password)))
                {
                    if (user_online_list.is_online(client_name))
                    {
                        msg = "Your account was login";
                    }
                    else
                    {
                        user_online_list.update_online_user(client_name, *new string(c_ip), atoi(port));
                        msg = "Account Balance: ";
                        msg += to_string(user_list.get_balance(client_name));
                        msg += "\n";
                        msg += user_online_list.list();
                    }
                }
                else
                {
                    client_name = "";
                    msg = "220 AUTH_FAIL";
                }
            }
            else if (strstr(client_msg, "LogOut") != NULL || strstr(client_msg, "Exit") != NULL)
            {
                user_online_list.update_offline_user(client_name);
                msg = "Bye " + client_name;
            }
            else
            {
                msg = "Please type the right option!";
            }

            SSL_write(ssl, msg.c_str(), msg.size());
            //Reset the buffer
            puts(("[server sent] " + msg).c_str());
            bzero(client_msg, sizeof(client_msg));
        }
        close(sock);
        if (n == 0)
        {
            user_online_list.update_offline_user(client_name);
            msg = "[server msg]" + client_name + " disconnected";
            puts(msg.c_str());
        }
        else
        {
            puts("error msg");
            perror("recv failed");
        }
        SSL_free(ssl);
    }

    return 0;
}
