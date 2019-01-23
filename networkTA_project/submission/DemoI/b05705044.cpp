#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <arpa/inet.h>

using namespace std;

int main(int argc, char *argv[])
{
	int sock;
	int err;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1)
	{
		cerr << "Fail to create a socket";
	}

	struct sockaddr_in server;
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(argv[1]);	
	server.sin_port = htons(stoi(argv[2]));

	err = connect(sock, (struct sockaddr *) &server, sizeof(server));
	if(err == -1)
	{
		cerr << "Connection error";
	}

	char receiveMessage[100] = {};
	err = recv(sock, receiveMessage, sizeof(receiveMessage), 0);
	if(err == -1)
		cerr << "Message Receiving Error";

	char message[50] = {};
	char name[50] = {};
	char portNumber[5] = {};
	while(true)
	{
		cout << "Enter 1 for Register, 2 for Login: ";
	int choice;
	cin >> choice;
	if(choice == 1)
	{
		cout << "Enter the name you want to register:";
		cin >> name;
		strcpy(message, "");
		strcat(message, "REGISTER#");
		strcat(message, name);
		strcat(message, "\n");
	}
	else if(choice == 2)
	{
		cout << "Enter your name: ";
		cin >> name;
		cout << "\n";
		cout << "Enter the port number: ";
		cin >> portNumber;
		strcpy(message, "");
		strcat(message, name);
		strcat(message, "#");
		strcat(message, portNumber);
		strcat(message, "\n");
	}
	char receiveMessage1[1000];
	send(sock, message, sizeof(message), 0);
	recv(sock, receiveMessage1, sizeof(receiveMessage1), 0);
	printf("%s\n", receiveMessage1);
	}
	
	close(sock);
	return 0;
}
