#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "server.h"
#include "common.h"

using namespace std;

static int PORTNUM = 12340;
static bool debug = false;

ParseCommand parseClientCommand(int clientfd, vector<string>& args) {
	char buf[MESSAGE_LEN] = {0};
	if (decrypted_recv(clientfd, buf, sizeof(buf), 0) == 0) return CMD_LOST;

	ParseCommand ret = CMD_ERROR;
	string cmd = buf;
	cmd.pop_back();   // delete newline character
	size_t d1 = cmd.find('#');
	if (d1 == string::npos) {
		if (cmd == "List") ret = CMD_LIST;
		else if (cmd == "Exit") ret = CMD_EXIT;
		else {
			cerr << "Unexpected protocol: " << cmd << endl;
			ret = CMD_ERROR;
		}
	}
	else {
		size_t d2 = cmd.find('#', d1+1);
		if (d2 != string::npos) {										  // <myAccount>#<amount>#<receiverAccount>
			args.push_back(cmd.substr(0, d1));
			args.push_back(cmd.substr(d1+1, d2-d1-1));
			args.push_back(cmd.substr(d2+1));
			ret = CMD_UPDATE_BALANCE;
		}
		else if (cmd.substr(0, d1) == "FIND") {
			args.push_back(cmd.substr(d1+1));
			ret = CMD_FIND_IP;
		}
		else if (cmd.substr(0, d1) == "REGISTER") {   // REGISTER#<UserAccountName>
			args.push_back(cmd.substr(d1+1));
			ret = CMD_REGISTER;
		}
		else {   // <UserAccountName>#<portNum>
			args.push_back(cmd.substr(0, d1));
			args.push_back(cmd.substr(d1+1));
			ret = CMD_LOGIN;
		}
	}

	return ret;
}

void clientRegister(int clientfd, const vector<string>& args, SharedDataPool& dataPool) {
	// REGISTER#<UserAccountName>
	assert(args.size() == 1);
	// have to receive initial balance
	char buf[INT_STR_LEN] = {0};
	if (decrypted_recv(clientfd, buf, sizeof(buf), 0) == 0) {
		if (debug) cerr << "Client socket: " << clientfd << " closed while entering initial balance." << endl;
		return;
	}

	int balance = 0;
	try { balance = stoi(buf); }
	catch (const exception& e) {
		if (debug) cerr << "Client socket: " << clientfd << " provided incorrect initial balance." << endl;
		return;
	}

	ClientInfo newClient(args[0], balance);
	string response;
	if (!dataPool.addClientInfo(newClient)) {
		// 210 FAIL
		if (debug) cerr << "Client socket: " << clientfd << " register fail." << endl;
		response = "210 FAIL\n";
	}
	else {
		// 100 OK
		if (debug) {
			cerr << "Client socket: " << clientfd << " register succeed.\n"
			     << "Account name: " << args[0] << ", initial balance: " << balance << endl;
		}
		response = "100 OK\n";
	}
	encrypted_send(clientfd, response.c_str(), strlen(response.c_str()), 0);
}

void clientLogin(int clientfd, const vector<string>& args, string& curClientName,
                 SharedDataPool& dataPool) 
{
	assert(args.size() == 2);
	if (curClientName != "") {
		if (debug) cerr << "Client socket: " << clientfd << " already logged in." << endl;
		return;
	}
	// <UserAccountName>#<portNum>
	string username = args[0];
	int port = 0;
	try { port = stoi(args[1]); }
	catch (const exception& e) {
		if (debug) cerr << "Client socket: " << clientfd << " provided incorrect port." << endl;
		return;
	}

	// get ip address
	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	getpeername(clientfd, (struct sockaddr*)&addr, &addr_size);
	string ip = inet_ntoa(addr.sin_addr);

	string response;
	if (!dataPool.login(username, ip, port)) {
		if (debug) cerr << "Client socket: " << clientfd << " login fail." << endl;
		response = "220 AUTH_FAIL\n";
	}
	else {
		curClientName = username;
		if (debug) cerr << "Client socket: " << clientfd << " (username: " << curClientName
										<< ") login succeeed. From " << ip << endl;
		dataPool.getOnlineInfo(username, response);
	}
	encrypted_send(clientfd, response.c_str(), strlen(response.c_str()), 0);
}

void sendList(int clientfd, const string& curClientName, SharedDataPool& dataPool) {
	if (curClientName == "") {
		if (debug) cerr << "Client socket: " << clientfd << " hasn't logged in hence cannot get list.\n";
		return;
	}
	string response;
	dataPool.getOnlineInfo(curClientName, response);
	encrypted_send(clientfd, response.c_str(), strlen(response.c_str()), 0);
}

void clientExit(int clientfd, const string& curClientName, SharedDataPool& dataPool, bool lost = false) {
	if (curClientName != "") dataPool.logout(curClientName);
	if (debug) cerr << "Client socket: " << clientfd << " (username: " << curClientName << ") logged out\n";
	if (!lost) {
		const char* buf = "Bye\n";
		encrypted_send(clientfd, buf, strlen(buf), 0);
	}
}

void findReceiverIpPort(int clientfd, const vector<string>& args, SharedDataPool& dataPool) {
	assert(args.size() == 1);
	string receiver = args[0];
	if (debug) cerr << "Finding Ip and Port of payment receiver: " << receiver << endl;
	string response = dataPool.getReceiverIpPort(receiver);
	if (debug) cerr << "Finding response: " << response << endl;
	encrypted_send(clientfd, response.c_str(), strlen(response.c_str()), 0);
}

