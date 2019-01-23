#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <iomanip>
using namespace std;

int main(int argc , char *argv[])
{

    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1)
    {
        printf("Fail to create a socket.");
    } 

    //socket的連線

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;

    //localhost testB
    inet_pton(AF_INET,argv[1],&info.sin_addr.s_addr);
    info.sin_port = htons(atoi(argv[2]));


    /*info.sin_port = htons(33120);
    inet_pton(AF_INET,"140.112.107.194",&info.sin_addr.s_addr);*/
    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    bool begin = true;
    if(err==-1)
    {
        printf("Connection error");
    	begin = false;
    }
 
    char connected[1000] = {};
	 recv(sockfd,connected,sizeof(connected),0);
    	printf("%s",connected);
  	
  	string input;
  	string order;
  	

  	while(begin)	
  	{
  		cout <<"Enter 1 for Register, 2 for Login: ";
  		getline(cin,order);
  		if(order == "1")
  		{
  			cout << "Enter the name you want to register: ";
  			getline(cin,input);
  			string to_send = "REGISTER#";
  			to_send = to_send + input;
  			char buffer[1000]="";
  			char receive[1000]="";
  			for(int i = 0;i < to_send.length();i++)
  			{
  				buffer[i] = to_send[i];
  			}
  			send(sockfd,buffer,to_send.length(),0);
  			//cout << buffer << endl;
  			recv(sockfd,receive,sizeof(receive),0);
    		printf("%s",receive);
  			input = "\0";
  			order = "\0";
  		}
  		else if(order == "2")
  		{
  			bool log_suc;
  			string port;
  			cout <<"Enter your name: "; 
  			getline(cin,input);
  			string to_send = input + "#";
  			cout <<"Enter your port number: ";
  			getline(cin,port);
  			to_send = to_send + port;
  			char buffer[1000]="";
  			char receive[1000]="";
  			for(int i = 0;i < to_send.length();i++)
  			{
  				buffer[i] = to_send[i];
  			}
  			send(sockfd,buffer,to_send.length(),0);
			  			

  			recv(sockfd,receive,sizeof(receive),0);
  			string return_ms = receive;
  			log_suc = (return_ms != "220 AUTH_FAIL\n");
			  cout << receive;
  			if(log_suc)
  			{	 
  				
  			}
  			else
  			{
  				cout << "The id haven't registered! Please try again" << endl;
  				continue;
  			}
  				
  			while(log_suc)
  			{	
  				
  				cout << "Enter 1 to view the latest list and 8 to Exit: ";
  				getline(cin,order);
  				if(order == "1")
  				{
  					//cout << "Ok, just a minute...." << endl;
            char receive[1000]=""; 
  					send(sockfd,"List",4,0);
  					recv(sockfd,receive,sizeof(receive),0);
  					recv(sockfd,receive,sizeof(receive),0);
            cout << receive;
  					order = "\0";
  				}
  				else if (order == "8")
  				{
  					char buffer[1000];
  					send(sockfd,"Exit",4,0);
  					char receive[1000];
  					recv(sockfd,receive,sizeof(receive),0);
  					printf("%s",receive);
  					printf("close Socket\n");
    				close(sockfd);
  					order = "\0";
  					begin = false;
  					break;
  				}

  			}
  		}
  		else	
  		{
  			cout << "We don't have that kind of order! Pleae try again!\n";
  			order = "\0";
  			continue;
  		}
  	}
    return 0;
}




















