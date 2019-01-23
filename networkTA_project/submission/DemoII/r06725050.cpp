#include <iostream>
#include <string>
#include <stdio.h>
#include <vector>
#include <queue>
#include <Winsock2.h>
#include <thread>
#include <chrono>
#include <mutex>
#define __USE_MINGW_ANSI_STDIO 0
#define MAX_BUFFER_SIZE 1024

using namespace std;
mutex PM;
//Mutex 爲了防止兩條綫程同時對同一公共資源

struct UserInfo
{
	string name;
	string addr;
	string port;
	int money;
	bool online;
	
	UserInfo(string clientN) {
		name = clientN;
		online = false;
	}
};
struct ClientSocket
{
	SOCKET sock;
	SOCKADDR_IN addr;
	
	ClientSocket(SOCKET socket, SOCKADDR_IN address) {
		sock = socket;
		addr = address;
	}
};


class Server
{
private:
	SOCKET _sock;
	SOCKADDR_IN _address;
	
	mutex LM;
	vector<UserInfo*> user_list;
	
	void sendMessage(SOCKET clientS, char* buffer);
	void sendMessage(SOCKET clientS, string buffer);
	string recvMessage(SOCKET clientS, int buffer_size);
	string recvMessage(SOCKET clientS);
	
	mutex TM;
	queue<ClientSocket*> taskQ;
	size_t numT;
	
	void worker_thread(int id);
	vector<thread> thread_pool;
	
	void addUser(string name);
	UserInfo* getUser(string name);
	string getUserList();
	
public:
	Server(char* ip, uint16_t port);
	
	void AC();	
	void connection(SOCKET clientS, SOCKADDR_IN clientAD);
	
};

int main(int argc, char** argv){
	char* server_ip;
	uint16_t port;
	if (argc == 1) {									
		server_ip = "127.0.0.1";//server 默認IP port 
		port = 800;
	}
	else {
		server_ip = argv[1];
		port = atoi(argv[2]);
	}
	
	Server server(server_ip, port);
	while (true)
		server.AC();
	
	return 0;
}



Server::Server(char* ip, uint16_t port) {
	WSAData wsaData;//初始化 WinSock
	WORD version = MAKEWORD(2, 2); // Version
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);	
	if (iResult != 0){//返回0 就成功
		cout << "The beginning of Sock do not work." << endl;
		exit(1);
	}
	
	// 創建 socket
	_sock = INVALID_SOCKET;
	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_sock == INVALID_SOCKET){
		cout << "The create socket do not work." << endl;
		exit(1);
	}
	
	memset(&_address, 0, sizeof(_address));				// 初始化
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = inet_addr(ip);			// 設置 IP 
	_address.sin_port = htons(port);					// 設置 port 
	
	int bResult = bind(_sock, (SOCKADDR*)&_address, sizeof(_address));
	if (bResult == SOCKET_ERROR){
		cout << "The address and port do not work." << endl;
		exit(1);
	}
	
	int lResult = listen(_sock, SOMAXCONN);
	if (lResult == SOCKET_ERROR){
		cout << "The Listen do not work." << endl;
		exit(1);
	}
	
	cout << "Currently,Server " << ip << ':' << port << endl;	
	numT = 1;
	for (int i = 1; i < 11; i++)
		thread_pool.push_back(thread(&Server::worker_thread, this, i));//開啓thread
}

void Server::AC() {//同意連接后的個人信息
	SOCKET clientS;
	SOCKADDR_IN clientAD;							
	int clientAD_length = sizeof(clientAD);
	
	clientS = accept(_sock, (SOCKADDR*)&clientAD, &clientAD_length);
	if (clientS != INVALID_SOCKET){
		PM.lock();
		cout << "The connection in " << inet_ntoa(clientAD.sin_addr) << endl;
		PM.unlock();
		
		TM.lock();
		ClientSocket* client = new ClientSocket(clientS, clientAD);
		taskQ.push(client);
		numT++;
		TM.unlock();
	}
}

void Server::worker_thread(int id) {
	while (true) {
		while (numT == 0) this_thread::sleep_for(chrono::milliseconds(80));	// 等待80毫秒在傳輸
		TM.lock();															
		if (!taskQ.empty()) {
			ClientSocket* client = taskQ.front();								// 從隊列中獲得task,先进先出
			taskQ.pop();
			numT--;//最开始的client服务完后，就以此減一
			TM.unlock();
			
			PM.lock();
			cout << "The Serve on worker thread " << id << endl;
			PM.unlock();			
			
			connection(client->sock, client->addr);									
			
			PM.lock();
			cout << "The service do not on worker thread " << id << endl;
			PM.unlock();
		}
		else {
			TM.unlock();
		}
	}
}

