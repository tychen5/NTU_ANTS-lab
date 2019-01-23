#define __USE_MINGW_ANSI_STDIO 0

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <Winsock2.h>
#include <thread>
#include <chrono>
#include <mutex>

#define MAX_BUFFER_SIZE 1024

using namespace std;

mutex send_mutex;	// mutex for sending to server

class Client
{
private:
	SOCKET _sock;
	SOCKADDR_IN _address;

public:
	Client(char* ip, uint16_t port);

	bool try_connect();

	void sendMessage(char* buffer);
	void sendMessage(string buffer);
	string recvMessage(int buffer_size);
	string recvMessage();

	void closeConnection();

	string account;
	bool logged_in;
	
	SOCKET getServerSock();

};

class P2PServer
{
private:
	SOCKET _sock;
	SOCKADDR_IN _address;
	
	SOCKET server_sock;
	
	void sendMessage(SOCKET client_sock, char* buffer);
	void sendMessage(SOCKET client_sock, string buffer);
	string recvMessage(SOCKET client_sock, int buffer_size);
	string recvMessage(SOCKET client_sock);
	
	void sendToServer(char* buffer);
	void sendToServer(string buffer);
	string recvFromServer(int buffer_size);
	string recvFromServer();

public:
	P2PServer(uint16_t port, SOCKET SERVER_SOCK);

	void acceptConnection();

};



void p2pServerThread(uint16_t port, SOCKET SERVER_SOCK){
	P2PServer server(port, SERVER_SOCK);
	while (true)
		server.acceptConnection();
}

