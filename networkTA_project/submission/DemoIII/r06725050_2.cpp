#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/pem.h>


using namespace std;
void* L_function(void* user_info);

struct Data
{
	string IP;
	string port;
	int skfd;
	string M_Name;
};
string E_coding(const string& F_Name, string& s,int m);
string D_coding(const string& F_Name, string& s,int m);

int main(int argc,char *argv[])
{
	if(argc != 3)
	{printf("It is wronge.\n");exit(-1);}

	int sockfd = 0;
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1)
	{
		printf("It is do not create a socket.\n");
		exit(-1);
	}
	
	struct sockaddr_in C_Information;
	bzero(&C_Information,sizeof(C_Information));
	C_Information.sin_family = PF_INET;
	C_Information.sin_addr.s_addr = inet_addr(argv[1]);
	C_Information.sin_port = htons(atoi(argv[2]));

	char receiveMessage[100000] = {};
	if(connect(sockfd,(struct sockaddr*)&C_Information,sizeof(C_Information)) < 0) //连接服务器
	{
		printf("It is do not connection.\n");
		exit(-1);
	}
	char* msgip = inet_ntoa(C_Information.sin_addr);
	string IPaddr = string(msgip);
	string IPport = "";

	send(sockfd,msgip,15,0); 
    memset(&receiveMessage,0,sizeof(receiveMessage));
	recv(sockfd,receiveMessage,sizeof(receiveMessage),0); 
	
    
    printf("message from server:'%s'\n", receiveMessage);

	char send_msg[100000] = {};
	bool flag = true;
	while(flag)
	{
		char input[100000] = {};
		cout << "Weclome!" << endl;
		cout << "Enter 1 is Register" << endl;
		cout << "Enter 2 is Login" << endl;
		cout << "Enter 3 is Exit" << endl;
		cout << "> ";
		if(fgets(input,sizeof(input),stdin)[0] < 32)
			continue;
		if(input[1] > 31)
			continue;
		if(input[0] == '1')
		{
			cout << "customer Input Rule" << endl;
			cout << "<register name>(space)<money>" << endl;
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
                
            memset(&receiveMessage,0,sizeof(receiveMessage));
			recv(sockfd,receiveMessage,sizeof(receiveMessage),0); 
            
            printf("message from server:%s\n", receiveMessage);
		}
		else if(input[0] == '2')
		{
			cout << "customer Input Rule" << endl;
			cout << "<login name>(space)<port number>" << endl;
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
            memset(&receiveMessage,0,sizeof(receiveMessage));
			recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
            printf("message from server:\n%s\n", receiveMessage);            
			bool login = false;
			if(strncmp(receiveMessage,"220",3) != 0 && strncmp(receiveMessage,"This",4) != 0)
				login = true;
			string login_name = name;

			pthread_t pthreadid;
			if(login)
			{
				struct Data kk; // 登陆者信息
				kk.IP = IPaddr;
				kk.port = IPport;
				kk.skfd = sockfd;
				kk.M_Name = login_name;
				if(pthread_create(&pthreadid,NULL,L_function,(void*)&kk)<0) 
				{
					printf("It is cannot create thread!\n");
					return 1;
				}
			}

			while(login)
			{
				cout << "Weclome!" << endl;
				cout << "Enter 1 is List" << endl;
				cout << "Enter 2 is Payment" << endl;
				cout << "Enter 3 is Exit" << endl;
				cout << login_name << "> ";
				if(fgets(input,sizeof(input),stdin)[0] < 32) 
					continue;
				if(input[1] > 31)
					continue;
				if(input[0] == '1')
				{
					char C_message[] = "List\n"; 
					send(sockfd,C_message,sizeof(C_message),0); 
					printf("%s\n",C_message);
                
                    memset(&receiveMessage,0,sizeof(receiveMessage));
					recv(sockfd,receiveMessage,sizeof(receiveMessage),0); 
					printf("%s\n",receiveMessage);
				}
				else if(input[0] == '2')
				{
					cout << "customer Input Rule" << endl;
					cout << "<Given Payment Nmae>(space)<money>" << endl;
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
					string REQUEST = "REQUEST#" + name2 + '\n';
					char C_message[100000] = {};
					for(int i=0;i<int(REQUEST.length());i++)
						C_message[i] = REQUEST[i];
                    
                    // 请求服务器检查payee是否在线
					send(sockfd,C_message,sizeof(C_message),0);   
					
                    memset(&receiveMessage,0,sizeof(receiveMessage));
                    recv(sockfd,receiveMessage,sizeof(receiveMessage),0); 

					if(strncmp(receiveMessage,"customer",4) != 0) 
					{
                        // 解析出 payee 的IP和port号
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

                        // 创建新Socket, 连接payee的socket
						int C_socking = 0;
						C_socking = socket(AF_INET,SOCK_STREAM,0);
						if(C_socking == -1)
						{
							printf("It is fail to create a socket.\n");
							exit(-1);
						}
	
                        // 信息发给对方
						struct sockaddr_in C_Information3;
						bzero(&C_Information3,sizeof(C_Information3));
						C_Information3.sin_family = PF_INET;
						C_Information3.sin_addr.s_addr = inet_addr(getIP.c_str());
						C_Information3.sin_port = htons(atoi(getPort.c_str()));

					
						if(connect(C_socking,(struct sockaddr*)&C_Information3,sizeof(C_Information3)) < 0)
						{
							printf("Connection Error.\n");
						}

						else //连接socket成功
						{
                            memset(&C_message,0,sizeof(C_message));
							for(int i=0;i<int(login_name.length());i++)
								C_message[i] = login_name[i];               
                            printf("\nThe connection from'%s'\n", C_message);
                            
							send(C_socking,C_message,sizeof(C_message),0); 
							string ss = login_name + "#" + payment + "#" + name2 + '\n'; // 交易信息
                            
                            printf("\nThe transaction:%s\n", ss.c_str());
                            
							string priA = "../k/private_"+login_name+"_1024.pem"; 
							string pubB = "../k/public_"+name2+"_2048.pem"; 

							ss = E_coding(priA,ss,1);
							ss = E_coding(pubB,ss,0);
							memset(&C_message,0,sizeof(C_message));
							for(int i=0;i<int(ss.size());i++)
								C_message[i] = char(int(ss[i]));
                            // 发送签章后的交易信息到对方
							send(C_socking,C_message,sizeof(C_message),0); 
						}
	
					}
				}
				else if(input[0] == '3')
				{
					char C_message[] = "Exit\n";
					send(sockfd,C_message,sizeof(C_message),0);  
					
                    memset(&receiveMessage,0,sizeof(receiveMessage));
                    recv(sockfd,receiveMessage,sizeof(receiveMessage),0); 
					login = false; // 结束 while(login)
					flag = false;  // 结束 while(flag)
                    printf("%s\n",receiveMessage);
				}
			}// 结束 while(login)
		}
		else if(input[0] == '3')
        {
			flag = false;// 结束 while(flag)
            printf("Bye\n");
        }
	}// 结束 while(flag)
	shutdown(sockfd,0);

	return 0;
}

