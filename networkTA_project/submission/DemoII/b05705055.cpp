#pragma comment(lib, "Ws2_32.lib")
 
#include <WinSock2.h>
#include <iostream>
#include <pthread.h>
#include <string>
#include <algorithm>
#include <sstream>
 
using namespace std;

const int PORT_NUM = 12345;
const int MSG_SIZE = 100;
const int RQT_SIZE = 100;
const int THREAD_LIMIT = 30;
int thread_occupy[THREAD_LIMIT] = {0};
int signin_status[THREAD_LIMIT] = {0};
int online_num = 0;
string client_name[THREAD_LIMIT] = {""};
string IP_address[THREAD_LIMIT] = {""};
int account_balance[THREAD_LIMIT] = {0};

struct CLIENT{
	SOCKET client_id;
	string IP;
};

void *service(void *socket_data){
	
	struct CLIENT *sock_data = (struct CLIENT *)socket_data;
	SOCKET sock = sock_data -> client_id;
	string now_IP = sock_data -> IP;
	
	char message[MSG_SIZE] = "Connection accept!";
    send(sock, message, (int)strlen(message), 0);
    Sleep(500);
    int whether_signin = 0;
    int thread_self = THREAD_LIMIT;
    
    while(true){
    	ZeroMemory(message, MSG_SIZE);
		recv(sock, message, sizeof(message), 0);
		Sleep(500);
		string request = message;
		
		// register
		string request_type = request;
		request_type = request_type.substr(0, 9);
		if (request_type == "REGISTER#"){
			
			if (whether_signin == 1){
				char reply[RQT_SIZE] = "You cannot register when you have signed in!\n";
    			send(sock, reply, (int)strlen(reply), 0);
    			Sleep(500);
    			continue;
			}
			
			int repeat = 0;
			string UserName = request.substr(9, RQT_SIZE);
			for (int name = 0; name < THREAD_LIMIT; name++){
				if (UserName == client_name[name]){
					char reply[RQT_SIZE] = "210 FAIL\n";
    				send(sock, reply, (int)strlen(reply), 0);
    				Sleep(500);
    				repeat = 1;
					break;
				}
			}
			
			if(repeat == 0){
				int thread_no = THREAD_LIMIT;
				for (int no = 0; no < THREAD_LIMIT; no++){
					if (thread_occupy[no] == 0){
						thread_no = no;
						break;
					}
				}
				if (thread_no == THREAD_LIMIT){
					char reply[RQT_SIZE] = "210 FAIL\n";
    				send(sock, reply, (int)strlen(reply), 0);
    				Sleep(500);
				}
				else{
					client_name[thread_no] = UserName;
					IP_address[thread_no] = now_IP;
					account_balance[thread_no] = 10000;
					thread_occupy[thread_no] = 1;
					char reply[RQT_SIZE] = "100 OK\n";
					cout << UserName << " is registered." << endl << endl
					<< "Waiting for client to connect..." << endl << endl;
    				send(sock, reply, (int)strlen(reply), 0);
    				Sleep(500);
				}
			}
			continue;
		}
		
		// list
		request_type = request;
		request_type = request_type.substr(0, 4);
		if (request_type == "List"){
			if (whether_signin == 0){
				char reply[RQT_SIZE] = "220 AUTH_FAIL\n";
    			send(sock, reply, (int)strlen(reply), 0);
    			Sleep(500);
    			continue;
			}
			
			char reply[RQT_SIZE];
			stringstream s;
			s << account_balance[thread_self];
			string ss = s.str() + "\n";
			strcpy(reply, ss.c_str());
    		send(sock, reply, (int)strlen(reply), 0);
    		Sleep(500);
    			
    		char reply_list_f[RQT_SIZE];
    		stringstream s2;
    		s2 << online_num;
    		string reply_list = "number of accounts online: " + s2.str() + "\n";
			
			for (int no = 0; no < THREAD_LIMIT; no++){
				if (signin_status[no] == 1){
						
					stringstream s3;
					s3 << PORT_NUM;
					reply_list = reply_list + client_name[no] + "#" + IP_address[no] + "#" + s3.str() + "\n";
				}
			}
			strcpy(reply_list_f, reply_list.c_str());
			send(sock, reply_list_f, (int)strlen(reply_list_f), 0);
			cout << client_name[thread_self] << " requests the newest online list." << endl;
			cout << "The newest online list:" << endl;
			cout << reply_list_f << endl << "Waiting for client to connect..." << endl << endl;
			Sleep(500);
			continue;
			
		}
		
		// exit
		request_type = request;
		request_type = request_type.substr(0, 4);
		if (request_type == "Exit"){
			
			if (whether_signin == 0){
				char reply[RQT_SIZE] = "You have not yet signed in!\n";
    			send(sock, reply, (int)strlen(reply), 0);
    			Sleep(500);
    			continue;
			}
			else{
				thread_occupy[thread_self] = 0;
				signin_status[thread_self] = 0;
				online_num -= 1;
				cout << client_name[thread_self] << " is offline." << endl << endl
				<< "Waiting for client to connect..." << endl << endl;
				client_name[thread_self] = "";
				IP_address[thread_self] = "";
				account_balance[thread_self] = 0;
				
				char reply[RQT_SIZE] = "Bye\n";
    			send(sock, reply, (int)strlen(reply), 0);
    			Sleep(500);
    			break;
			}
		}
		
		
		// signin
		request_type = request;
		size_t position = request_type.find("#");
		string signin_name = request_type.substr(0, position);
		string portnum = request_type.substr(position+1, RQT_SIZE);
		
		if (whether_signin == 1){
			char reply[RQT_SIZE] = "220 AUTH_FAIL\n";
    		send(sock, reply, (int)strlen(reply), 0);
    		Sleep(500);
    		continue;
		}
		
		int exist = 0;
		for (int thread_no = 0; thread_no < THREAD_LIMIT; thread_no++){
			if (client_name[thread_no] == signin_name){
				
				if (signin_status[thread_no] == 1)
    				break;
				
				online_num += 1;
				thread_self = thread_no;
				exist = 1;
				whether_signin = 1;
				signin_status[thread_no] = 1;
				
				char reply[RQT_SIZE];
				int index = find(client_name, client_name + THREAD_LIMIT, signin_name) - client_name;
				stringstream s;
				s << account_balance[index];
				string ss = s.str() + "\n";
				strcpy(reply, ss.c_str());
    			send(sock, reply, (int)strlen(reply), 0);
    			Sleep(500);
    			
    			char reply_list_f[RQT_SIZE];
    			stringstream s2;
    			s2 << online_num;
    			string reply_list = "number of accounts online: " + s2.str() + "\n";
    			
				for (int no = 0; no < THREAD_LIMIT; no++){
					if (signin_status[no] == 1){
						
						stringstream s3;
						s3 << PORT_NUM;
						reply_list = reply_list + client_name[no] + "#" + IP_address[no] + "#" + s3.str() + "\n";
					}
				}
				strcpy(reply_list_f, reply_list.c_str());
				send(sock, reply_list_f, (int)strlen(reply_list_f), 0);
				cout << signin_name << " has logged in." << endl;
				cout << "The newest online list:" << endl;
				cout << reply_list_f << endl << "Waiting for client to connect..." << endl << endl;
				Sleep(500);
				break;
			}
		}
		if (exist == 0){
			char reply[RQT_SIZE] = "220 AUTH_FAIL\n";
    		send(sock, reply, (int)strlen(reply), 0);
    		Sleep(500);
    		continue;
		}
	}
	pthread_exit(0);
}

