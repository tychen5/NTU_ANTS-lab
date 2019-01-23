#define __USE_MINGW_ANSI_STDIO 0

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <Winsock2.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <exception>

#define MAX_BUFFER_SIZE 1024

using namespace std;

char private_key[] = "-----BEGIN RSA PRIVATE KEY-----\n"\
"MIICXgIBAAKBgQDSZyn6Khegw6+DWPHMZyPsxg7sLNPREPCWLsRMtG3ooXWfGaau\n"\
"gU3BeOFBOVjB3w1j0UmJqSmrJrkPG9ie+Qk+Dn9SuAifI0TizIz3BY6sIgiSMEVo\n"\
"085x/JhsP22JgkxNtBlyfaMqyTUE1FosXDGhx53rZiM9fJOqRU/qfw0/3wIDAQAB\n"\
"AoGBAJPB7zDHrqAvzGopZGVvaUM2M/SX5ojDpLFTVnennifoe3mnwe56z+g2w7nS\n"\
"VAqSYgzfRQ1vxtty7jM2EwJRUbMEXvgPQqaukL5vryZ0m/6GruNMkji4iBFM3hPB\n"\
"IpPdtFitzAt4gI+6Z/erD10+ErtRgKJ5zkeNPjLknB+GjerBAkEA9k0Dq42la8EG\n"\
"HMIksuhZl8KKpTzNnaAB4/fFVj4f5FEO45Ijm0JcOPVntTSxU+B4sltYyy+GbwYv\n"\
"OVkX5NHtawJBANqwQmvH+CcTebg3RlqlU3WvZBSubEAZpZm4ohwDBQyDuzhzaQYt\n"\
"UiEMYmlJTNt2cUfVN7nJCI3853lY1HTeAF0CQHkgatulb2LMrJrcB3xMtDLkI5cb\n"\
"jesk04kvQsclCj4YdwAH0Kb8PaptVbFR1ptvWywrrEFQgAZ9vh+v5wZLDz0CQQCK\n"\
"+H2NHqucsWylW0LzMaKi481Wsy0JYwLwd/tUj7qypDfifLzd9wj9BXC4daNIx7Df\n"\
"NCjQgGdWS8QRQhjpF3blAkEA1e7+/4734IitAfUfBnEPz4BtoobUgw2gegzNV6nC\n"\
"GHGrhEROZsjZYWCwZA6O5e5KrelYBfecE2WSMBMvsrfPEg==\n"\
"-----END RSA PRIVATE KEY-----\n";

char public_key[] = "-----BEGIN PUBLIC KEY-----\n"\
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDSZyn6Khegw6+DWPHMZyPsxg7s\n"\
"LNPREPCWLsRMtG3ooXWfGaaugU3BeOFBOVjB3w1j0UmJqSmrJrkPG9ie+Qk+Dn9S\n"\
"uAifI0TizIz3BY6sIgiSMEVo085x/JhsP22JgkxNtBlyfaMqyTUE1FosXDGhx53r\n"\
"ZiM9fJOqRU/qfw0/3wIDAQAB\n"\
"-----END PUBLIC KEY-----\n";

RSA* own_pri_key;	// global var of own private key
RSA* own_pub_key;	// global var of own public key
mutex send_mutex;	// mutex for sending to server

class Client
{
private:
	SOCKET _sock;
	SOCKADDR_IN _address;

public:
	Client(char* ip, uint16_t port);

	bool try_connect();

	void sendMessage(char* buffer, RSA* pub_key = nullptr);
	void sendMessage(string buffer, RSA* pub_key = nullptr);
	string recvMessage(int buffer_size, RSA* pri_key = nullptr);
	string recvMessage(RSA* pri_key = nullptr);

	void closeConnection(RSA* server_pub_key);

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
	
