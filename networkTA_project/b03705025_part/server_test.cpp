#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <pthread.h>
#include <sstream>
#include <stdlib.h>
using namespace std;
static int clientfd;
void *task(void *);
static int onlineNum = 0;
struct person_account {
    bool isPerson;
    string userAccount;
    string userIP;
    string userPort;
    struct person_account *next;
};
struct person_account *head = new struct person_account;
struct person_account *tail = head;

int main()
{
    int sd;
    int threadNum = 0;
    const int maxThreadNum = 20;
    struct sockaddr_in dest,client_addr;
    
    //beginning -> no one online
    head->isPerson = false;
    //create socket
    sd = socket(PF_INET, SOCK_STREAM, 0);
    //setting up
    bzero(&dest, sizeof(dest));
    dest.sin_family = PF_INET;
    dest.sin_port = htons(8889);
    dest.sin_addr.s_addr = INADDR_ANY;
    
    //bind and listen
    bind(sd, (struct sockaddr*)&dest, sizeof(dest));
    listen(sd, maxThreadNum);
    cout << "I'm listening.\n";
    
    pthread_t thread[maxThreadNum];
    
    //how many user can be online at the same time (20)
    while(threadNum < maxThreadNum)
    {
        int addrlen = sizeof(client_addr);
        clientfd = accept(sd, (struct sockaddr*)&client_addr, (socklen_t*) &addrlen);
        pthread_create(&thread[threadNum],NULL,task,(void *) inet_ntoa(client_addr.sin_addr));
        threadNum++;
    }
    for(int i = 0; i < maxThreadNum; i++)
        pthread_join(thread[i], NULL);
    //end server
    close(sd);
    return 0;
}
void *task(void *IP){
    string thisIP((char *) IP);
    cout << "Thread No: " << pthread_self() << endl;
    char rcvbuf[1024];
    int thisClient = clientfd;
    
    
    while(1){      // connection success
        string greet = "Please Register or Login\n";
        send(thisClient, greet.c_str(), greet.length(), 0);
        
        bzero(rcvbuf, 1024);
        recv(thisClient, rcvbuf, sizeof(rcvbuf), 0);
        fstream file;
        string tmp(rcvbuf);
        string fileName;
        string hisMoney;
        int stopIndex = tmp.find("#",0);
        string prefix;
        //Case "Register"
        if(strncmp(rcvbuf, "Register", 8) == 0){
            //username input
            greet = "Username?\n";
            send(thisClient, greet.c_str(), greet.length(), 0);
            bzero(rcvbuf, 1024);
            recv(thisClient, rcvbuf, sizeof(rcvbuf), 0);
            string tmp(rcvbuf);
            string user;
            user.assign(tmp,0,tmp.length());
            cout << user << " has registered.\n";
            //deposit input
            greet = "How much money would you like to deposit?\n";
            send(thisClient, greet.c_str(), greet.length(), 0);
            bzero(rcvbuf, 1024);
            recv(thisClient, rcvbuf, sizeof(rcvbuf), 0);
            string money_tmp(rcvbuf);
            string money;
            money.assign(money_tmp,0,money_tmp.length());
            cout << money << " has been deposited.\n";
            
            bool isExist = false;
            file.open("register.txt",ios::out);
            if(!file){
                cerr << "Can't open register.txt\n";
                exit(1);
            }
            // check whether it has been register before (duplicate register)
            while(file >> fileName >> hisMoney){
                if(fileName == user){
                    string fail = "210 FAIL\n";
                    send(thisClient, fail.c_str(), fail.length(), 0);
                    isExist = true;
                    break;
                }
            }
            file.close();
            if(isExist == false){
                file.open("register.txt",ios::out | ios::app);
                if(!file){
                    cerr << "Can't write register.txt\n";
                    exit(1);
                }
                string succ = "100 OK\n";
                file << user << " " << money << endl;
                send(thisClient, succ.c_str(), succ.length(), 0);
                file.close();
            }
            cout<<"Register ends\n";
        }
        
        //Case "Login"
        else if(strncmp(rcvbuf, "Login", 5) == 0){
            bool isRegister = false;
            //Username input
            greet = "Username?\n";
            send(thisClient, greet.c_str(), greet.length(), 0);
            bzero(rcvbuf, 1024);
            recv(thisClient, rcvbuf, sizeof(rcvbuf), 0);
            string username_tmp(rcvbuf);
            string prefix;
            prefix.assign(username_tmp,0,tmp.length());
            cout << prefix << " has logged in.\n";
            //Port No input
            greet = "Port No?\n";
            send(thisClient, greet.c_str(), greet.length(), 0);
            bzero(rcvbuf, 1024);
            recv(thisClient, rcvbuf, sizeof(rcvbuf), 0);
            string portno_tmp(rcvbuf);
            string port;
            port.assign(portno_tmp, 0, portno_tmp.length());
            cout<< port << "port has been occupied.\n";
            
            //write account info in register.txt
            file.open("register.txt",ios::in);
            //check can open or not
            if(!file){
                cerr << "Can't open register.txt\n";
                exit(1);
            }
            //memorize account balance
            while(file >> fileName >> hisMoney)
            {
                if(fileName == prefix){
                    hisMoney = hisMoney+"\n";
                    isRegister = true;
                    break;
                }
            }
            file.close();
            
            //Login successfully (not leagl, it only means it has already registered before)
            if(isRegister == true){
                bool isLogin = false;
                struct person_account *cur = head;
                while(head->isPerson == true){
                    if(cur->userAccount == prefix){
                        isLogin = true;
                        break;
                    }
                    if(cur == tail)
                        break;
                    cur = cur->next;
                }
                //detect duplicate login
                if(isLogin == true)
                {
                    string hasLogin = prefix + " is online, we don't allow duplicate log in.\n";
                    send(thisClient, hasLogin.c_str(), hasLogin.length(), 0);
                    continue;
                }
                //Finally login successfully and legally
                cout << prefix << " log in.\n";
                onlineNum++;
                string portNumS;
                portNumS.assign(port,stopIndex+1,tmp.length());
                int portNum = atoi (portno_tmp.c_str());
                
                //detect illegal port 1024-65535
                if(portNum <1024 || portNum > 65535){
                    string notAllowPort = "This port isn't allowed. Please use port 1024 ~ 65535.\n";
                    send(thisClient, notAllowPort.c_str(), notAllowPort.length(),0);
                    continue;
                }
                if(head->isPerson == false)
                {
                    head->isPerson = true;
                    head->userAccount = prefix;
                    head->userPort = portNumS;
                    head->userIP = thisIP;
                    head->next = NULL;
                }
                else{
                    struct person_account *newPerson = new struct person_account;
                    newPerson->isPerson = true;
                    newPerson->userAccount = prefix;
                    newPerson->userPort = portNumS;
                    newPerson->userIP = thisIP;
                    newPerson->next = NULL;
                    tail->next = newPerson;
                    tail = newPerson;
                }
                string onNum;
                stringstream ss(onNum);
                ss << onlineNum;
                onNum = ss.str()+"\n";
                //print out online info
                string msg = "Account Balance: " + hisMoney + "Number of Accounts: " + onNum;
                int j=0;
                cur = head;
                while(head->isPerson == true){
                    msg += cur->userAccount + "#" + cur->userIP +"#" +cur->userPort+"\n";
                    j++;
                    if(cur == tail)
                        break;
                    cur = cur->next;
                }
                send(thisClient, msg.c_str(), msg.length(), 0);
                
                //Case "List or Exit"
                while(1){
                    greet = "List or Exit?\n";
                    send(thisClient, greet.c_str(), greet.length(), 0);
                    bzero(rcvbuf, 1024);
                    recv(thisClient, rcvbuf, sizeof(rcvbuf), 0);
                    
                    //Case "Exit"
                    if(strncmp(rcvbuf,"Exit",4) == 0){
                        cout << prefix << " calls exit command.\n";
                        
                        //send list to client
                        ss.str("");
                        ss.clear();
                        ss << onlineNum;
                        ss >> onNum;
                        msg = "Account Balance: " + hisMoney + "Number of Accounts Online: " + onNum +"\n";
                        cur = head;
                        while(head->isPerson == true){
                            msg += cur->userAccount + "#" + cur->userIP +"#" +cur->userPort+"\n";
                            if(cur == tail)
                                break;
                            cur = cur->next;
                        }
                        send(thisClient, msg.c_str(), msg.length(), 0);
                        
                        //online accounts number --
                        //deleting account info
                        onlineNum--;
                        cur = head;
                        struct person_account *pre = cur;
                        while(cur->userAccount != prefix){
                            pre = cur;
                            cur = cur->next;
                        }
                        cout << cur->userAccount <<" has exit server.\n";
                        if(cur == tail)
                            tail = pre;
                        pre->next = cur->next;
                        if(cur == head && cur->next == NULL){
                            head->isPerson = false;
                        }
                        else if(cur == head){
                            head = cur->next;
                        }
                        else
                            delete cur;
                        //send bye
                        string bye = "Bye\n";
                        send(thisClient, bye.c_str(), bye.length(), 0);
                        break;
                    }
                    
                    //Case "List"
                    else if(strncmp(rcvbuf,"List",4) == 0){
                        cout << prefix << " calls list command.\n";
                        //send list
                        ss.str("");
                        ss.clear();
                        ss << onlineNum;
                        ss >> onNum;
                        msg = "Account Balance: " + hisMoney + "Number of Accounts Online: " + onNum +"\n";
                        cur = head;
                        while(head->isPerson == true){
                            msg += cur->userAccount + "#" + cur->userIP +"#" +cur->userPort+"\n";
                            if(cur == tail)
                                break;
                            cur = cur->next;
                        }
                        send(thisClient, msg.c_str(), msg.length(), 0);
                    }
                    else{
                        string commErr = "Command not found\n";
                        send(thisClient, commErr.c_str(), commErr.length(), 0);
                        
                    }
                }
                break;
            }
            //user has not register before, so can not be found in register.txt
            else{
                string notRegist = "220 AUTH_FAIL\n";
                send(thisClient, notRegist.c_str(), notRegist.length(), 0);
            }
        }
    }
    close(thisClient);
    return 0;
}