void updateBalance(int clientfd, const vector<string>& args, SharedDataPool& dataPool) {
	assert(args.size() == 3);
	if (debug) cerr << "Sender name: " << args[0] << ", receiver name: " << args[2] << ", amount: " << args[1] << endl;
	if (!dataPool.updateBalance(args[0], args[2], stoi(args[1]))) {
		if (debug) cerr << "Sender: " << args[0] << " balance not enough. Update aborted.\n";
		encrypted_send(clientfd, "NAK\n", 4, 0);
	}
	else {
		if (debug) cerr << "Update committed.\n";
		encrypted_send(clientfd, "ACK\n", 4, 0);
	}
}

void handleClient(int id, SharedDataPool& dataPool) {
	while (true) {
		int clientfd = dataPool.getClient();
		if (debug) cout << "Thread #" << id << " is now serving client socket: " << clientfd << endl;

		string curClientName = "";
		while (true) {
			vector<string> args;
			ParseCommand curcmd = parseClientCommand(clientfd, args);
			
			bool disconnected = false;
			switch (curcmd) {
				case CMD_REGISTER:
					if (debug) cout << "Receive command REGISTER from client: " << clientfd << endl;
					clientRegister(clientfd, args, dataPool);
					break;
				case CMD_LOGIN:
					if (debug) cout << "Receive command LOGIN from client: " << clientfd << endl;
					clientLogin(clientfd, args, curClientName, dataPool);
					break;
				case CMD_LIST:
					if (debug) cout << "Receive command LIST from client: " << clientfd << endl;
					sendList(clientfd, curClientName, dataPool);
					break;
				case CMD_FIND_IP:
					if (debug) cout << "Receive command FIND_IP from client: " << clientfd << endl;
					findReceiverIpPort(clientfd, args, dataPool);
					break;
				case CMD_UPDATE_BALANCE:
					if (debug) cout << "Receive command UPDATE_BALANCE from client: " << clientfd << endl;
					updateBalance(clientfd, args, dataPool);
					break;
				case CMD_EXIT:
					if (debug) cout << "Receive command EXIT from client: " << clientfd << endl;
					clientExit(clientfd, curClientName, dataPool);
					disconnected = true;
					break;
				case CMD_LOST:
					if (debug) cout << "Connection lost from client: " << clientfd << endl;
					clientExit(clientfd, curClientName, dataPool, true);
					disconnected = true;
				  break;
				case CMD_ERROR:
					break;
			}

			if (disconnected) break;
		}
	}
}

int main(int argc, char** argv) {
	int maxThread = 8;

	if (argc == 1 || argc > 4) {
		cerr << "Usage: ./server <int portnum> [int maxThread] [-v]" << endl; exit(-1);
	}
	try { PORTNUM = stoi(argv[1]); }
	catch(const exception& e) {
		cerr << "Usage: ./server <int portnum> [int maxThread] [-v]" << endl; exit(-1);
	}
	if (PORTNUM <= 1024 || PORTNUM > 65535) {
		cerr << "Error: Port number should between 1024 and 65535" << endl; exit(-1);
	}
	if (argc >= 3) {
		try {
			maxThread = stoi(argv[2]);
			if (maxThread <= 0) {
				cerr << "Error: Max thread must > 0.\n"; exit(-1);
			}
		}
		catch(const exception& e) {
			cerr << "Usage: ./server <int portnum> [int maxThread] [-v]" << endl; exit(-1);
		}
	}
	if (argc == 4) {
		if (strcmp(argv[3], "-v") == 0) debug = true;
		else { cerr << "Usage: ./server <int portnum> [int maxThread] [-v]" << endl; exit(-1); }
	}

	SharedDataPool dataPool;
	/* Create thread pool */
	vector<thread> workerThreads;
	for (int i = 0; i < maxThread; ++i) {
		workerThreads.push_back(thread(handleClient, i, ref(dataPool)));
	}

	int sockfd = 0, forClientSockfd = 0;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		cerr << "Server: Failed to create socket\n"; 
	}
	struct sockaddr_in serverInfo, clientInfo;
	socklen_t addrlen = sizeof(clientInfo);
	memset(&serverInfo, 0, sizeof(serverInfo));
	serverInfo.sin_family = AF_INET;
	serverInfo.sin_addr.s_addr = INADDR_ANY;
	serverInfo.sin_port = htons(PORTNUM);
	bind(sockfd, (struct sockaddr* )&serverInfo, sizeof(serverInfo));
	listen(sockfd, maxThread);
	cout << "Server starts listening on port: " << PORTNUM << endl;
	
	string ackMsg = "";
	while (true) {
		forClientSockfd = accept(sockfd, (struct sockaddr* )&clientInfo, &addrlen);

		if (debug) {
			char cbuf[INET_ADDRSTRLEN];
			const char* addr = inet_ntop(AF_INET, &(clientInfo.sin_addr), cbuf, INET_ADDRSTRLEN);
			cout << "Receive client connection from address: " << addr << ", port: " 
				   << clientInfo.sin_port << endl;
		}
		
		/* Activate a worker thread, and make it handle forClientSockfd */
		if (!dataPool.enqueueClient(forClientSockfd)) {
			ackMsg = "The server is busy now, please try again later.\n";
		}
		else {
			ackMsg = "ACK\n";
		}
		// send(forClientSockfd, ackMsg.c_str(), strlen(ackMsg.c_str()), 0);
		encrypted_send(forClientSockfd, ackMsg.c_str(), strlen(ackMsg.c_str()), 0);
	}

	return 0;
}
