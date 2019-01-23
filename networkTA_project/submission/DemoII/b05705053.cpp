#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <queue>
using namespace std;
class UserOnline
{
  private:
    string name;
    string ip;
    int port;

  public:
    UserOnline(string name, string ip, int port) : name(name), ip(ip), port(port){}
    string getName() { return this->name; }
    string getIp() { return this->ip; }
    int getPort() { return this->port; }
};

class User
{
  private:
    string name;
    string password;
    int balance;

  public:
    User(string name, string password, int balance) : name(name), password(password), balance(balance) {}
    string getName() { return this->name; }
    string getPassword() { return this->password; }
    int getBalance() { return this->balance; }
};

class SocketQueue
{
  private:
    queue<int *> client_sock_queue;
    pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;

  public:
    SocketQueue(){};
    ~SocketQueue(){};
    int *pop()
    {
        pthread_mutex_lock(&mutex_queue);
        int *s = client_sock_queue.front();
        client_sock_queue.pop();
        pthread_mutex_unlock(&mutex_queue);
        return s;
    };
    void push(int *t)
    {
        pthread_mutex_lock(&mutex_queue);
        client_sock_queue.push(t);
        pthread_mutex_unlock(&mutex_queue);
    }
    size_t size()
    {
        pthread_mutex_lock(&mutex_queue);
        size_t s = client_sock_queue.size();
        pthread_mutex_unlock(&mutex_queue);
        return s;
    }
};

//the thread function
void *connection_handler(void *);

//global function
string list();
void update_user(string, string, int);
bool registered(string);
bool login(string, string);
bool is_online(string);
int get_balance(string);
void update_online_user(string, string, int);
void update_offline_user(string, string, int);

//global variable
const int namebuffer = 128;

//allow 100 users to register
const int max_user = 1000;
vector<User> user;
pthread_mutex_t mutex_user = PTHREAD_MUTEX_INITIALIZER;

//message from client
const int buffer_size = 1024;
//accommodate 20 users online at a time
vector<UserOnline> user_online;
pthread_mutex_t mutex_online = PTHREAD_MUTEX_INITIALIZER;

