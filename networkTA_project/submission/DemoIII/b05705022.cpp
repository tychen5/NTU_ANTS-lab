#include <iostream>
#include <arpa/inet.h>
#include <openssl/err.h>
#include <cstring>
#include <ctime>
#include "server.h"
#include "util.h"

using namespace std;

Server* server;

/********************** helper functions *********************/ 

void handleWrapper(int clientFD, sockaddr_in* info) {
    server->handle(clientFD, info);
}


/***************** Server's member functions *****************/

Server::Server(): _pool(new ThreadPool(MAX_CLIENTS)) {
    // initialize thread pool
    _pool->init();

    // create socket
    if ((_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Fail to create a socket\n";
    }

    memset(&_addr, '0', sizeof(_addr));
    _addr.sin_family = AF_INET;
    _addr.sin_addr.s_addr = INADDR_ANY; // inet_addr(IP);
    _addr.sin_port = htons(PORT);
    bind(_sock, (struct sockaddr*)&_addr, sizeof(_addr));
    listen(_sock, MAX_CLIENTS);

    // init ctx and ssl
    OpenSSL_add_ssl_algorithms();
    SSL_load_error_strings();
    _ctx = initServerCTX("servCert.pem", "servKey.pem");
}

Server::~Server() {
    close(_sock); 
    _pool->shutdown(); 
    delete _pool; 
    for (auto& u: _users) delete u;
    EVP_cleanup();
    SSL_CTX_free(_ctx);
}

void Server::run() {
    bool running = true;
    // thread catchEnd(catchInput, ref(_sock), ref(running));
    cout << "Waiting for connection..." << endl;
    while (running) {
        struct sockaddr_in client_addr;
        unsigned addrlen = sizeof(client_addr);
        int client_sock = accept(_sock, (struct sockaddr*)&client_addr, &addrlen);
        if (running) _pool->submit(handleWrapper, client_sock, &client_addr);
    }
}

void Server::handle(int clientFD, sockaddr_in* info) {
    ClientInfo* client = 0;

    // init ssl
    SSL* ssl = SSL_new(_ctx);
    
    SSL_set_fd(ssl, clientFD);
    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        return;
    }
    #ifdef VERBOSE
    showCerts(ssl);
    #endif // VERBOSE
    char msg[] = "Connection accepted";
    if (mySSL_write(ssl, msg, strlen(msg)) < 0)
        ERR_print_errors_fp(stderr);

    // handle client address
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &info->sin_addr, ip, INET_ADDRSTRLEN);
    cout << "[Info] New connection from " << ip << ":" << info->sin_port << endl;

    char buf[256] = {0};
    bool status = true;
    while (status) {
        memset(buf, 0, sizeof(buf));
        mySSL_read(ssl, buf, sizeof(buf));        
        string request(buf);

        if (request.compare(0, 8, "REGISTER") == 0) { // register
            size_t s = request.find_first_of('#');
            size_t m = request.find_first_of('#', s+1);
            size_t e = request.find_first_of('\n');
            string name = request.substr(s+1, m-s-1);
            int amount = stoi(request.substr(m+1, e-s-1));

            if (findUser(name)) {
                cout << "[Warn] User " << name << " exists!" << endl;
                char msg[] = "200 FAIL\n";
                mySSL_write(ssl, msg, sizeof(msg));
            } else {
                char msg[] = "100 OK\n";
                mySSL_write(ssl, msg, sizeof(msg));
                client = new ClientInfo(name, string(ip), amount);
                _users.push_back(client);
                cout << "[Info] New registration from " << name << endl;
            }
        }
        else if (request.compare(0, 4, "List") == 0) { // list
            if (!client || !(client->_hasLoggedin)) {
                char msg[] = "Please register and log in\n";
                mySSL_write(ssl, msg, sizeof(msg));
                cout << "[Warn] Client ID " << client << " requested list without login" << endl; 
                continue;
            } else {
                string bal = "$" + to_string(client->_balance) + "\n";
                char msg[256];
                string res = (bal + list());
                strncpy(msg, res.c_str(), res.size());
                msg[res.size()] = 0;
                mySSL_write(ssl, msg, sizeof(msg));
                cout << "[Info] " << client->_name << " required list" << endl; 
            }
        }
        else if (request.compare(0, 4, "Exit") == 0) { // close
            status = false;
            cout << "[Info] ";
            if (!client) cout << "Client ID " << clientFD;
            else         cout << client->_name;
            cout << " closed conncection" << endl;
            if (client) {
                client->_hasLoggedin = false;
                client->_isOnline = false;
            }
            char msg[] = "Bye\n";
            mySSL_write(ssl, msg, sizeof(msg));
        }
        else if (request.compare(0, 3, "Get") == 0) {
            string name = request.substr(request.find_first_of('#')+1);
            ClientInfo* user = findUser(name);
            if (!user) {
                mySSL_write(ssl, "NotFound", 8);
            } else if (!user->_hasLoggedin) {
                mySSL_write(ssl, "NotLogin", 8);
            }
            else {
                string msg = user->_ip + "#" + to_string(user->_port);
                mySSL_write(ssl, msg.c_str(), msg.size());
            }
        }
        else {
            size_t delim1 = request.find_first_of('#');
            if (delim1 == string::npos) {
                report_unknown(ssl, clientFD, client, request);
                continue;
            }
            size_t delim2 = request.find_first_of('#', delim1+1);
            bool isPayment = (delim2 != string::npos);
            int num = 0;
            // bool cast = true;
            try { 
                if (isPayment) num = stoi(request.substr(delim1+1, delim2-delim1));
                else           num = stoi(request.substr(delim1+1, request.size()-delim1-1)); 
            }
            catch (const invalid_argument& ia) { 
                report_unknown(ssl, clientFD, client, request);
                continue;
            }

            if (isPayment) { // payment
                bool usersExist = true;
                ClientInfo* payer, * payee;
                if ((payer = findUser(request.substr(0, delim1))) == 0) {
                    usersExist = false;
                    cout << "[Warn] Payer " << request.substr(0, delim1) << " not found" << endl;
                    mySSL_write(ssl, "PayFail", 7);
                }
                if ((payee = findUser(request.substr(delim2+1, request.size()-delim2-2))) == 0) {
                    usersExist = false;
                    cout << "[Warn] Payee " << request.substr(delim2+1, request.size()-delim2-2)
                         << " not found" << endl;
                    mySSL_write(ssl, "PayFail", 7);
                }
                if (usersExist) {
                    if (payer->_balance < num) {
                        cout << "[Warn] Balance in " << payer->_name << "'s account is not enough" << endl;
                        mySSL_write(ssl, "PayFail", 7);
                    }
                    else if (!payee->_hasLoggedin) {
                        cout << "[Warn] Payee " << payee->_name << " is offline" << endl;
                        mySSL_write(ssl, "PayFail", 7);
                    }
                    else {
                        payer->_balance -= num;
                        payee->_balance += num;
                        cout << "[Info] " << payer->_name << " pays $" << num << " to " << payee->_name << endl;
                        mySSL_write(ssl, "PaySucs", 11);
                    }
                }
            } 
            else { // log in
                string name = request.substr(0, delim1);
                client = findUser(name);
                if (client == 0) { // authentication fail
                    char msg[] = "220 AUTH_FAIL\n";
                    mySSL_write(ssl, msg, strlen(msg));
                    cout << "[Warn] Client ID " << client 
                         << " tried to log in without registration." << endl;
                }
                else { // authentication success
                    if (client->_hasLoggedin) {
                        char msg[] = "You have already logged in\n";
                        mySSL_write(ssl, msg, strlen(msg));
                        cout << "[Warn] " << client->_name
                             << " tried to log in again" << endl; 
                        continue;
                    }
                    client->_hasLoggedin = true;
                    client->_port = num;
                    string bal = "$" + to_string(client->_balance) + "\n";
                    char msg[256] = {0};
                    string res = (bal + list());
                    strncpy(msg, res.c_str(), res.size());
                    mySSL_write(ssl, msg, strlen(msg));
                    cout << "[Info] " << name << " logged in" << endl; 
                }
            }
        }
    }
    SSL_free(ssl);
}

ClientInfo* Server::findUser(string name) const {
    for (auto &user: _users) {
        if (user->_name == name)
            return user;
    }
    return 0;
}

string Server::list() const {
    size_t live_users = 0;
    string userInfo;
    for (auto const &user: _users) {
        if (user->_hasLoggedin) {
            ++live_users;
            userInfo += user->_name + "#" + user->_ip + "#" + to_string(user->_port) + "\n";
        }
    }
    string res = to_string(live_users) + " user" + (live_users > 1? "s":"") + " online\n" + userInfo;
    return res;
}

void Server::report_unknown(SSL* ssl, int clientFD, ClientInfo* client, string& request) {
    char msg[] = "WTF are you doing?\n";
    mySSL_write(ssl, msg, sizeof(msg));
    cout << "[Info] ";
    if (!client) cout << "Client ID " << clientFD;
    else         cout << client->_name;
    cout << " sent unidentified message: " << request << endl;
}


int main(int argc, char* argv[]) {   
    server = new Server();
    server->run();
    return 0;
}
