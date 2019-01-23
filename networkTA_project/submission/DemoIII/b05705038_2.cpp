#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <queue>
#include <Winsock2.h>
#include <thread>
#include <mutex>

#define MAX_BUFFER_SIZE 1024
#define WSA_VERSION MAKEWORD(2, 2) 	// using winsock 2.2
#define TIMEOUT 5000				// timeout for connection to host

using namespace std;

class Client
{
private:
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
	SOCKET nFd;
	bool isLogin;	// if user have logged in
	string account;	// user's account

	/*
	 * constuctor, create a socket
	 *
	 * @para ip IP of socket host
	 * @para nPort Port that bind with host's IP
	 */
	Client(const char* szHost, uint16_t nPort):isLogin(false),account("")
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
	 * connect to host
	 *
	 * @return bool success of connection
	 */
	bool initConnect() 
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
		
		return string(message);				// it is more convinient in type string
	}

	void disconnect() 
	{
		if (isLogin) 
		{
			emit("Exit\n");
			cout<<onMessage()<<endl;
		}
		closesocket(nFd);
		exit(0);
	}	
};


class Server
{
private:
	SOCKET nFd;
	SOCKET server_nFd;
	SOCKADDR_IN oAddr;

	/*
	 * listen on message from `_nFd`
	 *
	 * @param _nFd The sender of the message
	 * @param bufferSize Buffer size of message
	 * @return string Massage received
	 */
	string onMessage(SOCKET _nFd, int bufferSize = MAX_BUFFER_SIZE)
	{
		char message[bufferSize];
		ZeroMemory(message, bufferSize);	// clear buffer
		int nLen = recv(_nFd, message, sizeof(message), 0);
		if (nLen <= 0)						// end of file
		{
			shutdown(nFd, SD_RECEIVE); 		// gracefully shutdown required	
			return "shutdown";
		}
		return string(message);
	}
	
	/*
	 * emit a message to `_nFd`
	 *
	 * @param _nFd The receiver of the message
	 * @param message Message you want to send
	 */
	void emit(SOCKET _nFd, string message)
	{
		send(_nFd, message.c_str(), message.length(), 0);
	}
	
public:
	/*
	 * constuctor, create a server socket, bind, listen on `szHost:nPort`
	 * and start to work on threads
	 *
	 * @param szHost IP of socket host
	 * @param nPort Port that bind with host's IP
	 */	
	Server(SOCKET server_nFd, uint16_t nPort): server_nFd(server_nFd)
	{
		// create socket
		nFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (nFd == INVALID_SOCKET)
		{
			cout << "Fail to create socket: "<< WSAGetLastError() << endl;
			exit(6);
		}
		
		// record required data (host ip, nPort, etc)
		memset((void*)&oAddr, 0, sizeof(oAddr));	// Allocate memory
		oAddr.sin_family = AF_INET;
		// default local ip
		oAddr.sin_addr.s_addr = INADDR_ANY;

		oAddr.sin_port = htons(nPort);		// set host port number

		// bind address and port
		if (bind(nFd, (SOCKADDR*)&oAddr, sizeof(oAddr)) == SOCKET_ERROR)
		{
			cout << "Fail to bind with address and port." << endl;
			exit(7);
		}
		
		// listen on address and port
		if (listen(nFd, SOMAXCONN) == SOCKET_ERROR)
		{
			cout << "Fail to listen." << endl;
			exit(8);
		}

		while (true)
			accepts();
	}

	/*
	 * accept the connection from client
	 */		
	void accepts()
	{
		SOCKADDR_IN _oAddr;
		int oAddrlen = sizeof(_oAddr);
		SOCKET client_nFd = accept(nFd, (SOCKADDR*)&_oAddr, &oAddrlen);

		if (client_nFd != INVALID_SOCKET)
		{
			string transaction = onMessage(client_nFd);
			emit(server_nFd, "TRANSFER#" + transaction + "\n");
			closesocket(client_nFd);
		}
	}
};

// command status enumeration
enum Command
{
	REGISTER,
	LOGIN,
	LIST,
	EXIT,
	MONEY,
	TRANSFER,
	UNKNOWN_COMMAND,
};

