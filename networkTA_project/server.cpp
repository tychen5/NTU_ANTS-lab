#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<sstream>
#include<string>
#include<iostream>
#include<typeinfo>
using namespace std;
struct Account
{
	string acname;
	string cli_ad;
	string port;
	string money;
	bool online;
};
struct Account account[20];
int list=0,on=0;
struct sockaddr_in cli_addr;
void error(string msg)//use when error appears
{
	cout << msg << endl;
	exit(0);
}
void send(int sockfd, string msg) {
    if(write(sockfd,msg.c_str(), msg.length()) < 0)
        error("fail writing message\n");
}

string receive(int sockfd) {
    char buffer[1024];
    memset(buffer, 0, 1024);
    if(read(sockfd, buffer, 1023) < 0)
        error("fail reading message\n");
    string msg(buffer);
    return msg;
}
//handle every connection
void *connection_handler(void *socket_desc)
{
	//get the socket descriptor
	int sock = *(int*)socket_desc;
	int n;
	string entry,name,an;
	while(true)
	{
		entry=receive(sock);
		cout << entry;
		if(entry.substr(0,8)=="REGISTER") // client register
		{
			size_t pos=entry.find("#")+1;
			size_t end=entry.find("\n");
			string an=entry.substr(pos,end-pos);
			bool exist=false;
			for(int i=0;i<list;i++)
			{
				if(an==account[i].acname)
				{
					exist=true;
					break;
				}
			}
			if(exist==false)
			{
				account[list].acname=an;
				list++;
				send(sock,"100 OK\n");
			}
			else
			{
				send(sock,"210 FAIL\n");
			}
		}
		else  //client log in
		{
			size_t pos=entry.find("#");
			size_t end=entry.find("\n");
			string an=entry.substr(0,pos);
			string port=entry.substr(pos+1,end-pos-1);
			int po=0;
			bool exist=false;
			for(int i=0;i<list;i++)
			{
				if(an==account[i].acname)
				{
					exist=true;
					po=i;
					account[i].online=true;
					account[i].port=port;
					char *ip;
					ip=inet_ntoa(cli_addr.sin_addr);
					string realip(ip);
					account[i].cli_ad=realip;
					break;
				}
			}
			if(exist==false)
			{
				send(sock,"220 AUTH_FAIL\n");
			}
			else  // log in successfully
			{
				on++;
				string s;
				stringstream ss(s);
				ss << on;
				send(sock,account[po].money+"\n");
				send(sock,"number of accounts online: "+ss.str()+"\n");
				for(int i=0;i<list;i++)
				{
					if(account[i].online)
					{
						send(sock,account[i].acname+"#"+account[i].cli_ad+"#"+account[i].port+"\n");
					}
				}
				while(true)
				{
					entry=receive(sock);
					cout << entry;
					if(entry=="Exit\n")  //client exit
					{
						cout << "Enter response: Bye\n";
						on--;
						account[po].online=false;
						send(sock, "Bye");
						close(sock);
						return 0;
					}
					else if(entry=="List\n") //client demand online list
					{
						string news;
						stringstream newss(news);
						newss << on;
						send(sock,account[po].money+"\n");
						send(sock,"number of accounts online: "+newss.str()+"\n");
						for(int i=0;i<list;i++)
						{
							if(account[i].online)
							{
								send(sock,account[i].acname+"#"+account[i].cli_ad+"#"+account[i].port+"\n");
							}
						}
					}
					else
					{
						send(sock,"please type the right option number!\n");
					}
				}
			}
		}
	}	
}
int main(int argc, char *argv[])
{
	//check for port and correct input 
	string av0(argv[0]);
	if(argc<2)
	{
		error("please type "+av0+" serverPort\n");
	}
	int portnum=atoi(argv[1]);
	
	//set up socket
	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0)
	{
		error("fail to open a socket");
	}
	
	// setup server
	struct sockaddr_in serv_addr;
	memset((char*)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=INADDR_ANY;
	serv_addr.sin_port =htons(portnum);
	
	//bind to port
	if(bind(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr))<0)
	{
		error("fail binding");
	}
	
	//listen, with max 5 connections
	listen(sockfd,5);
	socklen_t clilen=sizeof(cli_addr);
	
	//infinite loop to accept cext request from clients
	int client_sock, *new_sock;
	for(int i=0;i<20;i++)
	{
		account[i].money="10000";
		account[i].online=false;
	}
	while(true)
	{
		cout <<"waiting for a connection\n";
		while(true)
		{
			pthread_t sniffer_thread;
			new_sock = (int*)malloc(sizeof(int));
			*new_sock = client_sock;
			if((*new_sock=accept(sockfd,(struct sockaddr *)&cli_addr,&clilen))!= -1)
			{
				cout << "Connection accepted"<<endl;
				send(*new_sock,"connection accepted\n");
				if(pthread_create(&sniffer_thread, NULL , connection_handler,(void*) new_sock)<0)
				{
				error("could not create thread!\n");
				return 1;
				}
			}
			else
			{
				error("fail to accept!\n");
			}
		}
		
		if(client_sock<0)
		{
			error("accept failed\n");
		}
	}
	
	close(sockfd);
	return 0;
}
