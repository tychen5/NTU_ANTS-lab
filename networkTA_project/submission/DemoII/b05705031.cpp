/**
	© 2017-2018 Tai-Yuan Kuo All Rights Reserved.
*/
#pragma GCC optimize("O3")
#include <bits/stdc++.h>
#include <winsock2.h>

#pragma comment(lib,"ws2_32.lib")

using namespace std;

#define for0(i, n) for(int i = 0; i < n; ++i)
#define for1(i, n) for(int i = 1; i <= n; ++i)
#define rfor0(i, n) for(int i = n - 1; i >= 0; --i)
#define rfor1(i, n) for(int i = n; i > 0; --i)
#define all(it, a) for(auto it = (a).begin(); it != (a).end(); ++it)
#define rall(it, a) for(auto it = (a).rbegin(); it != (a).rend(); ++it)
#define rep for0

class ThreadPool
{
public:
	unordered_map<thread::id,thread> threads;
	mutex threadsMutex;

	bool deleteThread(thread::id id)
	{
		unordered_map<thread::id,thread>::iterator it;
		unordered_map<thread::id,thread>::iterator endIt;

		do
		{
			this_thread::yield();
			lock_guard<mutex> lg(threadsMutex);
			it = threads.find(id);
			endIt = threads.end();
		}
		while (it == endIt);

		lock_guard<mutex> lg(threadsMutex);
		it->second.detach();
		threads.erase(it);
	}

public:
	template <typename Function, typename... Args>
	thread::id addThread(const Function& f, Args... args)
	{
		thread t([=, &f]()
		{
			thread::id id = this_thread::get_id();
			f(args...);
			async(launch::async, [&]()
			{
				deleteThread(id);
			});
		});

		lock_guard<mutex> lg(threadsMutex);
		threads.emplace(t.get_id(), move(t));
	}
};

class Job
{
public:
	SOCKET sock;
	struct sockaddr_in address;

	Job(){}
	Job(SOCKET sock, struct sockaddr_in address): sock(sock), address(address){}
};

class Semaphore
{
public:
	mutex takeMutex;
	mutex putMutex;
	mutex m;
	size_t maximum;
	size_t count;

	Semaphore(size_t count = 0, size_t maximum = -1):
		count(count), maximum(maximum)
	{
		if (count == 0)
		{
			takeMutex.lock();
		}
		if (count == maximum)
		{
			putMutex.lock();
		}
	}

	inline void take()
	{
		lock(takeMutex, m);
		--count;
		if (count != 0)
		{
			takeMutex.unlock();
		}
		putMutex.try_lock();
		putMutex.unlock();
	}

	inline void put()
	{
		lock(putMutex, m);
		++count;
		if (count != maximum)
		{
			putMutex.unlock();
		}
		takeMutex.try_lock();
		takeMutex.unlock();
	}
};

class Account
{
public:
	string name;
	long long balance;

	Account(){}
	Account(string name): name(name), balance(0){}
	Account(string name, long long balance): name(name), balance(balance){}
};

WSADATA wsa;
SOCKET mainSocket;
struct sockaddr_in server;
string mainPort;

bool displayLog;
mutex consoleMutex;

ThreadPool threads;
size_t threadPoolSize;

queue<Job> clientQueue;
Semaphore clientQueueSemaphore;

unordered_map<string,Account> accounts;
mutex accountsMutex;

unordered_map<thread::id,Account*> onlineAccounts;
mutex onlineAccountsMutex;

unordered_map<thread::id,struct sockaddr_in> onlineAddresses;
mutex onlineAddressesMutex;

int timeoutPeriod; // in milli seconds

/** Process run options */
inline void processArguments(int argc, char* argv[])
{
	mainPort = "31415";
	threadPoolSize = max(thread::hardware_concurrency() / 2, 2u);
	timeoutPeriod = 15 * 60 * 1000;
	displayLog = true;
	if (argc >= 2)
	{
		mainPort = argv[1];
	}
	if (argc >= 3)
	{
		threadPoolSize = max(atoi(argv[2]), 2);
	}
	if (argc >= 4)
	{
		timeoutPeriod = atoi(argv[3]) * 1000;
	}
	if (argc >= 5)
	{
		displayLog = false;
	}
}

