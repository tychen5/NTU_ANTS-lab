#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <iomanip>
#include <cstring>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
using namespace std;

// #define IP   "140.112.107.194"
// #define PORT 33120
#define IP   "127.0.0.1"
#define PORT 8080

// compare input with cmd for first n characters
int myStrNCmp(const string& cmd, const string& input, unsigned n);

class Client {
public:
    Client(): _ip(0), _port(0), _sock(0), _loggedin(false), _connected(false) { help(); cout << endl; }
    ~Client() { close(_sock); }
    void handle(string cmd);
    bool disconnect();
    void help();
private:
    char* _ip;
    int _port;
    int _sock;
    bool _connected;
    bool _loggedin;
    string _name;
    struct sockaddr_in _serv_addr;

    bool isConnected();
    bool connect_server(string ip, int port);
    bool reg(string name, int amount);
    bool login(string name, int port);
    bool list();
    bool pay(string name, int amount);
};

void Client::handle(string cmd) {
    stringstream ss(cmd);
    string request;
    if (ss >> request) {
        if (myStrNCmp("connect", request, 1) == 0) {
            string ip;
            int port;
            if (!(ss >> ip >> port)) { ip = IP; port = PORT; };
            connect_server(ip, port);
        }
        else if (myStrNCmp("register", request, 1) == 0) {
            string name;
            int amount;
            if (!(ss >> name >> amount)) { cerr << "Usage: register <name> <amount>\n"; return; }
            reg(name, amount);
        }
        else if (myStrNCmp("login", request, 2) == 0) {
            string name;
            int port;
            if (!(ss >> name >> port)) { cerr << "Usage: login <name> <port>\n"; return; }
            login(name, port);
        }
        else if (myStrNCmp("list", request, 2) == 0) {
            list();
        }
        else if (myStrNCmp("pay", request, 1) == 0) {
            string name;
            int amount;
            if (!(ss >> name >> amount)) { cerr << "Usage: pay <user name> <amount>\n"; return; }
            pay(name, amount);
        }
        else if (myStrNCmp("disconnect", request, 1) == 0) {
            disconnect();
        }
        else if (myStrNCmp("help", request, 1) == 0) {
            help();
        }
        else {
            cout << "Error: Invalid command. Type \"help\"" << endl;
        }
    }
}

bool Client::isConnected() {
    if (!_connected) {
        cout << "Error: Please first connect to a server\n";
        return false;
    }
    return true;
}


bool Client::connect_server(string ip, int port) {
    if (_connected) {
        cout << "Error: Now connected to " << _ip << ":" << _port
             << "\nReplace current connection? [y/n]";
        cout.flush();
        char c[5];
        cin >> c;
        if (strcmp(c, "n") == 0) return false; 
    }
    // create socket
    _ip = &ip[0];
    _port = port;
    cout << "Connecting to " << _ip << ":" << _port << " ..." << endl;

    struct sockaddr_in sa_dst;
    // struct sockaddr_in sa_loc;

    _sock = socket(AF_INET, SOCK_STREAM, 0);

    // Local
    // memset(&sa_loc, 0, sizeof(struct sockaddr_in));
    // sa_loc.sin_family = AF_INET;
    // sa_loc.sin_port = htons(12345);
    // sa_loc.sin_addr.s_addr = INADDR_ANY;

    // if (::bind(_sock, (struct sockaddr *)&sa_loc, sizeof(sa_loc)) < 0) {
    //     cerr << "Error: Binding failed\n";
    //     return false;
    // }
    
    // Remote
    memset(&sa_dst, 0, sizeof(struct sockaddr_in));
    sa_dst.sin_family = AF_INET;
    sa_dst.sin_port = htons(8080);
    sa_dst.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(_sock, (struct sockaddr *)&sa_dst, sizeof(sa_dst)) < 0) {
        cout << "Error: Connecting failed\n";
        return false;
    }

    // char msg[25] = "Hello! This is client\n";
    // send(_sock, msg, sizeof(msg), 0); 
    char buffer[25];
    memset(buffer, 0, sizeof(buffer));
    if (recv(_sock, buffer, sizeof(buffer), 0) < 0) { 
        cerr << "Receving error\n";
        return false;
    }
    cout << "Successfully connected!" << endl;
    cout << "Server response: " << buffer << endl;
    _connected = true;
    return true;
}

