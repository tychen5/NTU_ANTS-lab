//Client
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream> 
#include<string>
#include<algorithm>
#include<openssl/bio.h>
#include<openssl/ssl.h>
#include<openssl/err.h>
#include<openssl/bio.h>
#include "openssl/x509.h"   
#include "openssl/rand.h" 
#include <openssl/rsa.h>
#include <openssl/pem.h>
#pragma warning(disable:4996) 

#pragma comment(lib, "Ws2_32.lib")
using namespace std;

#define SERVER     "127.0.0.1"  
#define CACERT     "ca.crt"  
#define MYCERTF    "client.crt"  
#define MYKEYF     "client.key"  
#define CACERT2     "ca2.crt"  
#define MYCERTF2    "client2.crt"  
#define MYKEYF2     "client2.key"
#define CACERT3     "ca3.crt"  
#define MYCERTF3    "client3.crt"  
#define MYKEYF3     "client3.key"
#define MSGLENGTH  1024 

int main() {
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	sockaddr_in sin;
	int seed_int[100]; /*存放隨機串行*/
	bool hasend = FALSE;

	SSL*ssl;
	SSL*ssl2;
	const SSL_METHOD *meth;
	SSL_CTX *ctx;

	//SSL初始化  
	OpenSSL_add_ssl_algorithms();
	//SSL錯誤信息初始化  
	SSL_load_error_strings();

	//創建本次會話所使用的協議  
	meth = TLSv1_client_method();
	//申請SSL會話的環境  
	ctx = SSL_CTX_new(meth);
	if (NULL == ctx)
		cout << "NULL == ctx" << endl;

	//設置會話的握手方式並加載CA證書  
	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
	SSL_CTX_load_verify_locations(ctx, CACERT, NULL);
	//加載自己的證書  
	if (0 == SSL_CTX_use_certificate_file(ctx, MYCERTF, SSL_FILETYPE_PEM)) {
		ERR_print_errors_fp(stderr);
	}
	//加載自己的私鑰  
	if (0 == SSL_CTX_use_PrivateKey_file(ctx, MYKEYF, SSL_FILETYPE_PEM)) {
		ERR_print_errors_fp(stderr);
	}
	//檢查自己的證書和私鑰是否匹配  
	if (!SSL_CTX_check_private_key(ctx)) {
		printf("Private key does not match the certificate public key\n");
	}
	/*構建隨機數生成機制,WIN32平台必需*/
	srand((unsigned)time(NULL));
	for (int i = 0; i < 100; i++)
		seed_int[i] = rand();
	RAND_seed(seed_int, sizeof(seed_int));

	//加密方式  
	SSL_CTX_set_cipher_list(ctx, "RC4-MD5");
	//處理握手多次  
	SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);

	//原

	SOCKADDR_IN addr;
	int addrlen = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 設定 IP	Server IP	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345); 

								  //Set Connection socket
	SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL);
	//bind(Connection, (SOCKADDR*)&addr, addrlen);
	if (connect(Connection, (SOCKADDR*)&addr, addrlen) != 0) //If we are unable to connect...
	{
		cout << "failed to connect to server!!" << endl;
		return 0; //Failed to Connect
	}
	else
	{
		//SSL begin
		ssl = SSL_new(ctx);
		if (NULL == ssl)
			cout << "NULL == ssl" << endl;
		if (0 >= SSL_set_fd(ssl, Connection)) {
			printf("Attach to Line fail!\n");
		}

		//SSL握手  
		//SSL_connect(ssl);
		int k = SSL_connect(ssl);
		if (0 == k) {
			printf("%d\n", k);
			printf("SSL connect fail!\n");
		}
		printf("連接服務器成功\n");
		//SSL end

		cout << "Connect to server IP: " << inet_ntoa(addr.sin_addr) << ", Port: " << (int)ntohs(addr.sin_port) << "……" << endl;
		//cout << "connect to server successfully!!" << endl;
		char text[1000] = "";
		char command[1000] = "";
		string UID;
		string PAY;
		string UID_2;
		SSL_write(ssl, "加密成功!!", strlen("加密成功!!"));
		SSL_read(ssl, text, sizeof(text));
		//recv(Connection, text, sizeof(text), NULL);
		cout << "receive encrypted message from server: " << text << endl;
		
		while (true)
		{
			cout << "r:register, l:log in, o:money and online client list, t: transaction, w: wait message, s: send message, e:exit" << endl;
			memset(text, 0, sizeof(text));
			memset(command, 0, sizeof(command));
			cin.getline(command, sizeof(command));
			string mean = string(command);
			if (mean == "r") //register
			{
				//先送註冊名
				cout << "Please enter the UserAccountName:" << endl;
				cin.getline(command, sizeof(command));
				mean = string(command);
				mean = "REGISTER#" + mean + "\n";
				cout << "send to server: " << mean << endl;
				memset(command, 0, sizeof(command));
				strcpy(command, mean.c_str());
				SSL_write(ssl, command, sizeof(command));
				//接受server回傳之註冊結果
				memset(text, 0, sizeof(text));
				SSL_read(ssl, text, sizeof(text));
				cout << "response from server: " << text << endl;
				//如果通過再輸入初始金額
				if (string(text) == "100 OK\n")
				{
					cout << "Resgister sucess!! Please enter the StartAmount:" << endl;
					cin.getline(command, sizeof(command));
					mean = string(command);
					mean = mean + "\n";
					cout << "send to server: " << mean << endl;
					memset(command, 0, sizeof(command));
					strcpy(command, mean.c_str());
					SSL_write(ssl, command, sizeof(command));
					memset(text, 0, sizeof(text));
					SSL_read(ssl, text, sizeof(text));
					cout << "response from server: " << text << endl;
				}
				continue; //略過底下的那次統一的接收
			}
			else if (mean == "l") //log in
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
				SSL_write(ssl, command, sizeof(command));
				memset(text, 0, sizeof(text));
				SSL_read(ssl, text, sizeof(text));
				cout << "response from server: " << text << endl;
			}
			else if (mean == "o")
			{
				mean = "List\n";
				//cout << "send to server: " << mean << endl;
				memset(command, 0, sizeof(command));
				strcpy(command, mean.c_str());
				cout << "send to server: " << command << endl;
				SSL_write(ssl, command, sizeof(command));
				memset(text, 0, sizeof(text));
				SSL_read(ssl, text, sizeof(text));
				cout << "response from server: " << text << endl;
			}
			else if (mean == "t")
			{
				if (hasend == FALSE)
				{
					cout << "You have not send message to client!!" << endl;
					mean = "Failed\n";
					memset(command, 0, sizeof(command));
					strcpy(command, mean.c_str());
					cout << "send to server: " << command << endl;
					SSL_write(ssl, command, sizeof(command));
				}
				else
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
					SSL_write(ssl, command, sizeof(command));
					hasend = FALSE;
				}
				
			}
			else if (mean == "w") //wait for message from another client
			{
				cout << "Please enter your IP and Port: " << endl;
				char IPADDRESS[10];
				int PORTNUM;
				cin >> IPADDRESS >> PORTNUM;
				cout << "waiting for connection on IP: " << IPADDRESS << ", PORT: " << PORTNUM << "......" << endl;
				//SSL
				WSADATA wsaData2;
				WSAStartup(MAKEWORD(2, 2), &wsaData2);
				SOCKET sock2;
				const SSL_METHOD *meth2;
				SSL_CTX* ctx2;
				//SSL初始化  
				OpenSSL_add_ssl_algorithms();
				//SSL錯誤信息初始化  
				SSL_load_error_strings();

				//創建本次會話所使用的協議  
				meth2 = TLSv1_server_method();
				//申請SSL會話的環境  
				ctx2 = SSL_CTX_new(meth2);
				if (NULL == ctx2)
					printf("NULL == ctx2\n");

				//設置會話的握手方式並加載CA證書  
				SSL_CTX_set_verify(ctx2, SSL_VERIFY_NONE, NULL);
				SSL_CTX_load_verify_locations(ctx2, CACERT, NULL);
				//加載服務器端的證書  
				if (SSL_CTX_use_certificate_file(ctx2, MYCERTF, SSL_FILETYPE_PEM) == 0) {
					ERR_print_errors_fp(stderr);
					printf("加載服務器端的證書失敗！！\n");
				}
				//加載服務器端的私鑰  
				if (SSL_CTX_use_PrivateKey_file(ctx2, MYKEYF, SSL_FILETYPE_PEM) == 0) {
					ERR_print_errors_fp(stderr);
					printf("加載服務器私鑰失敗！！\n");
				}
				//檢查服務器端的證書和私鑰是否匹配  
				if (!SSL_CTX_check_private_key(ctx2)) {
					printf("Private key does not match the certificate public key\n");
				}

				//加密方式  
				SSL_CTX_set_cipher_list(ctx2, "RC4-MD5");
				//處理握手多次  
				SSL_CTX_set_mode(ctx2, SSL_MODE_AUTO_RETRY);

				//原

				SOCKADDR_IN addr2;
				int addrlen2 = sizeof(addr2);
				addr2.sin_addr.s_addr = inet_addr(IPADDRESS); // 設定 IP	   // addr.sin_addr.s_addr = INADDR_ANY; // 若設定 INADDR_ANY 表示任何 IP
				addr2.sin_family = AF_INET;
				addr2.sin_port = htons(PORTNUM); // 設定 port


											  //create socket descriptor
				SOCKET sListen2 = socket(AF_INET, SOCK_STREAM, NULL);
				bind(sListen2, (SOCKADDR*)&addr2, addrlen2);
				listen(sListen2, SOMAXCONN);

				cout << "IP: " << inet_ntoa(addr2.sin_addr) << endl;
				cout << "Port: " << (int)ntohs(addr2.sin_port) << endl;

				int newConnection2;
				newConnection2 = accept(sListen2, (SOCKADDR*)&addr2, &addrlen2);
				//SSL begin
				ssl2 = SSL_new(ctx2);
				if (ssl2 == NULL)
					cout << "ssl is null!!" << endl;
				if (SSL_set_fd(ssl2, newConnection2) == 0)
					cout << "set fd failed!!" << endl;
				int ssl_accept2 = SSL_accept(ssl2);
				if (ssl_accept2 <= 0)
				{
					cout << "ssl failed to connect!! ssl_accept = " << ssl_accept2 << endl;
				}
				//SSL over
				if (newConnection2 == 0)
				{
					cout << "failed in creating new connection!!" << endl;
				}
				else
				{
					char message[1000] = "";
					char response[1000] = "";
					//memset(message, 0, sizeof(message));
					SSL_read(ssl2, message, sizeof(message));
					cout << "Message from Client: " << message << endl;
					cout << "Please type your Response: " << endl;
					string K;
					cin >> K;
					//K =	K + "\n";
					strcpy(response, K.c_str());
					SSL_write(ssl2, response, sizeof(response));
					cout << "Response to Client: " << response << endl;
					closesocket(sListen2);
				}
				continue;
			}
			else if (mean == "s") //send message to another client
			{
				//先跟Server要求支付對象的IP和Port
				cout << "Please enter the Username you want to pay:" << endl;
				getline(cin, UID_2);
				mean = "###" + UID_2;
				memset(command, 0, sizeof(command));
				strcpy(command, mean.c_str());
				cout << "send to server: " << command << endl;
				SSL_write(ssl, command, sizeof(command));
				memset(text, 0, sizeof(text));
				SSL_read(ssl, text, sizeof(text));
				cout << "response from server: " << text << endl;
				
				//然後跟對方建立連線
				cout << "Please enter the payee Client IP and Port: " << endl;
				char IPADDRESS[10];
				int PORTNUM;
				cin >> IPADDRESS >> PORTNUM;
				cout << "Connecting to Client IP: " << IPADDRESS << ", PORT: " << PORTNUM << "......" << endl;
				//開始連線
				WSADATA wsadata;
				WSAStartup(MAKEWORD(2, 2), &wsadata);
				sockaddr_in sin;
				int seed_int[100]; /*存放隨機串行*/

				SSL*ssl;
				const SSL_METHOD *meth;
				SSL_CTX *ctx;

				//SSL初始化  
				OpenSSL_add_ssl_algorithms();
				//SSL錯誤信息初始化  
				SSL_load_error_strings();

				//創建本次會話所使用的協議  
				meth = TLSv1_client_method();
				//申請SSL會話的環境  
				ctx = SSL_CTX_new(meth);
				if (NULL == ctx)
					cout << "NULL == ctx" << endl;

				//設置會話的握手方式並加載CA證書  
				SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
				SSL_CTX_load_verify_locations(ctx, CACERT, NULL);
				//加載自己的證書  
				if (0 == SSL_CTX_use_certificate_file(ctx, MYCERTF, SSL_FILETYPE_PEM)) {
					ERR_print_errors_fp(stderr);
				}
				//加載自己的私鑰  
				if (0 == SSL_CTX_use_PrivateKey_file(ctx, MYKEYF, SSL_FILETYPE_PEM)) {
					ERR_print_errors_fp(stderr);
				}
				//檢查自己的證書和私鑰是否匹配  
				if (!SSL_CTX_check_private_key(ctx)) {
					printf("Private key does not match the certificate public key\n");
				}
				/*構建隨機數生成機制,WIN32平台必需*/
				srand((unsigned)time(NULL));
				for (int i = 0; i < 100; i++)
					seed_int[i] = rand();
				RAND_seed(seed_int, sizeof(seed_int));

				//加密方式  
				SSL_CTX_set_cipher_list(ctx, "RC4-MD5");
				//處理握手多次  
				SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);

				//原

				SOCKADDR_IN addr;
				int addrlen = sizeof(addr);
				addr.sin_addr.s_addr = inet_addr(IPADDRESS); // 設定 IP	Server IP	
				addr.sin_family = AF_INET;
				addr.sin_port = htons(PORTNUM);

				//Set Connection socket
				SOCKET Connection3 = socket(AF_INET, SOCK_STREAM, NULL);
				//bind(Connection, (SOCKADDR*)&addr, addrlen);
				if (connect(Connection3, (SOCKADDR*)&addr, addrlen) != 0) //If we are unable to connect...
				{
					cout << "failed to connect to server!!" << endl;
					return 0; //Failed to Connect
				}
				else
				{
					//SSL begin
					ssl = SSL_new(ctx);
					if (NULL == ssl)
						cout << "NULL == ssl" << endl;
					if (0 >= SSL_set_fd(ssl, Connection3)) {
						printf("Attach to Line fail!\n");
					}

					//SSL握手  
					//SSL_connect(ssl);
					int k = SSL_connect(ssl);
					if (0 == k) {
						printf("%d\n", k);
						printf("SSL connect fail!\n");
					}
					printf("連接服務器成功\n");
					//SSL end
					cout << "Connected!!" << endl;
					char message[1000] = "";
					char res[1000] = "";
					cout << "Please type your message: " << endl;
					string K;
					cin >> K;
					//K = K + "\n";
					//memset(message, 0, sizeof(message));
					strcpy(message, K.c_str());
					SSL_write(ssl, message, sizeof(message));
					cout << "Send to Client: " << message << endl;
					SSL_read(ssl, res, sizeof(res));
					cout << "Response: " << res << endl;
					closesocket(Connection3);
				}
				hasend = TRUE;
				continue;
			}
			else if (mean == "e")
			{
				mean = "Exit\n";
				//cout << "send to server: " << mean << endl;
				memset(command, 0, sizeof(command));
				strcpy(command, mean.c_str());
				cout << "send to server: " << command << endl;
				SSL_write(ssl, command, sizeof(command));
				memset(text, 0, sizeof(text));
				SSL_read(ssl, text, sizeof(text));
				cout << "response from server: " << text << endl;
				break;
			}
			else
			{
				SSL_write(ssl, command, sizeof(command));
			}

			if (string(text) == "220 AUTH_FAIL\n" | string(text) == "please type the right option number!\n")
			{
				continue;
			}
			memset(text, 0, sizeof(text));
			SSL_read(ssl, text, sizeof(text));
			cout << "response from server: " << text << endl;
		}
		cout << "Diconnect to server IP: " << inet_ntoa(addr.sin_addr) << ", Port: " << (int)ntohs(addr.sin_port) << " sucessfully!!" << endl;
		closesocket(Connection);


	}
	system("pause");





	return 0;
}