void* L_function(void* user_info) // 参数是struct Data kk;kk.IP = IPaddr;kk.port = IPport;kk.skfd = sockfd;kk.M_Name = login_name;
{
	struct Data a = *(Data*)user_info;
	string IP = a.IP;
	string port = a.port;
	int skfd = a.skfd;
	string M_Name = a.M_Name;
	int C_soket2, C_socks, c2;
	struct sockaddr_in server2,client2;

    // 创建新的socket, 等待payer连接
	C_soket2 = socket(AF_INET,SOCK_STREAM,0);
	if(C_soket2 == -1)
	{
		printf("It is do not to create a socket2.\n");
		exit(-1);
	}

	server2.sin_family = PF_INET;
	server2.sin_addr.s_addr = INADDR_ANY;
	server2.sin_port = htons(atoi(port.c_str()));

	if(bind(C_soket2,(struct sockaddr*)&server2,sizeof(server2)) < 0)
	{
		printf("Bind2 Error.\n");
		exit(-1);
	}

	listen(C_soket2,1);
	c2 = sizeof(struct sockaddr_in);

	while((C_socks = accept(C_soket2, (struct sockaddr*)&client2,(socklen_t*)&c2)) )
	{
		char receiveMessage2[100000] = {};        
		recv(C_socks,receiveMessage2,sizeof(receiveMessage2),0); 		
		string connect_namestr = "";
		for(int i=0;int(receiveMessage2[i])>31;i++)
			connect_namestr += receiveMessage2[i];
		memset(&receiveMessage2,0,sizeof(receiveMessage2));
		recv(C_socks,receiveMessage2,sizeof(receiveMessage2),0); 
		string ss = "";
		for(int i=0;i<256;i++)
			ss += receiveMessage2[i];
        
        string priA = "../k/private_"+M_Name+"_2048.pem";
		string pubB = "../k/public_"+connect_namestr+"_1024.pem";
		ss = D_coding(priA,ss,1);
		ss = D_coding(pubB,ss,0); //得到明文
		ss = E_coding(priA,ss,1);
		memset(&receiveMessage2,0,sizeof(receiveMessage2));
        receiveMessage2[0]='!';
		for(int i=0;i<int(ss.length());i++)
			receiveMessage2[i+1] = char(int(ss[i]));

		send(skfd,receiveMessage2,sizeof(receiveMessage2),0); //发给服务器
		
        memset(&receiveMessage2,0,sizeof(receiveMessage2));
		recv(skfd,receiveMessage2,sizeof(receiveMessage2),0); 
	}

	return 0;
}


