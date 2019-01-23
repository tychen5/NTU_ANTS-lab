#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/pem.h>


using namespace std;
void* listen_func(void* user_info);

struct Data
{
	string IP;
	string port;
	int skfd;
	string myname;
};
string Encode(const string& FileName, string& s,int m);
string Decode(const string& FileName, string& s,int m);

int main(int argc,char *argv[])
{
	if(argc != 3)
	{printf("Error Input Format.\n");exit(-1);}

	int sockfd = 0;
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1)
	{
		printf("Fail to create a socket.\n");
		exit(-1);
	}
	
	struct sockaddr_in clientInfo;
	bzero(&clientInfo,sizeof(clientInfo));
	clientInfo.sin_family = PF_INET;
	clientInfo.sin_addr.s_addr = inet_addr(argv[1]);
	clientInfo.sin_port = htons(atoi(argv[2]));

	char receiveMessage[100000] = {};
	if(connect(sockfd,(struct sockaddr*)&clientInfo,sizeof(clientInfo)) < 0)
	{
		printf("Connection Error.\n");
		exit(-1);
	}
	char* msgip = inet_ntoa(clientInfo.sin_addr);
	string IPaddr = string(msgip);
	string IPport = "";
	cout << IPaddr << endl;

	send(sockfd,msgip,15,0);
	recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
	printf("%s\n",receiveMessage);
	memset(&receiveMessage,0,sizeof(receiveMessage));

	char send_msg[100000] = {};
	bool flag = true;
	while(flag)
	{
		char input[100000] = {};
		cout << "*********************************************************************" << endl;
		cout << "* Menu: 1 for Register, 2 for Login, 3 for Exit Program.            *" << endl;
		cout << "*********************************************************************" << endl;
		cout << "> ";
		if(fgets(input,sizeof(input),stdin)[0] < 32)
			continue;
		if(input[1] > 31)
			continue;
		if(input[0] == '1')
		{
			memset(input,0,sizeof(input));
			cout << "*********************************************************************" << endl;
			cout << "* Please Type in Account Name and Amount                            *" << endl;
			cout << "* (Seperate with Space)                                             *" << endl;
			cout << "* E.g. Tim 10000                                                    *" << endl;
			cout << "*********************************************************************" << endl;
			cout << "> ";
			fgets(input,sizeof(input),stdin);
			bool space = false;
			string name = "";
			string amount = "";
			for(int i=0;int(input[i])>31;i++)
			{
				if(input[i] == ' ' && !space)
				{
					space = true;
					continue;
				}
				if(!space)
					name += input[i];
				else
					amount += input[i];
			}
			string s = "REGISTER#" + name + "#" + amount + '\n';
			char msg[100000] = {};
			for(int i=0;i<int(s.length());i++)
				msg[i] = s[i];
			send(sockfd,msg,sizeof(msg),0);
			printf("Send Message:\n");
			printf("%s\n",msg);
			recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
			printf("Receive Message:\n");
			printf("%s\n",receiveMessage);
			memset(&receiveMessage,0,sizeof(receiveMessage));
		}
		else if(input[0] == '2')
		{
			memset(input,0,sizeof(input));
			cout << "*********************************************************************" << endl;
			cout << "* Please Type in Account Name and Port Number                       *" << endl;
			cout << "* (Seperate with Space)                                             *" << endl;
			cout << "* E.g. Tim 8000                                                     *" << endl;
			cout << "*********************************************************************" << endl;
			cout << "> ";
			fgets(input,sizeof(input),stdin);
			bool space = false;
			string name = "";
			string port_num = "";
			for(int i=0;int(input[i])>31;i++)
			{
				if(input[i] == ' ' && !space)
				{
					space = true;
					continue;
				}
				if(!space)
					name += input[i];
				else
					port_num += input[i];
			}
			string s = name + "#" + port_num + '\n';
			IPport = port_num;
			
			char msg[100000] = {};
			for(int i=0;i<int(s.length());i++)
				msg[i] = s[i];
			send(sockfd,msg,sizeof(msg),0);
			printf("Send Message:\n");
			printf("%s\n",msg);
			recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
			printf("Receive Message:\n");
			printf("%s\n",receiveMessage);
			bool login = false;
			if(strncmp(receiveMessage,"220",3) != 0 && strncmp(receiveMessage,"This",4) != 0)
				login = true;
			string login_name = name;
			memset(&receiveMessage,0,sizeof(receiveMessage));

			pthread_t thread_id;
			if(login)
			{
				// create a new thread to accept the connection by other client.
				struct Data kk;
				kk.IP = IPaddr;
				kk.port = IPport;
				kk.skfd = sockfd;
				kk.myname = login_name;
				//string info_user = IPaddr+" "+IPport;
				//cout << "info_user: " << info_user << endl;
				if(pthread_create(&thread_id,NULL,listen_func,(void*)&kk)<0)
				{
					printf("Error: Cannot create thread!\n");
					return 1;
				}
			}

			while(login)
			{
				memset(input,0,sizeof(input));
				cout << "*********************************************************************" << endl;
				cout << "* Menu: 1 for List, 2 for Payment, 3 for Logout and Exit Program.   *" << endl;
				cout << "*********************************************************************" << endl;
				cout << login_name << "> ";
				if(fgets(input,sizeof(input),stdin)[0] < 32)
					continue;
				if(input[1] > 31)
					continue;
				if(input[0] == '1')
				{
					char msg2[] = "List\n";
					send(sockfd,msg2,sizeof(msg2),0);
					printf("Send Message:\n");
					printf("%s\n",msg2);
					recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
					printf("Receive Message:\n");
					printf("%s\n",receiveMessage);
					memset(&receiveMessage,0,sizeof(receiveMessage));
				}
				else if(input[0] == '2')
				{
					memset(input,0,sizeof(input));
					cout << "*********************************************************************" << endl;
					cout << "* Please Type in Payee Account Name and Amount                      *" << endl;
					cout << "* (Seperate with Space)                                             *" << endl;
					cout << "* E.g. Tim 500                                                      *" << endl;
					cout << "*********************************************************************" << endl;
					cout << "> ";
					fgets(input,sizeof(input),stdin);
					bool space = false;
					string name2 = "";
					string payment = "";
					for(int i=0;int(input[i])>31;i++)
					{
						if(input[i] == ' ' && !space)
						{
							space = true;
							continue;
						}
						if(!space)
							name2 += input[i];
						else
							payment += input[i];
					}
					// 1. request IP and Port from server
					string REQUEST = "REQUEST#" + name2 + '\n';
					char msg2[100000] = {};
					for(int i=0;i<int(REQUEST.length());i++)
						msg2[i] = REQUEST[i];
					send(sockfd,msg2,sizeof(msg2),0);
					printf("Send Message:\n");
					printf("%s\n",msg2);
					recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
					printf("Receive Message:\n");
					printf("%s\n",receiveMessage);

					if(strncmp(receiveMessage,"User",4) != 0)
					{
						// 2. establish connection to another client
						string getIP = "";
						string getPort = "";
						bool sep = false;
						for(int i=0;int(receiveMessage[i])>31;i++)
						{
							if(receiveMessage[i] == ' ')
							{
								sep = true;
								continue;
							}
							else if(!sep)
								getIP += receiveMessage[i];
							else
								getPort += receiveMessage[i];
						}
						
						memset(&receiveMessage,0,sizeof(receiveMessage));
						memset(&msg2,0,sizeof(msg2));
						int sockfd3 = 0;
						sockfd3 = socket(AF_INET,SOCK_STREAM,0);
						if(sockfd3 == -1)
						{
							printf("Fail to create a socket.\n");
							exit(-1);
						}
	
						struct sockaddr_in clientInfo3;
						bzero(&clientInfo3,sizeof(clientInfo3));
						clientInfo3.sin_family = PF_INET;
						clientInfo3.sin_addr.s_addr = inet_addr(getIP.c_str());
						clientInfo3.sin_port = htons(atoi(getPort.c_str()));

					
						if(connect(sockfd3,(struct sockaddr*)&clientInfo3,sizeof(clientInfo3)) < 0)
						{
							printf("Connection Error.\n");
						}

						// 3. send message to another client
						else
						{
							// told the another client who I am
							for(int i=0;i<int(login_name.length());i++)
								msg2[i] = login_name[i];
							send(sockfd3,msg2,sizeof(msg2),0);
							memset(&msg2,0,sizeof(msg2));
							string ss = login_name + "#" + payment + "#" + name2 + '\n';
							string priA = "../keys/private_"+login_name+"_1024.pem";
							string pubB = "../keys/public_"+name2+"_2048.pem";

							// Encode
							printf("=== Before Encrypted ===\n");
							cout << ss << endl ;
							ss = Encode(priA,ss,1);
							ss = Encode(pubB,ss,0);
							printf("=== After Encrypted ===\n");
							cout << ss << endl ;
							memset(&msg2,0,sizeof(msg2));
							for(int i=0;i<int(ss.size());i++)
							{
								msg2[i] = char(int(ss[i]));
							}
							send(sockfd3,msg2,sizeof(msg2),0);
							printf("=== Send Message To another client ===\n");
							printf("%s\n",msg2);

							memset(&msg2,0,sizeof(msg2));
							recv(sockfd3,msg2,sizeof(msg2),0);
							memset(&msg2,0,sizeof(msg2));
							//shutdown(sockfd3,0);
						}
						// 4. Done
	
					}
					memset(&receiveMessage,0,sizeof(receiveMessage));
				}
				else if(input[0] == '3')
				{
					char msg2[] = "Exit\n";
					send(sockfd,msg2,sizeof(msg2),0);
					printf("Send Message:\n");
					printf("%s\n",msg2);
					recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
					printf("Receive Message:\n");
					printf("%s\n",receiveMessage);
					memset(&receiveMessage,0,sizeof(receiveMessage));
					login = false;
					flag = false;
				}
			}
		}
		else if(input[0] == '3')
			flag = false;

		memset(&send_msg,0,sizeof(send_msg));
		memset(&receiveMessage,0,sizeof(receiveMessage));
	}
	shutdown(sockfd,0);

	return 0;
}

