#include <iostream>
#include <stdio.h>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/pem.h>
using namespace std;
void* C_functionc(void* C_soket);
string E_coding(const string& F_Name, string& s,int m);
string D_coding(const string& F_Name, string& s,int m);
string int2str(int Private_A )
{
	if(Private_A == 0)
	{return "";}
	else
	{
		int Public_B = Private_A %10;
		int Private_C = Private_A /10;
		char Public_D = Public_B+48;  
		// 装成ascii码, '1'=49
		string s = int2str(Private_C)+Public_D;
		return s;
	}
}

class customer
{
	public:
		customer(){sockn = -1;port = -1; IP = ""; name = "";amount = 0;}
		int sockn;
		int port;
		string IP;
		string name;
		int amount;
};

map<string,customer*> C_all;  // 记录全部的用户

int main(int argc,char *argv[])
{
	if(argc != 2)
	{printf("Error Format.\n");exit(-1);}

	int C_soket, client_sock, c;
	struct sockaddr_in server,client;

    
    //创建socket
	C_soket = socket(AF_INET,SOCK_STREAM,0);
	if(C_soket == -1)
	{
		printf("It is do not to create a socket.\n");
		exit(-1);
	}
	printf("Hello!The Socket Successfully\n");

	server.sin_family = PF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(atoi(argv[1]));

   
	if(bind(C_soket,(struct sockaddr*)&server,sizeof(server)) < 0)
	{
		printf("It is bind error.\n");
		exit(-1);
	}
	listen(C_soket,5);
	
	c = sizeof(struct sockaddr_in);
	pthread_t pthreadid;

    
	while((client_sock = accept(C_soket, (struct sockaddr*)&client,(socklen_t*)&c)) )
	{
		if(pthread_create(&pthreadid,NULL,C_functionc,(void*)&client_sock)<0) 
		{
			printf("It is do not create thread!\n");
			return 1;
		}
        printf("Success to create thread \n");
	}
	if(client_sock<0)
	{
		printf("It accept failed");
		return 1;
	}

	return 0;
}

