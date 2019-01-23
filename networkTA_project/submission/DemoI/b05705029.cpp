#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

using namespace std;

#define BufferLength 1000    // length of buffer
char buffer[BufferLength];
int n;
string temp;
int errno;


// checking overflow
void checkOverflow(char* buffer, int n, int mysocket)
{
	temp = buffer;
	while(n == BufferLength)
	{
		memset((char *)buffer, 0, BufferLength);
		n=recv(mysocket, buffer, BufferLength, 0);
		
		//overflow = 0;
		temp = temp + buffer;
		if(n < BufferLength)
			cout << temp << endl;
	}
}


void msgOption(char* buffer, int n, int mysocket)
{
	memset((char *)buffer, 0, BufferLength);
	// recv() is just a function for copy, if the received message is larger than the length 
	// of buffer, it need use several times of recv() to copy from received messgae to buffer
	// therefore there is a overflow checking
	// this format will also be used after
	n = recv(mysocket, buffer, BufferLength, 0);
	if(n < BufferLength)    // not overflow
		cout << buffer;
	else    // overflow
		checkOverflow(buffer, n, mysocket);
}


void registerOption(char* buffer, int n, int mysocket)
{
	memset((char *)buffer, 0, BufferLength);
	cout << " Enter the name you want to register: ";
	string registerName; 
	cin >> registerName;
	registerName = "REGISTER#" + registerName;
	send(mysocket, registerName.c_str(), registerName.size(), 0);
	n = recv(mysocket, buffer, BufferLength, 0);
	if(n < BufferLength)
		cout << buffer << endl;
	else
		checkOverflow(buffer, n, mysocket);
}

void loginOption(char* buffer, int n, int mysocket)
{
	memset((char *)buffer, 0, BufferLength);
	cout << " Enter username: ";
	string userName; 
	cin >> userName;
	cout << " Enter the port number: ";
	string portNum; 
	cin >> portNum;
	userName = userName + "#" + portNum;
	send(mysocket, userName.c_str(), userName.size(), 0);
	n = recv(mysocket, buffer, BufferLength, 0);
	if(n < BufferLength)
	{
		temp = buffer;
		cout << buffer << endl;
	}
	else
		checkOverflow(buffer, n, mysocket);
}

void listOption(char* buffer, int n, int mysocket)
{
	string msg;
	msg= "List";
	send(mysocket, msg.c_str(), msg.size(), 0);
	memset((char *)buffer, 0, BufferLength);
	n = recv(mysocket, buffer, BufferLength, 0);
	if(n < BufferLength)
		cout << buffer << endl;
	else
		checkOverflow(buffer, n, mysocket);
}

void quitOption(char* buffer, int n, int mysocket)
{
	string msg;
	msg= "Exit";
	send(mysocket, msg.c_str(), msg.size(), 0);
	memset((char *)buffer, 0, BufferLength);
	n = recv(mysocket, buffer, BufferLength, 0);
	cout << "bye" << endl;
	//if(n < BufferLength)
	//	cout << buffer << endl;
	//else
	//	checkOverflow(buffer, n, mysocket);
	close(mysocket);
	exit(0);
}

int main(int argc, char *argv[])
{

	// Wrong Input Format
	string fstSeg = argv[0];
	// if the argument number is less than 3, the input format is wrong
	if(argc < 3)    
	{
		// if the format is wrong, it will diaplay:
		// ERROR: Correct Inout Format: ./client hostname port
		cout << "ERROR: Correct Input Format: " + fstSeg + " hostname port\n";
		exit(0);
	}
	
	// Address
	struct sockaddr_in suckit;
	memset((char *) &suckit, 0, sizeof(suckit));    // initialize to 0
	suckit.sin_addr.s_addr = inet_addr(argv[1]);    // convert hostname
	suckit.sin_family = AF_INET;    // comunnitation type
	suckit.sin_port = htons(atoi(argv[2]));    // port number

	// creating socket
	int mysocket;
	mysocket = socket(PF_INET, SOCK_STREAM, 0);    // (IPv4, TCP, 0)
	if(mysocket < 0)    // if sd = -1, create error
	{
		cout << "ERROR creating socket.\n";
		exit(0);
	}

	

	// connecting
	if (connect(mysocket, (struct sockaddr*)&suckit, sizeof(suckit)) < 0)
	{
		cout << errno << " ERROR connecting.\n";
		exit(0);
	}

	msgOption(buffer, n, mysocket);

	// connected!
	
	while(true)
	{
	//two options to choose
		string opt; 
		cout<<"Enter 1 for Register, 2 for Login: ";
		cin>>opt;

		//Register
		if(opt=="1")
		{
			registerOption(buffer, n, mysocket);
		}

		//Login
		else if(opt=="2")
		{
			//LOGIN
			loginOption(buffer, n, mysocket);		

			string action;
			string msg;
			
			while(strcmp(temp.c_str(), "220 AUTH_FAIL\n")!=0)
			{
				cout<<"Enter the number of actions you want to take.\n";
				cout<<"1 to ask for the latest list, 8 to Exit: ";
				cin>>action;
				if(action=="1")
				{
					listOption(buffer, n, mysocket);
				}
				else if(action=="8")
				{
					quitOption(buffer, n, mysocket);
				}
			}
		}
	}
}










