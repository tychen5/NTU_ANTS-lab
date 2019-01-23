#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <cassert>
#include "common.h"
using namespace std;

void help();
bool build_connection(int&, const char*, int);
void client_register(const int&);
void client_login(const int&, bool&);
void client_list(const int&);
void client_exit(const int&);

static int myPort = 12345;
static string serverIP = "127.0.0.1";
static int serverPort = 12340;
const string serverCloseMsg = "Server is closed.\n";

int main(int argc, char** argv) {
    if (argc != 3) {
		cerr << "Usage: ./client <string serverIP> <int serverPort>\n"; exit(-1);
    }
	try { serverPort = stoi(argv[2]); }
	catch(const exception& e) {
		cerr << "Usage: ./client <string serverIP> <int serverPort>\n"; exit(-1);
	}
    serverIP = argv[1];

    int sockfd = 0;
    if (!build_connection(sockfd, argv[1], serverPort)) exit(-1);
    else {
    	cout << "Client: Connection success." << endl;
    }

    help();
    bool loggedin = false;
    while (1) {
        cout << "cmd> "; cout.flush();
        string buf;
        getline(cin, buf);
        if (buf == "register") {
            client_register(sockfd);
        } else if (buf == "login") {
            if (loggedin) {
                cout << "\nYou've already logged in.\n" << endl; continue;
            }
            client_login(sockfd, loggedin);
        } else if (buf == "list") {
            if (!loggedin) {
                cout << "\nPlease log in first\n" << endl; continue;
            }
            client_list(sockfd);
        } else if (buf == "exit") {
            client_exit(sockfd);
        } else if (buf == "help") {
            help();
        } else {
            cerr << "Invalid command!\n" << endl;
        }
    }

    return 0;
}

void help() {
    cout << "\nValid commands:" << endl;
    cout << "(register, login, list, exit, help)\n" << endl;
}

bool build_connection(int &sockfd, const char* address, int port) {
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		cerr << "Client: Failed to create socket\n"; return false;
	}

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(address);

    if (connect(sockfd, (struct sockaddr* )&serv_addr, sizeof(serv_addr)) < 0) {
    	cerr << "Client: Connection failed!\n"; 
        return false;
    }

    char buf[MESSAGE_LEN] = {0};
    if (recv(sockfd, buf, sizeof(buf), 0) == 0) {
        cerr << serverCloseMsg; exit(-1);
    }
    if (strcmp(buf, "ACK\n") == 0) {
        return true;
    }
    else {
        cerr << buf << endl; return false;
    }
}

int getBalance() {
    do {
        cout << "Enter initial balance: "; cout.flush();
        int ret = 0;
        string buf;
        getline(cin, buf);
        try {
            ret = stoi(buf);
            if (ret < 0)
                cout << "Please enter valid number.\n";
            else
                return ret;
        }
        catch (const exception& e) {
            cout << "Please enter valid number.\n";
        }
    } while (true);
}

void client_register(const int& sockfd) {
    cout << "Enter username: "; cout.flush();
    string buf;
    getline(cin, buf);
    if (buf == "") {
        cout << "\nUsername must not be empty\n" << endl; return;
    }

    buf = "REGISTER#" + buf + "\n";
    send(sockfd, buf.c_str(), strlen(buf.c_str()), 0);

    // send initial balance;
    int balance = getBalance();
    char num[INT_STR_LEN] = {0};
    sprintf(num, "%d", balance);
    send(sockfd, num, strlen(num), 0);

    char recvMes[MESSAGE_LEN] = {0};
    if (recv(sockfd, recvMes, sizeof(recvMes), 0) == 0) {
        cerr << serverCloseMsg; exit(-1);
    }
    if (strcmp(recvMes, "100 OK\n") == 0) {
        cout << "\nRegister successful!\n" << endl;
    }
    else if (strcmp(recvMes, "210 FAIL\n") == 0) {
        cout << "\nThe username is already used!\n" << endl;
    }
    else {
        cerr << "Undefined message in client_register: " << recvMes << endl; 
        exit(-1);
    }
}

int getPortNum() {
    do {
        cout << "Enter your port number (1024 ~ 65535): "; cout.flush();
        int ret = 0;
        string buf;
        getline(cin, buf);
        try {
            ret = stoi(buf);
            if (ret < 1024 || ret > 65535)
                cout << "Please enter valid port number.\n";
            else
                return ret;
        }
        catch (const exception& e) {
            cout << "Please enter valid port number.\n";
        }
    } while (true);
}

void client_login(const int& sockfd, bool& loggedin) {
    cout << "Enter username: "; cout.flush();
    string buf;
    getline(cin, buf);
    if (buf == "") {
        cout << "\nUsername must not be empty\n" << endl; return;
    }
    // get client port
    myPort = getPortNum();

    buf += "#"; buf += to_string(myPort); buf += "\n";
    send(sockfd, buf.c_str(), strlen(buf.c_str()), 0);

    char recvMes[MESSAGE_LEN] = {0};
    if (recv(sockfd, recvMes, sizeof(recvMes), 0) == 0) {
        cerr << serverCloseMsg; exit(-1);
    }
    if (strcmp(recvMes, "220 AUTH_FAIL\n") == 0) {
        cout << "\nLogin fail! Account hasn't been registered or has logged in\n" << endl;
        return;
    }
    else {
        cout << "\nLogin successful!\n";
        string data = recvMes;
        data.insert(0, "Account balance: ");
        size_t p = data.find("\n");
        data.insert(p+1, "Online user number: ");
        p = data.find("\n", p+1);
        data.insert(p+1, "Online clients info:\n");

        cout << data << endl;
    }
    loggedin = true;
}

void client_list(const int& sockfd) {
    const char* cmd = "List\n";
    send(sockfd, cmd, strlen(cmd), 0);

    char recvMes[MESSAGE_LEN] = {0};
    if (recv(sockfd, recvMes, sizeof(recvMes), 0) == 0) {
        cerr << serverCloseMsg; exit(-1);
    }
    if (strcmp(recvMes, "220 AUTH_FAIL\n") == 0) {
        cout << "\nPlease log in first\n" << endl;
    }
    else {
        string data = recvMes;
        data.insert(0, "Account balance: ");
        size_t p = data.find("\n");
        data.insert(p+1, "Online user number: ");
        p = data.find("\n", p+1);
        data.insert(p+1, "Online clients info:\n");

        cout << data << endl;
    }
}

void client_exit(const int& sockfd) {
    const char* cmd = "Exit\n";
    send(sockfd, cmd, strlen(cmd), 0);

    char recvMes[MESSAGE_LEN] = {0};
    if (recv(sockfd, recvMes, sizeof(recvMes), 0) == 0) {
        cerr << serverCloseMsg; exit(-1);
    }
    cout << recvMes << endl;

    close(sockfd);
    exit(0);
}
