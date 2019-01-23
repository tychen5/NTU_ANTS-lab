/**
	© 2017-2018 Tai-Yuan Kuo All Rights Reserved.
*/
#pragma GCC optimize("O3")
#include <bits/stdc++.h>
#include <winsock2.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

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

unordered_map<thread::id,string> onlineKeys;
mutex onlineKeysMutex;

unordered_map<thread::id,string> mainAddresses;
mutex mainAddressesMutex;

unordered_map<thread::id,string> onlineAddresses;
mutex onlineAddressesMutex;

unordered_map<string,thread::id> addressIDs;
mutex addressIDsMutex;

unordered_map<string,string> transferringTo;
unordered_map<string,string> transferringFrom;
mutex transferringMutex;

unordered_map<thread::id,string> threadCode;
mutex threadCodeMutex;

unordered_map<string,thread::id> codeThread;
mutex codeThreadMutex;

int timeoutPeriod; // in milli seconds

default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());

// RSA stuff
int padding = RSA_PKCS1_PADDING;
string privateKey = \
"-----BEGIN RSA PRIVATE KEY-----\n"\
"MIICWwIBAAKBgQC5ugEz1+9XOY7F6f3M6OzIZcnHtu/pYa2QRy/agEVoGhX8TxMb\n"\
"Mwyxe1UrAXO4et5gBeDFGe0HimtqUYwXHIHXhTsG7IRO251kYQFXUZY6vlDQdpYZ\n"\
"PDhKdBMf1K4bsXOb9kgu0DTVjsYd8GZXBN8HFqFV55jXJqjx00RAEVB2FQIDAQAB\n"\
"AoGACk9O/cfA24CwckAY/KT5b+5mkxWOn8/ySI4LFAAG2k6IZecl0l61F60W/zon\n"\
"aFg5u+7XmVllFQQUDmTUd/v3IR1EaXTUOc0JmJu/C+ow2M7tMtfZKCpMZ2uh0wwB\n"\
"RrVJS0nsbbCugPXsDFEMTmnbjae/sScNiYmNdOWvgpCjvYkCQQDy/+HRHyOF3tAn\n"\
"urFFedlZ9IMtUMev7g3dEVr1u59GKXh9tG7htUh6pcmsohlcZ9q7EWNTXgpH4RVS\n"\
"gBPb6iFjAkEAw6m2ipl6nxxVpGaepFWXB1VIAVq1pm1MxaTVxXN9yXE5DwtnMH13\n"\
"F/71MLkpGVayh2CYvJKUE6C0dBJDj3MgJwJAVpEHrksMiZ1VxEGC84A0CRLNRHB5\n"\
"otgIgk+zesUrOYB+lzGXKrs9Jcw361MX+85XorrQCpv+x5qM0QYljPt8hwJABlXW\n"\
"bUJu7/vw4fPYqyWCUGB4hmKzgwIC/FtL+Kq2pfEekdgirTVCx+ofckZsiD+AZFXC\n"\
"XEi6tq/7Z863lUt6/QJAU7lgvpvrOfx6UhwtrvyiqXsO99K+LA7utuurX0mZyQj9\n"\
"eorTxzLJK1D0jlbN6XK8QHFm/UcJ32luvJQfkTAlfQ==\n"\
"-----END RSA PRIVATE KEY-----\n"\
;
string publicKey = \
"-----BEGIN RSA PUBLIC KEY-----\n"\
"MIGJAoGBALm6ATPX71c5jsXp/czo7Mhlyce27+lhrZBHL9qARWgaFfxPExszDLF7\n"\
"VSsBc7h63mAF4MUZ7QeKa2pRjBccgdeFOwbshE7bnWRhAVdRljq+UNB2lhk8OEp0\n"\
"Ex/Urhuxc5v2SC7QNNWOxh3wZlcE3wcWoVXnmNcmqPHTREARUHYVAgMBAAE=\n"\
"-----END RSA PUBLIC KEY-----\n"\
;

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

