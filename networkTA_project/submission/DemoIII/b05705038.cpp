#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <queue>
#include <Winsock2.h>
#include <thread>
#include <chrono>
#include <mutex>

#define MAX_BUFFER_SIZE 1024
#define WSA_VERSION MAKEWORD(2, 2) 	// using winsock 2.2
#define TIMEOUT 5000				// timeout for connection to host

using namespace std;
using namespace chrono_literals;	// for simply use 0.1s to represent 0.1 second etc

struct User
{
	string oAddr;
	string port;
	string name;
	bool isLogin;
	int balance;
	
	User(string name, int balance):name(name), balance(balance), isLogin(false) {}
};

struct Client
{
	SOCKET nFd;
	SOCKADDR_IN oAddr;
	
	Client(SOCKET nFd, SOCKADDR_IN oAddr):nFd(nFd), oAddr(oAddr) {}
};

class Server
{
private:
	SOCKET nFd;
	SOCKADDR_IN oAddr;
	
	mutex mutex_list;
	mutex mutex_request;
	vector<User*> users;
	vector<thread> threads;	// thread pool
	queue<Client*> requests;	// use queue instead of vector for better efficiency

	
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

	/*
	 * add an user to vector `users`
	 *
	 * @param name Name of new user
	 */
	void registerUser(string name, int balance)
	{
		// start lock `list`
		mutex_list.lock();
		users.push_back(new User(name, balance));
		mutex_list.unlock();
		// release lock `list`
	}

	/*
	 * get stuct `User` if find identical name
	 *
	 * @param name Name of queried user
	 * @return User* A pointer of stuct `User`
	 */
	User* getUser(string name)
	{
		// start lock `list`
		mutex_list.lock();
		for (auto it = users.begin(); it!=users.end(); it++)
		{
			if ((*it)->name == name)
			{
				mutex_list.unlock();
				// release lock `list`
				return *it;
			}
		}
		// if not found
		mutex_list.unlock();
		// release lock `list`
		return nullptr;
	}

