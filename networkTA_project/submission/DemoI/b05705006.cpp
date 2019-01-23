#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <cassert>
using namespace std;

#define MESSAGE_LEN 256

void help();
void get_ip_and_port(string&, int&);
bool build_connection(int&, const char*, unsigned short);
void client_register(const int&);
void client_login(const int&);
void client_list(const int&);
void client_exit(const int&);

int main() {

    int sockfd = 0, port = 0;
    string addr;
    get_ip_and_port(addr, port);

    if (!build_connection(sockfd, addr.c_str(), port)) {
    // if (!build_connection(sockfd, "140.112.107.194", 33120)) {
    // if (!build_connection(sockfd, "140.112.106.45", 33220)) {
    	cerr << "Client: Connection failed!\n"; exit(-1);
    }
    else {
    	cout << "Client: Connection success." << endl;
    }

    // start communication
    char recvMes[MESSAGE_LEN] = {0};
    if (recv(sockfd, recvMes, sizeof(recvMes), 0) == 0) {
        cerr << "Socket closed by server\n"; exit(-1);
    }  // "connection accepted"

    help();
    while (1) {
        string buf;
        getline(cin, buf);
        if (buf == "register") {
            client_register(sockfd);
        } else if (buf == "login") {
            client_login(sockfd);
        } else if (buf == "list") {
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

void get_ip_and_port(string& addr, int& port) {
    cout << "Please enter ip address\n";
    cin >> addr;
    cout << "Please enter port number\n";
    string s;
    cin >> s; cin.ignore();
    try {
        port = stoi(s);
    } catch (const exception& e) {
        cerr << "Invalid port number!" << endl;
        exit(-1);
    }
}

bool build_connection(int &sockfd, const char* address, unsigned short port) {
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		cerr << "Client: Failed to create socket\n"; return false;
	}

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(address);

    if (connect(sockfd, (struct sockaddr* )&serv_addr, sizeof(serv_addr)) < 0)
    	return false;

    return true;
}

void client_register(const int& sockfd) {
    cout << "Enter username: " << endl;
    string buf;
    cin >> buf; cin.ignore();
    buf = "REGISTER#" + buf + "\n";
    send(sockfd, buf.c_str(), strlen(buf.c_str()), 0);
    
    char recvMes[MESSAGE_LEN] = {0};
    if (recv(sockfd, recvMes, sizeof(recvMes), 0) == 0) {
        cerr << "Socket closed by server\n"; exit(-1);
    }
    if (strcmp(recvMes, "100 OK\n") == 0) {
        cout << "\nRegister successful\n" << endl;
    }
    else if (strcmp(recvMes, "210 FAIL\n") == 0) {
        cout << "\nRegister fail\n" << endl;
    }
    else {
        cerr << "Undefined message in client_register: " << recvMes << endl; 
        exit(-1);
    }
}

void client_login(const int& sockfd) {
    cout << "Enter username: " << endl;
    string buf;
    cin >> buf; cin.ignore();
    buf = buf + "#12345\n";
    send(sockfd, buf.c_str(), strlen(buf.c_str()), 0);

    char recvMes[MESSAGE_LEN] = {0};
    if (recv(sockfd, recvMes, sizeof(recvMes), 0) == 0) {
        cerr << "Socket closed by server\n"; exit(-1);
    }
    if (strcmp(recvMes, "220 AUTH_FAIL\n") == 0) {
        cout << "\nLogin fail\n" << endl;
    }
    else {
        cout << "\nLogin successful\n";
        string data = recvMes;
        // data = "Account balance: " + data;
        // size_t p = data.find("\n");
        // data.insert(p+1, "Online number: ");
        // p = data.find("\n", p+1);
        // data.insert(p+1, "Online clients data:\n");

        cout << data << endl;
    }
}

void client_list(const int& sockfd) {
    const char* cmd = "List\n";
    send(sockfd, cmd, strlen(cmd), 0);

    char recvMes[MESSAGE_LEN] = {0};
    if (recv(sockfd, recvMes, sizeof(recvMes), 0) == 0) {
        cerr << "Socket closed by server\n"; exit(-1);
    }
    if (strcmp(recvMes, "220 AUTH_FAIL\n") == 0) {
        cout << "\nPlease log in first\n" << endl;
    }
    else {
        string data = recvMes;
        // data = "Account balance: " + data;
        // size_t p = data.find("\n");
        // data.insert(p+1, "Online number: ");
        // p = data.find("\n", p+1);
        // data.insert(p+1, "Online clients data:\n");

        cout << data << endl;        
    }
}

void client_exit(const int& sockfd) {
    const char* cmd = "Exit\n";
    send(sockfd, cmd, strlen(cmd), 0);

    char recvMes[MESSAGE_LEN] = {0};
    if (recv(sockfd, recvMes, sizeof(recvMes), 0) == 0) {
        cerr << "Socket closed by server\n"; exit(-1);
    }

    close(sockfd);
    exit(0);
}
