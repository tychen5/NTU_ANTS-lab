#ifdef WIN32
    #include <windows.h>
#else
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <string>
	#include <cstring>
	#include <iostream>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <thread>
#endif

using namespace std;

string message;
char* receiveMessage=new char[100];
string user[10]={"null"};
int status[10][3]={0};
string ipaddr;

int connect();


int main(int argc , char *argv[])
{   //int accept(int sockfd, struct sockaddr addr, socklen_t addrlen);
	#ifdef WIN32
        WSADATA ws;
        WSAStartup(MAKEWORD(2,2), &ws);
    #endif

   	char regMessage[10]={"REGISTER#"};
    char outMessage[10]={"Exit"};
    char listMessage[10]={"List"};

    int forClientSockfd=connect();

    while(1)
    {   char ACKMessage[100]={};
    	receiveMessage=new char[100];
    	recv(forClientSockfd,receiveMessage, 24,0);
    	int option=-1;

    	bool reg=true, out=true, list=true, login=true;
    	for(int i=0; i<4; i++)
    	{	if(receiveMessage[i]!=regMessage[i])
    			reg=false;
    		if(receiveMessage[i]!=outMessage[i])
    			out=false;
    		if(receiveMessage[i]!=listMessage[i])
    			list=false;
    	}
    	if(reg==true && out==true && list==true)
    		login=false;

		if(reg==true)
		{	cout<<"客戶註冊"<<endl;
			string username;
			for(int i=9; i<24;i++)
				username+=receiveMessage[i];
			bool used=false;
			for(int i=0; i<10; i++)
			{	if(username==user[i])
				{	//message="210 FAIL\n";
					//send(forClientSockfd,message.c_str(),sizeof(message),0);
					used=true;
					break;
				}
			}
			if(used==false)
			{	int usernum=0;
				for(int i=0; i<10; i++)
				{	if(status[i][1]==0)
					{	usernum=i;
						break;
					}
				}
				user[usernum]=username;
				status[usernum][0]=1;
				status[usernum][1]=100;
				cout<<"註冊名單"<<endl;
				for(int i=0; i<usernum+1; i++)
					cout<<i+1<<": "<<user[i]<<endl;
				message="100 OK\n";
				send(forClientSockfd,message.c_str(),sizeof(message),0);
				//recv(forClientSockfd,receiveMessage, 24,0);
				//status[usernum][1]=(int) receiveMessage;
			}
			else
			{	message="210 FAIL\n";
				send(forClientSockfd,message.c_str(),sizeof(message),0);
			}
		}
		else if(list==true)
		{	cout<<"客戶要求清單\n"<<endl;
			int onlinenum=0;
			for(int i=0; i<10; i++)
			{	if(status[i][2]!=0)
					onlinenum++;
			}
			for(int i=0; i<10; i++)
			{	if(status[i][2]!=0)
				{	string money=to_string(status[i][1]);
					string stport=to_string(status[i][2]);
					string online=to_string(onlinenum);
					string sp="#";
					message=money+"\n"+online+"\n";
					send(forClientSockfd,message.c_str(),sizeof(message),0);
					message=user[i]+sp+ipaddr+sp+stport;
					send(forClientSockfd,message.c_str(),sizeof(message),0);
				}
			}
		}
		else if(out==true)
		{	cout<<"客戶登出"<<endl;
			message="Bye";
			send(forClientSockfd,message.c_str(),sizeof(message),0);
			close(forClientSockfd);
			forClientSockfd=connect();
		}
		else if(login==true)
		{	string username;
			string userport;
			int port=0;
			int usernum=0;
			bool exist=false;
			int position=0;
			for(int i=0; i<24; i++)
			{	if(receiveMessage[i]!='#')
					username+=receiveMessage[i];
				else
				{	position=i;
					break;
				}
			}
			for(int i=position+1; i<24; i++)
				userport+=receiveMessage[i];
			port=atoi(userport.c_str());
			for(int i=0; i<10; i++)
			{	bool temp=true;
				for(int j=0; j<username.size(); j++)
				{	if(user[i][j]!=username[j])
					{	temp=false;
						break;
					}
				}
				if(temp==true)
				{	exist=true;
					usernum=i;
					break;
				}
			}
			if(exist==false)
			{	message="220 AUTH_FAIL\n";
				send(forClientSockfd,message.c_str(),sizeof(message),0);
			}
			else
			{	cout<<"客戶 "<<username<<" 登入"<<endl;
				status[usernum][2]=port;
				int onlinenum=0;
				for(int i=0; i<10; i++)
				{	if(status[i][2]!=0)
					onlinenum++;
				}
				string money=to_string(status[usernum][1]);
				string stport=to_string(status[usernum][2]);
				string online=to_string(onlinenum);
				string sp="#";
				message=money+"\n"+online+"\n";
				send(forClientSockfd,message.c_str(),sizeof(message),0);
				message=username+sp+ipaddr+sp+stport;
				send(forClientSockfd,message.c_str(),sizeof(message),0);
			}
		}
		delete [] receiveMessage;
	}	
}

int connect()
{	int sockfd=0,forClientSockfd=0;
    sockfd=socket(AF_INET,SOCK_STREAM,0);

    if (sockfd==-1)
   		cout<<"Fail to create a socket."<<endl;

    //socket的連線
    struct sockaddr_in serverInfo,clientInfo;
    socklen_t addrlen = (int)sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(8703);
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sockfd,5);

    forClientSockfd=accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);
	ipaddr=inet_ntoa(clientInfo.sin_addr);

	message="connected!!!!!!!!!!";
    send(forClientSockfd,message.c_str(),sizeof(message),0);
    cout<<"新連線進入"<<endl;
    return forClientSockfd;
}




