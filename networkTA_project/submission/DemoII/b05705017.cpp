#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <vector>
#include <sys/select.h>
#include <map>
#include <list>

using namespace std;

#define threadNum 2
#define buffer_size 1024

struct user{
    string ip;
    string name;
    string portNum;
    string balance;
    bool isOnline;
};

pthread_mutex_t locker;
pthread_mutex_t Select;

struct sockaddr_in server, client;
fd_set fd;
fd_set active;
list<int> queue;
vector<user> userList;
map<int, string> userSocket;

void* worker(void* data){
    while(true){
        pthread_mutex_lock(&locker);
        if(queue.size() > 0){
            int sock = queue.front();
            if(sock >= 0){
                queue.pop_front();
                pthread_mutex_unlock(&locker);
                char buffer[buffer_size];
                memset(buffer, 0, buffer_size);
                string nowUser;
                if(recv(sock, buffer, buffer_size, 0) == 0){ //ctrl-c
                    pthread_mutex_lock(&locker);
                    nowUser = userSocket[sock];
                    pthread_mutex_lock(&locker);
                    for(auto& i : userList){
                        if(nowUser == i.name)
                            i.isOnline = 0;
                    }
                    FD_CLR(sock, &fd);
                    close(sock);
                }
                else{
                    string message(buffer);
                    bool b = false;
                    if(strncmp(buffer, "REGISTER", 8) == 0){
                        bool isNew = true;
                        size_t begin = message.find("#");
                        message.erase(0, begin+1);
                        begin = message.find("#");
                        string newUser = message.substr(0, begin);
                        cerr << newUser << endl;
                        for(auto& i : userList){
                            if(i.name == newUser){
                                string sends = "210 FAIL\nName already been used. Please Register Again.\n";
                                send(sock, sends.c_str(), sends.length(), 0);
                                isNew = false;
                                break;
                            }
                        }
                        if(isNew){
                            user u;
                            pthread_mutex_lock(&locker);
                            u.name = newUser;
                            u.balance = message.substr(begin+1);
                            u.isOnline = 0;
                            userList.push_back(u);
                            pthread_mutex_unlock(&locker);
                            string sends = "100 OK\nNow you can login.\n";
                            send(sock, sends.c_str(), sends.length(), 0);  
                        }
                    }
                    else if(message.find_first_of("#") != string::npos){
                        cerr << "@" << endl;
                        size_t begin = message.find("#");
                        string nowUser = message.substr(0, begin);
                        string portNumber = message.substr(begin+1);
                        cerr << portNumber << endl;
                        cerr << nowUser << endl;
                        string IP(inet_ntoa(client.sin_addr));
                        bool isUsed;
                        for(auto& i : userList){
                            if(i.portNum == portNumber && (i.isOnline == 1)){
                                isUsed = true;
                                break;
                            }
                        }
                        for(auto& i : userList){
                            cerr << i.name << endl;
                            if(i.name == nowUser && !isUsed && (i.isOnline == 0)){
                                i.ip = IP;
                                i.portNum = portNumber;
                                i.isOnline = 1;
                                pthread_mutex_lock(&locker);
                                userSocket[sock] = nowUser;
                                pthread_mutex_unlock(&locker);
                                b = true;
                            }
                        }
                        string sends;
                        if(b){
                            for(auto& i : userList){
                                if(nowUser == i.name)
                                    sends = "Account balance : " + i.balance + "\n";
                            }
                            int isOnline = 0;
                            for(auto& i : userList){
                                if(i.isOnline){
                                    sends += i.name + "#" + i.ip + "#" + i.portNum + "\n";
                                    isOnline++;
                                }        
                            }
                            sends += "online user : " + to_string(isOnline) + "\n";
                            send(sock, sends.c_str(), sends.length(), 0);
                        }
                        else{
                            sends.clear();
                            sends = "220 AUTH_FAIL";
                            send(sock, sends.c_str(), sends.length(), 0);
                        }
                    }
                    else if(strncmp(buffer, "list", 4) == 0){
                        char buffer2[buffer_size];
                        memset(buffer2, 0, buffer_size);
                        string message(buffer);
                        string list = "";
                        pthread_mutex_lock(&locker);
                        nowUser = userSocket[sock];
                        pthread_mutex_unlock(&locker);
                        for(auto& i : userList){
                            if(nowUser == i.name)
                                list = "Account balance : " + i.balance + "\n";

                        }
                        int isOnline = 0;
                        for(auto& i : userList){
                            if(i.isOnline){
                                list += i.name + "#" + i.ip + "#" + i.portNum + "\n";
                                isOnline++;  
                            }   
                        }
                        list += "online user :" + to_string(isOnline) + "\n";
                        send(sock, list.c_str(), list.length(), 0);
                    }
                    else if(strncmp(buffer, "exit", 4) == 0){
                        string send1 = "Goodbye";
                        send(sock, send1.c_str(), send1.length(), 0);
                        pthread_mutex_lock(&locker);
                        nowUser = userSocket[sock];
                        pthread_mutex_unlock(&locker);
                        for(auto& i : userList){
                            if(nowUser == i.name)
                                i.isOnline = 0;
                        }
                        cerr << nowUser << " is leave."<< endl;
                        pthread_mutex_lock(&locker);
                        userSocket.erase(sock);
                        pthread_mutex_unlock(&locker);
                        FD_CLR(sock, &fd);
                    }
                }      
            }
            else{
                queue.pop_front();
                pthread_mutex_unlock(&Select);
                pthread_mutex_unlock(&locker);
            }
        }
        else{
            pthread_mutex_unlock(&locker);
        }
    }
}

int main(int argc, char const* argv[]){
    if(argc != 2)
        cout << "error argument\n";
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        cout << "socket create failed\n";
    
    memset((char*)&server, 0, sizeof(server));
    cerr << "!" << endl;
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[1]));
    server.sin_addr.s_addr = INADDR_ANY;
    if(bind(sock, (struct sockaddr*)& server, sizeof(server)) < 0)
        cout << "bind failed\n";
    listen(sock, 20);
    socklen_t addrLens = sizeof(client);

    pthread_mutex_init(&locker, NULL);
    pthread_mutex_init(&Select, NULL);
    pthread_t p_threads[threadNum];
    for (size_t i = 0; i < threadNum; ++i)
		pthread_create(&p_threads[i], NULL, &worker, NULL);
    FD_ZERO(&fd);
    FD_SET(sock, &fd);
    FD_ZERO(&active);
    active = fd;
    int maxSocket = sock;
    cerr << sock << endl;
    while(true){
        pthread_mutex_lock(&Select);
        select(maxSocket+1, &active, NULL, NULL, NULL);
        for(int i = 0; i <= maxSocket; ++i){
            if(FD_ISSET(i, &active)){
                int newfd = 0;
                if(i == sock){
                    if((newfd = accept(sock, (struct sockaddr*)& client, &addrLens))){
                        if(newfd < 0)
                            cout << "Can't accept connection.\n";
                        else{
                            string s = "successful connected.";
                            send(newfd, s.c_str(), s.length(), 0);
                            FD_SET(newfd, &fd);   
                        }
                    }
                }
                else{
                    pthread_mutex_lock(&locker);
                    queue.push_back(i);
                    cerr << queue.back() << endl;
                    sleep(1);
                    pthread_mutex_unlock(&locker);
                }
                if(newfd >= maxSocket) { maxSocket = newfd; }
            }
        }
        active = fd;
        pthread_mutex_lock(&locker);
        queue.push_back(-1);
        cerr << queue.back() << endl;
        pthread_mutex_unlock(&locker);
    }
    close(sock);
    return 0;
}