/** Convert to string representation of struct sockaddr_in (with #) */
inline string addressString(struct sockaddr_in address)
{
	ostringstream out;
	out << inet_ntoa(address.sin_addr) << "#" <<
		ntohs(address.sin_port);
	return out.str();
}

/** Convert from string representation to struct sockaddr_in (with #) */
inline struct sockaddr_in addressOf(const string& address)
{
	auto parts = split(address, '#');

	struct sockaddr_in result;
	result.sin_addr.s_addr = inet_addr(parts[0].c_str());
	result.sin_port = htons(atoi(parts[1].c_str()));
	return result;
}

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

/** Connect to server */
inline void connectToServer(const SOCKET& s, const struct sockaddr_in& server)
{
	int code;
	if ((code = connect(s , (struct sockaddr *)&server , sizeof(server))) < 0)
	{
		ostringstream message;
		message << "Failed to connect. Error Code: " << code;
		throw runtime_error(message.str());
	}
}

/** Bind socket */
inline void bindSocket(SOCKET s)
{
	if(bind(s, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		ostringstream message;
		message << "Bind failed with error code: " << WSAGetLastError();
		throw runtime_error(message.str());
	}
}

/** Listen to socket */
inline void listenSocket(SOCKET s)
{
	if(listen(s, SOMAXCONN) == SOCKET_ERROR)
	{
		ostringstream message;
		message << "Listen failed with error code: " << WSAGetLastError();
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
		cout << "\rRetrying in " << i << " second(s)...";
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
inline void log(const string& message)
{
	if (displayLog)
	{
		lock_guard<mutex> lg(consoleMutex);
		cout << message;
	}
}

inline RSA* createRSA(const string& k,int pub)
{
	unsigned char* key = (unsigned char*) k.c_str();

	RSA *rsa= NULL;
	BIO *keybio ;
	keybio = BIO_new_mem_buf(key, -1);
	if (keybio == NULL)
	{
		throw runtime_error("Failed to create key BIO");
		return 0;
	}
	if(pub)
	{
		rsa = PEM_read_bio_RSAPublicKey(keybio, &rsa, NULL, NULL);
	}
	else
	{
		rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
	}

	if(rsa == NULL)
	{
		throw runtime_error("Failed to create RSA " + ('0' + pub));
	}

	return rsa;
}

inline string publicEncrypt(const string& message, const string& k)
{
	unsigned char* data = (unsigned char*) message.c_str();
	unsigned char* key = (unsigned char*) k.c_str();
	unsigned char* encrypted = new unsigned char[k.size()];
	RSA * rsa = createRSA(k, 1);
	int length = RSA_public_encrypt(message.size(),data,encrypted,rsa,padding);

	string result((char*)encrypted, length);
	delete encrypted;
	RSA_free(rsa);
	return result;
}

inline string privateDecrypt(const string& message, const string& k)
{
	unsigned char* enc_data = (unsigned char*) message.c_str();
	unsigned char* decrypted = new unsigned char[k.size()];
	RSA * rsa = createRSA(k, 0);
	int length = RSA_private_decrypt(message.size(),enc_data,decrypted,rsa,padding);

	string result((char*)decrypted, length);
	delete decrypted;
	RSA_free(rsa);
	return result;
}

inline string privateEncrypt(const string& message, const string& k)
{
	unsigned char* data = (unsigned char*) message.c_str();
	unsigned char* encrypted = new unsigned char[k.size()];
	RSA * rsa = createRSA(k, 0);
	int length = RSA_private_encrypt(message.size(),data,encrypted,rsa,padding);

	string result((char*)encrypted, length);
	delete encrypted;
	RSA_free(rsa);
	return result;
}

inline string publicDecrypt(const string& message, const string& k)
{
	unsigned char* enc_data = (unsigned char*) message.c_str();
	unsigned char* decrypted = new unsigned char[k.size()];
	RSA * rsa = createRSA(k, 1);
	int length = RSA_public_decrypt(message.size(),enc_data,decrypted,rsa,padding);

	string result((char*)decrypted, length);
	delete decrypted;
	RSA_free(rsa);
	return result;
}

/** Encrypt PKI message */
inline string encrypt(const string& message, const string& privkey0,
	const string& pubkey1)
{
	string privateEncrypted;

	for (size_t i = 0; i < message.size(); i += 117)
	{
		string chunk(message, i, 117);
		privateEncrypted += privateEncrypt(chunk, privkey0);
	}

	string result;

	for (size_t i = 0; i < privateEncrypted.size(); i += 117)
	{
		string chunk(privateEncrypted, i, 117);
		result += publicEncrypt(chunk, pubkey1);
	}

	return result;
}

/** Decrypt PKI message */
inline string decrypt(const string& message, const string& privkey0,
	const string& pubkey1)
{
	string privateDecrypted;

	for (size_t i = 0; i < message.size(); i += 128)
	{
		string chunk(message, i, 128);
		privateDecrypted += privateDecrypt(chunk, privkey0);
	}


	string result;

	for (size_t i = 0; i < privateDecrypted.size(); i += 128)
	{
		string chunk(privateDecrypted, i, 128);
		result += publicDecrypt(chunk, pubkey1);
	}

	cout << result << endl;

	return result;
}

/** Receive into buffer */
inline void receiveToBuffer(const SOCKET& sock, iostream& stream)
{
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

	stream << string(buffer, receivedLength);
}

/** Receive a line from the client */
inline string receiveRawLine(const SOCKET& sock, iostream& stream)
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
				return result;
			}
		}
		stream.clear();
		stream.seekg(originalPos);

		// No complete line, receiving...
		receiveToBuffer(sock, stream);
	}
}

