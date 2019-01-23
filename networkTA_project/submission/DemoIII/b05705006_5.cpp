#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <cassert>
#include <thread>
#include "common.h"
using namespace std;

void help();
bool build_connection(int&, const char*, int);
void client_register(const int&);
void client_login(const int&, bool&);
void client_list(const int&);
void client_pay(const int&);
void client_exit(const int&);
bool connect_receiver(const string& recvName, const char* recvIP, const int recvPort, const int payAmount);
void wait_sender(int portNum, int serverfd);

static int myPort = 12345;
static string serverIP = "127.0.0.1";
static int serverPort = 12340;
const string serverCloseMsg = "Server is closed.\n";
string myAccountName = "";
bool debug = false;

int main(int argc, char** argv) {
    if (argc != 3) {
		cerr << "Usage: ./client <string serverIP> <int serverPort>\n"; exit(-1);
    }
	try { serverPort = stoi(argv[2]); }
	catch(const exception& e) {
		cerr << "Usage: ./client <string serverIP> <int serverPort>\n"; exit(-1);
	}
    serverIP = argv[1];

    int serverfd = 0;
    if (!build_connection(serverfd, argv[1], serverPort)) exit(-1);
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
            client_register(serverfd);
        } 
        else if (buf == "login") {
            if (loggedin) {
                cout << "\nYou've already logged in.\n" << endl;
            }
            else {
                client_login(serverfd, loggedin);
            }
        } 
        else if (buf == "list") {
            if (!loggedin) {
                cout << "\nPlease log in first\n" << endl;
            }
            else {
                client_list(serverfd);
            }
        } 
        else if (buf == "pay") {
            if (!loggedin) {
                cout << "\nPlease log in first\n" << endl;
            }
            else {
                client_pay(serverfd);
            }
        }
        else if (buf == "exit") {
            client_exit(serverfd);
        }
        else if (buf == "help") {
            help();
        }
        else {
            cerr << "Invalid command!\n" << endl;
        }
    }

    return 0;
}

void help() {
    cout << "\nValid commands:" << endl;
    cout << "(register, login, list, pay, exit, help)\n" << endl;
}

bool build_connection(int &serverfd, const char* address, int port) {
	serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverfd == -1) {
		cerr << "Client: Failed to create socket\n"; return false;
	}

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(address);

    if (connect(serverfd, (struct sockaddr* )&serv_addr, sizeof(serv_addr)) < 0) {
    	cerr << "Client: Connection failed!\n"; 
        return false;
    }

    char buf[MESSAGE_LEN] = {0};
    if (decrypted_recv(serverfd, buf, sizeof(buf), 0) == 0) {
        cerr << serverCloseMsg; return false;
    }
    if (strcmp(buf, "ACK\n") == 0) {
        return true;
    }
    else {
        cerr << buf << endl; return false;
    }
}