int main()
{
    pthread_t threadid[THREAD_LIMIT];
	int thread_now = 0;
    
    WSAData wsaData;
    WORD DLLVSERION;
    WSAData wsa;
	WORD version = MAKEWORD(2, 2);
	
	int Result = WSAStartup(version, &wsa);
    
	SOCKADDR_IN targetAddress;
	int addrlen = sizeof(targetAddress);
	targetAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    targetAddress.sin_family = AF_INET;
    targetAddress.sin_port = htons(PORT_NUM);

	SOCKET listen_client = socket(AF_INET, SOCK_STREAM, NULL);
    bind(listen_client, (SOCKADDR*)&targetAddress, sizeof(targetAddress));
    listen(listen_client, SOMAXCONN);
    
    SOCKET client = socket(AF_INET, SOCK_STREAM, NULL);
	SOCKADDR_IN clinetAddr;
    
    while (true){
    	cout << "Waiting for client to connect..." << endl << endl;
    	
    	if (client = accept(listen_client, (SOCKADDR*)&clinetAddr, &addrlen)){
    		cout << "A client conncets." << endl;
    		printf("Connection IP: %s\n\n", inet_ntoa(clinetAddr.sin_addr));
    		
    		struct CLIENT thread_data;
    		thread_data.client_id = client;
    		string IPs = inet_ntoa(clinetAddr.sin_addr);
    		thread_data.IP = IPs;
    		pthread_create(&threadid[thread_now], NULL, service, (void *)&thread_data);
    		thread_now += 1;
		}
	}
	return 0;
}
