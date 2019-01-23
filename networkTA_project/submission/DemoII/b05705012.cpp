//Server
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream> 
#include<string>
#include<algorithm>
#include<vector>
#pragma warning(disable:4996) 

#pragma comment(lib, "Ws2_32.lib")
using namespace std;

vector<pair<string, int>> user;
vector<vector<string>> Online; //記錄在線使用者的名字、IP、注明的port
vector<SOCKET> Pool;
vector<SOCKET> Queue; //排隊隊伍Queue
vector<int> Empty; //記錄空下來的線程的index值以再次利用
int maxQueue = 1; //排隊的最大人數，超過數量的client會連線到server失敗
int maxConnection = 2; //最多同時在線的人數

//為了傳入Thread三個值（index,IP,PORT）新建一個def
typedef struct data
{
	int index;
	char ip[20];
	int port;
}Data;

vector<Data> Queue_Data; //排隊隊伍對應的資料

//Client Thread
void ClientHandlerThread(LPVOID pM)
{
	Data *pt = (Data *)pM;
	string IP = pt->ip;
	int index = pt->index;
	int PORT = pt->port; //注意：小寫的port是client登入時注明（讓別的client透過此port來傳送訊息）的port，大寫PORT是client連線中的port
	char text[1000]; 
	bool login = FALSE;
	string name;
	while (true)
	{
		memset(text, 0, sizeof(text));
		recv(Pool[index], text, sizeof(text), NULL); //get message from client
		//如果登入會顯示接受訊息對象的名稱
		if (login == FALSE)
		{
			cout << "receive message from client: " << text << endl;
		}
		else
		{
			cout << "receive message from " << name << ": " << text << endl;
		}
		
		string mean = string(text);
		int jcount = 0; //"#"的個數
		for (int j = 0; j < mean.size(); j++) //計算Client請求中"#"的數量以辨別請求的類型
		{
			if (mean[j] == '#')
			{
				jcount++;
			}
		}
		if (mean.find("REGISTER#") == 0 & login == FALSE) //註冊(登入後無法註冊)
		{
			name = mean.substr(9);
			//去掉字尾的\r和\n
			name.erase(std::remove(name.begin(), name.end(), '\n'), name.end());
			name.erase(std::remove(name.begin(), name.end(), '\r'), name.end());
			cout << "REGISTER: Name: " << name << endl;
			bool find = FALSE;
			for (int i = 0; i < user.size(); i++)
			{
				if (user[i].first == name)
				{
					find = TRUE;
					break;
				}
			}
			if (find == TRUE) //註冊失敗
			{
				cout << "REGISTER: Name: " << name << " FAILED." << endl;
				mean = "210 FAIL\n";
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
			}
			else //註冊成功
			{
				cout << "REGISTER: Name: " << name << " SUCESSED." << endl;
				mean = "100 OK\n";
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
				//接收client傳來的金額
				recv(Pool[index], text, sizeof(text), NULL);
				cout << "receive message from client: " << text << endl;
				mean = string(text);
				mean.erase(std::remove(mean.begin(), mean.end(), '\n'), mean.end());
				mean.erase(std::remove(mean.begin(), mean.end(), '\r'), mean.end());
				int start = stoi(mean);
				user.push_back(make_pair(name, start));	
				//回傳成功
				mean = "Register Success\n";
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
			}
		}
		else if (jcount == 1 & login == FALSE) //登入
		{
			int k = mean.find("#");
			name = mean.substr(0, k);
			string port = mean.substr(k + 1);
			//去掉字尾的\r和\n
			port.erase(std::remove(port.begin(), port.end(), '\n'), port.end());
			port.erase(std::remove(port.begin(), port.end(), '\r'), port.end());
			cout << "LOG IN: Name: " << name << ", Port: "<< port << endl;
			//有註冊才能登入
			bool find = FALSE;
			for (int i = 0; i < user.size(); i++)
			{
				if (user[i].first == name)
				{
					find = TRUE;
					break;
				}
			}
			//防止重複登入
			bool isLogIn = FALSE;
			for (int i = 0; i < Online.size(); i++)
			{
				if (Online[i][0] == name) //名字一樣無法重複登入
				{
					isLogIn = TRUE;
					break;
				}
			}
			if (find == TRUE & isLogIn == FALSE) //登入成功
			{
				login = TRUE;
				cout <<"Client: " << name << " has logged in." << endl;
				vector<string> newOnline;
				newOnline.push_back(name);
				newOnline.push_back(IP);
				newOnline.push_back(port);
				Online.push_back(newOnline); //加入Online的list中
				//登入成功，輸出帳戶餘額
				for (int i = 0; i < user.size(); i++)
				{
					if (user[i].first == name)
					{
						mean = to_string(user[i].second) + "\n";
						break;
					}
				}
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
				//輸出上線清單
				mean = "number of accounts online: " + to_string(Online.size()) + "\n";
				for (int i = 0; i < Online.size(); i++)
				{
					mean += Online[i][0] + "#" + Online[i][1] + "#" + Online[i][2] + "\n";
				}
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
			}
			else if(isLogIn == FALSE) //登入失敗（未註冊）
			{
				cout << "Client: " << name << " failed to log in.(not registered)" << endl;
				mean = "220 AUTH_FAIL\n";
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
			}
			else //登入失敗（同名的帳戶已登入）
			{
				cout << "Client: " << name << " failed to log in.(already logged in)" << endl;
				mean = "220 AUTH_FAIL\n";
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
			}
		}
		else if (mean == "List\n" & login == TRUE) //餘額和上線帳號清單
		{
			//登入成功，輸出帳戶餘額
			for (int i = 0; i < user.size(); i++)
			{
				if (user[i].first == name)
				{
					mean = to_string(user[i].second) + "\n";
					break;
				}
			}
			memset(text, 0, sizeof(text));
			strcpy(text, mean.c_str());
			send(Pool[index], text, sizeof(text), NULL);
			//輸出上線清單
			mean = "number of accounts online: " + to_string(Online.size()) + "\n";
			for (int i = 0; i < Online.size(); i++)
			{
				mean += Online[i][0] + "#" + Online[i][1] + "#" + Online[i][2] + "\n";
			}
			memset(text, 0, sizeof(text));
			strcpy(text, mean.c_str());
			send(Pool[index], text, sizeof(text), NULL);
		}
		else if (jcount == 2 & login == TRUE) //匯款
		{
			//獲取交易對象名字和金額
			mean = mean.substr(mean.find("#")+1);
			int pay = stoi(mean.substr(0,mean.find("#")));
			mean = mean.substr(mean.find("#") + 1);
			//去掉字尾的\r和\n
			mean.erase(remove(mean.begin(), mean.end(), '\n'), mean.end());
			mean.erase(remove(mean.begin(), mean.end(), '\r'), mean.end());
			string other = mean; //other: 交易對象的名字
			for (int i = 0; i < user.size(); i++)
			{
				if (user[i].first == name)
				{
					user[i].second -= pay;
					break;
				}
			}
			for (int i = 0; i < user.size(); i++)
			{
				if (user[i].first == other)
				{
					user[i].second += pay;
					break;
				}
			}
			mean = name + "->" + to_string(pay) + "->" + other + "\n";
			memset(text, 0, sizeof(text));
			strcpy(text, mean.c_str());
			send(Pool[index], text, sizeof(text), NULL);
		}
		else if (mean == "Exit\n" & login == TRUE) //離開
		{
			//先發送Bye給Client
			mean = "Bye\n";
			memset(text, 0, sizeof(text));
			strcpy(text, mean.c_str());
			send(Pool[index], text, sizeof(text), NULL);
			//結束thread時退出登入，從在線清單中移除
			for (int i = 0; i < Online.size(); i++)
			{
				if (Online[i][0] == name)
				{
					Online.erase(Online.begin() + i);
					break;
				}
			}
			cout << "Client: " << name << ", IP: " << IP << ", Port: " << PORT << " is Disconnect." << endl;
			closesocket(Pool[index]);
			if (Queue.empty() == FALSE) //如果有人在Queue排隊, 就把這個空出來的thread給隊伍第一個，繼續迴圈
			{
				Pool[index] = Queue[0];
				Queue.erase(Queue.begin());
				//把新接納的Client資訊給拿出來
				Data data = Queue_Data[0];
				Queue_Data.erase(Queue_Data.begin());
				IP = string(data.ip);
				PORT = data.port;
				cout << "Connection from client IP: " << IP << ", Port: " << PORT << endl;
				string mean = "Hello, how can I serve you?";
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
				login = FALSE;
			}
			else //如果沒有人排隊，就把這個空的thread的index放到Empty裏面，結束迴圈
			{
				Empty.push_back(index);
				break;
			}	
		}
		else if (mean == "")
		{
			//Client間在通訊，什麼也不傳
			mean = "\n";
			memset(text, 0, sizeof(text));
			strcpy(text, mean.c_str());
			send(Pool[index], text, sizeof(text), NULL);
		}
		else
		{
			mean = "please type the right option number!\n";
			memset(text, 0, sizeof(text));
			strcpy(text, mean.c_str());
			send(Pool[index], text, sizeof(text), NULL);
		}
	}
	//跳出迴圈，tread結束
}


