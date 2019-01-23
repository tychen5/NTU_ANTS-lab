#ifndef SERVER_H
#define SERVER_H

#include <cassert>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <map>
#include <mutex>
#include <condition_variable>
#include "common.h"

enum ParseCommand {
    CMD_REGISTER = 1,
    CMD_LOGIN,
    CMD_LIST,
    CMD_FIND_IP,
    CMD_UPDATE_BALANCE,
    CMD_EXIT,
    CMD_LOST,

    CMD_ERROR
};

struct ClientInfo {
    std::string          username;
    std::string          ipAddress;
    int                  balance;
    bool                 login;
    int                  port;

    ClientInfo(): port(-1) {}
    ClientInfo(std::string u, int b): username(u), ipAddress(""), balance(b), login(false), port(-1) {}
};

struct SharedDataPool {
    std::queue<int>                       waitingClientFds;
    std::map<std::string, ClientInfo>     clientInfoMap;
    std::mutex                            lock;
    std::condition_variable               hasNewClient;
    int                                   idleThread;

    SharedDataPool(): idleThread(0) {}
    bool noClient() { return waitingClientFds.empty(); }
    bool enqueueClient(int);
    int getClient();
    bool addClientInfo(const ClientInfo&);
    bool login(const std::string&, const std::string&, int);
    void getOnlineInfo(const std::string&, std::string& response);
    std::string getReceiverIpPort(const std::string& recvName);
    bool updateBalance(const std::string& sender, const std::string& receiver, int amount);
    void logout(const std::string&);
};

bool SharedDataPool::enqueueClient(int clientfd) {
    std::unique_lock<std::mutex> lk(lock);
    // critical section
    if (idleThread == 0) return false;

    waitingClientFds.push(clientfd);
    lk.unlock();
    hasNewClient.notify_one();

    return true;
}

int SharedDataPool::getClient() {
    std::unique_lock<std::mutex> lk(lock);
    idleThread++;
    // if the queue is empty, go to sleep
    hasNewClient.wait(lk, [this]{ return !waitingClientFds.empty(); });
    // critical section
    idleThread--;
    int fd = waitingClientFds.front();
    waitingClientFds.pop();
    lk.unlock();
    return fd;
}

bool SharedDataPool::addClientInfo(const ClientInfo& newClient) {
    std::unique_lock<std::mutex> lk(lock);
    if (clientInfoMap.find(newClient.username) != clientInfoMap.end()) {
        lk.unlock(); return false;
    }
    clientInfoMap[newClient.username] = newClient;
    lk.unlock();
    return true;
}

bool SharedDataPool::login(const std::string& username, const std::string& ip, int portnum) {
    std::unique_lock<std::mutex> lk(lock);
    auto it = clientInfoMap.find(username);
    if (it == clientInfoMap.end() || it->second.login) {
        lk.unlock(); return false;
    }
    it->second.login = true;
    it->second.ipAddress = ip;
    it->second.port = portnum;
    lk.unlock();
    return true;
}

void SharedDataPool::getOnlineInfo(const std::string& username, std::string& response) {
    std::unique_lock<std::mutex> lk(lock);
    int numOfOnline = 0;
    std::string tmp = "";
    for (auto it = clientInfoMap.begin(); it != clientInfoMap.end(); ++it) {
        if (it->second.username == username) {
            response += std::to_string(it->second.balance);
            response += "\n";
        }
        if (it->second.login) {
            numOfOnline++;
            tmp += it->second.username; tmp += "#";
            tmp += it->second.ipAddress; tmp += "#";
            tmp += std::to_string(it->second.port); tmp += "\n";
        }

    }
    lk.unlock();
    response += std::to_string(numOfOnline); response += "\n";
    response += tmp;
}

void SharedDataPool::logout(const std::string& username) {
    std::unique_lock<std::mutex> lk(lock);
    auto it = clientInfoMap.find(username);
    assert(it != clientInfoMap.end());
    it->second.login = false;
    it->second.ipAddress = "";
    it->second.port = -1;

    lk.unlock();
}

std::string SharedDataPool::getReceiverIpPort(const std::string& recvName) {
    std::unique_lock<std::mutex> lk(lock);
    auto it = clientInfoMap.find(recvName);
    if (it == clientInfoMap.end() || !it->second.login) return "";
    std::string response = it->second.ipAddress;
    response += "#"; response += std::to_string(it->second.port);
    return response;
}

bool SharedDataPool::updateBalance(const std::string& sender, const std::string& receiver, int amount) {
    std::unique_lock<std::mutex> lk(lock);
    auto senderIt = clientInfoMap.find(sender);
    auto receiverIt = clientInfoMap.find(receiver);
    assert(senderIt != clientInfoMap.end() && receiverIt != clientInfoMap.end());
    if (senderIt->second.balance < amount) return false;
    senderIt->second.balance -= amount;
    receiverIt->second.balance += amount;
    return true;
}

#endif