/** Receive a specified number of raw characters from the client */
inline string receiveRawChars(const SOCKET& sock, iostream& stream, size_t length)
{
	string result;
	while (length)
	{
		char c;
		stream.clear();
		stream.get(c);

		stream.clear();
		size_t got = stream.gcount();

		length -= got;
		if (got)
		{
			result += c;
			continue;
		}
		if (length)
		{
			receiveToBuffer(sock, stream);
		}
	}
	return result;
}

/** Receive the raw message */
inline string receiveRaw(const SOCKET& sock, iostream& stream)
{
	size_t length;
	istringstream line(receiveRawLine(sock, stream));
	line >> length;

	return receiveRawChars(sock, stream, length);
}

/** Convert raw to plain stream */
inline void convertBunch(const SOCKET& sock, iostream& stream, iostream& rstream,
	const string& priv0 = "", const string& pub1 = "")
{
	string type = receiveRawLine(sock, rstream);
	if (type != "ENCRYPTED" && type != "PLAIN")
	{
		throw runtime_error("Network error!");
	}

	string message = receiveRaw(sock, rstream);
	if (type[0] == 'E')
	{
		if (!priv0.size() || !pub1.size())
		{
			throw runtime_error("Missing keys");
		}
		message = decrypt(message, priv0, pub1);
	}
	stream << message;
}

/** Receive a line of plain text from the client */
inline string receiveLine(const SOCKET& sock, iostream& stream, iostream& rstream,
	const string& priv0 = "", const string& pub1 = "")
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
		convertBunch(sock, stream, rstream, priv0, pub1);
	}
}

