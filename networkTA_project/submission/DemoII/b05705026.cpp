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

#define MAX_BUFFER_SIZE 1024

using namespace std;

// Global mutex for printing
mutex print_mutex;

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
	
	void sendMessage(SOCKET client_sock, char* buffer);
	void sendMessage(SOCKET client_sock, string buffer);
	string recvMessage(SOCKET client_sock, int buffer_size);
	string recvMessage(SOCKET client_sock);
	
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

int main(int argc, char** argv){
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

void Server::sendMessage(SOCKET client_sock, char* buffer) {
	send(client_sock, buffer, (int)strlen(buffer), 0);
}

void Server::sendMessage(SOCKET client_sock, string buffer) {
	char send_buffer[buffer.length()];
	strcpy(send_buffer, buffer.c_str());
	
	sendMessage(client_sock, send_buffer);
}

string Server::recvMessage(SOCKET client_sock, int buffer_size) {
	char recv_buffer[buffer_size];
	ZeroMemory(recv_buffer, buffer_size);				// Clean up buffer
	int status = recv(client_sock, recv_buffer, sizeof(recv_buffer), 0);

	if (status <= 0)
		return "disconnected";
	
	return string(recv_buffer);
}

string Server::recvMessage(SOCKET client_sock) {
	return recvMessage(client_sock, MAX_BUFFER_SIZE);
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
	while (true) {
		string recv_msg = recvMessage(client_sock);
		
		if (recv_msg == "disconnected") {
			break;
		}
		else if (recv_msg.substr(0, 9) == "REGISTER#") {						// Try to register
			if (connected_user == nullptr) {
				recv_msg.erase(0, 9);
				string user_name = recv_msg.substr(0, recv_msg.find(' '));
				recv_msg.erase(0, recv_msg.find(' ') + 1);
				string balance = recv_msg.substr(0, recv_msg.length() - 1);		// Not include <CRLF>
				if (user_name != "REGISTER" && user_name != "") {				// Invalid account name to avoid ambiguity
					if (getUser(user_name) == nullptr) {						// Not registered account name
						addUser(user_name, stoi(balance));
						sendMessage(client_sock, "100 OK\n");
						continue;
					}
				}
			}
			else {
				sendMessage(client_sock, "Invalid Action\n");
				continue;
			}
			
			// If not able to register
			sendMessage(client_sock, "210 FAIL\n");
		}
		else if (recv_msg == "List\n") {								// List all users
			if (connected_user != nullptr) {
				sendMessage(client_sock, to_string(connected_user->balance) + "\n");
				this_thread::sleep_for(chrono::milliseconds(500));		// To avoid client receiving both send at the same time
				sendMessage(client_sock, getUserList());
			}
			else {
				sendMessage(client_sock, "220 AUTH_FAIL\n");
			}
		}
		else if (recv_msg == "Exit\n") {								// Disconnect
			if (connected_user != nullptr) {
				sendMessage(client_sock, "Bye\n");
				connected_user->online = false;
				break;
			}
			else {
				sendMessage(client_sock, "220 AUTH_FAIL\n");
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
					connected_user->online = true;						// Set addr & port before setting online to avoid other threads from getting wrong information
					
					sendMessage(client_sock, to_string(connected_user->balance) + "\n");
					this_thread::sleep_for(chrono::milliseconds(500));	// To avoid client receiving both send at the same time
					sendMessage(client_sock, getUserList());
				}
				else {
					sendMessage(client_sock, "220 AUTH_FAIL\n");
				}
			}
			else {
				sendMessage(client_sock, "Invalid Action\n");
			}
		}
	}
	
	print_mutex.lock();
	cout << "Disconnected from " << inet_ntoa(client_address.sin_addr) << endl;
	print_mutex.unlock();
	
	closesocket(client_sock);
}


