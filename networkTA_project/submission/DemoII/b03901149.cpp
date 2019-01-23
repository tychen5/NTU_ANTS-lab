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
#include <map>
using namespace std;

void* connect_func(void* socket_desc);


class User
{
	public:
		User(){sockn = -1;port = -1; IP = ""; name = "";amount = 0;}
		int sockn;
		int port;
		string IP;
		string name;
		int amount;
};

map<string,User*> all_user;

int main(int argc,char *argv[])
{
	if(argc != 2)
	{printf("Error Format.\n");exit(-1);}

	//char inputBuffer[256] = {};
	//char in_message[1000] = {};
	//char message[] = {"Hi, this is server."};
	int socket_desc, client_sock, c;
	struct sockaddr_in server,client;

	// create socket
	socket_desc = socket(AF_INET,SOCK_STREAM,0);
	if(socket_desc == -1)
	{
		printf("Fail to create a socket.\n");
		exit(-1);
	}
	printf("Create Socket Successfully\n");

	// setting configuration	
	server.sin_family = PF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(atoi(argv[1]));

	// bind
	if(bind(socket_desc,(struct sockaddr*)&server,sizeof(server)) < 0)
	{
		printf("Bind Error.\n");
		exit(-1);
	}
	printf("Bind Successfully\n");

	// queueing list
	listen(socket_desc,5);
	printf("Waiting for incoming connections...\n");
	
	c = sizeof(struct sockaddr_in);
	pthread_t thread_id;

	

	while((client_sock = accept(socket_desc, (struct sockaddr*)&client,(socklen_t*)&c)) )
	{
		printf("Connection accepted\n");
		if(pthread_create(&thread_id,NULL,connect_func,(void*)&client_sock)<0)
		{
			printf("Error: Cannot create thread!\n");
			return 1;
		}
		//printf("Handler Assigned\n");
	}
	if(client_sock<0)
	{
		printf("Accept Failed");
		return 1;
	}

	return 0;
}

