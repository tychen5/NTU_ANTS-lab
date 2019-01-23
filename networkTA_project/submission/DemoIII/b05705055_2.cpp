#pragma comment(lib, "Ws2_32.lib")
 
#include <WinSock2.h>
#include <iostream>
#include <string>
#include <sstream>
 
using namespace std;
//Server IP: 140.112.107.194 port=33120 
int main(){
	
	const int MSG_SIZE = 100;
	const int CMD_SIZE = 100;
	WSAData wsa;
	WORD version = MAKEWORD(2, 2);
	
	int Result = WSAStartup(version, &wsa);
	
	SOCKET server = socket(AF_INET, SOCK_STREAM, NULL);
	
	char IP[CMD_SIZE];
	int portNum;
	char pNum[CMD_SIZE] = "#";
	cout << "Please enter the IP address you want to connect: ";
	cin >> IP;
	cout << "Please enter the port number: ";
	cin >> portNum;
	
	char tempNum[CMD_SIZE];
	sprintf(tempNum, "%d", portNum);
	strcat(pNum, tempNum);
	
	SOCKADDR_IN targetAddress;
	targetAddress.sin_addr.s_addr = inet_addr(IP);
    targetAddress.sin_family = AF_INET;
    targetAddress.sin_port = htons(portNum);
	
	connect(server, (SOCKADDR*) &targetAddress, sizeof(targetAddress));
	char message[MSG_SIZE];
	ZeroMemory(message, MSG_SIZE);
	Result = recv(server, message, sizeof(message), 0);
	if (server != SOCKET_ERROR){
		cout << "-------------------------------" << endl;
		cout << "Connection succeeded!" << endl;
		cout << "-------------------------------" << endl;
	}
	
	bool whetherSignIn = false;
	string signinName = "";
	
	while(true){
		
		char command[CMD_SIZE];
		cout << endl << "Now enter the command that you want to communicate with the server:" << endl;
		cout << "(Type 'help' can make you know what command you can request.)" << endl;
		cin >> command;
		for (int i = 0; command[i]; i++){
			if (command[i] >= 'A' && command[i] <= 'Z')
            	command[i] += 32;
		}
		string command_type;
		
		// Help
		command_type = command;
		command_type = command_type.substr(0,5);
		if (command_type == "help"){
			cout << endl << "All commands follow the following descriptions:" << endl << endl;
			cout << "'register': Register an account on the server." << endl;
			cout << "'signin': Sign in to an existing account on the server." << endl;
			cout << "The commands after signing in:" << endl;
			cout << "'pay': Do the micropayment with the other online client.";
			cout << "'list': Request the latest online list from the server." << endl;
			cout << "'exit': End the entire program." << endl;
			continue;
		}
		
		// Register
		command_type = command;
		command_type = command_type.substr(0, 9);
		if (command_type == "register"){
			
			if (whetherSignIn == true){
				cout << endl << "You have already signed in an account." << endl;
				cout << "You cannot register an account now!" << endl;
				continue;
			}
			
			char userAccountName[CMD_SIZE];
			cout << endl << "Enter the user account name that you wnat to register: ";
			cin >> userAccountName;
			
			char message[MSG_SIZE] = "REGISTER#";
			strcat(message, userAccountName);
			Result = send(server, message, (int)strlen(message), 0);
			
			ZeroMemory(message, MSG_SIZE);
			Result = recv(server, message, sizeof(message), 0);
			string registerResult = message;
			if (registerResult.substr(0,3) == "100")
				cout << endl << "Register succeeded!" << endl;
			else
				cout << endl << "Register failed!" << endl;
			continue;
		}
		
		// Signin
		command_type = command;
		command_type = command_type.substr(0, 7);
		if (command_type == "signin"){
			
			if (whetherSignIn == true){
				cout << endl << "You have already signed in an account." << endl;
				continue;
			}
			
			char message[CMD_SIZE];
			cout << endl << "Enter the user account name: ";
			cin >> message;
			
			strcat(message, pNum);
			Result = send(server, message, (int)strlen(message), 0);
			
			ZeroMemory(message, MSG_SIZE);
			Result = recv(server, message, sizeof(message), 0);
			string signinResult = message;
			if (signinResult.substr(0,13) == "220 AUTH_FAIL")
				cout << endl << "Sign in failed! The user name is not existed!" << endl;
			else{
				whetherSignIn = true;
				signinName = message;
				cout << endl << "Account balance: " << message;
				ZeroMemory(message, MSG_SIZE);
				Result = recv(server, message, sizeof(message), 0);
				cout << message;
			}
			continue;
		}
		
		// Pay
		command_type = command;
		command_type = command_type.substr(0, 4);
		if (command_type == "pay"){
			
			if (whetherSignIn == false){
				cout << endl << "You have not yet signed in any account." << endl;
				continue;
			}
			
			bool transactEnd = false;
			while(transactEnd == false){
				string payAccountName;
				cout << endl << "Enter the user account name that you wnat to transact: ";
				cin >> payAccountName;
				
				while(true){
					string payAmount;
					cout << endl << "Enter the amount that you wnat to pay(cannot exceed your account balance): ";
					cin >> payAmount;
					
					string payMessage = signinName + "#" + payAmount + "#" + payAccountName;
					char message[MSG_SIZE];
					strcpy(message, payMessage.c_str());
					Result = send(server, message, (int)strlen(message), 0);
					
					ZeroMemory(message, MSG_SIZE);
					Result = recv(server, message, sizeof(message), 0);
					string payResult = message;
					
					if (payResult.substr(0,3) == "100"){
						cout << endl << "Transaction succeeded!" << endl;
						transactEnd = true;
						break;
					}
					else if (payResult.substr(0,3) == "211"){
						cout << endl << "Account balance insufficient!" << endl;
						continue;
					}
					else{
						cout << endl << "Paid user name does not exist!" << endl;
						break;
					}
				}	
			}
			continue;
		}
		
		
		// List
		command_type = command;
		command_type = command_type.substr(0, 5);
		if (command_type == "list"){
			
			if (whetherSignIn == false){
				cout << endl << "You have not yet signed in any account." << endl;
				continue;
			}
			
			char message[MSG_SIZE] = "List\n";
			Result = send(server, message, (int)strlen(message), 0);
			
			ZeroMemory(message, MSG_SIZE);
			Result = recv(server, message, sizeof(message), 0);
			cout << endl << "Account balance: " << message;
			ZeroMemory(message, MSG_SIZE);
			Result = recv(server, message, sizeof(message), 0);
			cout << message;
			continue;
		}
		
		// Exit
		command_type = command;
		command_type = command_type.substr(0, 5);
		if (command_type == "exit"){
			
			if (whetherSignIn == false){
				cout << endl << "You have not yet signed in any account." << endl;
				continue;
			}
			
			char message[CMD_SIZE] = "Exit\n";
			Result = send(server, message, (int)strlen(message), 0);
			
			ZeroMemory(message, MSG_SIZE);
			Result = recv(server, message, sizeof(message), 0);
			cout << endl << message;
			cout << endl << "Thank you for your using!" << endl;
			break;
		}
		
		//Illegal input
		cout << endl << "This command is not existed!" << endl;
	}
	
	closesocket(server);
	
	return 0;
}