int getBalance(string prompt) {
    do {
        cout << prompt; cout.flush();
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

void client_register(const int& serverfd) {
    cout << "Enter username: "; cout.flush();
    string buf;
    getline(cin, buf);
    if (buf == "") {
        cout << "\nUsername must not be empty\n" << endl; return;
    }

    buf = "REGISTER#" + buf + "\n";
    encrypted_send(serverfd, buf.c_str(), strlen(buf.c_str()), 0);

    // send initial balance;
    int balance = getBalance("Enter initial balance: ");
    char num[INT_STR_LEN] = {0};
    sprintf(num, "%d", balance);
    encrypted_send(serverfd, num, strlen(num), 0);

    char recvMes[MESSAGE_LEN] = {0};
    if (decrypted_recv(serverfd, recvMes, sizeof(recvMes), 0) == 0) {
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

void client_login(const int& serverfd, bool& loggedin) {
    cout << "Enter username: "; cout.flush();
    string userName, buf;
    getline(cin, userName);
    if (userName == "") {
        cout << "\nUsername must not be empty\n" << endl; return;
    }
    // get client port
    myPort = getPortNum();

    buf += userName; buf += "#"; buf += to_string(myPort); buf += "\n";
    encrypted_send(serverfd, buf.c_str(), strlen(buf.c_str()), 0);

    char recvMes[MESSAGE_LEN] = {0};
    if (decrypted_recv(serverfd, recvMes, sizeof(recvMes), 0) == 0) {
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
    myAccountName = userName;
    
    // create another thread to wait other clients' connection
    thread waitingThread(wait_sender, myPort, serverfd);
    waitingThread.detach();

    help();
}

void client_list(const int& serverfd) {
    const char* cmd = "List\n";
    encrypted_send(serverfd, cmd, strlen(cmd), 0);

    char recvMes[MESSAGE_LEN] = {0};
    if (decrypted_recv(serverfd, recvMes, sizeof(recvMes), 0) == 0) {
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

void client_pay(const int& serverfd) {
    cout << "Who do you want to pay? "; cout.flush();
    string receiver;
    getline(cin, receiver);
    if (receiver == "") {
        cout << "\nReceiver name must not be empty\n" << endl; return;
    }
    if (receiver == myAccountName) {
        cout << "\nYou can't pay yourself!\n" << endl; return;
    }
    int amount = getBalance("How much money? ");
    
    // First, ask server what's the IP and port of receiver
    // string msg = myAccountName + "#" + to_string(amount) + "#" + receiver + "\n"; // <myAccount>#<amount>#<receiverAccount>
    string msg = "FIND#"; msg += (receiver + "\n");
    if (debug) cerr << "[Debug] Sending: " << msg << endl;
    encrypted_send(serverfd, msg.c_str(), strlen(msg.c_str()), 0);

    // expecting: <receiver ip>#<receiver port>
    char recvMes[MESSAGE_LEN] = {0};
    if (decrypted_recv(serverfd, recvMes, sizeof(recvMes), 0) == 0) {
        cerr << serverCloseMsg; exit(-1);
    }
    if (recvMes[0] == 0) {
        cout << "\nNo online receiver: " << receiver << "\n\n"; return;
    }

    char* pch = strtok(recvMes, "#");
    const char* receiverIP = pch;
    pch = strtok(NULL, "#");
    int receiverPort = stoi(pch);

    // p2p connection
    // if (!connect_receiver(receiverIP, receiverPort, amount)) {
    //     // connection fail: receiver has logged out
    // }
    connect_receiver(receiver, receiverIP, receiverPort, amount);
}

void client_exit(const int& serverfd) {
    const char* cmd = "Exit\n";
    encrypted_send(serverfd, cmd, strlen(cmd), 0);

    char recvMes[MESSAGE_LEN] = {0};
    if (decrypted_recv(serverfd, recvMes, sizeof(recvMes), 0) == 0) {
        cerr << serverCloseMsg; exit(-1);
    }
    cout << recvMes << endl;

    close(serverfd);
    exit(0);
}

bool connect_receiver(const string& recvName, const char* recvIP, const int recvPort, const int payAmount) {
    if (debug) cerr << "[Debug] Connecting receiver: " << recvIP << ":" << recvPort << "\n\n";
    int receiverFd = 0;
    if (!build_connection(receiverFd, recvIP, recvPort)) {
        cout << "Fail to connect receiver. Transaction aborted.\n\n"; return false;
    }

    if (debug) cerr << "[Debug] connection succeed!\n\n";
    string msg = myAccountName + "#" + to_string(payAmount) + "#" + recvName + "\n";
    if (debug) cerr << "[Debug] Sending msg: " << msg << " to receiver\n\n";
    encrypted_send(receiverFd, msg.c_str(), strlen(msg.c_str()), 0);

    // if ACK, transaction succeed; otherwise means I don't have enough money
    char recvMes[MESSAGE_LEN] = {0};
    if (decrypted_recv(receiverFd, recvMes, MESSAGE_LEN, 0) == 0) {
        cout << "Receiver disconnected. Transaction aborted.\n\n"; return false;
    }
    if (strcmp(recvMes, "ACK\n") == 0) {
        cout << "\nTransaction completed.\n\n";
    }
    else if (strcmp(recvMes, "NAK\n") == 0) {
        cout << "\nYour balance is not enough. Transaction aborted.\n\n";
    }

    close(receiverFd);
    return true;
}

void wait_sender(int portNum, int serverfd) {
    int sockfd = 0, forClientSockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        cerr << "In waiting sender: Fail to create socket\n\n"; return;
    }
    struct sockaddr_in serverInfo, clientInfo;
    socklen_t addrlen = sizeof(clientInfo);
    memset(&serverInfo, 0, sizeof(serverInfo));
	serverInfo.sin_family = AF_INET;
	serverInfo.sin_addr.s_addr = INADDR_ANY;
	serverInfo.sin_port = htons(portNum);
	bind(sockfd, (struct sockaddr* )&serverInfo, sizeof(serverInfo));
	listen(sockfd, 3);
    if (debug) cerr << "[Debug] Starts waiting sender on port: " << portNum << "\n\n";
    while (true) {
        forClientSockfd = accept(sockfd, (struct sockaddr* )&clientInfo, &addrlen);
        if (debug) {
			char cbuf[INET_ADDRSTRLEN];
			const char* addr = inet_ntop(AF_INET, &(clientInfo.sin_addr), cbuf, INET_ADDRSTRLEN);
			cerr << "[Debug] Receive sender connection from address: " << addr << ", port: " 
				 << clientInfo.sin_port << endl;            
        }

        encrypted_send(forClientSockfd, "ACK\n", 4, 0);
        // expecting: <senderName>#<amount>#<myName>
        char recvMes[MESSAGE_LEN] = {0};
        if (decrypted_recv(forClientSockfd, recvMes, MESSAGE_LEN, 0) == 0) {
            if (debug) cerr << "A transaction sender disconnected.\n\n";
            continue;
        }
        if (debug) cerr << "Receiving msg from sender: " << recvMes << "\n\n";
        
        // forward message to server to update the balance of both account
        encrypted_send(serverfd, recvMes, strlen(recvMes), 0);
        memset(recvMes, 0, MESSAGE_LEN);
        // if the response is NAK, means sender doesn't have enough money, transaction aborted
        // if ACK, transaction commitited
        if (decrypted_recv(serverfd, recvMes, MESSAGE_LEN, 0) == 0) {
            cerr << "Server is closed. Aborting thread...\n";
            return;
        }
        if (debug) cerr << "Receiving msg from server: " << recvMes << "\n\n";
        if (strcmp(recvMes, "ACK\n") == 0) {
            encrypted_send(forClientSockfd, "ACK\n", 4, 0);
        }
        else if (strcmp(recvMes, "NAK\n") == 0) {
            encrypted_send(forClientSockfd, "NAK\n", 4, 0);
        }
        else {
            cerr << "WTF is that, server: " << recvMes << endl;
        }
    }
}