int main(int argc, char** argv){
	// Initialize WinSock
	WSAData wsaData;
	WORD version = MAKEWORD(2, 2); // Version
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);	// Return 0 on success
	if (iResult != 0){
		cout << "Initial WinSock failed." << endl;
		exit(1);
	}

	char* server_ip;
	uint16_t port;
	if (argc == 1) {									// Default server IP and port
		server_ip = "127.0.0.1";
		port = 10000;
	}
	else {
		server_ip = argv[1];
		port = atoi(argv[2]);
	}

	Client client(server_ip, port);
	thread* t;
	while (!client.try_connect()) {
		while (true) {
			cout << "Reconnect? (Y/N): ";

			string user_input;
			cin >> user_input;
			if (user_input == "Y" || user_input == "y")
				break;
			else if (user_input == "N" || user_input == "n")
				exit(0);
			else
				cout << "illegal input" << endl;
		}

		cin.ignore();									// Clear input buffer
	}

	cout << "Type \"help\" for available commands." << endl;
	while (true) {
		if (client.logged_in)
			cout << client.account << ' ';
		cout << ">>> ";
		string user_input;
		getline(cin, user_input);						// Get user input

		if (user_input.substr(0, 9) == "register ") {	// Try to register
			if (!client.logged_in) {
                user_input.erase(0, 9);
                if (user_input.find(' ') != string::npos) {
                    string user_name = user_input.substr(0, user_input.find(' '));
                    if (user_name == "") {
                        cout << "Illegal input, use \"register <AccountName> <Initial>\" instead." << endl;
                        continue;
                    }

                    user_input.erase(0, user_input.find(' ') + 1);
                    // avoid illegal port number input
                    try {
                        int initial_balance = stoi(user_input);
                        if (initial_balance < 0)
                            throw "illegal number";
                    }
                    catch (...) {
                        cout << "illegal initial balance" << endl;
                        continue;
                    }
                    client.sendMessage("REGISTER#" + user_name + " " + user_input + "\n");
                }
                else {
                    cout << "Illegal input, use \"register <AccountName> <Initial>\" instead." << endl;
					continue;
                }
			}
			else {
				cout << "unknown command" << endl << endl;
				continue;
			}
		}
		else if (user_input.substr(0, 6) == "login ") {	// Try to login
			if (!client.logged_in) {					// Not available when logged in
				// No password? Serious?
				user_input.erase(0, 6);
				string user_name;
				if (user_input.find(' ') != string::npos) {
					user_name = user_input.substr(0, user_input.find(' '));
					user_input.erase(0, user_input.find(' ') + 1);
				}
				else {
					cout << "Illegal input, use \"login <AccountName> <Port>\" instead." << endl;
					continue;
				}

				// avoid illegal port number input
				int input_port;
				try {
					input_port = stoi(user_input);
					if (input_port > 65535 || input_port < 1024)
						throw "illegal number";
				}
				catch (...) {
					cout << "illegal port number" << endl;
					continue;
				}

				client.sendMessage(user_name + "#" + user_input + "\n");

				// Receive login result
				string result = client.recvMessage();
				if (result.substr(0, 3) == "220") {		// login failed
					cout << result << endl;
					continue;
				}
				else {
					t = new thread(p2pServerThread, input_port, client.getServerSock());		// Create P2PServer on new thread
					client.account = user_name;
					client.logged_in = true;
					cout << "Account Balance: " << result << endl;
				}
			}
			else {
				cout << "unknown command" << endl << endl;
				continue;
			}
		}
		else if (user_input == "list") {				// List all users
			if (client.logged_in) {						// Available when logged in
				send_mutex.lock();						// avoid concurrent send to server
				client.sendMessage("List\n");

				string result = client.recvMessage();
				send_mutex.unlock();
				if (result.substr(0, 3) == "220") {		// login failed
					cout << result << endl;
					continue;
				}
				else {
					cout << "Account Balance: " << result << endl;
				}
			}
			else {
				cout << "unknown command" << endl << endl;
				continue;
			}
		}
		else if (user_input.substr(0, 4) == "pay ") {
			if (client.logged_in) {						// Available when logged in
				user_input.erase(0, 4);
				string target = user_input.substr(0, user_input.find(' '));

				user_input.erase(0, user_input.find(' ') + 1);
				try {									// avoid illegal number input
					stoi(user_input);
				}
				catch (...) {
					cout << "illegal amount" << endl;
					continue;
				}

				send_mutex.lock();						// avoid concurrent send to server
				client.sendMessage("PAY#" + target + "\n");
				string result = client.recvMessage();
				send_mutex.unlock();
				
				if (result.substr(0, 3) == "404") {
					cout << result << endl;
				}
				else {
					string p2p_ip = result.substr(0, result.find(' '));
					char* p2p_ip_c_str = new char [p2p_ip.length()];
					strcpy(p2p_ip_c_str, p2p_ip.c_str());
					
					result.erase(0, result.find(' ') + 1);
					uint16_t p2p_port;
					try {
						p2p_port = stoi(result);
					}
					catch (...) {
						cout << result << endl;
					}
					Client p2pClient(p2p_ip_c_str, p2p_port);
					
					if (p2pClient.try_connect()) {
						p2pClient.sendMessage(client.account + "#" + user_input + "#" + target + "\n");
						try {							// Connection to p2p server may failed
							cout << p2pClient.recvMessage() << endl;
							closesocket(p2pClient.getServerSock());
						}
						catch (...) {
							cout << "403 FAIL" << endl;
						}
					}
					else {
						cout << "403 FAIL" << endl;
					}
				}
			}
			else {
				cout << "unknown command" << endl << endl;
			}
			continue;									// no more receive on client
		}
		else if (user_input == "exit") {				// Leave the system
			break;
		}
		else if (user_input == "help") {				// Get available commands
			cout << "Available commands:" << endl;
			if (client.logged_in) {
				cout << "'pay <AccountName> <PayAmount>': Pay specific amount to specific account." << endl;
				cout << "'list': List all users online." << endl;
			}
			else {
				cout << "'register <AccountName> <Initial>': To register an account with initial balance." << endl;
				cout << "'login <AccountName> <Port>': Login with this account at specific port." << endl;
			}
			cout << "'exit': Leave the system." << endl;

			cout << endl;
			continue;
		}
		else {
			cout << "unknown command" << endl << endl;
			continue;
		}

		cout << client.recvMessage() << endl;
	}

	client.closeConnection();

	return 0;
}





Client::Client(char* ip, uint16_t port) {
	// Create socket
	_sock = INVALID_SOCKET;
	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_sock == INVALID_SOCKET){
		cout << "Create socket failed." << endl;
		exit(1);
	}

	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = inet_addr(ip);			// Setting IP address
	_address.sin_port = htons(port);					// Setting port number

	account = "";
	logged_in = false;
}

bool Client::try_connect() {
	int result = connect(_sock, (SOCKADDR*)&_address, sizeof(_address));
	if (result == -1){
		cout << "Can not connect to server." << endl;
		return false;
	}

	return true;
}

void Client::sendMessage(char* buffer) {
	send(_sock, buffer, (int)strlen(buffer), 0);
}