string E_coding(const string& F_Name, string& s,int m)
{
	if(F_Name.empty() || s.empty())
		return "";
	FILE* F_key = fopen(F_Name.c_str(),"rb");
	if(F_key == NULL)
		return "";
	string r;
	RSA* R_key = RSA_new();
	if(m == 0)
	{
		if(PEM_read_RSA_PUBKEY(F_key, &R_key,0,0) == NULL)
			return "";
	}
	else
	{
		if(PEM_read_RSAPrivateKey(F_key, &R_key,0,0) == NULL)
			return "";
	}

	int nLen = RSA_size(R_key);
	char* pE_coding = new char[nLen+1];
	int ret;
	if(m == 0)
	{
	ret = RSA_public_encrypt(s.length(),(const unsigned char*)s.c_str(),\
			(unsigned char*)pE_coding,R_key, RSA_PKCS1_OAEP_PADDING);} // 
	else
	{
	ret = RSA_private_encrypt(s.length(),(const unsigned char*)s.c_str(),\
			(unsigned char*)pE_coding,R_key, RSA_PKCS1_PADDING);}
	if(ret >= 0)
		r = string(pE_coding,ret);

	delete[] pE_coding;
	RSA_free(R_key);
	fclose(F_key);
	CRYPTO_cleanup_all_ex_data();
	return r;
}
string D_coding(const string& F_Name, string& s,int m)
{
	if(F_Name.empty() || s.empty())
		return "";
	FILE* F_key = fopen(F_Name.c_str(),"rb");
	if(F_key == NULL)
		return "";
	string r;
	RSA* R_key = RSA_new();
	if(m == 0)
	{
		if(PEM_read_RSA_PUBKEY(F_key, &R_key,0,0) == NULL)
			return "";
	}
	else
	{
		if(PEM_read_RSAPrivateKey(F_key, &R_key,0,0) == NULL)
			return "";
	}

	int nLen = RSA_size(R_key);
	char* pD_coding = new char[nLen+1];
	int ret;
	if(m == 0) 
	{ret = RSA_public_decrypt(s.length(),(const unsigned char*)s.c_str(),\
					(unsigned char*)pD_coding,R_key, RSA_PKCS1_PADDING);}
	else
	{ret = RSA_private_decrypt(s.length(),(const unsigned char*)s.c_str(),\
					(unsigned char*)pD_coding,R_key, RSA_PKCS1_OAEP_PADDING);}
	if(ret >= 0)
		r = string((char*)pD_coding,ret);

	delete[] pD_coding;
	RSA_free(R_key);
	fclose(F_key);
	CRYPTO_cleanup_all_ex_data();
	return r;
}
