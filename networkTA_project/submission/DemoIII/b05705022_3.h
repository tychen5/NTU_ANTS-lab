#ifndef CLIENT_H
#define CLIENT_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <unistd.h>
#include <thread>
using namespace std;

// #define IP   "140.112.107.194"
// #define PORT 33120
#define IP          "127.0.0.1"
#define PORT        8080

class Client {
public:
    Client();
    ~Client();
    void handle(string cmd);
    bool disconnect();
    void help();
private:
    char*                   _ip;
    int                     _port;
    int                     _sock;
    SSL_CTX*                _ctx;
    SSL*                    _ssl;
    int                     _file;
    bool                    _run;
    thread*                 _bgServ;
    int                     _bgSock;

    bool                    _connected;
    bool                    _loggedin;
    string                  _payment;
    string                  _name;
    struct sockaddr_in      _serv_addr;

    bool isConnected();
    bool connectServer(string ip, int port);
    bool reg(string name, int amount);
    bool login(string name, int port);
    bool list();
    bool pay(string name, int amount);
};

#endif // CLIENT_H