void* listen_func(void* user_info)
{
	//cout << "This is listening function" << endl;
	struct Data a = *(Data*)user_info;
	string IP = a.IP;
	string port = a.port;
	int skfd = a.skfd;
	string myname = a.myname;
	// create new socket wait for connect
	int socket_desc2, client_sock2, c2;
	struct sockaddr_in server2,client2;

	// create socket
	socket_desc2 = socket(AF_INET,SOCK_STREAM,0);
	if(socket_desc2 == -1)
	{
		printf("Fail to create a socket2.\n");
		exit(-1);
	}
	//printf("Create Socket2 Successfully\n");

	// setting configuration	
	server2.sin_family = PF_INET;
	server2.sin_addr.s_addr = INADDR_ANY;
	server2.sin_port = htons(atoi(port.c_str()));

	// bind
	if(bind(socket_desc2,(struct sockaddr*)&server2,sizeof(server2)) < 0)
	{
		printf("Bind2 Error.\n");
		exit(-1);
	}
	//printf("Bind2 Successfully\n");

	// queueing list
	listen(socket_desc2,1);
	//printf("Waiting for incoming client connections...\n");
	
	c2 = sizeof(struct sockaddr_in);


	while((client_sock2 = accept(socket_desc2, (struct sockaddr*)&client2,(socklen_t*)&c2)) )
	{
		//printf("Connection between clients accepted\n");
		char receiveMessage2[100000] = {};
		recv(client_sock2,receiveMessage2,sizeof(receiveMessage2),0);
		send(client_sock2,receiveMessage2,sizeof(receiveMessage2),0);
		
		string connect_namestr = "";
		for(int i=0;int(receiveMessage2[i])>31;i++)
			connect_namestr += receiveMessage2[i];

		string priA = "../keys/private_"+myname+"_2048.pem";
		string pubB = "../keys/public_"+connect_namestr+"_1024.pem";
		memset(&receiveMessage2,0,sizeof(receiveMessage2));
		recv(client_sock2,receiveMessage2,sizeof(receiveMessage2),0);
		printf("\n=== Receive Message From other client ===\n\n");
		printf("%s\n",receiveMessage2);
		string ss = "";
		
		// Decode
		for(int i=0;i<256;i++)
			ss += receiveMessage2[i];
		ss = Decode(priA,ss,1);
		ss = Decode(pubB,ss,0);
		cout << "=== After Decrypted ===\n" << ss << endl << endl;

		// Encode
		ss = Encode(priA,ss,1);
		memset(&receiveMessage2,0,sizeof(receiveMessage2));
		for(int i=0;i<int(ss.length());i++)
			receiveMessage2[i] = char(int(ss[i]));

		// send to server
		send(skfd,receiveMessage2,sizeof(receiveMessage2),0);
		printf("=== Send Message To Server===\n");
		printf("%s\n",receiveMessage2);
		memset(&receiveMessage2,0,sizeof(receiveMessage2));
		recv(skfd,receiveMessage2,sizeof(receiveMessage2),0);
		printf("=== Receive Message From Server ===\n");
		printf("%s\n",receiveMessage2);
		memset(&receiveMessage2,0,sizeof(receiveMessage2));
	}

	return 0;
}


