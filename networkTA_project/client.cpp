#include <stdio.h>
#include <cstring>
#include <string>
#include <winsock2.h>
#pragma comment (lib, "ws2_32.lib")

#include <iostream>
// jijij
using namespace std;
//ksdjhflskdjf
void error(string printout) {
    cout << printout << endl;
    exit(0);
}
void output(string printout) {
    cout << printout;
}

int main()
{
	// 初始化windows socket DLL 
	WSAData wsaData;
	WORD version = MAKEWORD(2, 2); // 版本
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData); // 成功回傳 0
	if (iResult != 0) {
	    // 初始化Winsock 失敗
	}
	// 建立socket

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) 
		error("ERROR in setting socket");

	// 設定IP 跟 port
	SOCKADDR_IN addr;
	memset (&addr, 0, sizeof (addr)) ; // 清空,將資料設為 0
	addr.sin_family = AF_INET;

	string str;//輸入IP 
	cout << "Please enter the IP address: ";
	cin >> str;
	const char *cp = str.c_str();//儲存IP 
	u_short hostshort = 10000;//輸入port number 
	cout << "Please enter the portNumber: ";
	cin >> hostshort; 
	addr.sin_port = htons(hostshort); // 設定 port
	addr.sin_addr.s_addr = inet_addr(cp); // 設定 IP

	
	// 連線到socket server
	int r = connect(sock, (struct sockaddr *)&addr, sizeof (addr));
	if (r == SOCKET_ERROR)
		error("ERROR in connecting");
	
	//recieve server的訊息 
	char recv1[1024];
	if(recv(sock, recv1, sizeof(recv1), 0) < 0)
		cout << "Error of receive message." << endl;
	cout << recv1 << endl;
	
	//開始request server 

	cout << "Enter 1 for Register, 2 for Login: ";	
	int input;
	bool login = 0;//是否成功登入 
	bool end = 0;//是否結束 
	while(!end)
	{
		cin >> input;
		char name[100] = {0};
		char recvbuf[10000] = {0};
		if (input == 1)
		{
			if(!login)/*尚未登入成功*/ 
			{
				cout << "Enter the name u want to Register: ";
				cin >> name;
				char sendbuf[]= "REGISTER#";/*將REGISTER#name傳給server*/
				strcat(sendbuf, name);
				send(sock, sendbuf, (int)strlen(sendbuf), 0);
				recv(sock, recvbuf, sizeof(recvbuf), 0); 
				cout << "-----------------------" << endl;
				cout << recvbuf << endl;
				cout << "Enter 1 for Register, 2 for Login: ";
		
			}
			else/*已經成功登入*/ 
			{
				char sendbuf[] = "List";
				send(sock, sendbuf, (int)strlen(sendbuf), 0);
				recv(sock, recvbuf, sizeof(recvbuf), 0);
				cout << "-----------------------" << endl;
				cout << recvbuf << endl;
				cout << "1 to ask for the latest list, 8 to Exit: ";
			}
		}
		else if(input == 2)
		{
			cout << "Enter the name u want to Login: ";
			cin >> name;
			char port[10];
			cout << "Enter the port number: ";
			cin >> port;
			// 將name#portnum傳給server
			char sendbuf[]= "#";
			strcat(sendbuf, port);
			strcat(name, sendbuf);
			strcpy(sendbuf, name);
			send(sock, sendbuf, (int)strlen(sendbuf), 0);
			recv(sock, recvbuf, sizeof(recvbuf), 0);
            cout << "-----------------------" << endl;
			cout << recvbuf << endl;
			
			if(recvbuf[0] != '2')/*登入成功*/ 
            {
            	cout << "Log in successfully" << endl;
				cout << "Enter the number of actions u want to take." << endl;
				cout << "1 to ask for the latest list, 8 to Exit: ";
				login = 1;
			}
			else
			{
				cout << "Enter 1 for Register, 2 for Login: ";
			}
		}
		else if(input == 8)
		{
			char sendbuf[] = "Exit";
			send(sock, sendbuf, (int)strlen(sendbuf), 0);
			recv(sock, recvbuf, sizeof(recvbuf), 0);
			cout << recvbuf << endl;
			end = 1;
		}
		else
			cout << "ERROR, please input again\n";
	}
	
	closesocket(sock); // 結束socket 
	
}                                     
