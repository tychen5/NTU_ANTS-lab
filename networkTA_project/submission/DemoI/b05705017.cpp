#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <string.h> 
#include <unistd.h>
#include <iostream>
#include <string>
#include <cstring>

#define buffer_size 1024

using namespace std;
int main(int argc, char const *argv[]) 
{ 
	if(argc != 3)
	{
		cout << "\n Error arguments! \n";
		return -1;
	}

	struct sockaddr_in address; 
	int sock = 0;
	struct sockaddr_in serv_addr; 
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 

	memset(&serv_addr, '0', sizeof(serv_addr)); 

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(atoi(argv[2])); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		return -1; 
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n"); 
		return -1; 
	} 

	char buffer[buffer_size];
	recv(sock, buffer, buffer_size, 0); 
	cout << buffer << endl;
	bzero(buffer, 128);

	cout << "Enter 1 for register, 2 for login : ";
	int option;
	cin >> option;
	
	while(1)
	{
		if(option == 1)
		{
			cout << "\nEnter your name : ";
			string name;
			cin >> name;
			string r = "REGISTER#" + name;
			send(sock, r.c_str(), r.length(), 0);
			recv(sock, buffer, buffer_size, 0);
			cout << buffer;
			bzero(buffer, 128);
			cout << "Enter 1 for register, 2 for login : ";
			cin >> option;
		}
		if(option == 2)
		{
			cout << "\nEnter your name : ";
			string name;
			cin >> name;
			string portN;
			cout << "\nEnter port number : ";
			cin >> portN;
			if(stoi(portN) < 1024 || stoi(portN) > 65535)
			{
				cout << "invalid port number! automatic set your port to \"22222\"" << endl;
				portN = "22222";
			}
			string l = name + "#" + portN;
			send(sock, l.c_str(), l.length(), 0);
			recv(sock, buffer, buffer_size, 0);
			cout << buffer;
			bzero(buffer, 128);
			string option;
			cin >> option;
			if(option == "List")
			{
				send(sock, l.c_str(), l.length(), 0);
				recv(sock, buffer, buffer_size, 0);
				cout << buffer;
				bzero(buffer, 128);	
				break;
			}
		}
		
	}
	return 0;
} 