/** Initializes winsock */
inline WSADATA wsaInit()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2,2),&wsa))
	{
		ostringstream message;
		message << "Winsock initialization failed. Error Code: " << WSAGetLastError();
		throw runtime_error(message.str());
	}
	return wsa;
}

/** Creates socket */
inline SOCKET createSocket()
{
	SOCKET s;
	if((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		ostringstream message;
		message << "Failed to create socket. Error Code: " << WSAGetLastError();
		throw runtime_error(message.str());
	}
	return s;
}

/** Bind socket */
inline void bindSocket(SOCKET s)
{
	if(bind(s, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
		ostringstream message;
		message << "Bind failed with error code : %d" << WSAGetLastError();
		throw runtime_error(message.str());
    }
}

/** Listen to socket */
inline void listenSocket(SOCKET s)
{
	if(listen(s, SOMAXCONN) == SOCKET_ERROR)
    {
		ostringstream message;
		message << "Listen failed with error code : %d" << WSAGetLastError();
		throw runtime_error(message.str());
    }
}

/** Error prompt to retry */
template<typename E>
inline void errorWait(const E& e)
{
	lock_guard<mutex> lg(consoleMutex);
	cout << e.what() << endl;
	for (int i = 3; i > 0; --i)
	{
		cout << "\rRetrying in 3 second(s)...";
		this_thread::sleep_for (chrono::seconds(1));
	}
	cout << "\n";
}

/** Setup listening port */
inline void initializePort()
{
	// Initialize Winsock
init:
	try
	{
		wsa = wsaInit();
	}
	catch (const runtime_error& e)
	{
		errorWait(e);
		goto init;
	}

	// Create socket
cSock:
	try
	{
		mainSocket = createSocket();
	}
	catch (const runtime_error& e)
	{
		errorWait(e);
		goto cSock;
	}

	// Fill server info in
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(mainPort.c_str()));

	// Bind the Socket
bSocket:
	try
	{
		bindSocket(mainSocket);
	}
	catch (const runtime_error& e)
	{
		errorWait(e);
		goto bSocket;
	}

	// Start listening at the port
lPort:
	try
	{
		listenSocket(mainSocket);
	}
	catch (const runtime_error& e)
	{
		errorWait(e);
		goto lPort;
	}
}

/** Display log */
inline void log(string message)
{
	if (displayLog)
	{
		lock_guard<mutex> lg(consoleMutex);
		cout << message;
	}
}

/** Receive a line from the client */
inline string receiveLine(const SOCKET& sock, iostream& stream)
{
	while (true)
	{
		// Attempt to get line from the buffer stream
		string result;

		stream.clear();
		streampos originalPos = stream.tellg();

		getline(stream, result);

		stream.clear();
		streampos newPos = stream.tellg();

		if (newPos != originalPos)
		{
			stream.clear();
			stream.seekg(newPos - 1);
			char last;
			stream.clear();
			stream.get(last);

			if (last == '\n')
			{
				ostringstream message;
				message << "Worker #" << this_thread::get_id() << " receives: "
					<< result << "\n";
				log(message.str());
				return result;
			}
		}
		stream.clear();
		stream.seekg(originalPos);

		// No complete line, receiving...
		const size_t BUFFER_SIZE = 2048;
		int receivedLength;
		char buffer[BUFFER_SIZE];
		if((receivedLength = recv(sock, buffer, BUFFER_SIZE - 1, 0)) ==
			SOCKET_ERROR || receivedLength == 0)
		{
			ostringstream message;
			message << "Failed to receive. Error code: " << WSAGetLastError();
			throw runtime_error(message.str());
		}

		buffer[receivedLength] = '\0';

		stream << buffer;
	}
}

/** Splits the string by the delimiter into a vector */
inline vector<string> split(const string& s, char delim)
{
	vector<string> result(1);

	for (char c: s)
	{
		if (c == delim)
		{
			result.emplace_back();
		}
		else
		{
			result.back() += c;
		}
	}

	return result;
}

/** Send message */
inline void sendMessage(const SOCKET& s, const string& message)
{
	if (send(s, message.c_str(), message.size(), 0) < 0)
	{
		ostringstream message;
		message << "Failed to send message.";
		throw runtime_error(message.str());
	}
}

/** Send online list with balance */
inline void sendList(const SOCKET& sock)
{
	ostringstream message;
	{
		lock(accountsMutex, onlineAccountsMutex, onlineAddressesMutex);
		lock_guard<mutex> lga(accountsMutex, adopt_lock);
		lock_guard<mutex> lgoa(onlineAccountsMutex, adopt_lock);
		lock_guard<mutex> lgop(onlineAddressesMutex, adopt_lock);

		thread::id id = this_thread::get_id();
		message << onlineAccounts[id]->balance << "\n";
		message << onlineAccounts.size() << "\n";
		for (const auto& pair: onlineAccounts)
		{
			thread::id id = pair.first;
			string name = onlineAccounts[id]->name;
			struct sockaddr_in address = onlineAddresses[id];
			message << name << "#" << inet_ntoa(address.sin_addr) << "#" <<
				ntohs(address.sin_port) << "\n";
		}
	}

	sendMessage(sock, message.str());
}

/** Send a code response */
inline void sendCode(const SOCKET& sock, int code)
{
	switch(code)
	{
	case 100:
		sendMessage(sock, "100 OK\n");
		break;
	case 210:
		sendMessage(sock, "210 FAIL\n");
		break;
	case 220:
		sendMessage(sock, "220 AUTH_FAIL\n");
		break;
	case 400:
		sendMessage(sock, "400 Bad Request\n");
		break;
	case 408:
		sendMessage(sock, "408 Request Timeout\n");
		break;
	}
}

/** Worker that handles the clients */
inline void workerFunction()
{
	{
		ostringstream message;
		message << "Worker #" << this_thread::get_id() << " started.\n";
		log(message.str());
	}

	while(true)
	{
		Job job;
		{
			clientQueueSemaphore.take();
			lock_guard<mutex> lg(clientQueueSemaphore.m, adopt_lock);

			job = clientQueue.front();
			clientQueue.pop();
		}

		SOCKET sock = job.sock;
		struct sockaddr_in client = job.address;

		{
			ostringstream message;
			message << inet_ntoa(client.sin_addr) << "::" <<
				ntohs(client.sin_port) << " is assigned to worker #" <<
				this_thread::get_id() << "\n";
			log(message.str());
		}

		stringstream stream;

		try
		{
			while (true)
			{
				bool isLogin;
				{
					lock_guard<mutex> lg(onlineAccountsMutex);
					isLogin = onlineAccounts.count(this_thread::get_id());
				}

				auto parts = split(receiveLine(sock, stream), '#');
				if (parts.size() == 1)
				{
					if (parts[0] == "List" && isLogin)
					{
						sendList(sock);
						continue;
					}
					if (parts[0] == "Exit")
					{
						break;
					}
				}
				if (parts.size() == 2 && !isLogin)
				{
					if (parts[0] == "REGISTER")
					{
						lock_guard<mutex> lg(accountsMutex);
						if (accounts.count(parts[1]))
						{
							sendCode(sock, 210);
							continue;
						}
						sendCode(sock, 100);
						accounts[parts[1]] = Account(parts[1]);
						continue;
					}
					else
					{
						bool isRegistered;
						{
							lock_guard<mutex> lg(accountsMutex);
							isRegistered = accounts.count(parts[0]);
						}

						if (isRegistered)
						{
							int port = atoi(parts[1].c_str());
							if (port < 1024 || port > 65535)
							{
								throw runtime_error("Network error!");
							}
							struct sockaddr_in client = job.address;
							client.sin_port = htons(port);

							thread::id id = this_thread::get_id();
							{
								lock(accountsMutex, onlineAccountsMutex, onlineAddressesMutex);
								lock_guard<mutex> lgop(onlineAddressesMutex, adopt_lock);
								{
									lock_guard<mutex> lga(accountsMutex, adopt_lock);
									lock_guard<mutex> lgoa(onlineAccountsMutex, adopt_lock);

									onlineAccounts[id] = &accounts[parts[0]];
								}
								onlineAddresses[id] = client;
							}
							sendList(sock);
							continue;
						}
						sendCode(sock, 220);
						continue;
					}
				}
				if (parts.size() == 3 && !isLogin)
				{
					if (parts[0] == "REGISTER")
					{
						lock_guard<mutex> lg(accountsMutex);
						if (accounts.count(parts[1]))
						{
							sendCode(sock, 210);
							continue;
						}
						sendCode(sock, 100);
						accounts[parts[1]] = Account(parts[1], atoi(parts[2].c_str()));
						continue;
					}
				}
				throw runtime_error("Network error!");
			}
		}
		catch (const runtime_error& e)
		{
			try
			{
				if (strcmp(e.what(), "Failed to receive. Error code: 10060") == 0)
				{
					ostringstream message;
					message << "Client has exceeded timeout with worker #" <<
						this_thread::get_id() << "\n";
					log(message.str());
					sendCode(sock, 408);
				}
				else
				{
					ostringstream message;
					message << "Network error occured with worker #" <<
						this_thread::get_id() << "\n" << e.what();
					log(message.str());
					sendCode(sock, 400);
				}
			}
			catch (...)
			{
				// empty
			}
		}

		{
			ostringstream message;
			message << "Worker #" << this_thread::get_id() <<
				" has disconnected from " << inet_ntoa(client.sin_addr) << "::"
				<< ntohs(client.sin_port)<< "\n";
			log(message.str());
		}
		try
		{
			sendMessage(sock, "Bye\n");
		}
		catch (...)
		{
			// empty
		}
		closesocket(sock);

		// log out
		{
			lock(onlineAccountsMutex, onlineAddressesMutex);
			lock_guard<mutex> lgop(onlineAddressesMutex, adopt_lock);
			{
				lock_guard<mutex> lgoa(onlineAccountsMutex, adopt_lock);
				onlineAccounts.erase(this_thread::get_id());
			}
			onlineAddresses.erase(this_thread::get_id());
		}
	}
}

/** Accept connections */
inline void acceptConnection()
{
	struct sockaddr_in client;
	int c = sizeof(struct sockaddr_in);
	SOCKET clientSocket = accept(mainSocket, (struct sockaddr *)&client, &c);
	if(clientSocket == INVALID_SOCKET)
	{
		ostringstream message;
		message << "Failed to accept connection. Error code: %d" <<
			WSAGetLastError();
		throw runtime_error(message.str());
	}

	{
		ostringstream message;
		message << "Accepted connection from: " << inet_ntoa(client.sin_addr) <<
			"::" << ntohs(client.sin_port) << "\n";
		log(message.str());
	}

	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeoutPeriod,
		sizeof(int));

	{
		clientQueueSemaphore.put();
		lock_guard<mutex> lg(clientQueueSemaphore.m, adopt_lock);
		clientQueue.emplace(clientSocket, client);
	}
}

int main (int argc, char* argv[])
{
	processArguments(argc, argv);
	initializePort();

	// Initialize thread pool
	ThreadPool workers;
	while (workers.threads.size() < threadPoolSize)
	{
		workers.addThread(workerFunction);
	}

	while (true)
	{
		acceptConnection();
	}

	closesocket(mainSocket);
	WSACleanup();

	return 0;
}
