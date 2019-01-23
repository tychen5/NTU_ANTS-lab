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
using namespace std;

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
	send(sockfd,msgip,15,0);
	recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
	printf("%s\n",receiveMessage);
	memset(&receiveMessage,0,sizeof(receiveMessage));

	char send_msg[1000] = {};
	bool flag = true;
	while(flag)
	{
		char input[1000] = {};
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
			char msg[10000] = {};
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
			char msg[10000] = {};
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
					string ss = login_name + "#" + payment + "#" + name2 + '\n';
					char msg2[10000] = {};
					for(int i=0;i<int(ss.length());i++)
						msg2[i] = ss[i];
					send(sockfd,msg2,sizeof(msg2),0);
					printf("Send Message:\n");
					printf("%s\n",msg2);
					recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
					printf("Receive Message:\n");
					printf("%s\n",receiveMessage);
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
		//fgets(send_msg,sizeof(send_msg),stdin);
		//send(sockfd,send_msg,sizeof(send_msg),0);
		//printf("Send: %s\n",send_msg);
		//recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
		//printf("Receive Message:\n");
		//printf("%s\n",receiveMessage);

		memset(&send_msg,0,sizeof(send_msg));
		memset(&receiveMessage,0,sizeof(receiveMessage));
	}
	shutdown(sockfd,0);

	return 0;
}