int main() {

	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 1), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 0;
	}

	SOCKADDR_IN addr;
	int addrlen = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 設定 IP	   // addr.sin_addr.s_addr = INADDR_ANY; // 若設定 INADDR_ANY 表示任何 IP
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345); // 設定 port


	//create socket descriptor
	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
	bind(sListen, (SOCKADDR*)&addr, addrlen);
	listen(sListen, SOMAXCONN);

	cout << "IP: " << inet_ntoa(addr.sin_addr) << endl;
	cout << "Port: " << (int)ntohs(addr.sin_port) << endl;

	SOCKET newConnection;
	while (TRUE)
	{
		newConnection = accept(sListen, (SOCKADDR*)&addr, &addrlen);
		if (Pool.size() == maxConnection) //已經新建線程數達到最大值
		{
			if (Empty.empty() == FALSE) //有空餘的線程可以使用
			{
				int index = Empty[0]; //取出一個空thread的編號
				Empty.erase(Empty.begin());	
				cout << "Connection from client IP: " << inet_ntoa(addr.sin_addr) << ", Port: " << (int)ntohs(addr.sin_port) << endl;
				if (newConnection == 0)
				{
					cout << "failed in creating new connection!!" << endl;
				}
				else
				{
					//cout << "connect to client successfully!!" << endl;
					char text[1000] = "Hello, how can I serve you?";
					send(newConnection, text, sizeof(text), NULL);
					Pool[index] = newConnection;
					//data記錄此Client的ip, index, port
					Data data;
					data.index = index;
					memset(data.ip, 0, sizeof(data.ip));
					strcpy(data.ip, string(inet_ntoa(addr.sin_addr)).c_str());
					data.port = (int)ntohs(addr.sin_port);
					CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandlerThread, (LPVOID)(&data), NULL, NULL); //Create Thread to handle this client. The index in the socket array for this thread is the value (i).		
				}
			}
			else if(Queue.size() < maxQueue) //沒有空餘的thread, Queue還有空間，讓它去Queue排隊
			{	
				Data data;
				data.index = 0; //其實是多餘的值所以隨便設定（因為是用前一個使用者的index）
				memset(data.ip, 0, sizeof(data.ip));
				strcpy(data.ip, string(inet_ntoa(addr.sin_addr)).c_str());
				data.port = (int)ntohs(addr.sin_port);
				Queue.push_back(newConnection);
				Queue_Data.push_back(data);
				cout << "Client Request IP: " << inet_ntoa(addr.sin_addr) << ", Port: " << (int)ntohs(addr.sin_port) << " has been added to Queue." << endl;
			}
			else //Queue滿了，drop掉
			{
				cout << "Client Request has dropped!!" << endl;
			}
		}
		else //還沒到最大線程數的時候
		{
			cout << "Connection from client IP: " << inet_ntoa(addr.sin_addr) << ", Port: " << (int)ntohs(addr.sin_port) << endl;
			if (newConnection == 0)
			{
				cout << "failed in creating new connection!!" << endl;
			}
			else
			{
				//cout << "connect to client successfully!!" << endl;
				char text[1000] = "Hello, how can I serve you?";
				send(newConnection, text, sizeof(text), NULL);
				Pool.push_back(newConnection);
				//data記錄此Client的ip, index, port
				Data data;
				data.index = Pool.size() - 1;
				memset(data.ip, 0, sizeof(data.ip));
				strcpy(data.ip, string(inet_ntoa(addr.sin_addr)).c_str());
				data.port = (int)ntohs(addr.sin_port);
				CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandlerThread, (LPVOID)(&data), NULL, NULL); //Create Thread to handle this client. The index in the socket array for this thread is the value (i).		
			}
		}
		
	}
	system("pause");





	return 0;
}