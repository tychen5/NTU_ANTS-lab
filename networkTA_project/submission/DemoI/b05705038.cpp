#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <Winsock2.h>

#define MAX_BUFFER_SIZE 1024
#define WSA_VERSION MAKEWORD(2, 2) 	// using winsock 2.2
#define TIMEOUT 5000				// timeout for connection to host

using namespace std;

class Client
{
private:
	SOCKET nFd;
	SOCKADDR_IN oAddr;

	/*
	 * init Windows Socket
	 *
	 * @return bool success of initialization Windows Socket
	 */
	bool initWinsock()
	{
		WSADATA	WSAData = { 0 };
	    if (WSAStartup(WSA_VERSION, &WSAData) != 0)	// if fail to start up
	    {
	        // tell the user that we could not find a usable WinSock DLL.
	        if (LOBYTE(WSAData.wVersion) != LOBYTE(WSA_VERSION) ||
	            HIBYTE(WSAData.wVersion) != HIBYTE(WSA_VERSION))
	            cout<<"Incorrect winsock version"<<endl;

	        WSACleanup();
	        return false;
	    }
	    return true;
	}
	
public:

	bool isLogIn;	// if user have logged in
	string account;	// user's account

	/*
	 * constuctor, create a socket
	 *
	 * @para ip IP of socket host
	 * @para nPort Port that bind with host's IP
	 */
	Client(char* szHost, uint16_t nPort):isLogIn(false),account("")
	{
		if(!initWinsock()){	// if init Windows Socket return false
			cout << "Fail to init Windows Socket." << endl;
			exit(1);
		}
		
		// create socket
		nFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (nFd == INVALID_SOCKET){
			cout << "Fail to create socket: "<< WSAGetLastError() << endl;
			exit(2);
		}
		
		// record required data (host ip, nPort, etc)
		memset((void*)&oAddr, 0, sizeof(oAddr));	// Allocate memory
		oAddr.sin_family = AF_INET;
		unsigned long uIP = inet_addr(szHost);
		// check if host ip is INADDR_NONE
		if (uIP == INADDR_NONE)
		{
			hostent* poHost = 0;
		    poHost = gethostbyname(szHost);
		    if (poHost != 0)
		    {
		        struct in_addr** pptr = (struct in_addr**)poHost->h_addr_list;
		        oAddr.sin_addr = **pptr; 	// memberwise clone
		    }
		    else
		    {
		        cout<<"Invalid host name: "<< szHost<<endl;
		        closesocket(nFd);
		        exit(3);
		    }
		}
		else
		    oAddr.sin_addr.s_addr = uIP;	// set host ip address

		oAddr.sin_port = htons(nPort);		// set host port number
	}

	/*
	 * try to connect to host
	 *
	 * @return bool success of connection
	 */
	bool tryConnect() 
	{
		// nonblocking connect
		struct timeval oTV;
		oTV.tv_sec = TIMEOUT / 1000;
		oTV.tv_usec = TIMEOUT;
		fd_set oRead, oWrite;
		FD_ZERO(&oRead);
		FD_ZERO(&oWrite);
		int nResult;

		nResult = connect(nFd, (const SOCKADDR*)&oAddr, sizeof(oAddr));
		// if fail to connect
		if (nResult == SOCKET_ERROR)
		{
		    if (WSAGetLastError() != WSAEWOULDBLOCK)
		    {
		        cout<<"Fail to connect:"<< WSAGetLastError()<<endl;
		        closesocket(nFd);
		        return false;
		    }
		    else // need select
		    {
		        FD_SET(nFd, &oRead);
		        oWrite = oRead;
		        nResult = select(nFd+1, &oRead, &oWrite, 0, &oTV);
		        if (nResult == 0)
		            cout<<"Connection timeout"<<endl;
		        else if (FD_ISSET(nFd, &oRead) || FD_ISSET(nFd, &oWrite))
		        {
	                cout<<"Connection cannot W/R";
	                closesocket(nFd);
	                return false;
		        }
		        else
		        {
		            cout<<"Unknown error when connect"<<endl;
		            closesocket(nFd);
		            return false;
		        }
		    }
		} 
		return true;	// else connected immediately
	}

	void emit(string message) 
	{
		send(nFd, message.c_str(), message.length(), 0);
	}

