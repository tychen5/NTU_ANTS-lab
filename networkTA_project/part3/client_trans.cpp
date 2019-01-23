#include <stdio.h>
#include <cstring>
#include <string>
#include <windows.h>
#include <winsock2.h>
#include <sstream>
#include <iostream>
#include <cstdlib>
#pragma comment (lib, "ws2_32.lib")

#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#undef APPMACROS_ONLY 
#include <openssl/applink.c>

using namespace std;

void error(string printout) {
    cout << printout << endl;
    exit(0);
}
void output(string printout) {
    cout << printout;
}

u_short cliport;
string myname;
string clientname;
string payamount;
bool trans = false;
//record workers status
SOCKET counter[3];
bool idle_worker[3];
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
SSL_CTX* InitCTX(void)
{   const SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = SSLv23_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
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
// 與client之間互連 
LPVOID service(LPVOID ID)
{
	string request;
	string response;
	int worker_ID = *((int *)ID);
	SOCKET new_client = counter[worker_ID];
	
	SSL_CTX *ctx;
	SSL *ssl;
	SSL_library_init();
    ctx = InitServerCTX();
    char cert[100]= "mycert.pem";
    char key[100]= "mykey.pem";
	LoadCertificates(ctx, cert, key);
   	ssl = SSL_new(ctx);              // get new SSL state with context 
    SSL_set_fd(ssl, new_client);      // set connection socket to SSL state
    if (SSL_accept(ssl) == -1)  
    {  
		cout << "error in accepting to SSL." << endl;
    	perror("accept");  
    }
    else
    {
	    char recvbuf[10000] = {0};
		//wait for request
		SSL_read(ssl, recvbuf, sizeof(recvbuf));
		request.assign(recvbuf);
		int last = request.find_last_of('#');
		int first = request.find('#');
		
		if (request.substr(last + 1, (request.length() - last)) == myname)
		{
			clientname = request.substr(0,first);
			payamount = request.substr(first + 1, last);
			trans = true;
		}	
	}
    
	idle_worker[worker_ID] = true;	
	closesocket(new_client); 
	SSL_free(ssl);         // release SSL state   
}
// client 接收其他clients的連線要求 
LPVOID start_listen()
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
    //設定位址資訊的資料
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cliport);
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

