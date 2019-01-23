#define __USE_MINGW_ANSI_STDIO 0

#include <iostream>
#include <string>
#include <stdio.h>
#include <vector>
#include <queue>
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
"MIICXgIBAAKBgQCn4IDeDgf+gMFikk+VWDocsI4G6k2wErIPpL0GZw80/60vc0Md\n"\
"9t/x9oeO0xzqeKu1BIHMYNSLOAIIcv1uUOCLsbEPXNpr8LpUhKOge1gcwUm0qirP\n"\
"ZgjPNVHMaWiwmgV+WoytSWuCdynS3IED+SbXwGa1yfGPtyXACO5Dl7GU7QIDAQAB\n"\
"AoGBAICCIyPYqdXwyhii17kDKLj9jjtM8Ntv9E51VR2plsKDVheUrDQr35JhnbN/\n"\
"eAslBIw1PwcsuIc6AJRnMglYcApB3Au0XhHLiINeey//+qYmMHo+Wt5NQAgIdRmJ\n"\
"5HbxgLZRKQkSoOq2nSgb6ssITgwNsBjrqRwjYI8CMmDTJa4hAkEA2dpEfvLgeKgd\n"\
"pE06q6xcNxlef/djJ+KgX/Ohsp4XzmxgjXYZQXD8yqmbrYbd0NpvrxdpipZBgGRr\n"\
"sT/YYlRQ1wJBAMVF9yhQwS8G/mH1FXSpzVqYrXM080i7E+NNlSTxDDY6pz33dbjQ\n"\
"HuzWmmsYKMEXw33gl+nx5ujCddgP42dMW9sCQGBppJ66RnWfkV7BfxGy+iy4YYYo\n"\
"qg1g0rEkVY+DP+3vMNvqREseAgJ/BZLKeSiRQ5QtvFvFG8ACsVaEdvMtdYkCQQCJ\n"\
"d/D8mhGU2NXJo0T5UB525G/yGVLzOtJoEjc9T/BHleXXK6tQR09VkVJ4EJTNweaL\n"\
"wEd8UgKr7l66WpAH+tEDAkEAt8VpYKGg2VOxZKl9rPNsqDblvB2Q92BAaq+col4b\n"\
"6mt1WZAbhzcaTeKghWYKRwXjYmtN0JzvU4f0UDZMcj0D9g==\n"\
"-----END RSA PRIVATE KEY-----\n";

char public_key[] = "-----BEGIN PUBLIC KEY-----\n"\
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCn4IDeDgf+gMFikk+VWDocsI4G\n"\
"6k2wErIPpL0GZw80/60vc0Md9t/x9oeO0xzqeKu1BIHMYNSLOAIIcv1uUOCLsbEP\n"\
"XNpr8LpUhKOge1gcwUm0qirPZgjPNVHMaWiwmgV+WoytSWuCdynS3IED+SbXwGa1\n"\
"yfGPtyXACO5Dl7GU7QIDAQAB\n"\
"-----END PUBLIC KEY-----\n";

RSA* own_pri_key;	// global var of own private key
RSA* own_pub_key;	// global var of own public key
mutex print_mutex;	// Global mutex for printing

struct ClientSocket
{
	SOCKET sock;
	SOCKADDR_IN addr;

	ClientSocket(SOCKET socket, SOCKADDR_IN address) {
		sock = socket;
		addr = address;
	}
};

struct UserInfo
{
	string name;
	int balance;
	bool online;
	string addr;
	string port;
	RSA* public_key;

	UserInfo(string client_name, int init_balance) {
		name = client_name;
		balance = init_balance;
		online = false;
	}
};

class Server
{
private:
	SOCKET _sock;
	SOCKADDR_IN _address;

	mutex list_mutex;
	vector<UserInfo*> user_list;

	void sendMessage(SOCKET client_sock, char* buffer, RSA* pub_key = nullptr);
	void sendMessage(SOCKET client_sock, string buffer, RSA* pub_key = nullptr);
	string recvMessage(SOCKET client_sock, int buffer_size, RSA* pri_key = nullptr);
	string recvMessage(SOCKET client_sock, RSA* pri_key = nullptr);

	mutex task_mutex;
	queue<ClientSocket*> task_queue;
	size_t NUM_OF_TASK;

	void worker_thread(int id);
	vector<thread> thread_pool;

	void addUser(string name, int balance);
	UserInfo* getUser(string name);
	string getUserList();

public:
	Server(char* ip, uint16_t port);

