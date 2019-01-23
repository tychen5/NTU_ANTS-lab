#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <netdb.h>
#include <string>
#define LEN 1024

using namespace std;

bool print_recv(int &sockfd){
	char buf[LEN];
	memset(&buf, 0, sizeof(buf));
	recv(sockfd, buf, sizeof(buf), 0);
	if(strcmp(buf, "210 FAIL\n")==0 || strcmp(buf, "220 AUTH_FAIL\n")==0){
		return false;
	}else{
		printf("%s\n",buf);
		return true;
	}

}

void send_str(int &sockfd, string &str){
	char req[LEN];
	strncpy(req, str.c_str(), sizeof(req));
	send(sockfd, req, strlen(req), 0);
}

int main(int argc, char *argv[])
{
    /*client socket set up*/
		int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);//(IPv4, TCP, protocal)
    if (sockfd == -1){
    	cout << "Fail to create a socket.\n";
    }

    /*socket connect*/
    struct sockaddr_in server;// server address
    struct hostent *hostnm;//server host name info
    hostnm = gethostbyname(argv[1]);
    int port;
    port = atoi(argv[2]);

    bzero(&server,sizeof(server));
    server.sin_family = AF_INET; //IPv4
    server.sin_addr = *((struct in_addr *)hostnm->h_addr);
    server.sin_port = htons(port);

    int err = connect(sockfd,(struct sockaddr *)&server,sizeof(server));
    if(err == -1){
    	cout << "Connection Error\n";
    }


	  print_recv(sockfd);// Connection accepted

    /*main function*/
    bool is_leave = false;
		string username;
		string userPort;
		string command;
		string amount;

		while(!is_leave){
			cout << "Enter r for register, l for login: ";
			cin >> command;
			if(command == "r"){
				cout << "Enter username: ";
				cin >> username;
				cout << "The money you want to deposit: ";
				cin >> amount;
				string regis = "REGISTER#" + username + "\n";
				send_str(sockfd, regis);
				if(!print_recv(sockfd)){
					cout << "Fail to register\n";
				}


			}else if(command == "l"){
				cout << "Enter username: ";
				cin >> username;
				cout << "Enter port: ";
				cin >> userPort;
				string login = username + "#" + amount + "#" + userPort +"\n";
				send_str(sockfd, login);
				if(!print_recv(sockfd)){
					cout << "User has not registered yet\n";
				}else{
					while(!is_leave){
						cout << "Enter ls for list, q for exit: ";
						cin >> command;
						if(command == "ls"){
							string list = "List\n";
							send_str(sockfd, list);
							print_recv(sockfd);

						}else if(command == "q"){
							string exit = "Exit\n";
							send_str(sockfd, exit);
							print_recv(sockfd);
							is_leave = true;

						}else{
							cout << "Wrong Enter\n";
						}
					}

				}

			}else{
				cout << "Wrong Enter\n";
			}

		}
		close(sockfd);
    return 0;
}