void* connect_func(void* socket_desc)
{
	int sock = *(int*)socket_desc;
	char client_message[2000] = {};
	recv(sock,client_message,sizeof(client_message),0);
	string k = "";
	for(int i=0;int(client_message[i])>31;i++)
		k += client_message[i];
	memset(client_message,0,sizeof(client_message));
	char message[] = "Hello I am a handler!";
	send(sock,message,sizeof(message),0);
	string username = "";
	int read_size;
	User* p = NULL;
	bool login_successful = false;
	while((read_size = recv(sock,client_message,sizeof(client_message),0)) > 0)
	{
		if(int(client_message[0])<32)
			continue;
		cout << "Receive Message: " << client_message << endl;
		if(strncmp(client_message,"REGISTER#",9) == 0)
		{
			User* u = new User;
			string amount_string = "";
			bool find_sep = false;
			for(int i=9;int(client_message[i])>31;i++)
			{
				if(client_message[i] == '#' && !find_sep)
				{
					find_sep = true;
					continue;
				}
				if(!find_sep)
					u->name += client_message[i];
				else
					amount_string += client_message[i];
				
			}
			u->amount = atoi(amount_string.c_str());
			u->sockn = sock;
			u->IP = "";
			u->port = -1;
			if(all_user.find(u->name) == all_user.end())
			{
				all_user[u->name] = u;
				char msg[] = "100 OK\n";
				send(sock,msg,sizeof(msg),0);
			}
			else
			{
				char msg[] = "210 FAIL\n";
				send(sock,msg,sizeof(msg),0);
			}
		}
		else
		{
			username = "";
			string port_string = "";
			bool find_sep = false;
			for(int i=0;int(client_message[i])>31;i++)
			{
				if(client_message[i] == '#' && !find_sep)
				{
					find_sep = true;
					continue;
				}
				if(!find_sep)
					username += client_message[i];
				else
					port_string += client_message[i];
			}
			cout << "username = " << username << endl;
			if(all_user.find(username) == all_user.end())
			{
				char msg[] = "220 AUTH_FAIL\n";
				send(sock,msg,sizeof(msg),0);
			}
			else
			{
				p = all_user.find(username)->second;
				int pp = atoi(port_string.c_str());
				map<string,User*>::iterator iter;
				bool port_use = false;
				for(iter = all_user.begin();iter != all_user.end();iter++)
				{
					if(iter->second->port == pp)
					{
						port_use = true;
						break;
					}
				}
				if(p->port > 0)
				{
					char errmsg[] = "This account is already login!!!\n";
					send(sock,errmsg,sizeof(errmsg),0);
					memset(client_message,0,sizeof(client_message));
					continue;
				}
				if(port_use)
				{
					char errmsg[] = "This port is already used!!!\n";
					send(sock,errmsg,sizeof(errmsg),0);
					memset(client_message,0,sizeof(client_message));
					continue;
				}
				p->port = pp;
				p->IP = k;
				//char msg[] = "Login Successful!\n";
				//send(sock,msg,sizeof(msg),0);
				
				string s = "";
				char s1[20] = {};
				sprintf(s1,"%d",p->amount);
				s += s1;
				s += '\n';
				int number_online = 0;
				for(iter = all_user.begin();iter != all_user.end();iter++)
				{
					if(iter->second->port > 0)
						number_online++;
				}
				sprintf(s1,"%d",number_online);
				s += "number of accounts online: ";
				s += s1;
				s += '\n';
				for(iter = all_user.begin();iter != all_user.end();iter++)
				{
					if(iter->second->port < 0)
						continue;
					sprintf(s1,"%d",iter->second->port);
					s += (iter->first + "#" + iter->second->IP + "#" + s1 + '\n');
				}
				char msg[10000] = {};
				for(int i=0;i<int(s.length());i++)
					msg[i] = s[i];
				send(sock,msg,sizeof(msg),0);
				login_successful = true;
				memset(client_message,0,sizeof(client_message));
				break;
			}
		}
		memset(client_message,0,sizeof(client_message));
	}
	memset(client_message,0,sizeof(client_message));
	while((read_size = recv(sock,client_message,sizeof(client_message),0)) > 0 && login_successful)
	{
		if(int(client_message[0])<32)
			continue;
		cout << "Receive Message: " << client_message << endl;
		if(strncmp(client_message,"Exit",4) == 0)
		{
			char msg[] = "Bye\n";
			p->IP = "";
			p->port = -1;
			send(sock,msg,sizeof(msg),0);
		}
		else if(strncmp(client_message,"List",4) == 0)
		{
			map<string,User*>::iterator iter;
			string s = "";
			char s1[20] = {};
			sprintf(s1,"%d",p->amount);
			s += s1;
			s += '\n';
			int number_online = 0;
			for(iter = all_user.begin();iter != all_user.end();iter++)
			{
				if(iter->second->port > 0)
					number_online++;
			}
			sprintf(s1,"%d",number_online);
			s += "number of accounts online: ";
			s += s1;
			s += '\n';
			for(iter = all_user.begin();iter != all_user.end();iter++)
			{
				if(iter->second->port < 0)
					continue;
				sprintf(s1,"%d",iter->second->port);
				s += (iter->first + "#" + iter->second->IP + "#" + s1 + '\n');
			}
			char msg[10000] = {};
			for(int i=0;i<int(s.length());i++)
				msg[i] = s[i];
			send(sock,msg,sizeof(msg),0);
		}
		else
		{
			int find_sep = 0;
			string account1 = "";
			string account2 = "";
			string payment = "";
			for(int i=0;int(client_message[i])>31;i++)
			{
				if(client_message[i] == '#')
				{
					find_sep++;
					continue;
				}
				else if(find_sep == 0)
					account1 += client_message[i];
				else if(find_sep == 1)
					payment += client_message[i];
				else if(find_sep == 2)
					account2 += client_message[i];
			}
			int pay_amount = atoi(payment.c_str());
			User* a1 = NULL;
			User* a2 = NULL;
			if(all_user.find(account1) == all_user.end() || all_user.find(account2) == all_user.end())
			{
				char msg[] = "Account not Exist\n";
				send(sock,msg,sizeof(msg),0);
			}
			else
			{
				a1 = all_user[account1];
				a2 = all_user[account2];
				if(a1->amount < pay_amount)
				{
					char msg[] = "The amount is not enough!!!\n";
					send(sock,msg,sizeof(msg),0);
				}
				else
				{
					a1->amount -= pay_amount;
					a2->amount += pay_amount;
					string s = "";
					s = (account1 + " paid " + payment + " to " + account2 + " successfully!\n");
					char msg[10000] = {};
					for(int i=0;i<int(s.length());i++)
						msg[i] = s[i];
					send(sock,msg,sizeof(msg),0);
				}
			}
		}
		memset(client_message,0,sizeof(client_message));
	}


	printf("Client Finish Connection\n");
	return 0;
}
