#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <string>
#include <queue>
#include <sys/select.h>
#include <sys/time.h>
#include <vector>
#include <iostream>
using namespace std;

const int MaxThread = 8;
pthread_mutex_t cdm;
//pthread_mutex_t udm;
queue<int> wait_line;
struct sockaddr_in clientinfo[MaxThread];
const int buf_len = 1024;
const int ini_cash = 12980;

// struct for account

int Account_num  = 0;
int online_num = 0;

struct Account
{
	int client_fd;
	int cash;

	string id;	
	string ip;
	string port;
	bool ifonline;

};
string getOnlineList(Account* acc)
{
	string list="";
	for(int i = 0; i < Account_num;i++)
	{
		if(acc[i].ifonline)
		{
			
			list += acc[i].id;
			list += "#";
			list += acc[i].ip;
			list += "#";
			list += acc[i].port;
			list += "\n";
		}	
	}
	return list;
}
bool ifused(Account* acc, string id)
{
	bool boolean = false;
	for(int i = 0; i < Account_num;i++)
	{
		if(acc[i].id == id)
			boolean = true;
	}
	return boolean;
}
bool ifPortused(Account* acc, string port)
{
	bool boolean = false;
	for(int i = 0; i < Account_num;i++)
	{
		if(acc[i].port == port)
			boolean = true;
	}
	return boolean;
}

int findidIndex(Account* acc, string id)
{
	int index = -1;
	for(int i = 0; i<Account_num; i++)
	{
		if(acc[i].id == id)
		{	
			index = i;
			break;
		}
	}
	return index;
}
string current_id[MaxThread];
Account account[buf_len];

// for pthread_create(), we need to write a mainly processing function
void* processing(void* data)
{
	int client_fd = 0;
	while(true)
	{
		pthread_mutex_lock(&cdm);
		if(wait_line.size()>0)
		{
			client_fd = wait_line.front();
			wait_line.pop();
		}
		pthread_mutex_unlock(&cdm);
		while(client_fd > 0)
		{
			char buffer[buf_len]="";
			recv(client_fd,buffer,sizeof(buffer),0);
			string order(buffer);
			

			//REGISTER#
			if(order.compare(0,9,"REGISTER#") == 0)
			{	
				char reg_suc[10] = "100 OK\n";				
				char reg_fail[10] = "210 Fail\n";

				string to_submit = order.substr(9);
				pthread_mutex_lock(&cdm);
				if(!ifused(account,to_submit))
				{
					account[Account_num].id = to_submit;
					account[Account_num].cash = ini_cash;
					Account_num++;
					send(client_fd,reg_suc,strlen(reg_suc),0);
					cout << "A client has registered." << endl;
				}
				else
				{
					send(client_fd,reg_fail,strlen(reg_fail),0);

				}
				pthread_mutex_unlock(&cdm);

			}
			//Login
			else if(order.find("#") != string::npos && order.compare(0,9,"REGISTER#") != 0)
			{
				int len = order.find("#");
				string to_log = order.substr(0,len);
				string to_log_port = order.substr(len+1);
				pthread_mutex_lock(&cdm);
				if(ifused(account,to_log) && !ifPortused(account,to_log_port))
				{
					current_id[client_fd] = to_log;
					int index = findidIndex(account,to_log);
					account[index].port = to_log_port;
					account[index].ifonline = true;
					account[index].ip = inet_ntoa(clientinfo[client_fd].sin_addr);
					online_num ++;
					string list = "Your account balance: ";
					list += to_string(account[index].cash);
					list += "\n" ;
					list += "Number of online people: ";
					list += to_string(online_num);
					list += "\n";
					list += getOnlineList(account);	
					char send_list[buf_len]="";
					strcpy(send_list,list.c_str());
					send(client_fd,send_list,sizeof(send_list),0);
					
					//the things only when logging client can do
					bool keep_log =true;
					cout << "A client has logged in." << endl;
				}	
				else
				{
					char log_fail[20]="220 AUTH_FAIL\n";
					send(client_fd,log_fail,sizeof(log_fail),0);
				}	
				pthread_mutex_unlock(&cdm);
			}
			else if(order.compare(0,4,"List") == 0)
			{
				cout << "Just got a requset for the online list." << endl;
				int index = findidIndex(account,current_id[client_fd]);
				//cout << "我進來了～～" << endl;
				pthread_mutex_lock(&cdm);
				
				string list2 = "Your account balance: ";
				list2 += to_string(account[index].cash);
				list2 += "\n" ;
				list2 += "Number of online people: ";
				list2 += to_string(online_num);
				list2 += "\n" ;
		        list2 += getOnlineList(account);	
				char send_list[buf_len]="";
				strcpy(send_list,list2.c_str());
					
				send(client_fd,send_list,sizeof(send_list),0);
				
				pthread_mutex_unlock(&cdm);
				//cout << send_list << endl;
				//cout << "successfully send: " << send_list;
				cout << "The list has been sent successfully." << endl;
			}	
						//exit
			else if(order.compare(0,4,"Exit") == 0)
			{
				int index = findidIndex(account,current_id[client_fd]);
				pthread_mutex_lock(&cdm);
				send(client_fd, "Bye\n", strlen("Bye\n"), 0);
        		account[index].ifonline = false;
        		account[index].port = "\0";
        		account[index].ip = "\0";
        		online_num--;
        		close(client_fd);
				cout << "A client has logged out." << endl;
				pthread_mutex_unlock(&cdm);
				client_fd = -1;
			}
			else
				continue;
			
		}
		client_fd = -1;
	}
}


int main(int argc , char *argv[])

{
    //socket的建立
    int sockfd = 0, clientfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1)
    {
       cout << "Fail to create a socket." << endl;
    }

    //socket的連線
    struct sockaddr_in serverInfo;
    bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverInfo.sin_port = htons(atoi(argv[1]));
    if((bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo))) == -1)
    {	
    	cout << "Fail to bind." << endl;
    	exit(0);
    }
    listen(sockfd, MaxThread);
    pthread_t pthreads[MaxThread];
  	pthread_mutex_init(&cdm, NULL);
 	//pthread_mutex_init(&udm, NULL);
    
 	for (int i = 0; i < MaxThread; i++) 
 	{
    pthread_create(&pthreads[i],NULL,&processing,NULL);
    }
    while((clientfd = accept(sockfd,0, 0)))
    {
        char connected[] = "You have successfully connected!\n";
        cout << "Has accepted a client's connect request" << endl;
        send(clientfd,connected,sizeof(connected),0);
    	pthread_mutex_lock(&cdm);
    	wait_line.push(clientfd);
    	pthread_mutex_unlock(&cdm);
    }
    return 0;
}