string Encode(const string& FileName, string& s,int m)
{
	// read RSA file and text
	if(FileName.empty() || s.empty())
		return "";
	FILE* hKeyFile = fopen(FileName.c_str(),"rb");
	if(hKeyFile == NULL)
		return "";
	string r;
	RSA* pRSAKey = RSA_new();
	if(m == 0)
	{
		if(PEM_read_RSA_PUBKEY(hKeyFile, &pRSAKey,0,0) == NULL)
			return "";
	}
	else
	{
		if(PEM_read_RSAPrivateKey(hKeyFile, &pRSAKey,0,0) == NULL)
			return "";
	}

	// Encode
	int nLen = RSA_size(pRSAKey);
	char* pEncode = new char[nLen+1];
	int ret;
	if(m == 0)
	{
	ret = RSA_public_encrypt(s.length(),(const unsigned char*)s.c_str(),\
			(unsigned char*)pEncode,pRSAKey, RSA_PKCS1_OAEP_PADDING);} // PKCS1
	else
	{
	ret = RSA_private_encrypt(s.length(),(const unsigned char*)s.c_str(),\
			(unsigned char*)pEncode,pRSAKey, RSA_PKCS1_PADDING);}
	if(ret >= 0)
		r = string(pEncode,ret);

	// Free Space
	delete[] pEncode;
	RSA_free(pRSAKey);
	fclose(hKeyFile);
	CRYPTO_cleanup_all_ex_data();
	return r;
}
string Decode(const string& FileName, string& s,int m)
{
	// Read RSA File and text
	if(FileName.empty() || s.empty())
		return "";
	FILE* hKeyFile = fopen(FileName.c_str(),"rb");
	if(hKeyFile == NULL)
		return "";
	string r;
	RSA* pRSAKey = RSA_new();
	if(m == 0)
	{
		if(PEM_read_RSA_PUBKEY(hKeyFile, &pRSAKey,0,0) == NULL)
			return "";
	}
	else
	{
		if(PEM_read_RSAPrivateKey(hKeyFile, &pRSAKey,0,0) == NULL)
			return "";
	}

	// Encode
	int nLen = RSA_size(pRSAKey);
	char* pDecode = new char[nLen+1];
	int ret;
	if(m == 0) 
	{ret = RSA_public_decrypt(s.length(),(const unsigned char*)s.c_str(),\
					(unsigned char*)pDecode,pRSAKey, RSA_PKCS1_PADDING);}
	else
	{ret = RSA_private_decrypt(s.length(),(const unsigned char*)s.c_str(),\
					(unsigned char*)pDecode,pRSAKey, RSA_PKCS1_OAEP_PADDING);}
	if(ret >= 0)
		r = string((char*)pDecode,ret);

	// Free Space
	delete[] pDecode;
	RSA_free(pRSAKey);
	fclose(hKeyFile);
	CRYPTO_cleanup_all_ex_data();
	return r;
}