/** Receive a specified number of characters from the client */
inline string receiveChars(const SOCKET& sock, iostream& stream,
	iostream& rstream, size_t length,
	const string& priv0 = "", const string& pub1 = "")
{
	string result;
	while (length)
	{
		char c;
		stream.clear();
		stream.get(c);

		stream.clear();
		size_t got = stream.gcount();

		length -= got;
		if (got)
		{
			result += c;
			continue;
		}
		if (length)
		{
			convertBunch(sock, stream, rstream, priv0, pub1);
		}
	}
	ostringstream message;
	message << "Worker #" << this_thread::get_id() << " receives: "
		<< result << "\n";
	log(message.str());

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

/** Send encrypted message */
inline void sendSecureMessage(const SOCKET& s, const string& message,
	const string& priv0, const string& pub1)
{
	string encrypted = encrypt(message, priv0, pub1);

	ostringstream mout;
	mout << "ENCRYPTED\n";
	mout << encrypted.size() << "\n";
	mout << encrypted;

	sendMessage(s, mout.str());
}

/** Send plain message */
inline void sendPlainMessage(const SOCKET& s, const string& message)
{
	ostringstream mout;
	mout << "PLAIN\n";
	mout << message.size() << "\n";
	mout << message;

	sendMessage(s, mout.str());
}

/** Send online list with balance */
inline void sendList(const SOCKET& sock,
	const string& priv0, const string& pub1)
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
			string address = onlineAddresses[id];
			message << name << "#" << address << "\n";
		}
	}

	sendSecureMessage(sock, message.str(), priv0, pub1);
}

/** Send a code response */
inline void sendCode(const SOCKET& sock, int code)
{
	switch(code)
	{
	case 100:
		sendPlainMessage(sock, "100 OK\n");
		break;
	case 210:
		sendPlainMessage(sock, "210 FAIL\n");
		break;
	case 220:
		sendPlainMessage(sock, "220 AUTH_FAIL\n");
		break;
	case 400:
		sendPlainMessage(sock, "400 Bad Request\n");
		break;
	case 408:
		sendPlainMessage(sock, "408 Request Timeout\n");
		break;
	}
}

/** Send a encrypted code response */
inline void sendSecureCode(const SOCKET& sock, int code,
	const string& priv0, const string& pub1)
{
	switch(code)
	{
	case 100:
		sendSecureMessage(sock, "100 OK\n", priv0, pub1);
		break;
	case 210:
		sendSecureMessage(sock, "210 FAIL\n", priv0, pub1);
		break;
	case 220:
		sendSecureMessage(sock, "220 AUTH_FAIL\n", priv0, pub1);
		break;
	case 400:
		sendSecureMessage(sock, "400 Bad Request\n", priv0, pub1);
		break;
	case 408:
		sendSecureMessage(sock, "408 Request Timeout\n", priv0, pub1);
		break;
	}
}