//thread pool
SocketQueue sock_queue;
int active_thread_num = 0;
const int max_active_thread = 3;
pthread_mutex_t mutex_thread = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[])
{
    int socket_desc, client_sock, c, *new_sock;
    struct sockaddr_in server, client;

    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("[server ]Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
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
        //print the error message
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
string list()
{
    string msg;
    pthread_mutex_lock(&mutex_online);
    msg = "There is " + to_string(user_online.size()) + " users online\n";
    for (int i = 0; i < user_online.size(); i++)
    {
        UserOnline *u = &user_online[i];
        msg += u->getName();
        msg += "#";
        msg += u->getIp();
        msg += "#";
        msg += to_string(u->getPort());
        msg += "\n";
    }
    pthread_mutex_unlock(&mutex_online);
    return msg;
}
bool registered(string username)
{
    bool registered = false;
    pthread_mutex_lock(&mutex_user);
    for (vector<User>::iterator it = user.begin(); it != user.end(); it++)
    {
        if (it->getName() == username)
        {
            registered = true;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_user);
    return registered;
}
bool login(string username, string password)
{
    bool login = false;
    pthread_mutex_lock(&mutex_user);
    for (vector<User>::iterator it = user.begin(); it != user.end(); it++)
    {
        if (it->getName() == username && it->getPassword() == password)
        {
            login = true;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_user);
    return login;
}
bool is_online(string username)
{
    bool is_online = false;
    pthread_mutex_lock(&mutex_online);
    for (vector<UserOnline>::iterator it = user_online.begin(); it != user_online.end(); it++)
    {
        if (it->getName() == username)
        {
            is_online = true;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_online);
    return is_online;
}
int get_balance(string username)
{
    int balance = 10000;
    pthread_mutex_lock(&mutex_user);
    for (vector<User>::iterator it = user.begin(); it != user.end(); it++)
    {
        if (it->getName() == username)
        {
            balance = it->getBalance();
            break;
        }
    }
    pthread_mutex_unlock(&mutex_user);
    return balance;
}
void update_user(string username, string password, int balance)
{
    pthread_mutex_lock(&mutex_user);
    User *u = new User(username, password, balance);
    user.push_back(*u);
    pthread_mutex_unlock(&mutex_user);
}
void update_online_user(string username, string ip, int port)
{
    pthread_mutex_lock(&mutex_online);
    UserOnline *u = new UserOnline(username, ip, port);
    user_online.push_back(*u);
    pthread_mutex_unlock(&mutex_online);
}
void update_offline_user(string username)
{
    pthread_mutex_lock(&mutex_online);
    for (vector<UserOnline>::iterator it = user_online.begin(); it < user_online.end(); it++)
    {
        if (it->getName() == username)
        {
            user_online.erase(it);
        }
    }
    pthread_mutex_unlock(&mutex_online);
}
// This will handle connection for each client
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    while (true)
    {
        int *new_sock = NULL;
        pthread_mutex_lock(&mutex_thread);
        if (sock_queue.size() <= 0)
        {
            pthread_mutex_unlock(&mutex_thread);
            continue;
        }
        new_sock = sock_queue.pop();
        pthread_mutex_unlock(&mutex_thread);

        //if the socket is not NULL
        if (!new_sock)
        {
            continue;
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
        getpeername(sock, (struct sockaddr *)&c, &cLen); //use connectSock here.
        char *c_ip = inet_ntoa(c.sin_addr);

        // Client connection success!
        msg = "connection accepted!";
        send(sock, msg.c_str(), msg.size(), 0);

        while ((n = recv(sock, client_msg, sizeof(client_msg), 0)) > 0)
        {
            // Output the client msg
            puts(("[client msg] " + string(client_msg)).c_str());
            if (strstr(client_msg, "REGISTER#") != NULL)
            {
                char *account = strtok(client_msg, "REGISTER#");
                string *user = new string(account);
                char *password = strtok(NULL, "#");
                string *pass = new string(password);
                char *balance = strtok(NULL, "#");

                if (registered(*user))
                {
                    msg = "The account have been registered!";
                }
                else
                {
                    update_user(*user, *pass, atoi(balance));
                    msg = "100 OK";
                }
                send(sock, msg.c_str(), msg.size(), 0);
            }
            else if (strstr(client_msg, "#") != NULL)
            {
                char *username = (strtok(client_msg, "#"));
                char *password = (strtok(NULL, "#"));
                char *port = (strtok(NULL, "#"));
                client_name = *new string(username);

                if (login(client_name, *new string(password)))
                {
                    if (is_online(client_name))
                    {
                        msg = "Your account was login";
                        send(sock, msg.c_str(), msg.size(), 0);
                    }
                    else
                    {
                        update_online_user(client_name, *new string(c_ip), atoi(port));
                        msg = "";
                        msg += to_string(get_balance(client_name));
                        msg += "\n";
                        msg += list();
                        send(sock, msg.c_str(), msg.size(), 0);
                    }
                }
                else
                {
                    client_name = "";
                    msg = "220 AUTH_FAIL";
                    send(sock, msg.c_str(), msg.size(), 0);
                }
            }
            else if (strstr(client_msg, "List") != NULL)
            {
                if (is_online(client_name))
                {
                    msg = list();
                    send(sock, msg.c_str(), msg.size(), 0);
                }
                else
                {
                    msg = "401 UNAUTHORIZED";
                    send(sock, msg.c_str(), msg.size(), 0);
                }
            }
            else if(strstr(client_msg, "LogOut") != NULL ||strstr(client_msg, "Exit") != NULL )
            {
                update_offline_user(client_name);
                msg = "Bye " + client_name;
                send(sock, msg.c_str(), msg.size(), 0);
            }
            else
            {
                msg = "Please type the right option!";
                new_sock = NULL;

            }
            //Reset the buffer
            puts(("[server sent] " + msg ).c_str());
            bzero(client_msg, sizeof(client_msg));
        }
        close(sock);

        if (n == 0)
        {
            update_offline_user(client_name);
            msg = "[server msg]" + client_name + " disconnected";
            puts(msg.c_str());
        }
        else
        {
            perror("recv failed");
        }
        new_sock = NULL;
    }

    return 0;
}