void* C_functionc(void* C_soket)
{
	int sock = *(int*)C_soket;
    
    // send 和 recv 都是用char数组, 但是在server上都是用string保存和处理
	char C_message[100000] = {};
	recv(sock,C_message,sizeof(C_message),0);
    

	string k = "";
	for(int i=0;int(C_message[i])>31;i++)
		k += C_message[i];
    

	char message[] = "Beginning!";
	send(sock,message,sizeof(message),0);
	string U_Name = "";
	int read_size;
	customer* p = NULL;
	bool login_successful = false;
	
    // 不断接收信息, recv返回读取的字节数(正常), 不然返回-1(异常)或0(对方socket已关闭)
    while((read_size = recv(sock,C_message,sizeof(C_message),0)) > 0)
	{
        printf("Server: receive message '%s'\n", C_message);
		if(int(C_message[0])<32) // 可显示字符的ascii码>=32(32是空格), 再次监听
		{
            printf("Server: not going to process this message.\n");
            memset(&C_message,0,sizeof(C_message));
            continue;
        }
        
        // 比较前9位是否 "REGISTER#"
		if(strncmp(C_message,"REGISTER#",9) == 0)
		{
			customer* u = new customer; // 新建用户, 加入到C_all(map类)
            
            // 解释传来的字符串, 分离出用户名和金额
			string amount_string = "";
			bool find_sep = false;
			for(int i=9;int(C_message[i])>31;i++) // 前9位是"REGISTER#", 所以从9开始
			{
				if(C_message[i] == '#' && !find_sep)
				{
					find_sep = true;
					continue;
				}
				if(!find_sep)
					u->name += C_message[i];
				else
					amount_string += C_message[i];
				
			}
			u->amount = atoi(amount_string.c_str());
			u->sockn = sock;
			u->IP = "";
			u->port = -1; // 未登陆时port号为-1
            
			if(C_all.find(u->name) == C_all.end()) // 就是说在C_all中找不到u->name
			{
                // 增加新用户
				C_all[u->name] = u;
				
				string Private_A = "openssl genrsa -out ../k/private_"+u->name+"_1024.pem 1024";
				string Public_B = "openssl rsa -in ../k/private_"+u->name+"_1024.pem -pubout -out ../k/public_"+u->name+"_1024.pem";
				string Private_C = "openssl genrsa -out ../k/private_"+u->name+"_2048.pem 2048";
				string Public_D = "openssl rsa -in ../k/private_"+u->name+"_2048.pem -pubout -out ../k/public_"+u->name+"_2048.pem";
				system(Private_A.c_str());
				system(Public_B .c_str());
				system(Private_C.c_str());
				system(Public_D.c_str());
                
                printf("Server: New user '%s' created!\n", u->name.c_str());


				char msg[] = "100 OK\n";
				send(sock,msg,sizeof(msg),0);
                memset(&C_message,0,sizeof(C_message));
			}
			else
			{
                printf("Server: customer '%s' exists!\n", u->name.c_str());
				char msg[] = "210 FAIL\n";
				send(sock,msg,sizeof(msg),0);
                memset(&C_message,0,sizeof(C_message));
			}
		}
		else // 比较前9位 "REGISTER#"
		{
            
			U_Name = "";
			string port_string = "";
			bool find_sep = false;
			for(int i=0;int(C_message[i])>31;i++)
			{
				if(C_message[i] == '#' && !find_sep)
				{
					find_sep = true;
					continue;
				}
				if(!find_sep)
					U_Name += C_message[i];
				else
					port_string += C_message[i]; // 解析出用户名和port
			}
            
			if(C_all.find(U_Name) == C_all.end()) // 找不到用户
			{
                printf("Server: customer '%s' not exist\n", U_Name.c_str());
				char msg[] = "220 AUTH_FAIL\n";
				send(sock,msg,sizeof(msg),0);
                memset(&C_message,0,sizeof(C_message));
			}
			else // 找到用户
			{
				p = C_all.find(U_Name)->second; // p是customer*, map.find()返回iterator类, ->second取得map的键值即customer类
				int pp = atoi(port_string.c_str());  // 得到port号
				
                
                if(p->port > 0) // 检查是否已经登陆用户
				{
                    printf("Server: customer '%s' already login.\n", U_Name.c_str());
					char errmsg[] = "This account is already login!!!\n";
					send(sock,errmsg,sizeof(errmsg),0);
                    memset(&C_message,0,sizeof(C_message));
					
					continue;
				}
                
                // 检查port号是否已经被占用
                map<string,customer*>::iterator iter;
				bool port_use = false;
				for(iter = C_all.begin();iter != C_all.end();iter++)
				{
					if(iter->second->port == pp) 
					{
						port_use = true;
						break;
					}
				}
				if(port_use)
				{
                    printf("Server: port '%d' already login.\n", pp);
					char errmsg[] = "This port is already used!!!\n";
					send(sock,errmsg,sizeof(errmsg),0);
					memset(C_message,0,sizeof(C_message));
					continue;
				}
                
                // 能正常登陆
				p->port = pp;
				p->IP = k;     // k在C_functionc()的开头就从recv中得到
                
                
                // 解析出p的金额并转成string到s
				string s = "";
				char s1[20] = {};
				sprintf(s1,"%d",p->amount); //sprintf是写格式化字符串到s1
				s += s1;
				s += '\n'; 
                
                // 统计在线用户数量并转成string加到s
				int C_online = 0;
				for(iter = C_all.begin();iter != C_all.end();iter++)
				{
					if(iter->second->port > 0) // 未登陆时port号为-1
						C_online++;
				}
				sprintf(s1,"%d",C_online);
				s += "At this time:Accounts online: ";
				s += s1;
				s += '\n';
                
                // 生成在线用户列表并转成string加到s
				for(iter = C_all.begin();iter != C_all.end();iter++)
				{
					if(iter->second->port < 0)
						continue;
					sprintf(s1,"%d",iter->second->port);
					s += (iter->first + "#" + iter->second->IP + "#" + s1 + '\n');
				}
                
                // 发送信息到client
				char msg[10000] = {};
				for(int i=0;i<int(s.length());i++)
					msg[i] = s[i];
				send(sock,msg,sizeof(msg),0);
                
				login_successful = true; // 进入下一个层次的标记
                printf("Server: customer '%s' login successfully!\n", U_Name.c_str());
				memset(C_message,0,sizeof(C_message));
                
				
                break; // 退出第一层的while
			}
		}
		
	}
	
    
    
    // 已经LOGIN成功才会进入这个while, 第二次输入信息
	while((read_size = recv(sock,C_message,sizeof(C_message),0)) > 0 && login_successful)
	{
        printf("Server: receive message '%s'\n", C_message);
        if(int(C_message[0])<32) // 可显示字符的ascii码>=32(32是空格), 再次监听
		{
            printf("Server: not going to process this message.\n");
            memset(&C_message,0,sizeof(C_message));
            continue;
        }
        // 比较前4个字符是否Exit, p仍然指向登陆进来的用户
		if(strncmp(C_message,"Exit",4) == 0)
		{
            printf("Server: customer '%s' exits.\n", p->name.c_str());
			char msg[] = "Bye\n";
			p->IP = "";
			p->port = -1;
			send(sock,msg,sizeof(msg),0);
            memset(&C_message,0,sizeof(C_message));
		}
        // 比较前4个字符是否List
		else if(strncmp(C_message,"List",4) == 0)
		{
            // 解析出p的金额并转成string到s
			map<string,customer*>::iterator iter;
			string s = "";
			char s1[20] = {};
			sprintf(s1,"%d",p->amount); //p仍然指向登陆进来的用户
			s += s1;
			s += '\n';
            // 统计在线用户数量并转成string加到s
			int C_online = 0;
			for(iter = C_all.begin();iter != C_all.end();iter++)
			{
				if(iter->second->port > 0)
					C_online++;
			}
			sprintf(s1,"%d",C_online);
			s += "Accounts online: ";
			s += s1;
			s += '\n';
            printf("Server: '%d' accounts online.\n", C_online);
            // 生成在线用户列表并转成string加到s
			for(iter = C_all.begin();iter != C_all.end();iter++)
			{
				if(iter->second->port < 0)
					continue;
				sprintf(s1,"%d",iter->second->port);
				s += (iter->first + "#" + iter->second->IP + "#" + s1 + '\n');
			}
			char msg[10000] = {};
			for(int i=0;i<int(s.length());i++)
				msg[i] = s[i];
			send(sock,msg,sizeof(msg),0); // 返回s
            memset(&C_message,0,sizeof(C_message));
		}
        // 比较前7个字符是否REQUEST
		else if(strncmp(C_message,"REQUEST",7) == 0)
		{
            // 解析出payee用户名
			string u = "";
			for(int i=8;int(C_message[i])>31;i++) // 前8位是"REGISTER#", 所以从8开始
				u+=C_message[i];
                
            // 查找payee
			map<string,customer*>::iterator iter;
			iter = C_all.find(u);
			string s = "";
			if(iter == C_all.end())
				s = "customer not found\n";
			else if(iter->second->port < 0)
				s = "customer not online\n";
			else
			{
                // payee在线
				s += iter->second->IP;
				s += " ";
				s += int2str(iter->second->port);
				s += "\n";
			}
			char msg[10000] = {};
			for(int i=0;i<int(s.length());i++)
				msg[i] = s[i];
			send(sock,msg,sizeof(msg),0); //传回 "payee名字 payeeIP payeePort号"
			memset(&C_message,0,sizeof(C_message));
		}
        // REQUEST的第二次请求
		else
		{
			string ss = "";
			for(int i=0;i<256;i++)
				ss += C_message[i+1];
			string pubB = "../k/public_"+U_Name+"_2048.pem";
			ss = D_coding(pubB,ss,0);
			
            
            printf("transaction message: '%s'\n", ss.c_str());
            
			for(int i=0;i<ss.length();i++)
				C_message[i] = ss[i] ;

			int find_sep = 0;
			string account1 = "";
			string account2 = "";
			string payment = "";
			for(int i=0;int(C_message[i])>31;i++)
			{
				if(C_message[i] == '#')
				{
					find_sep++;
					continue;
				}
				else if(find_sep == 0)
					account1 += C_message[i];
				else if(find_sep == 1)
					payment += C_message[i];
				else if(find_sep == 2)
					account2 += C_message[i];
			}
			int p_number = atoi(payment.c_str());
			customer* a1 = NULL;
			customer* a2 = NULL;
			if(C_all.find(account1) == C_all.end() || C_all.find(account2) == C_all.end())
			{
				char msg[] = "Account not Exist\n";
				send(sock,msg,sizeof(msg),0);
                memset(&C_message,0,sizeof(C_message));
			}
			else
			{
				a1 = C_all[account1];
				if(a1->amount < p_number)
				{
					char msg[] = "The amount is not enough!!!\n";
					send(sock,msg,sizeof(msg),0);
                    memset(&C_message,0,sizeof(C_message));
				}
				else if(C_all.find(account2) == C_all.end())
				{
					char msg[] = "This account does not exist!!!\n";
					send(sock,msg,sizeof(msg),0);
                    memset(&C_message,0,sizeof(C_message));
				}
				else if(C_all[account2]->port < 0)
				{
					char msg[] = "This account is not online!!!\n";
					send(sock,msg,sizeof(msg),0);
                    memset(&C_message,0,sizeof(C_message));
				}
				else
				{
                    
					a2 = C_all[account2];
					a1->amount -= p_number;
					a2->amount += p_number;
					string s = "";
					s = (account1 + " paid " + payment + " to " + account2 + " successfully!\n");
                    printf("Server: Payment executed! %s\n", s.c_str());
					char msg[10000] = {};
					for(int i=0;i<int(s.length());i++)
						msg[i] = s[i];
					send(sock,msg,sizeof(msg),0);
                    memset(&C_message,0,sizeof(C_message));
				}
			}
		}

	}

	if(p != NULL)
	{
		p->IP = "";
		p->port = -1;
	}
	return 0;
}


