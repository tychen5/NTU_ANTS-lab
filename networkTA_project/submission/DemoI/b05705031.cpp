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

vector<thread> threads;
bool connected = true;
WSADATA wsa;
SOCKET sock;
struct sockaddr_in server;
stringstream stream;
mutex stream_m;
mutex console_m;
string serverIP;
string serverPort;
bool showServerMessages = true;

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

/** Display server message */
inline void displayServerMessage(const string& message)
{
	if (!showServerMessages)
	{
		return;
	}

	static stringstream displayBuffer;
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

/** Store all messages from server to stream */
inline void receiveStream(const SOCKET& s, ostream& out, mutex& m)
{
	try
	{
		while (true)
		{
			const size_t BUFFER_SIZE = 4;
			int receivedLength;
			char buffer[BUFFER_SIZE];
			if((receivedLength = recv(s, buffer, BUFFER_SIZE - 1, 0)) == SOCKET_ERROR ||
				receivedLength == 0)
			{
				ostringstream message;
				message << "Failed to receive. Error code: " << WSAGetLastError();
				throw runtime_error(message.str());
			}
			buffer[receivedLength] = '\0';

			displayServerMessage(buffer);
			lock_guard<mutex> streamLock(m);
			out.clear();
			out << buffer;
		}
	}
	catch (...)
	{
		connected = false;
		return;
	}
}

/** Get a line from the stream */
inline string getStreamLine(istream& in, mutex& m)
{
	while (connected)
	{
		{
			lock_guard<mutex> lg(m);
			string result;

			in.clear();
			streampos originalPos = in.tellg();

			getline(in, result);

			streampos newPos = in.tellg();

			if (newPos != originalPos)
			{
				in.seekg(newPos - 1);
				char last;
				in.get(last);

				if (last == '\n')
				{
					return result;
				}
			}
			in.clear();
			in.seekg(originalPos);
		}
		this_thread::yield();
	}

	throw runtime_error("Network error!");
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

/** Empties the whole stream */
inline void emptyStream(istream& in, mutex& m)
{
	lock_guard<mutex> lg(m);
	emptyStream(in);
}

/** Setup connection */
inline void initializeConnection(int argc, char* argv[], WSADATA& wsa,
	SOCKET& sock,struct sockaddr_in& server, stringstream& stream,
	mutex& stream_m)
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

	// Start receiving stream
	threads.emplace_back(receiveStream, sock, ref(stream), ref(stream_m));
}

/** Send message to server */
inline void sendToServer(const SOCKET& s, const string& message)
{
	if (send(s, message.c_str(), message.size(), 0) < 0)
	{
		ostringstream message;
		message << "Failed to send message.";
		throw runtime_error(message.str());
	}
}

/** Exit program */
inline void disconnect(const SOCKET& s)
{
	sendToServer(s, "Exit\n");
	connected = false;
	lock_guard<mutex> lg(console_m);
	cout << "Disconnected from the server." << endl;
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

/** Display prompt message */
inline void prompt(istream& in, ostream& out, const string& p)
{
	emptyStream(in);
	out << p << flush;
}

/** Prompt server */
inline void promptServer(const string& message)
{
	emptyStream(stream, stream_m);
	sendToServer(sock, message);
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

/** Registration menu */
inline void registerMenu(istream& in, ostream& out, const SOCKET& s, iostream& buffer,
	mutex& m)
{
	string username = promptUsername("Enter your new username>>");
	if (username.empty())
	{
		return;
	}

	promptServer("REGISTER#" + username + "\n");
	if (connected)
	{
		istringstream serverResponse(getStreamLine(buffer, m));
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

/** Get the balance */
inline int getBalance()
{
	istringstream lin(getStreamLine(stream, stream_m));
	int balance;
	lin >> balance;

	string remaining;
	if (lin >> remaining)
	{
		int& code = balance;
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

/** Fetch and display online list */
inline void fetchList()
{
	int users;
	istringstream lin(getStreamLine(stream, stream_m));
	lin >> users;

	vector<string> usernames;
	vector<string> ips;
	vector<string> ports;

	for (int i = 0; i < users; ++i)
	{
		auto parts = split(getStreamLine(stream, stream_m), '#');
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
inline void processList()
{
	int balance = getBalance();

	{
		lock_guard<mutex> lg(console_m);
		cout << "Your balance: " << balance << "\n";
	}

	fetchList();
}

/** Login menu */
inline void loginMenu()
{
	string username = promptUsername("Enter your username>>");
	if (username.empty())
	{
		return;
	}

	int port = promptInt("Enter port number>>", 1024, 65535);
	if (port == 1024 - 1)
	{
		return;
	}

	stringstream message;
	message << username << "#" << port << "\n";
	promptServer(message.str());

	while (connected)
	{
		try
		{
			processList();
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
				"Check balance and see a list of online users"
			});
		}

		switch(key)
		{
		case 1:
			promptServer("List\n");
			break;
		case 0:
			disconnect(sock);
			return;
		}
	}


}

/** Main menu */
inline void mainMenu(istream& in, ostream& out, const SOCKET& s, iostream& buffer,
	mutex& m)
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
			registerMenu(in, out, s, buffer, m);
			break;
		case 2:
			loginMenu();
			break;
		case 0:
			return;
		}
	}
}

/** Process run options */
inline void processArguments(int argc, char* argv[])
{
	serverIP = "140.112.106.45";
	serverPort = "33220";
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

int main(int argc, char* argv[])
{
	processArguments(argc, argv);
	initializeConnection(argc, argv, wsa, sock, server, stream, stream_m);

	try
	{
		mainMenu(cin, cout, sock, stream, stream_m);
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