int main(int argc, char** argv)
{
	// create client instance	
	char* ip = (char*)"localhost";
	uint16_t port = 6666;
	if (argc > 1) 
	{
		ip = argv[1];
		port = atoi(argv[2]);
	}
	Client client(ip, port);

	// start connecting
	if (!client.initConnect()) 
	{
		cout<<"Fail to connect to host."<<endl;
		exit(4);
	}

	mutex mutex_pair;	// mutex for pair
	thread* _thread;
	
	// command prefix
	cout << "[";
	if (client.isLogin)
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
		else if(cmd_str=="money")
			cmd=Command::MONEY;
		else if(cmd_str=="transfer")
			cmd=Command::TRANSFER;
		else if(cmd_str=="list")
			cmd=Command::LIST;
		else if(cmd_str=="exit")
			cmd=Command::EXIT;
		else
			cmd=Command::UNKNOWN_COMMAND;

		switch(cmd)
		{

		case Command::REGISTER:
			if (!client.isLogin)	// not logged in
			{
				cout<<"Account Name:";
				string name;
				cin>> name;
				cout<<"Balance:";
				string balance;
				cin>> balance;
				client.emit("REGISTER#" + name +":"+balance+ "\n");

				// receive register result
				cout << client.onMessage() << endl;
			}
			else 
				cout << "You have logged in." << endl;
			break;

		case Command::LOGIN:
			if (!client.isLogin) 	// not logged in
			{
				cout<<"Account Name:";
				string name;
				cin>> name;
				cout<<"Port:";
				string port_str;
				cin>>port_str;
				uint16_t port;
				// port cast
				try
				{
					port = stoi(port_str);
				}
				catch (...)
				{
					cout<<"Invaid port number."<<endl;
					exit(5);
				}
				
				client.emit(name + "#" + port_str + "\n");
				
				// receive login result
				string result = client.onMessage();
				if (result.substr(0, 3) == "220") 	// fail to login	
					cout << result << endl;
				else 								// success to login
				{
					_thread = new thread([](SOCKET nFd, uint16_t port)
					{
						Server s(nFd, port);
					}, client.nFd, port);
					client.account = name;
					client.isLogin = true;
					cout << "Account Balance: " << result;
				}
			}
			else
				cout << "You have logged in." << endl;
			break;

		case Command::LIST:
			if (client.isLogin) 	// have logged in
			{
				client.emit("List\n");
				cout << "Account Balance: " << client.onMessage();
			}
			else
				cout << "You need to login first." << endl;
			break;

		case Command::MONEY:
			if (client.isLogin) 	// have logged in
			{
				cout<<"Amount:";
				string amount;
				cin>>amount;
				client.emit("MONEY#"+amount+'\n');
				cout << client.onMessage();
			}
			else
				cout << "You need to login first." << endl;
			break;	

		case Command::TRANSFER:
			if (client.isLogin)		// have logged in
			{			
				cout<<"Transferee:";
				string transferee;
				cin>>transferee;
				cout<<"Amount:";
				string amount;
				cin>>amount;	
				if(stoi(amount)<0)
				{
					cout<<"You cannot transfer a negative amount."<<endl;
					break;
				}

				// pair
				mutex_pair.lock();
				client.emit("PAIR#" + transferee + "\n");
				string pair_response = client.onMessage();
				cout << pair_response << endl;
				mutex_pair.unlock();

				// if pair success
				if (pair_response.substr(0, 3) != "300")
				{	
					string transferee_ip = pair_response.substr(0, pair_response.find(':'));

					// extract pair response
					pair_response.erase(0, pair_response.find(':') + 1);
					uint16_t transferee_port = stoi(pair_response);

					Client client_transferee(transferee_ip.c_str(), transferee_port);
					
					if (client_transferee.initConnect()) 
					{
						client_transferee.emit(client.account + "#" + amount + "#" + transferee + "\n");
						cout << client_transferee.onMessage() << endl;
						closesocket(client_transferee.nFd);
					}
					else
						cout << "Transaction failed." << endl;
				}
			}
			else
				cout << "You need to login first." << endl;
			break;

		case Command::EXIT:
			client.disconnect();
			break;
		
		default:
			cout << "Allowed commands are below:" << endl;
			if (client.isLogin)
			{
				cout << "'list': List all users online." << endl;
				cout << "'money': change your balance." << endl;
				cout << "'transfer': transfer an amount to target." << endl;
			}
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
		if (client.isLogin)
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
 * 6 Fail to create server socket
 * 7 Fail to bind with address and port
 * 8 Fail to listen
 *
 */