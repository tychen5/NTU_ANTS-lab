#define __USE_MINGW_ANSI_STDIO 0

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <Winsock2.h>

#define MAX_BUFFER_SIZE 1024

using namespace std;

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
	
};

int main(int argc, char** argv){
	char* server_ip;
	uint16_t port;
	if (argc == 1) {									// Default server IP and port
		server_ip = "140.112.107.194";
		port = 33120;
	}
	else {
		server_ip = argv[1];
		port = atoi(argv[2]);
	}
	
	Client client(server_ip, port);
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
	
	client.recvMessage(128);							// Receive welcome message
	cout << "Type \"help\" for available commands." << endl;
	while (true) {
		if (client.logged_in)
			cout << client.account << ' ';
		cout << ">>> ";
		string user_input;
		getline(cin, user_input);						// Get user input
		
		if (user_input.substr(0, 9) == "register ") {	// Try to register
			if (!client.logged_in) {					// Not available when logged in
				string user_name = user_input.substr(9, user_input.length() - 9);
				if (user_name == "") {
					cout << "Illegal input, use \"register <AccountName>\" instead." << endl;
					continue;
				}
				client.sendMessage("REGISTER#" + user_name + "\n");
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
				try {
					int input_port = stoi(user_input);
					if (input_port > 65535 || input_port < 0)
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
				client.sendMessage("List\n");
				
				string result = client.recvMessage();
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
		/*else if (user_input.substr(0, 4) == "pay ") {
			if (client.logged_in) {						// Available when logged in
				user_input.erase(0, 4);
				string target = user_input.substr(0, user_input.find(' '));
				
				user_input.erase(0, user_input.find(' ') + 1);
				try {									// avoid illegal number input
					stoi(user_input);
				}
				catch (...) {
					cout << "illegal number" << endl;
					continue;
				}
			
				client.sendMessage(client.account + "#" + user_input + "#" + target + "\n");
			}
			else {
				cout << "unknown command" << endl << endl;
				continue;
			}
		}*/
		else if (user_input == "exit") {				// Leave the system
			break;
		}
		else if (user_input == "help") {				// Get available commands
			cout << "Available commands:" << endl;
			if (client.logged_in) {
				//cout << "'pay <AccountName> <PayAmount>': Login with this account." << endl;
				cout << "'list': List all users online." << endl;
			}
			else {
				cout << "'register <AccountName>': To register an account." << endl;
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
	// Initialize WinSock
	WSAData wsaData;
	WORD version = MAKEWORD(2, 2); // Version
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);	// Return 0 on success
	if (iResult != 0){
		cout << "Initial WinSock failed." << endl;
		exit(1);
	}
	
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
	char send_buffer[buffer.length()];
	strcpy(send_buffer, buffer.c_str());
	
	sendMessage(send_buffer);
}

string Client::recvMessage(int buffer_size) {
	char recv_buffer[buffer_size];
	ZeroMemory(recv_buffer, buffer_size);				// Clean up buffer
	int status = recv(_sock, recv_buffer, sizeof(recv_buffer), 0);
	if (status == 0) {
		cout << "Operation failed." << endl;
		cout << "The server has closed the connection." << endl;
		exit(0);
	}
	
	return string(recv_buffer);
}

string Client::recvMessage() {
	return recvMessage(MAX_BUFFER_SIZE);
}

void Client::closeConnection() {
	if (account != "") {								// Have logged in
		sendMessage("Exit\n");
		cout << recvMessage(128);
	}
	
	closesocket(_sock);
}

