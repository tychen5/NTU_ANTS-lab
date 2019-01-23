//Client
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream> 
#include<string>
#include<algorithm>
#pragma warning(disable:4996) 

#pragma comment(lib, "Ws2_32.lib")
using namespace std;

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
	addr.sin_addr.s_addr = inet_addr("140.112.107.194"); // 設定 IP	Server IP: 140.112.107.194 port=33120							   // addr.sin_addr.s_addr = INADDR_ANY; // 若設定 INADDR_ANY 表示任何 IP
	addr.sin_family = AF_INET;
	addr.sin_port = htons(33120); // 設定 port 33120

	//Set Connection socket
	SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL); 
	bind(Connection, (SOCKADDR*)&addr, addrlen);
	if (connect(Connection, (SOCKADDR*)&addr, addrlen) != 0) //If we are unable to connect...
	{
		cout << "failed to connect to server!!" << endl;
		return 0; //Failed to Connect
	}
	else
	{
		cout << "Connect to server IP: " << inet_ntoa(addr.sin_addr) << ", Port: " << (int)ntohs(addr.sin_port) << "……" << endl;
		//cout << "connect to server successfully!!" << endl;
		char text[1000] = "";
		char command[1000] = "";
		string UID;
		string PAY;
		string UID_2;
		recv(Connection, text, sizeof(text), NULL);
		cout << text << endl;
		//CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientThread, NULL, NULL, NULL); //Create the client thread that will receive any data that the server sends.

		while (true)
		{
			cout << "r:register, l:log in, o:money and online client list, t: transaction, e:exit" << endl;
			memset(text, 0, sizeof(text));
			memset(command, 0, sizeof(command));
			cin.getline(command, sizeof(command));
			string mean = string(command);
			if (mean == "r")
			{
				cout << "Please enter the UserAccountName:" << endl;
				cin.getline(command, sizeof(command));
				mean = string(command);
				mean = "REGISTER#" + mean + "\n";
				cout << "send to server: " << mean << endl;
				memset(command, 0, sizeof(command));
				strcpy(command, mean.c_str());
				send(Connection, command, sizeof(command), NULL);
			}
			else if (mean == "l")
			{
				cout << "Please enter UserAccountName:" << endl;
				cin.getline(command, sizeof(command));
				UID = string(command);
				cout << "Please enter Port Number:" << endl;
				cin.getline(command, sizeof(command));
				string PORT = string(command);
				mean = UID + "#" + PORT + "\n";
				//cout << "send to server: " << mean << endl;
				memset(command, 0, sizeof(command));
				strcpy(command, mean.c_str());
				cout << "send to server: " << command << endl;
				send(Connection, command, sizeof(command), NULL);
				memset(text, 0, sizeof(text));
				recv(Connection, text, sizeof(text), NULL);
				cout << "response from server: " << text << endl;
			}
			else if (mean == "o")
			{
				mean = "List\n";
				//cout << "send to server: " << mean << endl;
				memset(command, 0, sizeof(command));
				strcpy(command, mean.c_str());
				cout << "send to server: " << command << endl;
				send(Connection, command, sizeof(command), NULL);
				memset(text, 0, sizeof(text));
				recv(Connection, text, sizeof(text), NULL);
				cout << "response from server: " << text << endl;
			}
			else if (mean == "t")
			{
				cout << "Please enter UserAccountName:" << endl;
				getline(cin, UID);
				cout << "Please enter payAmount:" << endl;
				getline(cin, PAY);
				cout << "Please enter PayeeUserAccountName:" << endl;
				getline(cin, UID_2);
				mean = UID + "#" + PAY + "#" + UID_2 + "\n";
				memset(command, 0, sizeof(command));
				strcpy(command, mean.c_str());
				cout << "send to server: " << command << endl;
				send(Connection, command, sizeof(command), NULL);
			}
			else if (mean == "e")
			{
				mean = "Exit\n";
				//cout << "send to server: " << mean << endl;
				memset(command, 0, sizeof(command));
				strcpy(command, mean.c_str());
				cout << "send to server: " << command << endl;
				send(Connection, command, sizeof(command), NULL);
				memset(text, 0, sizeof(text));
				recv(Connection, text, sizeof(text), NULL);
				cout << "response from server: " << text << endl;
				break;
			}
			else
			{
				send(Connection, command, sizeof(command), NULL);
			}
			
			if (string(text) == "220 AUTH_FAIL\n" | string(text) == "please type the right option number!\n")
			{
				continue;
			}
			memset(text, 0, sizeof(text));
			recv(Connection, text, sizeof(text), NULL);
			cout << "response from server: " << text << endl;
		}
		cout << "Diconnect to server IP: " << inet_ntoa(addr.sin_addr) << ", Port: " << (int)ntohs(addr.sin_port) << " sucessfully!!" << endl;
		closesocket(Connection);
		
		
	}
	system("pause");





	return 0;
}