void Client::sendMessage(string buffer) {
	char* send_buffer = new char[buffer.length()];
	strcpy(send_buffer, buffer.c_str());

	sendMessage(send_buffer);
}

string Client::recvMessage(int buffer_size) {
	char recv_buffer[buffer_size];
	int status = recv(_sock, recv_buffer, buffer_size, 0);
	if (status <= 0)
		throw "Operation failed.\nThe server has closed the connection.\n";

	return string(recv_buffer, status);
}

string Client::recvMessage() {
	return recvMessage(MAX_BUFFER_SIZE);
}

void Client::closeConnection() {
	if (logged_in) {									// Have logged in
		send_mutex.lock();								// avoid concurrent send to server
		sendMessage("Exit\n");
		cout << recvMessage(128);
		send_mutex.unlock();
	}

	closesocket(_sock);
}

SOCKET Client::getServerSock() {
	return _sock;
}



P2PServer::P2PServer(uint16_t port, SOCKET SERVER_SOCK) {
	server_sock = SERVER_SOCK;							// Record server socket
	
	// Create socket
	_sock = INVALID_SOCKET;
	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_sock == INVALID_SOCKET){
		cout << "Create socket failed." << endl;
		exit(1);
	}
	
	memset(&_address, 0, sizeof(_address));				// Initialize
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;				// Default local IP address
	_address.sin_port = htons(port);					// Setting port number
	
	int bResult = bind(_sock, (SOCKADDR*)&_address, sizeof(_address));
	if (bResult == SOCKET_ERROR){
		cout << "Binding address and port failed." << endl;
		exit(1);
	}
	
	int lResult = listen(_sock, SOMAXCONN);
	if (lResult == SOCKET_ERROR){
		cout << "Listen connection failed." << endl;
		exit(1);
	}
}

void P2PServer::acceptConnection() {
	SOCKET client_sock;
	SOCKADDR_IN client_address;
	int client_address_length = sizeof(client_address);
	
	client_sock = accept(_sock, (SOCKADDR*)&client_address, &client_address_length);
	if (client_sock != INVALID_SOCKET){
		string pay = recvMessage(client_sock);
		
		// decrypt with own private key
		// string pay_msg = decrypt with A's public key
		string user_from = pay.substr(0, pay.find('#')); // pay_msg.substr(......)
		// encrypt with own private key
		
		send_mutex.lock();	// avoid concurrent send to server
		sendToServer("FROM#" + user_from + "\n");
		if (recvFromServer() == "disconnected") {
			sendMessage(client_sock, "403 FAIL\n");
		}
		else {
			sendToServer(pay);
			if (recvFromServer() == "disconnected") {
				sendMessage(client_sock, "403 FAIL\n");
			}
			else {
				sendMessage(client_sock, "400 OK\n");
			}
		}
		send_mutex.unlock();
		
		closesocket(client_sock);
	}
}

void P2PServer::sendMessage(SOCKET client_sock, char* buffer) {
	send(client_sock, buffer, (int)strlen(buffer), 0);
}

void P2PServer::sendMessage(SOCKET client_sock, string buffer) {
	char* send_buffer = new char[buffer.length()];
	strcpy(send_buffer, buffer.c_str());
	
	sendMessage(client_sock, send_buffer);
}

string P2PServer::recvMessage(SOCKET client_sock, int buffer_size) {
	char* recv_buffer = new char[buffer_size];
	int status = recv(client_sock, recv_buffer, buffer_size, 0);

	if (status <= 0)
		return "disconnected";
	
	return string(recv_buffer, status);
}

string P2PServer::recvMessage(SOCKET client_sock) {
	return recvMessage(client_sock, MAX_BUFFER_SIZE);
}

void P2PServer::sendToServer(char* buffer) {
	send(server_sock, buffer, (int)strlen(buffer), 0);
}

void P2PServer::sendToServer(string buffer) {
	char* send_buffer = new char[buffer.length()];
	strcpy(send_buffer, buffer.c_str());
	
	sendToServer(send_buffer);
}

string P2PServer::recvFromServer(int buffer_size) {
	char* recv_buffer = new char[buffer_size];
	int status = recv(server_sock, recv_buffer, sizeof(recv_buffer), 0);

	if (status <= 0){
		return "disconnected";
	}
	
	return string(recv_buffer, status);
}

string P2PServer::recvFromServer() {
	return recvFromServer(MAX_BUFFER_SIZE);
}