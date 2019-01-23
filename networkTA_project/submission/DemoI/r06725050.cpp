#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <Winsock2.h>//引用socket 
#define __USE_MINGW_ANSI_STDIO 0
#define MAX_BUFFER_SIZE 1024//初始空間 

using namespace std;

class Client//物件client
{
private:
	SOCKET _sock;//类别SOCKET,名称sock 
	SOCKADDR_IN _address;//类别SOCKADDR,名称address 
	
public: 
	Client(char* ip, uint16_t port);//传入client 函數名稱，contrusted建構物件 
	string account;// 字符串類型 
	bool logged_in;//登入，成功或失敗 
	bool connecttry();//呼叫，尝试连线 	
	void sendMessage(char* buffer); 
	void sendMessage(string buffer);//传信息 
	string recvMessage(int buffer_size);
	string recvMessage();	
	void closeConnection();	
};

int main(int argc, char** argv){//argv 字串整列，argc 一共幾個傳入的東西，便於cmd運行 
	char* server_ip;
	uint16_t port;
	if (argc == 1) {//如果只有打檔案名稱沒有輸入port ip，就用預設									
		server_ip = "140.112.107.194";//預設 
		port = 33120;//預設 
	}
	else {
		server_ip = argv[1];//如果有打port IP，就用輸入的 
		port = atoi(argv[2]);
	}
	
	Client client(server_ip, port);//類比 變數名稱 constructing 調用的 
	while (!client.connecttry()) {//当这个function回传一个false，就run下面的 
		while (true) {
			cout << "Do you want connect again? (please enter true/false): ";		
			string user_input;
			cin >> user_input;
			if (user_input == "true" || user_input == "True" ||user_input == "TRUE")//如果兩個運算元中任一個或兩個都是true，則邏輯OR 運算子( || ) 會傳回布林值true，否則會傳回false 
				break;
			else if (user_input == "false" || user_input == "False"|| user_input == "FALSE")//如果兩個運算元中任一個或兩個都是true，則邏輯OR 運算子( || ) 會傳回布林值true，否則會傳回false
				exit(0);
			else
				cout << "Input is wrong" << endl;
		}
		
		cin.ignore();									// 清除使用者的 input  
	}
	
	client.recvMessage(128);							// 空間128 
	cout << "connection accepted " << endl;//成功连接 
	cout << "User Input Rule" << endl;
	cout << "register(space)<register name> " << endl;
	cout << "login (space) <register Name> (space)<Port> " << endl;
	cout << "list " << endl;
	cout << "exit " << endl;
	while (true) {
		if (client.logged_in)
			cout << client.account << ' ';
		cout << ">>> ";
		string user_input;
		getline(cin, user_input);						// 讓使用者輸入 
		
		if (user_input.substr(0, 9) == "register ") {	// 一共9字(包括空格) ，就会run下面functuion 
			if (!client.logged_in) {					// 無法成功時候，直接login時候，才执行下面 
				string user_name = user_input.substr(9, user_input.length() - 9);
				if (user_name == "") {
					cout << "Input is wrong, please input \"register (space) <Register Name>\" ." << endl;
					continue;
				}
				client.sendMessage("REGISTER#" + user_name + "\n");
			}
			else {
				cout << "Input is informal command" << endl << endl;
				continue;
			}
		}
		else if (user_input.substr(0, 6) == "login ") {	// 一共6個字（包括空格）如果是login 就run 下面，Try to login
			if (!client.logged_in) {					// 無法成功登錄時，才執行下面 
				user_input.erase(0, 6);
				string user_name;
				if (user_input.find(' ') != string::npos) {
					user_name = user_input.substr(0, user_input.find(' '));
					user_input.erase(0, user_input.find(' ') + 1);
				}
				else {
					cout << "Input is wrong, please \"login (space) <Register Name> (space)<Port>\"." << endl;
					continue;
				}
				try {
					int input_port = stoi(user_input);
					if (input_port > 65535 || input_port < 0)//Port 不能大於65535，小於0 
						throw "illegal number";
				}
				catch (...) {
					cout << "Input port number is wrong" << endl;
					continue;
				}
				
				client.sendMessage(user_name + "#" + user_input + "\n");
				string result = client.recvMessage();//接收login的result 
				if (result.substr(0, 3) == "220") {		// login 失敗 
					cout << result << endl;
					continue;
				}
				else {
					client.account = user_name;
					client.logged_in = true;
					cout << "This is Account Balance: " << result << endl;
				}
			}
			else {
				cout << "Input is informal command" << endl << endl;
				continue;
			}
		}
		else if (user_input == "list") {				// 目前List的用戶 
			if (client.logged_in) {						// 成功登入的時候 
				client.sendMessage("List\n");
				
				string result = client.recvMessage();
				if (result.substr(0, 3) == "220") {		// login失敗 
					cout << result << endl;
					continue;
				}
				else {
					cout << "This is Account Balance: " << result << endl;
				}
			}
			else {
				cout << "Input is informal command" << endl << endl;
				continue;
			}
		}
		else if (user_input == "exit") {				// 離開 
			break;
		}
		else {
			cout << "Input is informal command" << endl << endl;
			continue;
		}
		
		cout << client.recvMessage() << endl;
	}
	
	client.closeConnection();
	
	return 0; 
}

Client::Client(char* ip, uint16_t port) {//調用 物件 +函數名稱  ，constructed 
	WSAData wsaData;//初始化 
	WORD version = MAKEWORD(2, 2); // 版本 
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);	//返回值 0 是 success，1是失敗 
	if (iResult != 0){
		cout << "Initial WinSock failed." << endl;
		exit(1);
	}
	_sock = INVALID_SOCKET;//創建socket 
	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_sock == INVALID_SOCKET){
		cout << "Create socket failed." << endl;
		exit(1);
	}
	
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = inet_addr(ip);			// 設置 IP address
	_address.sin_port = htons(port);					// 設置 port number
	
	account = "";//初始化，使用者名稱是空的  
	logged_in = false;//初始化，是false 
}

bool Client::connecttry() {
	int result = connect(_sock, (SOCKADDR*)&_address, sizeof(_address));
	if (result == -1){//連綫失敗 
		cout << "Can not connect to server." << endl;
		return false;
	}
	
	return true;
}

void Client::sendMessage(char* buffer) {
	send(_sock, buffer, (int)strlen(buffer), 0);
}

void Client::sendMessage(string buffer) {//物件的一個函數 ，轉行格式char 在呼叫 
	char send_buffer[buffer.length()];
	strcpy(send_buffer, buffer.c_str());
	
	sendMessage(send_buffer);
}

string Client::recvMessage(int buffer_size) {//物件的一個函數
	char recv_buffer[buffer_size];
	ZeroMemory(recv_buffer, buffer_size);				// 清除 
	int status = recv(_sock, recv_buffer, sizeof(recv_buffer), 0);
	if (status == 0) {//失敗連綫時 
		cout << "Currently,server has closed the connection." << endl;
		exit(0);
	}
	
	return string(recv_buffer);
}

string Client::recvMessage() {
	return recvMessage(MAX_BUFFER_SIZE);
}

void Client::closeConnection() {
	if (logged_in){								
		sendMessage("Exit\n");
		cout << recvMessage(128);
	}
	
	closesocket(_sock);
}