	void sendMessage(SOCKET client_sock, char* buffer, RSA* pub_key = nullptr);
	void sendMessage(SOCKET client_sock, string buffer, RSA* pub_key = nullptr);
	string recvMessage(SOCKET client_sock, int buffer_size, RSA* pri_key = nullptr);
	string recvMessage(SOCKET client_sock, RSA* pri_key = nullptr);
	
	void sendToServer(char* buffer, RSA* pub_key = nullptr);
	void sendToServer(string buffer, RSA* pub_key = nullptr);
	string recvFromServer(int buffer_size, RSA* pri_key = nullptr);
	string recvFromServer(RSA* pri_key = nullptr);

public:
	P2PServer(uint16_t port, SOCKET SERVER_SOCK);

	void acceptConnection(RSA* server_pub_key);

};


RSA * createRSA(unsigned char * key,int pub)
{
    RSA *rsa= NULL;
    BIO *keybio ;
    keybio = BIO_new_mem_buf(key, -1);
    if (keybio==NULL)
    {
        printf( "Failed to create key BIO");
        return 0;
    }
    if(pub)
    {
        rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa,NULL, NULL);
    }
    else
    {
        rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa,NULL, NULL);
    }
	BIO_free(keybio);
    return rsa;
}

RSA* getPublicKey(string public_key){
	unsigned char* pub_key = new unsigned char[public_key.length()];
	strcpy((char*)pub_key, public_key.c_str());
	
	RSA* rsa = createRSA(pub_key, 1);
	return rsa;
}

int private_encrypt(unsigned char* data, int data_len, RSA* rsa, unsigned char *encrypted)
{
    int result = RSA_private_encrypt(data_len, data, encrypted, rsa, RSA_PKCS1_PADDING);
    return result;
}

int private_decrypt(unsigned char* enc_data, int data_len, RSA* rsa, unsigned char *decrypted)
{
    int  result = RSA_private_decrypt(data_len, enc_data, decrypted, rsa, RSA_PKCS1_PADDING);
    return result;
}

int public_encrypt(unsigned char* data, int data_len, RSA* rsa, unsigned char *encrypted)
{
    int result = RSA_public_encrypt(data_len, data, encrypted, rsa, RSA_PKCS1_PADDING);
    return result;
}

int public_decrypt(unsigned char* enc_data, int data_len, RSA* rsa, unsigned char *decrypted)
{
    int  result = RSA_public_decrypt(data_len, enc_data, decrypted, rsa, RSA_PKCS1_PADDING);
    return result;
}