/** Worker that handles the clients */
inline void workerFunction()
{
	thread::id id = this_thread::get_id();
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
			message << inet_ntoa(client.sin_addr) << ":" <<
				ntohs(client.sin_port) << " is assigned to worker #" <<
				this_thread::get_id() << "\n";
			log(message.str());
		}

		string clientAddress;
		{
			lock_guard<mutex> lg(mainAddressesMutex);
			mainAddresses[id] = addressString(client);
			clientAddress = mainAddresses[id];
		}

		stringstream stream;
		stringstream rstream;

		try
		{
			while (true)
			{
				string clientKey;
				{
					lock_guard<mutex> lg(onlineKeysMutex);
					clientKey = onlineKeys[id];
				}

				bool isLogin;
				{
					lock_guard<mutex> lg(onlineAccountsMutex);
					isLogin = onlineAccounts.count(this_thread::get_id());
				}

				vector<string> parts;
				{
					string s = receiveLine(sock, stream, rstream, privateKey,
						clientKey);
					parts = split(s, '#');
				}
				if (parts.size() == 1)
				{
					if (parts[0] == "List" && isLogin)
					{
						sendList(sock, privateKey, clientKey);
						continue;
					}
					if (parts[0] == "Exit")
					{
						break;
					}
					if (parts[0] == "REQUEST_CODE")
					{
						uniform_int_distribution<char> distribution('A', 'Z');
					gCode:
						string code;
						for (size_t i = 0; i < 32; ++i)
						{
							code += distribution(generator);
						}
						{
							lock_guard<mutex> lg(codeThreadMutex);
							if (codeThread.count(code))
							{
								goto gCode;
							}
							codeThread[code] = id;
						}
						{
							lock_guard<mutex> lg(threadCodeMutex);
							threadCode[id] = code;
						}

						sendSecureMessage(sock,
							"VERIFICATION_CODE#" + code + "\n",
							privateKey, clientKey);
						continue;
					}
				}
				if (parts.size() == 2)
				{
					if (parts[0] == "KEY")
					{
						// receive key
						size_t length;
						{
							istringstream temp(parts[1]);
							temp >> length;
						}
						onlineKeys[this_thread::get_id()] = receiveChars(sock, stream, rstream, length);

						// send key
						{
							ostringstream message;
							message << "KEY#" << publicKey.size() << "\n";
							message << publicKey;
							sendPlainMessage(sock, message.str());
						}

						continue;
					}
					if (parts[0] == "VERIFY")
					{
						string code = parts[1];
						thread::id mainID;
						{
							lock_guard<mutex> lg(codeThreadMutex);
							if (!codeThread.count(code))
							{
								sendCode(sock, 210);
								continue;
							}
							mainID = codeThread[code];
						}
						{
							lock_guard<mutex> lg(onlineKeysMutex);
							clientKey = onlineKeys[mainID];
						}

						SOCKET sock = createSocket();
						setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeoutPeriod,
							sizeof(int));
						try
						{
							connectToServer(sock, client);
						}
						catch(...)
						{
							sendCode(sock, 210);
							continue;
						}

						sendSecureCode(sock, 100, privateKey, clientKey);

						{
							lock(mainAddressesMutex, onlineAddressesMutex, addressIDsMutex);
							lock_guard<mutex> lgma(mainAddressesMutex, adopt_lock);
							lock_guard<mutex> lgaid(addressIDsMutex, adopt_lock);
							{
								lock_guard<mutex> lgoa(onlineAddressesMutex, adopt_lock);
								onlineAddresses[mainID] = mainAddresses[id];
							}
							addressIDs[mainAddresses[id]] = mainID;
						}

						closesocket(sock);

						continue;
					}
					if (!isLogin)
					{
						if (parts[0] == "REGISTER")
						{
							lock_guard<mutex> lg(accountsMutex);
							if (accounts.count(parts[1]))
							{
								sendSecureCode(sock, 210, privateKey, onlineKeys[this_thread::get_id()]);
								continue;
							}
							sendSecureCode(sock, 100, privateKey, onlineKeys[this_thread::get_id()]);
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

								thread::id id = this_thread::get_id();
								{
									lock(accountsMutex, onlineAccountsMutex);
									lock_guard<mutex> lga(accountsMutex, adopt_lock);
									lock_guard<mutex> lgoa(onlineAccountsMutex, adopt_lock);

									onlineAccounts[id] = &accounts[parts[0]];
								}
								sendList(sock, privateKey, onlineKeys[this_thread::get_id()]);
								continue;
							}
							sendSecureCode(sock, 220, privateKey, onlineKeys[this_thread::get_id()]);
							continue;
						}
					}
				}
				if (parts.size() == 3)
				{
					if (parts[0] == "REGISTER" && !isLogin)
					{
						lock_guard<mutex> lg(accountsMutex);
						if (accounts.count(parts[1]))
						{
							sendSecureCode(sock, 210, privateKey, onlineKeys[this_thread::get_id()]);
							continue;
						}
						sendSecureCode(sock, 100, privateKey, onlineKeys[this_thread::get_id()]);
						accounts[parts[1]] = Account(parts[1], atoi(parts[2].c_str()));
						continue;
					}
					if (parts[0] == "TRANSFER" && isLogin)
					{
						string& sAddress = clientAddress;
						string rAddress = parts[1] + "#" + parts[2];
						thread::id rID;
						{
							lock_guard<mutex> lg(addressIDsMutex);
							if (!addressIDs.count(rAddress))
							{
								sendCode(sock, 210);
								continue;
							}
							rID = addressIDs[rAddress];
						}

						{
							lock_guard<mutex> lg(transferringMutex);
							if (transferringFrom.count(rAddress))
							{
								sendCode(sock, 210);
								continue;
							}
							transferringTo[clientAddress] = rAddress;
							transferringFrom[rAddress] = clientAddress;
						}

						string& sKey = clientKey;
						string rKey;
						{
							lock_guard<mutex> lg(onlineKeysMutex);
							rKey = onlineKeys[rID];
						}

						// Connect to receiver and send pkey
						SOCKET rSock = createSocket();
						struct sockaddr_in receiver = addressOf(rAddress);
						receiver.sin_family = AF_INET;
						connectToServer(rSock, receiver);
						{
							stringstream message;
							message << sKey.size() << "\n";
							message << sKey;
							sendSecureMessage(rSock, message.str(),
								privateKey, rKey);
						}

						// Send receiver pkey
						sendSecureCode(sock, 100, privateKey, clientKey);
						{
							stringstream message;
							message << rKey.size() << "\n";
							message << rKey;
							sendSecureMessage(sock, message.str(),
								privateKey, sKey);
						}

						// Receive transaction from the receiver
						stringstream stream;
						stringstream rstream;
						string line = receiveLine(rSock, stream, rstream, privateKey, rKey);
						auto parts = split(line, '#');
						if (parts[0] != "TRANSACTION")
						{
							sendCode(sock, 210);
							sendCode(rSock, 210);
							closesocket(rSock);
							continue;
						}

						size_t length;
						{
							stringstream tin(parts[1]);
							tin >> length;
						}

						string encrypted = receiveChars(rSock, stream, rstream,
							length, privateKey, rKey);

						string rDecrypted;
						for (size_t i = 0; i < encrypted.size(); i += 128)
						{
							string chunk(encrypted, i, 128);
							rDecrypted += publicDecrypt(chunk, rKey);
						}

						string sDecrypted;
						for (size_t i = 0; i < rDecrypted.size(); i += 128)
						{
							string chunk(rDecrypted, i, 128);
							sDecrypted += publicDecrypt(chunk, sKey);
						}

						string& transaction = sDecrypted;
						parts = split(transaction, '#');

						int amount;
						{
							stringstream tin(parts[1]);
							tin >> amount;
						}

						if (amount <= 0)
						{
							sendCode(sock, 210);
							sendCode(rSock, 210);
							closesocket(rSock);
							continue;
						}

						{
							Account* sAccount;
							Account* rAccount;
							lock(accountsMutex, onlineAccountsMutex);
							lock_guard<mutex> lg(accountsMutex, adopt_lock);
							{
								lock_guard<mutex> lg(onlineAccountsMutex, adopt_lock);
								sAccount = onlineAccounts[id];
								rAccount = onlineAccounts[rID];
							}
							if (sAccount->name != parts[0] ||
								sAccount->balance < amount)
							{
								sendCode(sock, 210);
								sendCode(rSock, 210);
								closesocket(rSock);
								continue;
							}

							sAccount->balance -= amount;
							rAccount->balance += amount;
						}

						sendSecureCode(sock, 100, privateKey, sKey);
						sendSecureCode(rSock, 100, privateKey, rKey);

						closesocket(rSock);
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
						this_thread::get_id() << "\n" << e.what() << "\n";
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
			sendPlainMessage(sock, "Bye\n");
		}
		catch (...)
		{
			// empty
		}
		shutdown(sock, SD_SEND);
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
		message << "Failed to accept connection. Error code: " <<
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
