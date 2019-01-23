#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <stdlib.h>
#include <cstdlib>
#include <unistd.h>
using namespace std;

void regist(int);
char login(int);
void logout(int);


string Int_to_String(int);

string message;
int sockfd = 0;

int main()
{   //$ ./<server_name><space><portNum><space><Option>
    //int port , char* addr[]
    int port=65536;
    string addr;
    cout<< "請輸入ip位置\n";
    cin>>addr;
    while(port>65535)
    {   cout<< "請輸入連接port\n";
        cin>> port;
        if(port>65535)
            cout<<"\nport不符合規定，請重新輸入\n";
    }
    //socket的連線
    int err=0;
    do
    {   sockfd=socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd==-1)
        cout<< "Fail to create a socket.";
        struct sockaddr_in info;
        bzero(&info,sizeof(info));
        info.sin_family=PF_INET;
        info.sin_addr.s_addr=inet_addr(addr.c_str());
        info.sin_port=htons(port);

        err=connect(sockfd,(struct sockaddr *)&info,sizeof(info));

        if(err==-1)
        {   cerr<< "連接失敗!! 3秒後重試.\r";
            sleep(1);
            cerr<< "連接失敗!! 2秒後重試..\r";
            sleep(1);
            cerr<< "連接失敗!! 1秒後重試...\r";
            sleep(1);
            cout<<"\n\n";
            /*for(int i=0; i<3; i++)
            {   sleep(1);
                cerr<<".";
            }
            sleep(1);
            cout<<"\n";*/
        }
    }
    while(err==-1);

//-------------------------------------------

    char ans;
    char receiveMessage[1000]={};
    char connectMessage[1000]={"connection accepted"};
    recv(sockfd,receiveMessage, sizeof(receiveMessage),0);
    cout<< receiveMessage;
    /*if(receiveMessage[0]==connectMessage[0])
        cout<< "\n-----連線成功！-----";
    else
    {   cout<< "\n-----連線失敗！-----";
        logout(sockfd);
    }*/

    while(1)
    {   cout<< "\n登入(L)\n註冊(R)\n線上清單(T)\n離開(X)\n";
        cin>> ans;
        if(ans=='R')
       {   regist(sockfd);
            cout<< "登入(L)，離開(X)，其餘鍵回主畫面，\n";
            cin>> ans;
            if(ans=='L')
                login(sockfd);
            else if(ans=='X')
            {   logout(sockfd);
               return 0;
            }
            else;
        }
        else if(ans=='L')
        {   char opt=login(sockfd);
            if(opt=='R')
                regist(sockfd);
            else if(opt=='X')
                logout(sockfd);
            else;
        }
        else if(ans=='T')
        {   message="List";
            send(sockfd,message.c_str(), sizeof(message),0);
            char receiveMessage[1000]={};
            recv(sockfd,receiveMessage, sizeof(receiveMessage),0);
            cout<<receiveMessage;
        }
        else if(ans=='X')
        {   logout(sockfd);
            return 0;
        }
    }
    cout<<"\n";
}

void regist(int sd)
{   string user;
    char OKMessage[10]={"100 OK"};
    cout<< "請輸入使用者名稱\n";
    cin>> user;
    message="REGISTER#"+user+"\n";

    send(sd,message.c_str(), sizeof(message),0);
    char receiveMessage[1000]={};
    recv(sd,receiveMessage, sizeof(receiveMessage),0);
    if(receiveMessage[0]==OKMessage[0])
        cout<<"註冊成功！！\n";
}

char login(int sd)
{   string user;
    char FailMessage[15]={"220 AUTH_FAIL"};
    char ans;
    int port=65536;
    cout<< "\n請輸入使用者名稱\n";
    cin>> user;
    while(port>65535)
    {   cout<< "請輸入連接port\n";
        cin>> port;
        if(port>65535)
            cout<<"\nport不符合規定，請重新輸入\n";
    }

    string portstring=to_string(port);
    message=user+"#"+portstring;

    send(sd,message.c_str(), sizeof(message),0);
    char receiveMessage[1000]={};
    recv(sd,receiveMessage, sizeof(receiveMessage),0);
    if(receiveMessage[5]==FailMessage[5])
    {   cout<<"授權失敗!! 註冊(R) 離開(X)，其餘鍵回主畫面\n";
        cin>> ans;
        if(ans=='R')
            return 'R';
        else if(ans=='X')
            return 'X';
        else
            return 'M';
    }
    else
    {   cout<< "\n-----登入成功！-----\n";
        cout<< "帳戶餘額："<<receiveMessage;
        recv(sd,receiveMessage, sizeof(receiveMessage),0);
        cout<< receiveMessage;
        cout<< "\n按任意鍵回主畫面";
        cin>> ans;
        cout<<"\n\n";
        return 'M';
    }
}

void logout(int sd)
{   message="Exit\n";
    char receiveMessage[1000]={};
    char logoutMessage[10]={"Bye"};
    send(sd,message.c_str(),sizeof(message),0);
    recv(sd,receiveMessage,sizeof(receiveMessage),0);
    if(receiveMessage[0]==logoutMessage[0])
        cout<< "登出成功！"<<"\n";
    close(sd);
}
 