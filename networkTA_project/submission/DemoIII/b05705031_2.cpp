/**
	© 2017-2018 Tai-Yuan Kuo All Rights Reserved.
*/
#pragma GCC optimize("O3")
#include <bits/stdc++.h>
#include <winsock2.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
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

vector<thread> threads;
bool connected = true;
WSADATA wsa;
SOCKET sock;
struct sockaddr_in server;
mutex console_m;
string serverIP;
string serverPort;
bool showServerMessages = true;
string username;

SOCKET p2pSock;
struct sockaddr_in p2p;
int p2pPort;
mutex p2pSignal;

string verificaitonCode;

// RSA stuff
int padding = RSA_PKCS1_PADDING;
string privateKey;
string publicKey;
string serverKey;

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
	auto parts = split(address, ':');

	struct sockaddr_in result;
	result.sin_addr.s_addr = inet_addr(parts[0].c_str());
	result.sin_port = htons(atoi(parts[1].c_str()));
	return result;
}

/** Error prompt to retry */
template<typename E>
inline void errorPrompt(const E& e)
{
	lock_guard<mutex> lg(console_m);
	cout << e.what() << endl;
	cout << "Press enter to retry>>";
	string temp;
	getline(cin, temp);
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
	if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
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
inline void bindSocket(SOCKET s, struct sockaddr_in address)
{
	// Force at most one bind per port
	int iOptval = 1;
	setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
		(char *) &iOptval, sizeof (iOptval));

	if(bind(s, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR)
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

/** Accept connections */
inline void acceptConnection(SOCKET s, SOCKET& clientSocket, struct sockaddr_in& client)
{
	int c = sizeof(struct sockaddr_in);
	clientSocket = accept(s, (struct sockaddr *)&client, &c);
	if(clientSocket == INVALID_SOCKET)
	{
		ostringstream message;
		message << "Failed to accept connection. Error code: " <<
			WSAGetLastError();
		throw runtime_error(message.str());
	}
}

/** Display server message */
inline void displayServerMessage(const string& message)
{
	if (!showServerMessages)
	{
		return;
	}

	static stringstream displayBuffer;
	displayBuffer.clear();
	for (char c: message)
	{
		switch(c)
		{
		case '\n':
			displayBuffer << "\\n";
			break;
		case '\r':
			displayBuffer << "\\r";
			break;
		case ' ':
			displayBuffer << "\\ ";
			break;
		case '\\':
			displayBuffer << "\\\\";
			break;
		default:
			displayBuffer << c;
		}
	}

	if (console_m.try_lock())
	{
		lock_guard<mutex> lg(console_m, adopt_lock);
		cout << "Server: " << displayBuffer.str() << endl;
		displayBuffer.str(string());
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
		throw runtime_error("Failed to create RSA" + ('0' + pub));
	}

	return rsa;
}

inline RSA* generateRSA()
{
	int ret = 0;
	RSA *r = NULL;
	BIGNUM *bne = NULL;

	int bits = 1024;
	unsigned long e = RSA_F4;

	bne = BN_new();
	ret = BN_set_word(bne,e);
	if(ret != 1){
		goto free_all;
	}

	r = RSA_new();
	ret = RSA_generate_key_ex(r, bits, bne, NULL);
	if(ret != 1){
		goto free_all;
	}

free_all:

	BN_free(bne);
	return r;
}

inline string rsaPublicString(RSA* rsa)
{
	int rc = 0;
	unsigned long err = 0;

	using BIO_MEM_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;

	BIO_MEM_ptr bio(BIO_new(BIO_s_mem()), ::BIO_free);

	rc = PEM_write_bio_RSAPublicKey(bio.get(), rsa);
	err = ERR_get_error();

	if (rc != 1)
	{
		ostringstream message;
		message << "PEM_write_bio_RSAPublicKey failed, error " << err << ", ";
		message << std::hex << "0x" << err << std::dec;
		throw runtime_error(message.str());
	}

	BUF_MEM *mem = NULL;
	BIO_get_mem_ptr(bio.get(), &mem);
	err = ERR_get_error();

	if (!mem || !mem->data || !mem->length)
	{
		ostringstream message;
		message << "BIO_get_mem_ptr failed, error " << err << ", ";
		message << std::hex << "0x" << err << std::dec;
		throw runtime_error(message.str());
	}

	string pem(mem->data, mem->length);

	return pem;
}

inline string rsaPrivateString(RSA* rsa)
{
	int rc = 0;
	unsigned long err = 0;

	using BIO_MEM_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;

	BIO_MEM_ptr bio(BIO_new(BIO_s_mem()), ::BIO_free);

	rc = PEM_write_bio_RSAPrivateKey(bio.get(), rsa, NULL, NULL, 0, NULL, NULL);
	err = ERR_get_error();

	if (rc != 1)
	{
		ostringstream message;
		message << "PEM_write_bio_RSAPrivateKey failed, error " << err << ", ";
		message << std::hex << "0x" << err << std::dec;
		throw runtime_error(message.str());
	}

	BUF_MEM *mem = NULL;
	BIO_get_mem_ptr(bio.get(), &mem);
	err = ERR_get_error();

	if (!mem || !mem->data || !mem->length)
	{
		ostringstream message;
		message << "BIO_get_mem_ptr failed, error " << err << ", ";
		message << std::hex << "0x" << err << std::dec;
		throw runtime_error(message.str());
	}

	string pem(mem->data, mem->length);

	return pem;
}

inline string publicEncrypt(const string& message, const string& k)
{
	unsigned char* data = (unsigned char*) message.c_str();
	unsigned char* key = (unsigned char*) k.c_str();
	unsigned char* encrypted = new unsigned char[k.size()];
	RSA * rsa = createRSA(k, 1);
	int length = RSA_public_encrypt(message.size(),data,encrypted,rsa,padding);

	if (length == -1)
	{
		cout << "error: " << ERR_get_error() << "\n";
	}

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
				displayServerMessage(result);
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
	displayServerMessage(result);
	return result;
}

/** Setup connection */
inline void initializeConnection(int argc, char* argv[], WSADATA& wsa,
	SOCKET& sock,struct sockaddr_in& server,
	iostream& stream, iostream& rstream)
{
	// Initialize Winsock
init:
	try
	{
		wsa = wsaInit();
	}
	catch (const runtime_error& e)
	{
		errorPrompt(e);
		goto init;
	}

	// Create socket
cSock:
	try
	{
		sock = createSocket();
	}
	catch (const runtime_error& e)
	{
		errorPrompt(e);
		goto cSock;
	}

	// Fill server info in
	server.sin_addr.s_addr = inet_addr(serverIP.c_str());
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(serverPort.c_str()));

	// Connect to server
cServer:
	try
	{
		connectToServer(sock, server);
	}
	catch (const runtime_error& e)
	{
		errorPrompt(e);
		goto cServer;
	}
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

/** Exit program */
inline void disconnect(const SOCKET& s)
{
	sendPlainMessage(s, "Exit\n");
}

/** Display menu */
inline void displayOptions(ostream& out, const vector<string>& choices)
{
	for (int i = 0; i < choices.size(); ++i)
	{
		out << (i + 1) << ". " << choices[i] << "\n";
	}
	out << "0. Exit the program" << "\n";
}

/** Empties the whole stream */
inline void emptyStream(istream& in)
{
	in.clear();
	in.seekg(0, ios_base::end);
	if (in.fail())
	{
		in.clear();
	}
}

/** Display prompt message */
inline void prompt(istream& in, ostream& out, const string& p)
{
	emptyStream(in);
	out << p << flush;
}

/** Menu */
inline int menu(istream& in, ostream& out, const vector<string>& choices)
{
	displayOptions(out, choices);
	prompt(in, out, "Enter the number of your choice>>");
	while (true)
	{
		int key;
		if ((in >> key) && key >= 0 && key <= choices.size())
		{
			return key;
		}
		prompt(in, out, "Invalid input! Please try again>>");
	}
}

/** Prompt and get username */
inline string promptUsername(const string& p)
{
	string username;
	{
		lock_guard<mutex> lg(console_m);
		prompt(cin, cout, p);
		getline(cin, username);
	}
	for (char c: username)
	{
		if (!isalnum(c) && c != '_')
		{
			lock_guard<mutex> lg(console_m);
			cout << "Invalid username. A username must be alphanumeric or with the underscore.\n\n" << endl;
			return string();
		}
	}
	return username;
}

/** Prompt for a integer */
inline int promptInt(const string& p, int l, int r)
{
	string line;
	int result;
	{
		lock_guard<mutex> lg(console_m);
		prompt(cin, cout, p);
		getline(cin, line);
	}
	istringstream buffer(line);
	buffer >> result;
	if (!buffer || buffer >> line || result < l || result > r)
	{
		lock_guard<mutex> lg(console_m);
		cout << "Invalid input. Accepted inputs are numbers from " << l <<
			" to " << r << ".\n\n" << endl;
		return l - 1;
	}
	return result;
}

/** Registration menu */
inline void registerMenu(istream& in, ostream& out, const SOCKET& s,
	iostream& stream, iostream& rstream)
{
	string username = promptUsername("Enter your new username>>");
	if (username.empty())
	{
		return;
	}
	int balance = promptInt("Enter the initial balance>>", 0, 2000000000);
	if (balance == -1)
	{
		return;
	}

	stringstream balanceSS;
	balanceSS << balance;

	sendSecureMessage(s, "REGISTER#" + username + "#" + balanceSS.str() + "\n",
		privateKey, serverKey);
	if (connected)
	{
		istringstream serverResponse(receiveLine(
			s, stream, rstream, privateKey, serverKey));
		int code;
		serverResponse >> code;
		lock_guard<mutex> lg(console_m);
		switch(code)
		{
		case 100:
			out << "Registration success! Username: " << username << "\n" <<
				endl;
			break;
		case 210:
			out << "Registration failed!" << "\n" << endl;
			break;
		default:
			throw runtime_error("Network error!");
		}

	}
	else
	{
		throw runtime_error("Network error!");
	}
}

/** Get the balance */
inline long long getBalance(const SOCKET& s, iostream& stream, iostream& rstream)
{
	istringstream lin(receiveLine(
		s, stream, rstream, privateKey, serverKey));
	long long balance;
	lin >> balance;

	string remaining;
	if (lin >> remaining)
	{
		long long& code = balance;
		switch(code)
		{
		case 220:
			throw runtime_error("Authentication failed!");
			break;
		default:
			throw runtime_error("Network error!");
		}
	}

	return balance;
}

/** Fetch and display online list */
inline void fetchList(const SOCKET& s, iostream& stream, iostream& rstream)
{
	int users;
	istringstream lin(receiveLine(
		s, stream, rstream, privateKey, serverKey));
	lin >> users;

	vector<string> usernames;
	vector<string> ips;
	vector<string> ports;

	for (int i = 0; i < users; ++i)
	{
		auto parts = split(receiveLine(
			s, stream, rstream, privateKey, serverKey), '#');
		usernames.emplace_back(parts[0]);
		ips.emplace_back(parts[1]);
		ports.emplace_back(parts[2]);
	}

	string ip_h = "IP";
	string port_h = "Port";
	string username_h = "Username";

	int ip_w = max(ip_h.size(), max_element(ips.begin(), ips.end(),
		[](auto a, auto b){return a.size() < b.size();})->size()) + 1;
	int port_w = max(port_h.size(), max_element(ports.begin(), ports.end(),
		[](auto a, auto b){return a.size() < b.size();})->size()) + 1;
	int username_w = max(username_h.size(), max_element(usernames.begin(),
		usernames.end(),
		[](auto a, auto b){return a.size() < b.size();})->size()) + 1;

	lock_guard<mutex> lg(console_m);
	cout << "Online users:\n";
	ip_h.resize(ip_w, ' ');
	port_h.resize(port_w, ' ');
	username_h.resize(username_w, ' ');
	cout << ip_h << port_h << username_h << "\n";
	for (int i = 0; i < users; ++i)
	{
		ips[i].resize(ip_w, ' ');
		ports[i].resize(port_w, ' ');
		usernames[i].resize(username_w, ' ');
		cout << ips[i] << ports[i] << usernames[i] << "\n";
	}
	cout << endl;
}

/** Process list response and display */
inline void processList(const SOCKET& s, iostream& stream, iostream& rstream)
{
	long long balance = getBalance(s, stream, rstream);

	{
		lock_guard<mutex> lg(console_m);
		cout << "Your balance: " << balance << "\n";
	}

	fetchList(s, stream, rstream);
}

/** Transfer money to peer */
inline void transfer(const SOCKET& s, iostream& stream, iostream& rstream)
{
	int a = promptInt("Enter the first part of IP of the receiver >>", 0, 255);
	if (a == -1)
	{
		return;
	}
	int b = promptInt("Enter the second part of IP of the receiver >>", 0, 255);
	if (b == -1)
	{
		return;
	}
	int c = promptInt("Enter the third part of IP of the receiver >>", 0, 255);
	if (c == -1)
	{
		return;
	}
	int d = promptInt("Enter the fourth part of IP of the receiver >>", 0, 255);
	if (d == -1)
	{
		return;
	}
	int p = promptInt("Enter the port of the receiver >>", 0, 65535);
	if (p == -1)
	{
		return;
	}
	int amount = promptInt("Enter amount >>", 1, 2000000000);
	if (amount == 0)
	{
		return;
	}

	string rAddress;
	{
		stringstream tout;
		tout << a << "." << b << "." << c << "." << d << "#"
			<< p;
		rAddress = tout.str();
	}

	// Tell server who we want to transfer
	{
		stringstream message;
		message << "TRANSFER#" << rAddress << "\n";
		sendSecureMessage(s, message.str(), privateKey, serverKey);
	}

	string line = receiveLine(s, stream, rstream, privateKey, serverKey);
	if (line != "100 OK")
	{
		lock_guard<mutex> lg(console_m);
		cout << "Transfer failed!\n";
		return;
	}

	line = receiveLine(s, stream, rstream, privateKey, serverKey);

	string rKey;

	size_t length;
	{
		istringstream temp(line);
		temp >> length;
	}
	rKey = receiveChars(s, stream, rstream, length, privateKey, serverKey);

	try
	{
		SOCKET rSock = createSocket();
		struct sockaddr_in receiver = addressOf(rAddress);
		receiver.sin_family = AF_INET;
		connectToServer(rSock, receiver);

		string message;
		{
			ostringstream tout;
			tout << username << "#" << amount << "#receiver\n";
			message = tout.str();
		}

		string encrypted = encrypt(message, privateKey, rKey);

		{
			ostringstream tout;
			tout << encrypted.size() << "\n";
			tout << encrypted;
		}

		sendSecureMessage(rSock, encrypted, privateKey, rKey);

		stringstream streamP;
		stringstream rstreamP;
		line = receiveLine(rSock, streamP, rstreamP, privateKey, rKey);
		if (line != "100 OK")
		{
			lock_guard<mutex> lg(console_m);
			cout << "Transfer failed!\n";
			return;
		}

		line = receiveLine(s, stream, rstream, privateKey, serverKey);
		if (line != "100 OK")
		{
			lock_guard<mutex> lg(console_m);
			cout << "Transfer failed!\n";
			return;
		}

		lock_guard<mutex> lg(console_m);
		cout << "Transfer success!\n";
		return;
	}
	catch (const runtime_error& e)
	{
		lock_guard<mutex> lg(console_m);
		cout << e.what() << endl;
		cout << "Transfer failed!\n";
		return;
	}
	catch (...)
	{
		lock_guard<mutex> lg(console_m);
		cout << "Transfer failed!\n";
		return;
	}
}

/** Login menu */
inline void loginMenu(const SOCKET& s, iostream& stream, iostream& rstream)
{
	username = promptUsername("Enter your username>>");
	if (username.empty())
	{
		return;
	}

	stringstream message;
	message << username << "#" << 1024 << "\n";
	sendSecureMessage(s, message.str(), privateKey, serverKey);

	bool displayList = true;

	while (connected)
	{
		try
		{
			if (displayList)
			{
				processList(s, stream, rstream);
				displayList = false;
			}
		}
		catch (const runtime_error& e)
		{
			if (string(e.what()) == string("Authentication failed!"))
			{
				lock_guard<mutex> lg(console_m);
				cout << e.what() << "\n" << endl;
				return;
			}
			else
			{
				throw e;
			}
		}

		int key;
		{
			lock_guard<mutex> lg(console_m);
			cout << "Dashboard:" << endl;
			key = menu(cin, cout, vector<string>
			{
				"Check balance and see a list of online users",
				"Transfer money"
			});
		}

		switch(key)
		{
		case 1:
			sendSecureMessage(s, "List\n", privateKey, serverKey);
			displayList = true;
			break;
		case 2:
			transfer(s, stream, rstream);
			break;
		case 0:
			disconnect(sock);
			connected = false;
			lock_guard<mutex> lg(console_m);
			cout << "Disconnected from the server." << endl;
			return;
		}
	}
}

/** Main menu */
inline void mainMenu(istream& in, ostream& out, const SOCKET& s,
	iostream& stream, iostream& rstream)
{
	{
		lock_guard<mutex> lg(console_m);
		out << "Welcome!\n" << endl;
	}
	while (connected)
	{
		int key;
		{
			lock_guard<mutex> lg(console_m);
			out << "Login:" << endl;
			key = menu(in, out, vector<string>
			{
				"Register",
				"Login"
			});
		}

		switch(key)
		{
		case 1:
			registerMenu(in, out, s, stream, rstream);
			break;
		case 2:
			loginMenu(s, stream, rstream);
			break;
		case 0:
			return;
		}
	}
}

/** Process run options */
inline void processArguments(int argc, char* argv[])
{
	serverIP = "127.0.0.1";
	serverPort = "31415";
	showServerMessages = true;
	if (argc >= 3)
	{
		serverIP = argv[1];
		serverPort = argv[2];
	}
	if (argc >= 4)
	{
		showServerMessages = false;
	}
}

/** Generate RSA keys for this process */
inline void initializeRSA()
{
	//return;
	RSA* rsa = generateRSA();
	privateKey = rsaPrivateString(rsa);
	publicKey = rsaPublicString(rsa);
	//cout << privateKey;
	//cout << publicKey;
	RSA_free(rsa);
}

/** Exchange keys with the server */
inline void exchangeKeys(const SOCKET& s, iostream& stream, iostream& rstream)
{
	// send key
	{
		ostringstream message;
		message << "KEY#" << publicKey.size() << "\n";
		message << publicKey;
		sendPlainMessage(s, message.str());
	}

	// receive key
	string line = receiveLine(s, stream, rstream);
	auto parts = split(line, '#');
	if (parts[0] != "KEY")
	{
		throw runtime_error("Network error!");
	}

	size_t length;
	{
		istringstream temp(parts[1]);
		temp >> length;
	}
	serverKey = receiveChars(s, stream, rstream, length);
}

/** Request a verification code from the server */
inline void requestCode(const SOCKET& s, iostream& stream, iostream& rstream)
{
	// Request code
	sendSecureMessage(s, "REQUEST_CODE\n", privateKey, serverKey);

	// Receive code
	string line = receiveLine(s, stream, rstream, privateKey, serverKey);
	auto parts = split(line, '#');
	if (parts[0] != "VERIFICATION_CODE")
	{
		throw runtime_error("Network error!");
	}

	verificaitonCode = parts[1];
}

/** The p2p server function */
inline void p2pServer()
{
	try
	{
		p2p = server;

		// Read a number from the memory indeterministically for an indeterministic
		// port number
		{
			int temp;
			p2pPort = temp;
		}

		// Create socket
	cSock:
		try
		{
			p2pSock = createSocket();
		}
		catch (const runtime_error& e)
		{
			errorPrompt(e);
			goto cSock;
		}

		// Fill server info in
		p2p.sin_family = AF_INET;
		p2p.sin_addr.s_addr = INADDR_ANY;

		// Bind the Socket
		int highestPort = 65536;
		int lowestPort = 1024;
		int range = highestPort - lowestPort;
	bSocket:
		try
		{
			p2pPort -= lowestPort;
			p2pPort %= range;
			p2pPort += (p2pPort < 0) * range;
			p2pPort += lowestPort;
			p2p.sin_port = htons(p2pPort);
			bindSocket(p2pSock, p2p);
		}
		catch (const runtime_error& e)
		{
			p2pPort += (range / 2) - 1;
			goto cSock;
		}

		// Connect to server
	cServer:
		try
		{
			connectToServer(p2pSock, server);
		}
		catch (const runtime_error& e)
		{
			errorPrompt(e);
			goto cServer;
		}

		// Verify
		stringstream stream;
		stringstream rstream;
		sendPlainMessage(p2pSock, "VERIFY#" + verificaitonCode + "\n");

		// Restart socket as a server socket
		//disconnect(p2pSock);
		closesocket(p2pSock);
		p2pSock = createSocket();
		try
		{
			p2pPort -= lowestPort;
			p2pPort %= range;
			p2pPort += (p2pPort < 0) * range;
			p2pPort += lowestPort;
			p2p.sin_port = htons(p2pPort);
			bindSocket(p2pSock, p2p);
		}
		catch (const runtime_error& e)
		{
			p2pPort += (range / 2) - 1;
			goto cSock;
		}

		// Start listening at the port
	lPort:
		try
		{
			listenSocket(p2pSock);
		}
		catch (const runtime_error& e)
		{
			this_thread::yield();
			goto lPort;
		}

		SOCKET clientSock;
		struct sockaddr_in client;
		acceptConnection(p2pSock, clientSock, client);

		string message = receiveLine(clientSock, stream, rstream, privateKey, serverKey);
		if (message != "100 OK")
		{
			throw runtime_error("Network error!");
		}
		closesocket(clientSock);

		p2pSignal.unlock();

		while (connected)
		{
			bool withServer = false;
			SOCKET toServerSock;
			struct sockaddr_in toServer;
			bool withPeer = false;
			SOCKET toPeerSock;
			struct sockaddr_in toPeer;

			try
			{
				while (!withServer || !withPeer)
				{
					acceptConnection(p2pSock, clientSock, client);
					string incomingIP = inet_ntoa(client.sin_addr);
					string incomingPort;
					{
						stringstream tin;
						tin << ntohs(client.sin_port);
						tin >> incomingPort;
					}
					if (incomingIP == serverIP && incomingPort == serverPort)
					{
						withServer = true;
						toServerSock = clientSock;
						toServer = client;
					}
					else
					{
						withPeer = true;
						toPeerSock = clientSock;
						toPeer = client;
					}
				}

				string peerKey;
				{
					stringstream stream;
					stringstream rstream;

					// Get peer key
					size_t length;
					{
						string line = receiveLine(toServerSock, stream, rstream);
						istringstream tin(line);
						tin >> length;
					}
					peerKey = receiveChars(toServerSock, stream, rstream, length);
				}

				string message = receiveLine(toPeerSock, stream, rstream,
					privateKey, peerKey);
				auto parts = split(message, '#');
				if (parts[0] != "TRANSACTION")
				{
					throw runtime_error("Network error!");
				}
				size_t length;
				{
					istringstream tin(parts[1]);
					tin >> length;
				}
				string encrypted = receiveChars(toPeerSock, stream, rstream, length,
					privateKey, peerKey);
				string decrypted = decrypt(encrypted, privateKey, peerKey);

				parts = split(decrypted, '#');
				int amount;
				{
					istringstream tin(parts[1]);
					tin >> amount;
				}
				if (parts[0] == username || amount <= 0)
				{
					sendPlainMessage(toPeerSock, "210 FAIL\n");
					sendPlainMessage(toServerSock, "210 FAIL\n");
					throw runtime_error("Network error!");
				}

				// Prepare to send to server
				string privateDecrypted;
				for (size_t i = 0; i < encrypted.size(); i += 128)
				{
					string chunk(encrypted, i, 128);
					privateDecrypted += privateDecrypt(chunk, privateKey);
				}

				string privateEncrypted;
				for (size_t i = 0; i < privateDecrypted.size(); i += 117)
				{
					string chunk(privateDecrypted, i, 117);
					privateEncrypted += privateEncrypt(chunk, privateKey);
				}

				{
					stringstream message;
					message << "TRANSACTION#" << privateEncrypted.size() <<
						"\n";
					message << privateEncrypted;
					sendSecureMessage(toServerSock,message.str(),
						privateKey, serverKey);
				}
			}
			catch (const runtime_error& e)
			{
				// empty
			}

			if (withServer)
			{
				closesocket(toServerSock);
			}
			if (withPeer)
			{
				closesocket(toPeerSock);
			}
		}
	}
	catch (const runtime_error& e)
	{
		lock_guard<mutex> lg(console_m);
		cout << e.what();
	}
}

int main(int argc, char* argv[])
{
	processArguments(argc, argv);
	// Initialize openssl random seed
	initializeRSA();
	RAND_poll();
	stringstream stream;
	stringstream rawStream;
	initializeConnection(argc, argv, wsa, sock, server, stream, rawStream);

	try
	{
		exchangeKeys(sock, stream, rawStream);
		requestCode(sock, stream, rawStream);
		p2pSignal.lock();
		threads.emplace_back(p2pServer);
		p2pSignal.lock();
		mainMenu(cin, cout, sock, stream, rawStream);
	}
	catch (const runtime_error& e)
	{
		lock_guard<mutex> lg(console_m);
		cout << e.what() << endl;
	}
	catch (...)
	{
		lock_guard<mutex> lg(console_m);
		cout << "Network error!" << endl;
	}

	if (connected)
	{
		disconnect(sock);
		connected = false;
		lock_guard<mutex> lg(console_m);
		cout << "Disconnected from the server." << endl;
	}

	// Clean up the socket and the winsock
	closesocket(sock);
	WSACleanup();

	for (auto& t: threads)
	{
		t.join();
	}

	prompt(cin, cout, "Goodbye! Press enter to exit>>");
	string temp;
	getline(cin, temp);

	return 0;
}