	/*
	 * get all online users' list
	 *
	 * @return string A string printed all online users
	 */
	string getList()
	{
		string list;	
		// start lock `list`
		mutex_list.lock();
		for (auto it = users.begin(); it!=users.end(); it++)
		{
			if ((*it)->isLogin)
				list += (*it)->name+'#'+(*it)->oAddr+'#'+(*it)->port+'\n';
		}
		mutex_list.unlock();
		// release lock `list`
		return list;
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
	 * work that assign to every thread to do
	 *
	 * @param id ID of thread
	 */
	void onThread(int id)
	{
		while (true)
		{
			// wait for request in every 0.1 second
			while (requests.empty())
				this_thread::sleep_for(0.1s);

			// start lock `request`
			mutex_request.lock();
			// access task queue
			if (!requests.empty())
			{
				Client* client = requests.front();
				requests.pop();
				mutex_request.unlock();
				// release lock `request`

				// Communicating with client
				communicate(id, client->nFd, client->oAddr);
			}
			else// release lock `request`
				mutex_request.unlock();
		}
	}
	
public:
	/*
	 * constuctor, create a server socket, bind, listen on `szHost:nPort`
	 * and start to work on threads
	 *
	 * @param szHost IP of socket host
	 * @param nPort Port that bind with host's IP
	 */	
	Server(char* szHost, uint16_t nPort)
	{
		if(!initWinsock())
		{	// if init Windows Socket return false
			cout << "Fail to init Windows Socket." << endl;
			exit(1);
		}

		// create socket
		nFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (nFd == INVALID_SOCKET)
		{
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

		// bind address and port
		if (bind(nFd, (SOCKADDR*)&oAddr, sizeof(oAddr)) == SOCKET_ERROR)
		{
			cout << "Fail to bind with address and port." << endl;
			exit(4);
		}
		
		// listen on address and port
		if (listen(nFd, SOMAXCONN) == SOCKET_ERROR)
		{
			cout << "Fail to listen." << endl;
			exit(5);
		}

		cout << "Server is now on " << szHost << ':' << nPort << endl;
		
		// work on threads
		size_t max_thread = thread::hardware_concurrency();
		cout << "Space for " << max_thread << " users." << endl;	
		for (auto i = 0; i < max_thread; i++)
			threads.push_back(thread(&Server::onThread, this, i));
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
			// start lock `request`
			mutex_request.lock();
			requests.push(new Client(client_nFd, _oAddr));
			mutex_request.unlock();
			// release lock `request`
		}
	}

	/*
	 * communicate with client
	 * 
	 * @param _nFd Client's socket object
	 * @param _oAddr Client's socket address information
	 */	
	void communicate(int id, SOCKET _nFd, SOCKADDR_IN _oAddr)
	{
		User* user = nullptr;
		while (true)
		{
			string cmd = onMessage(_nFd);

			if (cmd.substr(0, 6) == "MONEY#")
			{
				if(user)
				{
					string amount = cmd.substr(6, cmd.length() - 7); 
					// exclude <crlf> (Thanks to classmate's remind)
					try
					{
						user->balance = stoi(amount);
					}
					catch (...)
					{
						cout<<"Invaid number."<<endl;
						exit(7);
					}
					emit(_nFd, "Your balance has been changed to "+amount+".\n");
				}
				else
					emit(_nFd, "220 AUTH_FAIL\nPlease login first.\n");
			}
			else if (cmd.substr(0, 9) == "REGISTER#")
			{
				if (!user) 
				{
					cmd.erase(0, 9);
					string name = cmd.substr(0, cmd.find(':'));
					cmd.erase(0, cmd.find(':') + 1);
					// exclude <crlf> (Thanks to classmate's remind)
					string balance_str = cmd.substr(0, cmd.length() - 1);
					unsigned int balance;
					// balance cast
					try
					{
						balance = stoi(balance_str);
					}
					catch (...)
					{
						cout<<"Invaid balance number."<<endl;
						exit(7);
					}

					// avoid name's exception (Thanks to classmate's remind)
					if (name != "REGISTER" && name != "") 
					{
						// make sure that this name has not been registered
						if (!getUser(name)) 
						{
							registerUser(name, balance);
							emit(_nFd, "100 OK\n");
						}
						else
							emit(_nFd, "210 FAIL\nYour name has been taken.\n");
					}
					else
						emit(_nFd, "210 FAIL\nYour name has ambiguity.\n");
				}
				else 
					emit(_nFd, "210 FAIL\nYou have logged in, you cannot register.\n");	
			}
			else if (cmd.substr(0, 5) == "PAIR#")
			{
				if (user)
				{
					cmd.erase(0, 5);
					// exclude <crlf> (Thanks to classmate's remind)
					string transferee_name = cmd.substr(0, cmd.length() - 1);	
					// get transferee instance
					User* transferee = getUser(transferee_name);
					if (!transferee)
						emit(_nFd, "300 FAIL\nCannot find transferee.\n");
					else 
					{
						if(transferee->isLogin)
							emit(_nFd, transferee->oAddr + ":" + transferee->port + "\n");
						else
							emit(_nFd, "301 FAIL\nPayee has not logged in.\n");
					}
				}
				else
					emit(_nFd, "220 AUTH_FAIL\nPlease login first.\n");
			}
			else if (cmd.substr(0, 9) == "TRANSFER#") 
			{
				if (user) 
				{
					cmd.erase(0, 9);
					string transferer_name = cmd.substr(0, cmd.find('#'));
					// get transferer
					User* transferer = getUser(transferer_name);
					cmd.erase(0, cmd.find('#') + 1);
					int amount = stoi(cmd.substr(0, cmd.find('#')));
					// transfer amount
					int old_transferer_balance = transferer->balance;
					int old_user_balance = user->balance;
					transferer->balance -= amount;
					user->balance += amount;
					if(old_transferer_balance + old_user_balance != (transferer->balance + user->balance))
					{
						cout<<"Balance overflow."<<endl;
						exit(8);
					}
				}
				else
					emit(_nFd, "220 AUTH_FAIL\nPlease login first.\n");
			}
			else if (cmd == "List\n") 
			{
				if (user)
					emit(_nFd, to_string(user->balance) + '\n' + getList());
				else
					emit(_nFd, "220 AUTH_FAIL\nPlease login first.\n");
			}
			else if (cmd == "Exit\n")
			{
				if (user)
				{
					emit(_nFd, "Bye\n");
					user->isLogin = false;
				}
				else
					emit(_nFd, "220 AUTH_FAIL\nPlease login first.\n");
				break;
			}
			else if (cmd == "shutdown") // this is for forced termination message from client
				break;
			else	// login
			{
				if (!user)
				{	
					// find name
					user = getUser(cmd.substr(0, cmd.find('#')));
					if (user)
					{
						if(user->isLogin)
						{
							emit(_nFd, "220 AUTH_FAIL\nYou have logged in.\n");
							break;
						}
						// port
						cmd.erase(0, cmd.find('#') + 1);
						// erase <crlf> (Thanks to classmate)
						cmd.erase(cmd.length() - 1, 1);
						user->oAddr = inet_ntoa(_oAddr.sin_addr);
						user->port = cmd;
						user->isLogin = true;

						emit(_nFd, to_string(user->balance) + '\n' + getList());
					}
					else
						emit(_nFd, "220 AUTH_FAIL\nYou have not registered yet.\n");
				}
				else
					emit(_nFd, "220 AUTH_FAIL\nYou have logged in.\n");
			}
		}
		closesocket(_nFd);
	}
};

int main(int argc, char** argv){
	// create client instance	
	char* ip = (char*)"0.0.0.0";
	uint16_t port = 6666;
	if (argc > 1) 
	{
		ip = argv[1];
		port = atoi(argv[2]);
	}
	Server server(ip, port);
	while (true)
		server.accepts();
	
	return 0;
}

/*
 * exit code
 *
 * 0 Normally exit
 * 1 Fail to init Windows Socket
 * 2 Fail to create socket
 * 3 Invalid host name
 * 4 Fail to bind with address and port
 * 5 Fail to listen
 * 6 Fail to accept client
 * 7 Invalid number
 * 8 Overflow
 *
 */