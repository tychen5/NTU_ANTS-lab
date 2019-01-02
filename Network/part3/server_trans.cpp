#include <stdio.h>
#include <cstring>
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <sstream>
#include <cstdlib>
#include <iostream>
#pragma comment (lib, "ws2_32.lib")
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#undef APPMACROS_ONLY 
#include <openssl/applink.c>

using namespace std;

int user_count = 0;
string user_name [100];
bool user_state [100] = {0};
string user_port [100];
string user_IP[100];
string user_account [100];

//Init server instance and context
SSL_CTX* InitServerCTX(void)
{   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc. 
    SSL_load_error_strings();   // load all error messages */
    method = SSLv3_server_method();  // create new server-method instance 
    ctx = SSL_CTX_new(method);   // create new context from method
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
//Load the certificate 
void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
    //set the local certificate from CertFile
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //set the private key from KeyFile
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //verify private key 
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}
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
string online_list(string name)
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

	int usercount = -1;
	for (int i = 0;i < user_count;i ++)
	{
		if(user_name[i] == name)
			usercount = i;
	}

	string re = "AccountBalence:" + user_account[usercount] + "\n";
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
string get_IP(SOCKET socket_ID)
{
	int len;
	struct sockaddr_in addr;
	char *some;
	int port;
	len = sizeof addr;
	getpeername(socket_ID, (struct sockaddr*)&addr, &len);
	some = inet_ntoa(addr.sin_addr);

	string re(some);
	return re;
}
//與client連線 
LPVOID service(LPVOID ID)
{
	string request;
	string response;
	string thisname;
	bool online = true;
	int worker_ID = *((int *)ID);
	int thisUser = -1;
	SOCKET new_socket = counter[worker_ID];
	
	SSL_library_init();
    SSL_CTX *ctx;
	ctx = InitServerCTX();
	char cert[100]= "mycert.pem";
    char key[100]= "mykey.pem";
	LoadCertificates(ctx, cert, key);
	SSL *ssl;
	ssl = SSL_new(ctx);              // get new SSL state with context 
    SSL_set_fd(ssl, new_socket);      // set connection socket to SSL state
    if (SSL_accept(ssl) == -1)  
    {  
		cout << "error in accepting to SSL." << endl;
    	perror("accept");  
    }
    
    //welcome msg
	char sendbuf[] = "successful connection";
	SSL_write(ssl, sendbuf, (int)strlen(sendbuf));
	
	while (online)
	{
		char recvbuf[10000] = {0};
		//wait for request
		SSL_read(ssl, recvbuf, sizeof(recvbuf));
		request.assign(recvbuf);
		//return online_list
		if (request == "List")
			response = online_list(thisname);
		//user logout
		else if (request == "Exit")
		{
			char sendbuf[] = "Bye";
			SSL_write(ssl, sendbuf, (int)strlen(sendbuf));
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
				user_account[user_count] = "10000";
				user_count ++;
				response = "100 OK\n";
			}
		}
		//client要求轉帳 
		else if (request.substr(0, 11) == "TRANSACTION")
		{
			string name = request.substr(12, (request.length() - 12));
			if (check_user(name, user_name) < 0)
				response = "320 FAIL\n";
			else
			{
				for (int i = 0;i < user_count;i ++)
				{
					if(user_name[i] == name)
						response = user_IP[i] + "#" + user_port[i];
				}
			}
		}
		//client 回報轉帳結果 
		else if (request.substr(0, 6) == "REPORT")
		{
			int last = request.find_last_of('#');
			int first = request.find('#');
	  		string clientname = request.substr(last +1, (request.length() - last));
			string payamount = request.substr(first + 1, last);
			stringstream ss;
			int pay;
			ss << payamount;
			ss >> pay;
			for (int i = 0;i < user_count;i ++)
			{
				if (user_name[i] == thisname)
				{
					stringstream ss1;
					int temp;
					ss1 << user_account[i];
					ss1 >> temp;
					temp += pay;
					
					stringstream ss;
					ss << temp;
					ss >> user_account[i];
				}
				if (user_name[i] == clientname)
				{
					stringstream ss1;
					int temp;
					ss1 << user_account[i];
					ss1 >> temp;
					temp -= pay;
					
					stringstream ss;
					ss << temp; 
					ss >> user_account[i];
				}
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
					thisname = login_name;
					thisUser = temp;
					user_state[thisUser] = true;
					user_port[thisUser] = port_num;
					user_IP[thisUser] = get_IP(new_socket);
					
					//return online_list
					response = online_list(thisname);
				}
			}
		}
			
			const char *sendbuf = response.c_str();
			SSL_write(ssl, sendbuf, (int)strlen(sendbuf));		
	
	}
	
	//when user_name logout, free the worker
	idle_worker[worker_ID] = true;
	SSL_free(ssl);         /* release SSL state */
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
    SOCKET sListen;//listening for an incoming connection
    //設定位址資訊的資料
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    //設定 Listen
    sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
		new_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		new_client = accept(sListen, (SOCKADDR*)&clinetAddr, &addrlen);
		
		int worker_ID = find_idle_worker();
    
    	if (worker_ID != -1)
		{
			idle_worker[worker_ID] = false;
			counter[worker_ID] = new_client;
			int *worker_pointer = &worker_ID;
			hthread[worker_ID] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)service, worker_pointer, 0, &threadId[worker_ID]);
			//close socket and thread
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