	string onMessage(int bufferSize = MAX_BUFFER_SIZE) 
	{
		char message[bufferSize];
		ZeroMemory(message, bufferSize);	// clear buffer
		int nLen = recv(nFd, message, sizeof(message), 0);
		if (nLen == 0) 						// end of file
			shutdown(nFd, SD_RECEIVE); 		// gracefully shutdown required
		
		return string(message);				// it is more conivent in type string
	}

	void disconnect() 
	{
		if (isLogIn) 
		{
			emit("Exit\n");
			cout<<onMessage()<<endl;
		}
		closesocket(nFd);
		exit(0);
	}	
};


// command status enumeration
enum Command
{
	REGISTER,
	LOGIN,
	LIST,
	EXIT,
	UNKNOWN_COMMAND,
};

int main(int argc, char** argv){
	// create client instance	
	char* ip = "140.112.107.194";
	uint16_t port = 33120;
	if (argc > 1) 
	{
		ip = argv[1];
		port = atoi(argv[2]);
	}
	Client client(ip, port);

	// start connecting
	if (!client.tryConnect()) 
	{
		cout<<"Fail to connect to host."<<endl;
		exit(4);
	}
	
	cout<<client.onMessage();		// welcome message from host

	// command prefix
	cout << "[";
	if (client.isLogIn)
		cout << client.account<<"@";
	cout<<ip<<"]$ ";
	string cmd_str;			// command input in type string
	Command cmd;
	while (cin>>cmd_str) 
	{	
		if(cmd_str=="register")
			cmd=Command::REGISTER;
		else if(cmd_str=="login")
			cmd=Command::LOGIN;
		else if(cmd_str=="list")
			cmd=Command::LIST;
		else if(cmd_str=="exit")
			cmd=Command::EXIT;
		else
			cmd=Command::UNKNOWN_COMMAND;

		switch(cmd)
		{

		case Command::REGISTER:
			if (!client.isLogIn)	// not logged in
			{
				cout<<"Account Name:";
				string name;
				cin>> name;
				client.emit("REGISTER#" + name + "\n");

				// receive register result
				cout << client.onMessage() << endl;
			}
			else 
				cout << "You have logged in." << endl;
			break;

		case Command::LOGIN:
			if (!client.isLogIn) 	// not logged in
			{
				cout<<"Account Name:";
				string name;
				cin>> name;
				cout<<"Port:";
				string port;
				cin>>port;

				// check port number's validation
				if(stoi(port)>65535 || stoi(port)<0)
				{
					cout<<"Invaid port number."<<endl;
					exit(5);
				}
				
				client.emit(name + "#" + port + "\n");
				
				// receive login result
				string result = client.onMessage();
				if (result.substr(0, 3) == "220") 	// fail to login	
					cout << result << endl;
				else 								// success to login
				{
					client.account = name;
					client.isLogIn = true;
					// message have to seperate into 2 buffer
					cout << "Account Balance: " << result;
					cout <<client.onMessage() << endl;
				}
			}
			else
				cout << "You have logged in." << endl;
			break;

		case Command::LIST:
			if (client.isLogIn) 	// have logged in
			{
				client.emit("List\n");
				// message have to seperate into 2 buffer
				cout << "Account Balance: " << client.onMessage();
				cout<<client.onMessage() << endl;
			}
			else
				cout << "You need to login first." << endl;
			break;

		case Command::EXIT:
			client.disconnect();
			break;
		
		default:
			cout << "Allowed commands are below:" << endl;
			if (client.isLogIn)  	// not logged in
				cout << "'list': List all users online." << endl;
			else 
			{
				cout << "'register': Register an account." << endl;
				cout << "'login': Login to host." << endl;
			}
			cout << "'exit': Disconnect." << endl;
			
			cout << endl;
			break;
		}

		// command prefix
		cout << "[";
		if (client.isLogIn)
			cout << client.account<<"@";
		cout<<ip<<"]$ ";
	}

	return 0;
}


/*
 * exit code
 *
 * 0 Normally exit
 * 1 Fail to init Windows Socket
 * 2 Fail to create socket
 * 3 Invalid host name
 * 4 Fail to connect to host
 * 5 Invaid port number
 *
 */