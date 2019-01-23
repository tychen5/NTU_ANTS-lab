#include <iostream>
#include <iomanip>
#include <cstring>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cassert>
#include <openssl/err.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "cmdParser.h"
#include "client.h"
#include "util.h"
using namespace std;

mutex mtx;
condition_variable cv;


void listenForPayment(int& sock, int port, bool& run, int file, string& pipe);

/* Public function */

Client::Client(): _ip(0), _port(0), _sock(0), _ctx(0), _ssl(0),
                  _run(true), _bgServ(0), _connected(false), _loggedin(false) {
    help(); 
    cout << endl;

    // init ssl
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
}

Client::~Client() {
    close(_sock);
    close(_bgSock);
    if (_ssl) SSL_free(_ssl);
    if (_ctx) SSL_CTX_free(_ctx);
    _run = false;
    if (_bgServ) _bgServ->join(); 
}

void Client::handle(string cmd) {
    stringstream ss(cmd);
    string request;
    if (ss >> request) {
        if (myStrNCmp("connect", request, 1) == 0) {
            string ip;
            int port;
            if (!(ss >> ip >> port)) { ip = IP; port = PORT; };
            connectServer(ip, port);
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
            if (port < 1024 || port > 65535) { cerr << "Error: Invalid port number\n"; return; }
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
        else if (myStrNCmp("send", request, 1) == 0) {
            mySSL_write(_ssl, _payment.c_str(), _payment.size());
            char buf[32] = {0};
            mySSL_read(_ssl, buf, sizeof(buf));
            if (string(buf) == "PaySucs") {
                cout << "Payment completed" << endl;
                _payment = "Success";
            }
            else { 
                cerr << "Payment failed\n";
                _payment = "Fail";
            }
            cv.notify_all();
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

bool Client::disconnect() {
    if (!_connected) return false;
    char buf[256] = {0};
    if (mySSL_write(_ssl, "Exit\n", 5) && mySSL_read(_ssl, buf, sizeof(buf))) {
        cout << "Disconnected" << endl;
        _connected = false;
        _loggedin = false;
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

/* Private function */

bool Client::isConnected() {
    if (!_connected) {
        cout << "Error: Please first connect to a server\n";
        return false;
    }
    return true;
}


bool Client::connectServer(string ip, int port) {
    if (_connected) {
        cout << "Error: Now connected to " << _ip << ":" << _port
             << "\nReplace current connection? [y/n]: ";
        cout.flush();
        char c[5];
        cin.getline(c, 2);
        if (strcmp(c, "n") == 0) return false; 
    }
    // create socket
    _ip = &ip[0];
    _port = port;
    cout << "Connecting to " << _ip << ":" << _port << " ..." << endl;

    struct sockaddr_in sa_dst;
    _sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&sa_dst, 0, sizeof(struct sockaddr_in));
    sa_dst.sin_family = AF_INET;
    sa_dst.sin_port = htons(_port);
    sa_dst.sin_addr.s_addr = inet_addr(_ip);

    if (connect(_sock, (struct sockaddr *)&sa_dst, sizeof(sa_dst)) < 0) {
        cout << "Error: Connecting failed\n";
        return false;
    }
    
    _ctx = initClientCTX(_file);
    if (!_ctx) return false;
    _ssl = SSL_new(_ctx);
    SSL_set_fd(_ssl, _sock);
    if (SSL_connect(_ssl) <= 0) {
        cerr << "Error: SSL connection failed\n";
        ERR_print_errors_fp(stderr);
        return false;
    }
    #ifdef VERBOSE
    showCerts(_ssl);
    #endif // VERBOSE

    char buf[100];
    mySSL_read(_ssl, buf, sizeof(buf));

    if (string(buf) == "Connection accepted") {
        cout << "Successfully connected!" << endl;
        _connected = true;
        return true;
    }
    return false;
}

bool Client::reg(string name, int amount) {
    if (!isConnected()) return false;

    cout << "Registering user \"" << name << "\"..." << endl;

    string msg = "REGISTER#" + name + "#" + to_string(amount) + "\n";
    if (mySSL_write(_ssl, msg.c_str(), msg.size()) < 0) return false;

    // read response
    char buf[256] = {0};
    mySSL_read(_ssl, buf, sizeof(buf));
    string response = string(buf);
    if (response == "100 OK\n") {
        cout << "Successfully Resistered!" << endl;
        return true;
    } else {
        cout << "Registered failed!" << endl;
    }
    return false;
}

bool Client::login(string name, int port) {
    if (!isConnected()) return false;
    
    cout << "Require to login..." << endl;
    string msg = name + "#" + to_string(port) + "\n";
    if (mySSL_write(_ssl, msg.c_str(), msg.size()) < 0) return false;

    // read response
    char buf[256] = {0};
    mySSL_read(_ssl, buf, sizeof(buf));
    string response = string(buf);
    if (response.front() != '$') {
        cerr << "Error: Failed to log in!" << endl;
        return false;
    } else {
        cout << "Successfully log in" << endl;
        cout << "Server response: " << response << endl;
        _loggedin = true;
        _name = name;
    }
    _bgServ = new thread(listenForPayment, ref(_bgSock), port, ref(_run), _file, ref(_payment));
    return true; 
}

bool Client::list() {
    if (!isConnected()) return false;
    cout << "Require the list..." << endl;
    if (mySSL_write(_ssl, "List\n", 5) < 0) return false;

    char buf[256] = {0};
    if (mySSL_read(_ssl, buf, sizeof(buf)) < 0) return false;
    cout << "Server response: " << buf;
    return true;
}

bool Client::pay(string name, int amount) {
    if (!isConnected()) return false;
    if (!_loggedin) {
        cerr << "Error: Please first log in\n";
        return false;
    }
    cout << "Pay $" << amount <<  " to " << name << endl;

    // get the target address from server
    string msg = "Get#" + name;
    mySSL_write(_ssl, msg.c_str(), msg.size());
    char buf[64] = {0};
    mySSL_read(_ssl, buf, sizeof(buf));
    msg = string(buf);
    if (msg == "NotLogin") {
        cerr << "Error: the user didn't log in\n";
        return false;
    }
    else if (msg == "NotFound") {
        cerr << "Error: No such user\n";
        return false;
    }
    string ip = msg.substr(0, msg.find_first_of('#'));
    int port = stoi(msg.substr(msg.find_first_of('#')+1));

    // disconnect from server
    disconnect();

    // connect to the target
    cout << "Connecting to " << name << " " << ip << ":" << port << " ..." << endl;

    struct sockaddr_in sa_dst;
    _sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&sa_dst, 0, sizeof(struct sockaddr_in));
    sa_dst.sin_family = AF_INET;
    sa_dst.sin_port = htons(port);
    sa_dst.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(_sock, (struct sockaddr *)&sa_dst, sizeof(sa_dst)) < 0) {
        cout << "Error: Connecting failed\n";
        return false;
    }
    
    // _ctx = initCTX();
    if (!_ctx) return false;
    // _ssl = SSL_new(_ctx);
    SSL_set_fd(_ssl, _sock);
    if (SSL_connect(_ssl) <= 0) {
        cerr << "Error: SSL connection failed\n";
        ERR_print_errors_fp(stderr);
        return false;
    }
    #ifdef VERBOSE
    showCerts(_ssl);
    #endif
    memset(buf, 0, sizeof(buf));
    mySSL_read(_ssl, buf, sizeof(buf));

    if (string(buf) == "Connection accepted") {
        cout << "Successfully connected!" << endl;
    }
    else {
        cerr << "Error: connection failed\n";
        return false;
    }

    // send the target payment information
    string payInfo = _name + "#" + to_string(amount) + "#" + name + "\n";
    if (mySSL_write(_ssl, payInfo.c_str(), payInfo.size()) < 0) return false;
    cout << "Waiting for reply..." << endl;
    
    memset(buf, 0, sizeof(buf));
    mySSL_read(_ssl, buf, sizeof(buf));
    if (strcmp(buf, "PaySucs") == 0) {
        cout << "Payment completed" << endl;
    }
    else {
        cout << "Payment failed" << endl;
    }
    return true;
}

/********************     Helper Function     ********************/


void listenForPayment(int& sock, int port, bool& run, int file, string& pipe) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, '0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(sock, 5);

    // init ctx and ssl
    OpenSSL_add_ssl_algorithms();
    SSL_load_error_strings();

    char mcert[20] = {0};
    char mkey[20] = {0};
    if (file == 1) { strcpy(mcert, "cliCert.pem"); strcpy(mkey, "cliKey.pem"); }
    else           { strcpy(mcert, "cli2Cert.pem"); strcpy(mkey, "cli2Key.pem"); }
    SSL_CTX* ctx = initServerCTX(mcert, mkey);
    assert(ctx != 0);
    
    while (run) {
        struct sockaddr_in client_addr;
        unsigned addrlen = sizeof(client_addr);
        int clientFD = accept(sock, (struct sockaddr*)&client_addr, &addrlen);

        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, clientFD);
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            return;
        }
        char msg[] = "Connection accepted";
        mySSL_write(ssl, msg, strlen(msg));

        // handle client address
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
        cout << "\n[Info] New connection from " << ip << ":" << client_addr.sin_port << endl;
        #ifdef VERBOSE
        showCerts(ssl);
        #endif

        char buf[256] = {0};
        mySSL_read(ssl, buf, sizeof(buf));

        pipe = string(buf);
        cout << "[Info] Receive new payment! Use \"send\" to notify server\n\nClient> ";
        cout.flush();

        unique_lock<mutex> lck(mtx);
        cv.wait(lck);

        if (pipe == "Success") mySSL_write(ssl, "PaySucs", 7);
        else mySSL_write(ssl, "PayFail", 7);

        SSL_free(ssl);
    }
    SSL_CTX_free(ctx);
}


int main(int argc, char** argv) {
   auto cmd = CmdParser<Client>();
   cmd.readCmd();
   cout << endl;
   return 0;
}