int main()
{
	SSL_library_init();
    SSL_load_error_strings();
    SSL_CTX *ctx;
    SSL *ssl;
    ctx = InitCTX();
    ssl = SSL_new(ctx);
	// 初始化windows socket DLL 
	WSAData wsaData;
	WORD version = MAKEWORD(2, 2); // 版本
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData); // 成功回傳 0
	if (iResult != 0) 
	{
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

//	string str;//輸入IP 
//	cout << "Please enter the IP address: ";
//	cin >> str;
//	const char *cp = str.c_str();//儲存IP 
	
//	u_short hostshort = 10000;//輸入port number 
//	cout << "Please enter the portNumber: ";
//	cin >> hostshort; 
	
//	addr.sin_port = htons(hostshort); // 設定 port
//	addr.sin_addr.s_addr = inet_addr(cp); // 設定 IP

	addr.sin_port = htons(1234); // 設定 port
	addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 設定 IP	
	// 連線到socket server
	int r = connect(sock, (struct sockaddr *)&addr, sizeof (addr));
	if (r == SOCKET_ERROR)
		error("ERROR in connecting");
	
	SSL_set_fd(ssl, sock);
	if (SSL_connect(ssl) == -1)  
    	ERR_print_errors_fp(stderr);
    else
    {
    	//recieve server的訊息 
		char recv1[1024];
		if(SSL_read(ssl, recv1, sizeof(recv1)) < 0)
			cout << "Error of receive message." << endl;
		cout << recv1 << endl;
		
		//開始request server 
		cout << "Enter 1 for Register, 2 for Login: ";	
		int input;
		bool login = 0;//是否成功登入 
		bool end = 0;//是否結束 
	
		while(!end)
		{
			//client 對 client 的交易結果傳回server
			if(trans)
			{
				string response;
				response += ("REPORT#" + payamount + "#" + clientname);
				const char *sendbuf = response.c_str();
				SSL_write(ssl, sendbuf, (int)strlen(sendbuf));
				trans = false;
			}
	
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
					SSL_write(ssl, sendbuf, (int)strlen(sendbuf));
					SSL_read(ssl, recvbuf, sizeof(recvbuf)); 
					cout << "-----------------------" << endl;
					cout << recvbuf << endl;
					cout << "Enter 1 for Register, 2 for Login: ";
				}
				else/*已經成功登入*/ 
				{
					char sendbuf[] = "List";
					SSL_write(ssl, sendbuf, (int)strlen(sendbuf));
					SSL_read(ssl, recvbuf, sizeof(recvbuf)); 
					cout << "-----------------------" << endl;
					cout << recvbuf << endl;
					cout << "1 to ask for the latest list, 8 to Exit, 9 for transaction: ";
				}
			}
			else if(input == 2)
			{
				cout << "Enter the name u want to Login: ";
				cin >> name;
				string temp(name);
				char port[10];
				cout << "Enter the port number: ";
				cin >> port;
				// 將name#portnum傳給server
				myname = temp;
			   	stringstream ss;
		       	ss.str(port);
		       	ss >> cliport;
				char sendbuf[]= "#";
				strcat(sendbuf, port);
				strcat(name, sendbuf);
				strcpy(sendbuf, name);
				SSL_write(ssl, sendbuf, (int)strlen(sendbuf));
				SSL_read(ssl, recvbuf, sizeof(recvbuf)); 
		        cout << "-----------------------" << endl;
				cout << recvbuf << endl;
				
				if(recvbuf[0] != '2')/*登入成功*/ 
		        {
		        	login = 1;
		           	cout << "Log in successfully" << endl;
		           
				
		           	//thread for listen to clients
		           	{
		           		DWORD listen_trans;
		           		HANDLE thread_trans;
		           		thread_trans = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_listen, NULL, 0, &listen_trans);
						//close socket and thread
						if (end)
							CloseHandle(thread_trans);
		           	}
				
					
					cout << "Enter the number of actions u want to take." << endl;
					cout << "1 to ask for the latest list, 8 to Exit, 9 for transaction: ";
				}
				else
					cout << "Enter 1 for Register, 2 for Login: ";
			}
			else if(input == 8)
			{
				char sendbuf[] = "Exit";	
				SSL_write(ssl, sendbuf, (int)strlen(sendbuf));
				SSL_read(ssl, recvbuf, sizeof(recvbuf)); 
				cout << recvbuf << endl;
				end = 1;
			}
			
			else if(input == 9)
			{
				cout << "Please enter the client name: ";
				string name;
				cin >> name;
				string temp;
				temp = ("TRANSACTION#" + name);
				
				const char *sendbuf = temp.c_str();
				SSL_write(ssl, sendbuf, (int)strlen(sendbuf));
				SSL_read(ssl, recvbuf, sizeof(recvbuf)); 
				if(recvbuf[1] == 2)
				{
					cout << recvbuf << endl;
					cout << "Enter the number of actions u want to take." << endl;
					cout << "1 to ask for the latest list, 8 to Exit, 9 for transaction: ";
				}
				else
				{
					string receive(recvbuf);
					int first = receive.find('#');
					string str = receive.substr(0, first);
					string port = receive.substr(first + 1, (receive.length() - first));
				
					SSL_library_init();
					SSL_CTX *ctx1;
    				ctx1 = InitCTX(); 
					SSL *ssltoClient;
					ssltoClient = SSL_new(ctx1);
					
					WSAData wsaData;
					WORD version = MAKEWORD(2, 2); // 版本
					int iResult = WSAStartup(MAKEWORD(2,2), &wsaData); // 成功回傳 0
					if (iResult != 0) {
					    // 初始化Winsock 失敗
					}
					// 建立socket
	
					SOCKET socktocli = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					if (sock < 0) 
						error("ERROR in setting socket");
	
					// 設定IP 跟 port
					SOCKADDR_IN addr;
					memset (&addr, 0, sizeof (addr)) ; // 清空,將資料設為 0
					addr.sin_family = AF_INET;
					const char *cp = str.c_str();//儲存IP 
					stringstream ss;
			        ss.str(port);
			        u_short cliport;
			        ss >> cliport; 
					addr.sin_port = htons(cliport); // 設定 port
					addr.sin_addr.s_addr = inet_addr(cp); // 設定 IP
					
					// 連線到socket server
					int r = connect(socktocli, (struct sockaddr *)&addr, sizeof (addr));
					if (r == SOCKET_ERROR)
						error("ERROR in connecting");
						
					
					SSL_set_fd(ssltoClient, socktocli);
					if (SSL_connect(ssltoClient) == -1)  
				    	ERR_print_errors_fp(stderr);
				    else
					{
						string amount;
						cout << "Please enter payamount: ";
						cin >> amount;
						string cliname;
						cliname = name;
						string response;
						response += (myname + "#" + amount + "#" + cliname);
						const char *sendbuf = response.c_str();
						SSL_write(ssltoClient, sendbuf, (int)strlen(sendbuf));
						closesocket(socktocli);
						SSL_free(ssltoClient);
						cout << "Enter the number of actions u want to take." << endl;
						cout << "1 to ask for the latest list, 8 to Exit, 9 for transaction: ";
					}	
					
				}
			}
			else
				cout << "ERROR, please input again\n";
	
		}
	}
	
	closesocket(sock); // 結束socket 
	SSL_CTX_free(ctx); // release context
}                                     