string E_coding(const string& F_Name, string& s,int m)
{
	if(F_Name.empty() || s.empty())
		return "";
	FILE* F_key = fopen(F_Name.c_str(),"rb");
	if(F_key == NULL)
	{	cout << "Fail 1" << endl;return "";}
	string r;
	RSA* R_key = RSA_new();
	if(m == 0)
	{
		if(PEM_read_RSA_PUBKEY(F_key, &R_key,0,0) == NULL)
		{	cout << "Fail 2" << endl;return "";}
	}
	else
	{
		if(PEM_read_RSAPrivateKey(F_key, &R_key,0,0) == NULL)
		{	cout << "Fail 2" << endl;return "";}
	}
	int nLen = RSA_size(R_key);
	char* pE_coding = new char[nLen+1];
	int ret;
	if(m == 0)
	{ret = RSA_public_encrypt(s.length(),(const unsigned char*)s.c_str(),\
			(unsigned char*)pE_coding,R_key, RSA_PKCS1_PADDING);}
	else
	{ret = RSA_private_encrypt(s.length(),(const unsigned char*)s.c_str(),\
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
					(unsigned char*)pD_coding,R_key, RSA_PKCS1_PADDING);}

	if(ret >= 0)
		r = string((char*)pD_coding,ret);
	delete[] pD_coding;
	RSA_free(R_key);
	fclose(F_key);
	CRYPTO_cleanup_all_ex_data();
	return r;
}