bool Client::reg(string name, int amount) {
    if (!isConnected()) return false;

    cout << "Registering user \"" << name << "\"..." << endl;

    string msg = "REGISTER#" + name + "#" + to_string(amount) + "\n";
    const char* user = &msg[0];
    send(_sock, user, msg.size(), 0); 

    // read response
    char buffer[1024] = {0};
    if (recv(_sock, buffer, sizeof(buffer), 0) < 0) cerr << "Receving error\n";
    string response = string(buffer);
    if (response == "100 OK\n") {
        cout << "Successfully Resistered!" << endl;
        return true;
    } else {
        cout << "Registered failed!" << endl;
    }
    cout << "Server response: " << buffer << endl;
    return false;
}

bool Client::login(string name, int port) {
    if (!isConnected()) return false;
    
    cout << "Require to login..." << endl;
    string msg = name + "#" + to_string(port) + "\n";
    const char* user = &msg[0];
    send(_sock, user, msg.size(), 0); 

    // read response
    char buffer[256] = {0};
    if (recv(_sock, buffer, sizeof(buffer), 0) < 1) cerr << "Receving error\n";
    string response = string(buffer);
    if (response.front() != '$') {
        cerr << "Error: Failed to log in!" << endl;
        return false;
    } else {
        cout << "Successfully log in" << endl;
        cout << "Server response: " << response << endl;
        _loggedin = true;
        _name = name;
    }
    
    return true; 
}

bool Client::list() {
    if (!isConnected()) return false;
    cout << "Require the list..." << endl;
    char msg[] = "List\n";
    send(_sock, msg, sizeof(msg), 0);

    char buffer[256] = {0};
    if (recv(_sock, buffer, sizeof(buffer), 0) < 1) return false;
    cout << "Server response: " << buffer;
    return true;
}

bool Client::pay(string name, int amount) {
    if (!isConnected()) return false;
    if (!_loggedin) {
        cerr << "Error: Please first log in\n";
        return false;
    }
    cout << "Pay $" << amount <<  " to " << name << endl;
    string payInfo = _name + "#" + to_string(amount) + "#" + name + "\n";
    char msg[256];
    strncpy(msg, payInfo.c_str(), payInfo.size());
    msg[payInfo.size()] = 0;
    return (send(_sock, msg, payInfo.size(), 0));
}

bool Client::disconnect() {
    if (!isConnected()) return false;
    char msg[] = "Exit\n";
    char buffer[256] = {0};
    if (send(_sock, msg, sizeof(msg), 0) && recv(_sock, buffer, sizeof(buffer), 0)) {
        cout << "Disconnected" << endl;
        cout << "Server response: " << buffer << endl;
        _connected = false;
        _loggedin = false;
        _name = "";
        return true;
    }
    return false;
}

void Client::help() {
    cout << "Usage: (characters in lower cases are dispensable)\n" 
    << left << setw(30) << "• Connect [ip] [port]" << "connect to [ip]:[port] (default 127.0.0.1:8080)\n"
    << left << setw(30) << "• Register <name> <amount>" << "register a user with name <name> and balance <amount>\n" 
    << left << setw(30) << "• LOgin <name> <port>" << "login with name <name> and port <port>\n"
    << left << setw(30) << "• Pay <user name> <amount>" << "pay <user name> <amount> dollars\n"
    << left << setw(30) << "• LIst" << "list all online users\n" 
    << left << setw(30) << "• Help" << "list all available commands\n"
    << left << setw(30) << "• Disconnect" << "close the currect connection\n" 
    << left << setw(30) << "• Ctrl-D" << "disconnect and quit" << endl;
}

int myStrNCmp(const string& cmd, const string& input, unsigned n)
{
   assert(n > 0);
   unsigned n2 = input.size();
   if (n2 == 0) return -1;
   unsigned n1 = cmd.size();
   assert(n1 >= n);
   for (unsigned i = 0; i < n1; ++i) {
      if (i == n2)
         return (i < n)? 1 : 0;
      char ch1 = (isupper(cmd[i]))? tolower(cmd[i]) : cmd[i];
      char ch2 = (isupper(input[i]))? tolower(input[i]) : input[i];
      if (ch1 != ch2)
         return (ch1 - ch2);
   }
   return (n1 - n2);
}

#endif // CLIENT_H