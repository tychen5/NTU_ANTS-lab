#include <stdio.h>
#include <cstring>
#include <string>
//#include <winsock2.h>
//#include <windows.h>
#include <sstream>
#pragma comment (lib, "ws2_32.lib")

using namespace std;

int user_count = 0;
string user_name [100];
bool user_state [100] = {0};
string user_port [100];
string user_IP[100];
//record workers status
SOCKET counter[3];
bool idle_worker[3];
//find idle worker to serve client
int find_idle_worker()
{
	for (int i = 0; i < 3; i++)
	{
		if (idle_worker[i])
		{
			return i;
		}
	}
	return -1;
}

string online_list()
{
	int online_count = 0;
	for(int i = 0;i < user_count;i ++)
	{
		if(user_state[i])
			online_count ++;
	}
	stringstream ss;
	ss << online_count;
	string temp_count;
	ss >> temp_count; 

	string re = "AccountBalence:10000\n";
	re += ("Number of users:" + temp_count + "\n\n");
	for (int i = 0;i < user_count;i ++)
	{
		if(user_state[i])
			re += (user_name[i] + "#" + user_IP[i] + "#" + user_port[i] + "\n");
	}

	return re;
}
//check if user_name is in the list
int check_user(string name, string list[])
{
	for (int i = 0;i < user_count;i ++)
	{
		if (name == list[i])
			return i;
	}
	return -1;
}
//adjust input login_port to correct form
string form_login_port(string input)
{
	int toInt = atoi(input.c_str());

	stringstream toStr;
	toStr << toInt;
	string re = toStr.str();

	return re;
}
//get connection IP by connected socket
/*
string get_IP(SOCKET socket_ID)
{
	int len;
	struct sockaddr_storage addr;
	len = sizeof addr;
	getpeername(socket_ID, (struct sockaddr*)&addr, &len);

	char ipbuff[INET_ADDRSTRLEN];
	inet_ntop(addr.ss_family, &(((struct sockaddr_in *)&addr)->sin_addr), ipbuff, INET_ADDRSTRLEN);
	char *ip = inet_nota(addr.sin_addr);
	string re(ip);

	return re;
}
*/
LPVOID service(LPVOID ID)
{
	string request;
	string response;
	bool online = true;
	int worker_ID = *((int *)ID);
	int thisUser = -1;
	SOCKET new_socket = counter[worker_ID];
//	int login_pos;

	//welcome msg
	char sendbuf[] = "successful connection";
	send(new_socket, sendbuf, (int)strlen(sendbuf), 0);
	char recvbuf[10000] = {0};

	while (online)
	{
		//wait for request
		recv(new_socket, recvbuf, sizeof(recvbuf), 0);
		request.assign(recvbuf);
		//return online_list
		if (request == "List")
			response = online_list();
		//user logout
		else if (request == "Exit")
		{
			char sendbuf[] = "Bye";
			send(new_socket, sendbuf, (int)strlen(sendbuf), 0);
			user_state[thisUser] = 0;
			online = false;
		}
		//register
		else if (request.substr(0, 8) == "REGISTER")
		{
			string name = request.substr(9, (request.length() - 9)); 
		
			if (check_user(name, user_name) >= 0)
				response = "210 FAIL\n";
			else
			{
				user_name[user_count] = name;
				user_count ++;
				response = "100 OK\n";
			}
		}
		//login or forbidden request
		else
		{
			int pos_hashtag = request.find("#");
			string login_name = request.substr(0, pos_hashtag);
			string port_num;
			if (login_name.length() >= request.length())
				response = "??unknown??";
			else
			{
				port_num = form_login_port(request.substr(pos_hashtag + 1, request.length() - pos_hashtag));
				int temp = check_user(login_name, user_name);
				//check if user_name has registered
				if (temp < 0)
				{
					response = "220 AUTH_FAIL\n";
				}
				//check if user_name has login
				else if (user_state[temp] || (check_user(port_num, user_port) >= 0))
				{
					response = "230 user or port occupied!";
				}
				//user_name can login
				else
				{
					//login
					thisUser = temp;
					user_state[thisUser] = true;
					user_port[thisUser] = port_num;
				//	user_IP[thisUser] = get_IP(new_socket);
					
					//return online_list
					response = online_list();

				}
			}
		}
		
		const char *sendbuf = response.c_str();
		send(new_socket, sendbuf, (int)strlen(sendbuf), 0);
		
		
	}
	//when user_name logout, free the worker
	idle_worker[worker_ID] = true;
	closesocket(new_socket);

}

int main()
{
    int r;
    WSAData wsaData;
    WORD DLLVSERION;
    DLLVSERION = MAKEWORD(2,1);//Winsocket-DLL 版本
 
    //用 WSAStartup 開始 Winsocket-DLL
    r = WSAStartup(DLLVSERION, &wsaData);
 
    //宣告 socket 位址資訊(不同的通訊,有不同的位址資訊,所以會有不同的資料結構存放這些位址資訊)
    SOCKADDR_IN addr;
    int addrlen = sizeof(addr);
 
    //建立 socket
    SOCKET sListen; //listening for an incoming connection
  //  SOCKET sConnect; //operating if a connection was found
 
    //AF_INET：表示建立的 socket 屬於 internet family
    //SOCK_STREAM：表示建立的 socket 是 connection-oriented socket 
  //  sConnect = socket(AF_INET, SOCK_STREAM, NULL);
 
    //設定位址資訊的資料
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
 
    //設定 Listen
    sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);//SOMAXCONN: listening without any limit


	bool online = true;
	bool busy = false;

	for (int i = 0; i < 3; i++)
		idle_worker[i] = true;
	
	DWORD threadId[3];
	HANDLE hthread[3];
	
	
    //等待連線
  	SOCKADDR_IN clinetAddr;
    while (online)
	{
		//accept connection
		SOCKET new_client;
		new_client = socket(AF_INET, SOCK_STREAM, NULL);
		new_client = accept(sListen, (SOCKADDR*)&clinetAddr, &addrlen);
		
		int worker_ID = find_idle_worker();
    
    	if (worker_ID != -1)
		{
			idle_worker[worker_ID] = false;
			counter[worker_ID] = new_client;
			int *worker_pointer = &worker_ID;
			hthread[worker_ID] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)service, worker_pointer, 0, &threadId[worker_ID]);
			/*close socket and thread*/
			CloseHandle(hthread[worker_ID]);
		
		}
		//busy - refuse connection
		else
		{
			char sendbuf[] = "200 Busy. Please try again later.";
			send(new_client, sendbuf, (int)strlen(sendbuf), 0);
			closesocket(new_client);
		}
      
    }
 	
 	closesocket(sListen);
}
