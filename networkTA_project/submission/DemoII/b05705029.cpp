#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include <pthread.h>
#include <queue>

using namespace std;

#define PORT 1000
#define BACKLOG 5
#define BLEN 1000    // buffer length
#define NUM_THREADS 10    // number of pthread
#define LISTSIZE 1024

struct sockaddr_in serv_addr, cli_addr;    // server and client address

string s_msg;    // send message
string r_msg;    // receive message
int sd, pos;
fd_set fdset, readfds;
queue<int> todo;
pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

string list[LISTSIZE];    //register list
string balance[LISTSIZE];    //online account balance
string online[LISTSIZE];    //online list
int regiCount = 0;    //num of register
int onlineCount = 0;    //num of online
bool login = 0;    //check login
bool check = 0;    //check register

string trans(int i)
{
    stringstream ss;
    ss << i;
    return ss.str();
}

void registerOption()
{
    cout << "register\n";
       
    r_msg = r_msg.substr(9);    //name@balance
    pos = r_msg.find("@");
    string regiName = r_msg.substr(0, pos);

    check = 0;
    for(int i=1; i<LISTSIZE; i++)
    {
        pos = list[i].find("@");
        if(list[i].substr(0, pos) == regiName)    // already register
            check = 1;
    }

    // check the register whether been registered
    if(check == 0)    // new register
    {
        regiCount++;    // number of register add
        list[regiCount] = r_msg;    // add this register into register list 
        s_msg = "100 OK\n";    // return successful signal
        cout << regiName << " new rigister\n";          
    }
    else    // have already been registered
    {
        s_msg = "210 FAIL\n";
        cout << "fail register\n";
    }
}

void listOption(int cur)
{
    cout << "list\n";
    s_msg = "AccountBalance:" + balance[cur] + "\n";
    string c = trans(onlineCount);
    s_msg = s_msg + "Number of users:" + c + "\n";
    for(int i=1; i<LISTSIZE; i++)
        s_msg = s_msg + online[i];
}

void exitOption(int cur)
{
    pos = online[cur].find("#");
    string name = online[cur].substr(0, pos);
    cout << name << " Exit\n";
    s_msg = "Bye\n";
    onlineCount--;
    online[cur].clear();
    FD_CLR(cur, &fdset);    // delete
}

void loginOption(int cur, char buf[])
{
    cout << "login\n";
    pos = r_msg.find("#");
    string name = r_msg.substr(0, pos);
    string port = r_msg.substr(pos+1);
    for(int i=1; i<LISTSIZE; i++)
    {
        pos = list[i].find("@");
        if(list[i].substr(0, pos) == name)
        {
            login = 1;
            memset((char *)buf, 0, BLEN);
            inet_ntop(AF_INET, &cli_addr.sin_addr, buf, INET_ADDRSTRLEN);    // transfer IP address
            string IP = buf;
            online[cur] = name + "#" + IP + "#" + port + "\n";
            balance[cur] = list[i].substr(pos+1);
            onlineCount++;
        }
    }
    if(login == 1)    // login successful
    {
        s_msg = "AccountBalance:" + balance[cur] + "\n";
        cout << name << " new login\n";
    }
    else    // havn't been register
    {
        s_msg = "220 AUTH_FAIL\n";
        cout << "fail login\n";
    }
}

void communicate(int cur)
{
    login = 0;
    char buf[BLEN];
    memset(&buf,'\0', sizeof(buf));
    recv(cur,&buf,sizeof(buf),0);
    r_msg = buf;
    
    //register#NAME@BALANCE
    if(r_msg.find("REGISTER#") != string::npos)
    {
        registerOption();
    }

    //list
    else if (r_msg.find("List") != string::npos)
    {
        listOption(cur);
    }

    //exit
    else if (r_msg.find("Exit") != string::npos)
    {
        exitOption(cur);
    }

    //login
    else if(r_msg.find("#") != string::npos)
    {
        loginOption(cur, buf);
    }
    else
    {
        cout << r_msg << "\n";
    }
}

void* request(void* arg)
{
    //cout << "request/n";
    while(true)
    {
        pthread_mutex_lock(&mutex1);
        int temp = -100;
        if(!todo.empty())
        {
            temp = todo.front();
            
            //cout << "temp" << temp;
            todo.pop();
        }
        pthread_mutex_unlock(&mutex1);

        if(temp > 0)    // have something to communicate
        {
            communicate(temp);
            send(temp, s_msg.c_str(), s_msg.size(), 0);
        }
        //pthread lock
        else if(temp == -1)
        {
            pthread_mutex_unlock(&mutex2);
        }
    }
}

void accept(int sockfd)
{
    int new_fd;
    int max_fd = sockfd;
    FD_ZERO(&fdset);    // initial 
    FD_SET(sockfd, &fdset);
    while(true)
    {
        FD_ZERO(&readfds);
        //cout<<"loop\n";
        readfds = fdset;
        pthread_mutex_lock(&mutex2);
        if(select(max_fd+1, &readfds, NULL, NULL, 0) > 0)
        {
            for(int i=0; i<=max_fd; i++)
            {
                //cout<<i<<endl;
                if(FD_ISSET(i, &readfds))    // exist or not
                {
                    if(i == sockfd)
                    {
                        socklen_t len = sizeof(cli_addr);
                        new_fd = accept(sockfd, (struct sockaddr*)&cli_addr, &len);
                        string str = "sucessful connection\n";
                        
                        send(new_fd, str.c_str(), str.size(), 0);
                        FD_SET(new_fd, &fdset);    // add
                        if(new_fd > max_fd)
                            max_fd = new_fd;
                        //cout<<"accept "<<"\n";
                    }
                    else if(i != sockfd)
                    {
                        pthread_mutex_lock(&mutex1);
                        todo.push(i);
                        //cout<<" push " << i <<"\n";
                        pthread_mutex_unlock(&mutex1);
                        
                    }
                }
            }
        }
        pthread_mutex_lock(&mutex1);
        todo.push(-1);
        //cout<<" push " << -1 <<"\n";
        pthread_mutex_unlock(&mutex1);
        //pthread_mutex_unlock(&mutex2);
    }
}

int main(int argc, char *argv[])
{
    
    //wrong input
    if (argc < 2)
    {
        cout<<"Error input\n";
        exit(0);
    }
    
    //thread
    pthread_t threadID[NUM_THREADS];
    for(int i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threadID[i], NULL, request, NULL);
    }
    pthread_mutex_init(&mutex1, NULL);
    pthread_mutex_init(&mutex2, NULL);
    
    //cout << "addr\n";
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));
    
    //cout << "socket\n";
    int sockfd;
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        cout<<"Error creating socket\n";
        exit(0);
    }
    //cout << "socket" << sockfd;
    //cout << "bind\n";
    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cout<<"Error binding\n";
        exit(0);
    }
    
    //cout << "listen\n";
    listen(sockfd, BACKLOG);

    //cout << "accept\n";
    accept(sockfd);

}
