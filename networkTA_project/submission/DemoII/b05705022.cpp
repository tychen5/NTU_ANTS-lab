#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <ctime>
#include "server.h"

using namespace std;

Server* server;

/********************** helper functions *********************/ 

void handleWrapper(int clientFD, sockaddr_in* info) {
    server->handle(clientFD, info);
}

void catchInput(int& sockSD, bool& status) {
    while (getchar() != EOF) {}
    status = false;
    try {
        shutdown(sockSD, 2);
    }
    catch (const exception&) {}
    close(sockSD);
}

int recvtimeout(int s, char *buf, int len, int timeout) {
    fd_set fds;
    int n;
    struct timeval tv;

    // set file descriptor
    FD_ZERO(&fds);
    FD_SET(s, &fds);

    // set struct timeval
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    // wait until receive or timeout
    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    // normal recv
    return recv(s, buf, len, 0);
}

/***************** Server's member functions *****************/

Server::Server(): _pool(new ThreadPool(MAX_CLIENTS)) {
    _pool->init();
    if ((_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Fail to create a socket\n";
    }

    memset(&_addr, '0', sizeof(_addr));
    _addr.sin_family = AF_INET;
    _addr.sin_addr.s_addr = inet_addr(IP);
    _addr.sin_port = htons(PORT);
    bind(_sock, (struct sockaddr*)&_addr, sizeof(_addr));
    listen(_sock, MAX_CLIENTS);
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
    char msg[] = "Connection accepted";
    send(clientFD, msg, sizeof(msg), 0);
    ClientInfo* client = 0;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &info->sin_addr, ip, INET_ADDRSTRLEN);
    cout << "[Info] New connection from " << ip << ":" << info->sin_port << endl;

    char buffer[256] = {};
    bool status = true;
    while (status) {
        memset(buffer, 0, sizeof buffer);

        // handle timeout
        if (recvtimeout(clientFD, buffer, sizeof(buffer), MAX_WAITING_TIME) == -2) {
            cout << "[Warn] Timeout! Closed connection from "  << ip << ":" << info->sin_port << endl;
            char msg[] = "Connection timeout";
            send(clientFD, msg, sizeof(msg), 0);
            if (client) { 
                client->_isOnline = false; 
                client->_hasLoggedin = false; 
            }
            break;
        }
        
        // cout << "Receive: " << buffer << endl;
        string request = buffer;
        if (request.compare(0, 8, "REGISTER") == 0) { // register
            size_t s = request.find_first_of('#');
            size_t m = request.find_first_of('#', s+1);
            size_t e = request.find_first_of('\n');
            string name = request.substr(s+1, m-s-1);
            int amount = stoi(request.substr(m+1, e-s-1));

            if (find_user(name)) {
                cout << "User " << name << " exists!" << endl;
                char msg[] = "200 FAIL\n";
                if (send(clientFD, msg, sizeof(msg), 0) < 0) cerr << "Sending error\n";
            } else {
                char msg[] = "100 OK\n";
                if (send(clientFD, msg, sizeof(msg), 0) < 0) cerr << "Sending error\n";
                ClientInfo* nc = new ClientInfo(name, string(ip), amount);
                _users.push_back(nc);
                cout << "[Info] New registration from " << name << endl;
            }
        }
        else if (request.compare(0, 4, "List") == 0) { // list
            if (!client || !(client->_hasLoggedin)) {
                char msg[] = "Please register and log in\n";
                if (send(clientFD, msg, sizeof(msg), 0) < 0) cerr << "Sending error\n";
                cout << "[Warn] Client ID " << client << " requested list without login" << endl; 
                continue;
            } else {
                string bal = "$" + to_string(client->_balance) + "\n";
                char msg[256];
                string res = (bal + list());
                strncpy(msg, res.c_str(), res.size());
                msg[res.size()] = 0;
                if (send(clientFD, msg, sizeof(msg), 0) < 0) cerr << "Sending error\n";
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
            if (send(clientFD, msg, sizeof(msg), 0) < 0) cerr << "Sennding error\n";
        }
        else {
            size_t delim1 = request.find_first_of('#');
            if (delim1 == string::npos) {
                report_unknown(clientFD, client, request);
                continue;
            }
            size_t delim2 = request.find_first_of('#', delim1+1);
            bool isPayment = (delim2 != string::npos);
            int num = 0;
            bool cast = true;
            try { 
                if (isPayment) num = stoi(request.substr(delim1+1, delim2-delim1));
                else           num = stoi(request.substr(delim1+1, request.size()-delim1-1)); 
            }
            catch (const invalid_argument& ia) { 
                report_unknown(clientFD, client, request);
                continue;
            }

            if (isPayment) { // payment
                bool usersExist = true;
                ClientInfo* payer, * payee;
                if ((payer = find_user(request.substr(0, delim1))) == 0) {
                    usersExist = false;
                    cout << "[Warn] Payer " << request.substr(0, delim1) << " not found\n";
                }
                if ((payee = find_user(request.substr(delim2+1, request.size()-delim2-2))) == 0) {
                    usersExist = false;
                    cout << "[Warn] Payee " << request.substr(delim2+1, request.size()-delim2-2)
                         << " not found\n";
                }
                if (usersExist) {
                    payer->_balance -= num;
                    payee->_balance += num;
                    cout << "[Info] " << payer->_name << " pays $" << num << " to " << payee->_name << endl;
                }
            } 
            else { // log in
                string name = request.substr(0, delim1);
                client = find_user(name);
                if (client == 0) { // authentication fail
                    char msg[] = "220 AUTH_FAIL\n";
                    if (send(clientFD, msg, sizeof(msg), 0) < 0) cerr << "Sennding error\n";
                    cout << "[Warn] Client ID " << client 
                         << " tried to log in without registration." << endl;
                }
                else { // authentication success
                    if (client->_hasLoggedin) {
                        char msg[] = "You have already logged in\n";
                        if (send(clientFD, msg, sizeof(msg), 0) < 0) cerr << "Sennding error\n";
                        cout << "[Warn] " << client->_name
                             << " tried to log in again" << endl; 
                        continue;
                    }
                    client->_hasLoggedin = true;
                    client->_port = num;
                    string bal = "$" + to_string(client->_balance) + "\n";
                    char msg[256];
                    string res = (bal + list());
                    strncpy(msg, res.c_str(), res.size());
                    msg[res.size() - 1] = 0;
                    if (send(clientFD, msg, sizeof(msg), 0) < 0) cerr << "Sending error\n";
                    cout << "[Info] " << name << " logged in" << endl; 
                }
            }
        }
    }
}

ClientInfo* Server::find_user(string name) const {
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

void Server::report_unknown(int clientFD, ClientInfo* client, string& request) {
    char msg[] = "WTF are you doing?\n";
    if (send(clientFD, msg, sizeof(msg), 0) < 0) cerr << "Sending error\n";
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