void p2pServerThread(uint16_t port, SOCKET SERVER_SOCK, RSA* server_pub_key){
	P2PServer server(port, SERVER_SOCK);
	while (true)
		server.acceptConnection(server_pub_key);
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
	
	// Generate own rsa keys
	own_pri_key = createRSA((unsigned char*)private_key, 0);
	own_pub_key = createRSA((unsigned char*)public_key, 1);
	
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

	thread* mThread;											// For p2p server
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
	
	client.sendMessage(public_key);						// Exchange public key with server
	RSA* server_pub_key = getPublicKey(client.recvMessage());

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
                    client.sendMessage("REGISTER#" + user_name + " " + user_input + "\n", server_pub_key);
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

				client.sendMessage(user_name + "#" + user_input + "\n", server_pub_key);

				// Receive login result
				string result = client.recvMessage(own_pri_key);
				if (result.substr(0, 3) == "220") {		// login failed
					cout << result << endl;
					continue;
				}
				else {
					mThread = new thread(p2pServerThread, input_port, client.getServerSock(), server_pub_key);		// Create P2PServer on new thread
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
				client.sendMessage("List\n", server_pub_key);

				string result = client.recvMessage(own_pri_key);
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
				client.sendMessage("PAY#" + target + "\n", server_pub_key);
				string result = client.recvMessage(own_pri_key);
				send_mutex.unlock();
				
				if (result.substr(0, 3) == "404") {
					cout << result << endl;
				}
				else {
					string p2p_ip = result.substr(0, result.find(' '));
					char* p2p_ip_c_str = new char [p2p_ip.length()];
					strcpy(p2p_ip_c_str, p2p_ip.c_str());
					
					result.erase(0, result.find(' ') + 1);
					uint16_t p2p_port = stoi(result);
					Client p2pClient(p2p_ip_c_str, p2p_port);
					
					if (p2pClient.try_connect()) {
						// Exchange public key with server
						p2pClient.sendMessage(public_key);
						RSA* server_pub_key = getPublicKey(client.recvMessage());
						
						// encrypt with own private key
						string msg = client.account + "#" + user_input + "#" + target + "\n";
						unsigned char encrypted[4098] = {};
						int encrypted_length = private_encrypt((unsigned char*)msg.c_str(), msg.length(), own_pri_key, encrypted);
						msg = string((char*)encrypted, encrypted_length);
						
						p2pClient.sendMessage(msg, server_pub_key);
						try {	// Connection to p2p server may failed
							cout << p2pClient.recvMessage(own_pri_key) << endl;
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

		cout << client.recvMessage(own_pri_key) << endl;
	}

	client.closeConnection(server_pub_key);

	RSA_free(own_pri_key);
	RSA_free(own_pub_key);
	RSA_free(server_pub_key);
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

void Client::sendMessage(char* buffer, RSA* pub_key) {
	if (pub_key != nullptr) {
		unsigned char encrypted[4098] = {};
		int encrypted_length = public_encrypt((unsigned char*)buffer, strlen(buffer), pub_key, encrypted);
		send(_sock, (char*)encrypted, encrypted_length, 0);
		return;
	}
	
	send(_sock, buffer, (int)strlen(buffer), 0);
}

void Client::sendMessage(string buffer, RSA* pub_key) {
	char* send_buffer = new char[buffer.length()];
	strcpy(send_buffer, buffer.c_str());

	sendMessage(send_buffer, pub_key);
}

string Client::recvMessage(int buffer_size, RSA* pri_key) {
	char recv_buffer[buffer_size];
	int status = recv(_sock, recv_buffer, buffer_size, 0);
	if (status <= 0)
		throw "Operation failed.\nThe server has closed the connection.\n";
	
	if (pri_key != nullptr) {
		unsigned char decrypted[4098] = {};
		int decrypted_length = private_decrypt((unsigned char*)recv_buffer, status, pri_key, decrypted);
		string result((char*)decrypted, decrypted_length);
		
		return result;
	}
	
	string result(recv_buffer, status);
	return result;
}

string Client::recvMessage(RSA* pri_key) {
	return recvMessage(MAX_BUFFER_SIZE, pri_key);
}

void Client::closeConnection(RSA* server_pub_key) {
	if (logged_in) {									// Have logged in
		send_mutex.lock();								// avoid concurrent send to server
		sendMessage("Exit\n", server_pub_key);
		cout << recvMessage(128, own_pri_key);
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

void P2PServer::acceptConnection(RSA* server_pub_key) {
	SOCKET client_sock;
	SOCKADDR_IN client_address;
	int client_address_length = sizeof(client_address);
	
	client_sock = accept(_sock, (SOCKADDR*)&client_address, &client_address_length);
	if (client_sock != INVALID_SOCKET){
		// Exchange public key with A
		RSA* client_pub_key = getPublicKey(recvMessage(client_sock));
		sendMessage(client_sock, public_key);
		
		string pay = recvMessage(client_sock, own_pri_key);
		
		// string pay_msg = decrypt with A's public key
		unsigned char decrypted[4098] = {};
		int decrypted_length = public_decrypt((unsigned char*)pay.c_str(), pay.length(), client_pub_key, decrypted);
		string pay_msg((char*)decrypted, decrypted_length);
		
		string user_from = pay_msg.substr(0, pay.find('#'));
		
		// encrypt with own private key
		unsigned char encrypted[4098] = {};
		int encrypted_length = private_encrypt((unsigned char*)pay.c_str(), pay.length(), own_pri_key, encrypted);
		pay = string((char*)encrypted, encrypted_length);
		
		send_mutex.lock();		// avoid concurrent send to server
		sendToServer("FROM#" + user_from + "\n", server_pub_key);
		if (recvFromServer(own_pri_key) == "disconnected") {
			sendMessage(client_sock, "403 FAIL\n", client_pub_key);
		}
		else {
			sendToServer(pay, server_pub_key);
			if (recvFromServer(own_pri_key) == "disconnected") {
				sendMessage(client_sock, "403 FAIL\n", client_pub_key);
			}
			else {
				sendMessage(client_sock, "400 OK\n", client_pub_key);
			}
		}
		send_mutex.unlock();
		
		closesocket(client_sock);
	}
}

void P2PServer::sendMessage(SOCKET client_sock, char* buffer, RSA* pub_key) {
	if (pub_key != nullptr) {
		unsigned char encrypted[4098] = {};
		int encrypted_length = public_encrypt((unsigned char*)buffer, strlen(buffer), pub_key, encrypted);
		send(client_sock, (char*)encrypted, encrypted_length, 0);
		return;
	}
	
	send(client_sock, buffer, (int)strlen(buffer), 0);
}

void P2PServer::sendMessage(SOCKET client_sock, string buffer, RSA* pub_key) {
	char* send_buffer = new char[buffer.length()];
	strcpy(send_buffer, buffer.c_str());
	
	sendMessage(client_sock, send_buffer, pub_key);
}

string P2PServer::recvMessage(SOCKET client_sock, int buffer_size, RSA* pri_key) {
	char* recv_buffer = new char[buffer_size];
	int status = recv(client_sock, recv_buffer, buffer_size, 0);

	if (status <= 0)
		return "disconnected";
	
	if (pri_key != nullptr) {
		unsigned char decrypted[4098] = {};
		int decrypted_length = private_decrypt((unsigned char*)recv_buffer, status, pri_key, decrypted);
		string result((char*)decrypted, decrypted_length);
		
		return result;
	}
	
	string result(recv_buffer, status);
	return result;
}

string P2PServer::recvMessage(SOCKET client_sock, RSA* pri_key) {
	return recvMessage(client_sock, MAX_BUFFER_SIZE, pri_key);
}

void P2PServer::sendToServer(char* buffer, RSA* pub_key) {
	if (pub_key != nullptr) {
		unsigned char encrypted[4098] = {};
		int encrypted_length = public_encrypt((unsigned char*)buffer, strlen(buffer), pub_key, encrypted);
		send(server_sock, (char*)encrypted, encrypted_length, 0);
		return;
	}
	
	send(server_sock, buffer, (int)strlen(buffer), 0);
}

void P2PServer::sendToServer(string buffer, RSA* pub_key) {
	char* send_buffer = new char[buffer.length()];
	strcpy(send_buffer, buffer.c_str());
	
	sendToServer(send_buffer, pub_key);
}

string P2PServer::recvFromServer(int buffer_size, RSA* pri_key) {
	char* recv_buffer = new char[buffer_size];
	int status = recv(server_sock, recv_buffer, sizeof(recv_buffer), 0);

	if (status <= 0){
		return "disconnected";
	}
	
	if (pri_key != nullptr) {
		unsigned char decrypted[4098] = {};
		int decrypted_length = private_decrypt((unsigned char*)recv_buffer, status, pri_key, decrypted);
		string result((char*)decrypted, decrypted_length);
		
		return result;
	}
	
	string result(recv_buffer, status);
	return result;
}

string P2PServer::recvFromServer(RSA* pri_key) {
	return recvFromServer(MAX_BUFFER_SIZE, pri_key);
}