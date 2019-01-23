#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

using namespace std;

#define BLEN 9
char buf[BLEN];
int n;
string temp;

//checking overflow
void check(char* buf, int n, int sd)
{
	temp = buf;
	while(n == BLEN)
	{
		memset((char *)buf, 0, BLEN);
		n=recv(sd, buf, BLEN, 0);
		//overflow = 0;
		temp = temp + buf;
		if(n < BLEN)
			cout<<temp<<endl;
	}
}

int main(int argc, char *argv[]){
	//wrong input form
	string fstSeg = argv[0];
	if(argc < 3)
	{
		cout<<"ERROR: Input format: " + fstSeg + "hostname port\n";
		exit(0);
	}
	
	//address
	struct sockaddr_in suckit;
	memset((char *) &suckit, 0, sizeof(suckit));
	suckit.sin_addr.s_addr = inet_addr(argv[1]);
	suckit.sin_family = AF_INET;
	suckit.sin_port = htons(atoi(argv[2]));

	//creating socket
	int sd;
	sd = socket(PF_INET, SOCK_STREAM, 0);
	if(sd < 0)
	{
		cout<<"ERROR creating socket\n";
		exit(0);
	}
	
	//connecting
	if (connect(sd, (struct sockaddr*)&suckit, sizeof(suckit)) < 0)
	{
		cout<<"ERROR connecting\n";
		exit(0);
	}

	//sucessful connection
	memset((char *)buf, 0, BLEN);
	n=recv(sd, buf, BLEN, 0);
	if(n < BLEN)
		cout<<buf<<endl;
	else
		check(buf, n, sd);

	//connected!
	while(true)
	{
		//two options to choose
		string opt; 
		cout<<"Enter 1 for Register, 2 for Login: ";
		cin>>opt;

		//Register
		if(opt=="1")
		{
			//REGISTER#name
			memset((char *)buf, 0, BLEN);
			cout<<"Enter the name you want to register: ";
			string input; cin>>input;
			input = "REGISTER#" + input;
			send(sd, input.c_str(), input.size(), 0);
			n=recv(sd, buf, BLEN, 0);
			if(n < BLEN)
				cout<<buf<<endl;
			else
				check(buf, n, sd);
		}

		//Login
		else if(opt=="2")
		{
			//LOGIN
			memset((char *)buf, 0, BLEN);
			cout<<"Enter your name: ";
			string inputName; cin>>inputName;
			cout<<"Enter the port number: ";
			string inputNum; cin>>inputNum;
			inputName = inputName + "#" + inputNum;
			send(sd, inputName.c_str(), inputName.size(), 0);
			n=recv(sd, buf, BLEN, 0);
			if(n < BLEN)
			{
				temp = buf;
				cout<<buf<<endl;
			}
			else
				check(buf, n, sd);			

			string action;
			string msg;
			
			while(strcmp(temp.c_str(), "220 AUTH_FAIL\n")!=0)
			{
				cout<<"Enter the number of actions you want to take.\n";
				cout<<"1 to ask for the latest list, 8 to Exit: ";
				cin>>action;
				if(action=="1")
				{
					msg="List";
					send(sd, msg.c_str(), msg.size(), 0);
					memset((char *)buf, 0, BLEN);
					n= recv(sd, buf, BLEN, 0);
					if(n < BLEN)
						cout<<buf<<endl;
					else
						check(buf, n, sd);
				}
				else if(action=="8")
				{
					msg="Exit";
					send(sd, msg.c_str(), msg.size(), 0);
					memset((char *)buf, 0, BLEN);
					n=recv(sd, buf, BLEN, 0);
					if(n < BLEN)
						cout<<buf<<endl;
					else
						check(buf, n, sd);
					close(sd);
					exit(0);
					//Bye	
				}
			}
		}
	}
}