	void acceptConnection();
	void connection(SOCKET client_sock, SOCKADDR_IN client_address);

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

int main(int argc, char** argv){
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

	Server server(server_ip, port);
	while (true)
		server.acceptConnection();

	RSA_free(own_pri_key);
	RSA_free(own_pub_key);
	return 0;
}



Server::Server(char* ip, uint16_t port) {
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

	memset(&_address, 0, sizeof(_address));				// Initialize
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = inet_addr(ip);			// Setting IP address
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

	size_t NUM_OF_THREAD = thread::hardware_concurrency();
	cout << "Server starts at " << ip << ':' << port << endl;
	cout << "Max concurrent service: " << NUM_OF_THREAD << endl;
	cout << "=================================" << endl;

	NUM_OF_TASK = 0;
	for (int i = 0; i < NUM_OF_THREAD; i++)
		thread_pool.push_back(thread(&Server::worker_thread, this, i));
}

void Server::acceptConnection() {
	SOCKET client_sock;
	SOCKADDR_IN client_address;							// store client address information
	int client_address_length = sizeof(client_address);

	client_sock = accept(_sock, (SOCKADDR*)&client_address, &client_address_length);
	if (client_sock != INVALID_SOCKET){
		print_mutex.lock();
		cout << "Got connection from " << inet_ntoa(client_address.sin_addr) << endl;
		print_mutex.unlock();

		task_mutex.lock();
		ClientSocket* client = new ClientSocket(client_sock, client_address);
		task_queue.push(client);
		NUM_OF_TASK++;
		task_mutex.unlock();
	}
}

void Server::worker_thread(int id) {
	while (true) {
		while (NUM_OF_TASK == 0) this_thread::sleep_for(chrono::milliseconds(100));	// Wait for task
		task_mutex.lock();															// To access task queue
		if (!task_queue.empty()) {
			ClientSocket* client = task_queue.front();								// Get task from queue and process it
			task_queue.pop();
			NUM_OF_TASK--;
			task_mutex.unlock();

			print_mutex.lock();
			cout << "Serve connection on worker thread " << id << endl;
			print_mutex.unlock();

			connection(client->sock, client->addr);									// Communicating with client

			print_mutex.lock();
			cout << "End service on worker thread " << id << endl;
			print_mutex.unlock();
		}
		else {
			task_mutex.unlock();
		}
	}
}

void Server::sendMessage(SOCKET client_sock, char* buffer, RSA* pub_key) {
	if (pub_key != nullptr) {
		unsigned char encrypted[4098] = {};
		int encrypted_length = public_encrypt((unsigned char*)buffer, strlen(buffer), pub_key, encrypted);
		send(client_sock, (char*)encrypted, encrypted_length, 0);
		return;
	}

	send(client_sock, buffer, (int)strlen(buffer), 0);
}

void Server::sendMessage(SOCKET client_sock, string buffer, RSA* pub_key) {
	char* send_buffer = new char[buffer.length()];
	strcpy(send_buffer, buffer.c_str());

	sendMessage(client_sock, send_buffer, pub_key);
}

string Server::recvMessage(SOCKET client_sock, int buffer_size, RSA* pri_key) {
	char* recv_buffer = new char[buffer_size];
	int status = recv(client_sock, recv_buffer, buffer_size, 0);
	if (status <= 0)
		return "disconnected";

	if (pri_key != nullptr) {
		unsigned char decrypted[1024] = {};
		int decrypted_length = private_decrypt((unsigned char*)recv_buffer, status, pri_key, decrypted);
		string result((char*)decrypted, decrypted_length);
		return result;
	}

	string result(recv_buffer, status);
	return result;
}

string Server::recvMessage(SOCKET client_sock, RSA* pri_key) {
	return recvMessage(client_sock, MAX_BUFFER_SIZE, pri_key);
}

void Server::addUser(string name, int balance) {
	list_mutex.lock();									// Can not access user list concurrently
	user_list.push_back(new UserInfo(name, balance));
	list_mutex.unlock();
}

UserInfo* Server::getUser(string name) {
	UserInfo* user = nullptr;

	list_mutex.lock();									// Can not access user list concurrently
	for (size_t i = 0; i < user_list.size(); i++) {
		if (user_list.at(i)->name == name) {
			user = user_list.at(i);
			break;
		}
	}
	list_mutex.unlock();

	return user;
}

string Server::getUserList() {
	string result;

	list_mutex.lock();									// Can not access user list concurrently
	for (size_t i = 0; i < user_list.size(); i++) {
		UserInfo* user = user_list.at(i);
		if (user->online) {
			result += user->name;
			result += "#";
			result += user->addr;
			result += "#";
			result += user->port;
			result += "\n";
		}
	}
	list_mutex.unlock();

	return result;
}

void Server::connection(SOCKET client_sock, SOCKADDR_IN client_address) {
	UserInfo* connected_user = nullptr;

	// Exchange public key with client
	RSA* client_pub_key = getPublicKey(recvMessage(client_sock));
	sendMessage(client_sock, public_key);

	while (true) {
		string recv_msg = recvMessage(client_sock, own_pri_key);
		cout << recv_msg << endl;
		if (recv_msg == "disconnected") {
			break;
		}
		else if (recv_msg.substr(0, 9) == "REGISTER#") {						// Try to register
			if (connected_user == nullptr) {
				recv_msg.erase(0, 9);
				string user_name = recv_msg.substr(0, recv_msg.find(' '));
				recv_msg.erase(0, recv_msg.find(' ') + 1);
				string balance = recv_msg.substr(0, recv_msg.length() - 1);		// Not include <CRLF>
				if (user_name != "REGISTER" && user_name != "PAY" && user_name != "FROM" && user_name != "") {	// Invalid account name to avoid ambiguity
					if (getUser(user_name) == nullptr) {						// Not registered account name
						addUser(user_name, stoi(balance));
						sendMessage(client_sock, "100 OK\n", client_pub_key);
						continue;
					}
				}
			}
			else {
				sendMessage(client_sock, "Invalid Action\n", client_pub_key);
				continue;
			}

			// If not able to register
			sendMessage(client_sock, "210 FAIL\n", client_pub_key);
		}
		else if (recv_msg.substr(0, 4) == "PAY#") {								// Try to get peer client
			if (connected_user != nullptr) {
				recv_msg.erase(0, 4);
				string user_name = recv_msg.substr(0, recv_msg.length() - 1);	// Not include <CRLF>
				UserInfo* peer = getUser(user_name);
				if (peer != nullptr && peer->online) {							// Exist peer online
					sendMessage(client_sock, peer->addr + " " + peer->port + "\n", client_pub_key);
				}
				else {
					sendMessage(client_sock, "404 NOT_FOUND\n", client_pub_key);
				}
			}
			else {
				sendMessage(client_sock, "Invalid Action\n", client_pub_key);
			}
		}
		else if (recv_msg.substr(0, 5) == "FROM#") {							// Receive payment from peer
			if (connected_user != nullptr) {
				recv_msg.erase(0, 5);
				string user_name = recv_msg.substr(0, recv_msg.length() - 1);	// Not include <CRLF>
				UserInfo* peer = getUser(user_name);

				sendMessage(client_sock, "100 OK\n", client_pub_key);
				string payment = recvMessage(client_sock, own_pri_key);

				// decrypt with B's public key
				unsigned char decrypted[4098] = {};
				int decrypted_length = public_decrypt((unsigned char*)payment.c_str(), payment.length(), client_pub_key, decrypted);
				payment = string((char*)decrypted, decrypted_length);

				// decrypt with A's public key
				unsigned char decrypted2[4098] = {};
				decrypted_length = public_decrypt((unsigned char*)payment.c_str(), payment.length(), peer->public_key, decrypted2);
				payment = string((char*)decrypted2, decrypted_length);

				payment.erase(0, payment.find('#') + 1);
				int amount = stoi(payment.substr(0, payment.find('#')));
				peer->balance -= amount;
				connected_user->balance += amount;

				sendMessage(client_sock, "100 OK\n", client_pub_key);
			}
			else {
				sendMessage(client_sock, "Invalid Action\n", client_pub_key);
			}
		}
		else if (recv_msg == "List\n") {								// List all users
			if (connected_user != nullptr) {
				sendMessage(client_sock, to_string(connected_user->balance) + "\n", client_pub_key);
				this_thread::sleep_for(chrono::milliseconds(500));		// To avoid client receiving both send at the same time
				sendMessage(client_sock, getUserList(), client_pub_key);
			}
			else {
				sendMessage(client_sock, "220 AUTH_FAIL\n", client_pub_key);
			}
		}
		else if (recv_msg == "Exit\n") {								// Disconnect
			if (connected_user != nullptr) {
				sendMessage(client_sock, "Bye\n", client_pub_key);
				connected_user->online = false;
				break;
			}
			else {
				sendMessage(client_sock, "220 AUTH_FAIL\n", client_pub_key);
			}
		}
		else {															// Try to login
			if (connected_user == nullptr) {							// Not available when logged in
				string user_name = recv_msg.substr(0, recv_msg.find('#'));
				connected_user = getUser(user_name);
				if (connected_user != nullptr) {
					recv_msg.erase(0, recv_msg.find('#') + 1);			// Get client port
					recv_msg.erase(recv_msg.length() - 1, 1);			// Erase <CRLF>
					connected_user->addr = inet_ntoa(client_address.sin_addr);
					connected_user->port = recv_msg;
					connected_user->public_key = client_pub_key;
					connected_user->online = true;						// Set addr & port before setting online to avoid other threads from getting wrong information

					sendMessage(client_sock, to_string(connected_user->balance) + "\n", client_pub_key);
					this_thread::sleep_for(chrono::milliseconds(500));	// To avoid client receiving both send at the same time
					sendMessage(client_sock, getUserList(), client_pub_key);
				}
				else {
					sendMessage(client_sock, "220 AUTH_FAIL\n", client_pub_key);
				}
			}
			else {
				sendMessage(client_sock, "Invalid Action\n", client_pub_key);
			}
		}
	}

	print_mutex.lock();
	cout << "Disconnected from " << inet_ntoa(client_address.sin_addr) << endl;
	print_mutex.unlock();

	closesocket(client_sock);
	RSA_free(client_pub_key);
}