void Server::sendMessage(SOCKET clientS, char* buffer) {
	send(clientS, buffer, (int)strlen(buffer), 0);
}

void Server::sendMessage(SOCKET clientS, string buffer) {
	char send_buffer[buffer.length()];
	strcpy(send_buffer, buffer.c_str());
	
	sendMessage(clientS, send_buffer);
}

string Server::recvMessage(SOCKET clientS, int buffer_size) {
	char reB[buffer_size];
	ZeroMemory(reB, buffer_size);				// 清除 buffer
	int status = recv(clientS, reB, sizeof(reB), 0);

	if (status <= 0)
		return "The do not connectioned";
	
	return string(reB);
}

string Server::recvMessage(SOCKET clientS) {
	return recvMessage(clientS, MAX_BUFFER_SIZE);
}

void Server::addUser(string name) {
	LM.lock();		// 无法同时访问用户列表							
	user_list.push_back(new UserInfo(name));
	LM.unlock();
}

UserInfo* Server::getUser(string name) {
	UserInfo* user = nullptr;//无法同时访问用户列表
	
	LM.lock();									
	for (size_t i = 0; i < user_list.size(); i++) {
		if (user_list.at(i)->name == name) {
			user = user_list.at(i);
			break;
		}
	}
	LM.unlock();
	
	return user;
}

string Server::getUserList() {
	string result;
	
	LM.lock();		// 无法同时访问用户列表							
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
	LM.unlock();
	
	return result;
}

void Server::connection(SOCKET clientS, SOCKADDR_IN clientAD) {//通過字串接收處理 Client 端離線前的通知，發送上線清單給 client,
	UserInfo* connected_user = nullptr;
	while (true) {
		string recv_msg = recvMessage(clientS);
		
		if (recv_msg == "The do not connectioned") {
			break;
		}
		else if (recv_msg.substr(0, 9) == "REGISTER#") {						// 輸入 register+空格
			if (connected_user == nullptr) {
				string user_name = recv_msg.substr(9, recv_msg.length() - 10);	// 輸入 register+空格后是用戶名字
				if (user_name != "REGISTER" && user_name != "") {				// 不是輸入register和名字沒寫，都無法注冊成功
					if (getUser(user_name) == nullptr) {						// 需要未被注冊的名字
						addUser(user_name);
						sendMessage(clientS, "100 OK\n");
						continue;
					}
				}
				
			}
			else {
				sendMessage(clientS, "The Invalid input\n");
				continue;
			}
			
			sendMessage(clientS, "210 FAIL\n");//無法成功注冊
		}
		else if (recv_msg.substr(0, 6) == "MONEY#") {						// 輸入money+空格
			if (connected_user != nullptr) {
				int money = stoi(recv_msg.substr(6, recv_msg.length()-7));	// 輸入money+空格是數字
				connected_user->money = money;
				sendMessage(clientS, to_string(connected_user->money) + "\n");
			}
			else {
				sendMessage(clientS, "The Invalid input\n");
				continue;
			}
			
		}
		else if (recv_msg == "List\n") {								// 輸入list
			if (connected_user != nullptr) {
				sendMessage(clientS, to_string(connected_user->money) + "\n");
				this_thread::sleep_for(chrono::milliseconds(100));		
				sendMessage(clientS, getUserList());
			}
			else {
				sendMessage(clientS, "220 AUTH_FAIL\n");
			}
		}
		else if (recv_msg == "Exit\n") {								// 輸入exit,不再想連接
			if (connected_user != nullptr) {//nullptr空的意思
				sendMessage(clientS, "Bye\n");
				connected_user->online = false;
				break;
			}
			else {
				sendMessage(clientS, "220 AUTH_FAIL\n");
			}
		}
		else {															
			if (connected_user == nullptr) {							
				string user_name = recv_msg.substr(0, recv_msg.find('#'));
				connected_user = getUser(user_name);
				if (connected_user != nullptr) {
					recv_msg.erase(0, recv_msg.find('#') + 1);			// 獲得 client port
					recv_msg.erase(recv_msg.length() - 1, 1);			// Erase 回車換行
					connected_user->addr = inet_ntoa(clientAD.sin_addr);
					connected_user->port = recv_msg;
					connected_user->online = true;						// 以免錯誤信息
					
					sendMessage(clientS, to_string(connected_user->money) + "\n");
					this_thread::sleep_for(chrono::milliseconds(350));	
					sendMessage(clientS, getUserList());
				}
				else {
					sendMessage(clientS, "220 AUTH_FAIL\n");
				}
			}
			else {
				sendMessage(clientS, "The Invalid input\n");
			}
		}
	}
	
	PM.lock();
	cout << "Disconnected from " << inet_ntoa(clientAD.sin_addr) << endl;
	PM.unlock();
	
	closesocket(clientS);
}


