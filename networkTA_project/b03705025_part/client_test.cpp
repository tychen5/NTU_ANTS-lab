#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;
int main(int argc, char* argv[])
{
    int sd;
    struct sockaddr_in dest;
    char rcvbuf[1024];
    char sndbuf[1024];
    bool check = 0;
    
    sd = socket(PF_INET, SOCK_STREAM, 0);
    
    bzero(&dest, sizeof(dest));
    dest.sin_family = PF_INET;
    dest.sin_port = htons(atoi(argv[2]));
    dest.sin_addr.s_addr = inet_addr(argv[1]);
    
    connect(sd, (struct sockaddr*)&dest, sizeof(dest));
    //	bzero(rcvbuf, 1024);
    //	recv(sd, rcvbuf, sizeof(rcvbuf), 0);
    //	cout << rcvbuf << endl;
    //	printf("%s",rcvbuf);
    while(1)
    {
        bzero(rcvbuf, 1024);
        recv(sd, rcvbuf, sizeof(rcvbuf), 0);
        cout<<rcvbuf;
        //        cout<<"client\n";
        scanf("%s",sndbuf);
        if(strncmp(sndbuf, "Register", 8) == 0){
            //register command
            send(sd, sndbuf, sizeof(sndbuf), 0);
//            cout<<"command register send ok\n";
            //username
            bzero(rcvbuf, 1024);
            recv(sd, rcvbuf, sizeof(rcvbuf), 0);
            printf("%s",rcvbuf);
            scanf("%s",sndbuf);
            send(sd, sndbuf, sizeof(sndbuf), 0);
//            cout<<"register name send ok\n";
            //deposit
            bzero(rcvbuf, 1024);
            recv(sd, rcvbuf, sizeof(rcvbuf), 0);
            printf("%s",rcvbuf);
            scanf("%s",sndbuf);
            send(sd, sndbuf, sizeof(sndbuf), 0);
//            cout<<"deposit send ok\n";
            bzero(rcvbuf, 1024);
            recv(sd, rcvbuf, sizeof(rcvbuf), 0);
            cout<<rcvbuf;
            //result
        }
        else if(strncmp(sndbuf, "Login", 5) == 0){
            //login command
            send(sd, sndbuf, sizeof(sndbuf), 0);
//            cout<<"login command send ok\n";
            //username
            bzero(rcvbuf, 1024);
            recv(sd, rcvbuf, sizeof(rcvbuf), 0);
            cout<<rcvbuf;
            scanf("%s",sndbuf);
            send(sd, sndbuf, sizeof(sndbuf), 0);
//            cout<<"loging username send ok\n";
            //portno
            bzero(rcvbuf, 1024);
            recv(sd, rcvbuf, sizeof(rcvbuf), 0);
            cout<<rcvbuf;
            scanf("%s",sndbuf);
            send(sd, sndbuf, sizeof(sndbuf), 0);
//            cout<<"login portno send ok\n";
            //balance list
            bzero(rcvbuf, 1024);
            recv(sd, rcvbuf, sizeof(rcvbuf), 0);
            cout<<rcvbuf;
        }
        else if(strncmp(sndbuf, "List", 4) == 0){
            send(sd, sndbuf, sizeof(sndbuf), 0);
//            cout<<"list command send ok\n";
            bzero(rcvbuf, 1024);
            recv(sd, rcvbuf, sizeof(rcvbuf), 0);
            cout<<rcvbuf;
        }
        else if(strncmp(sndbuf, "Exit", 4)== 0){
            send(sd, sndbuf, sizeof(sndbuf), 0);
//            cout<<"exit command send ok\n";
            //list
            bzero(rcvbuf, 1024);
            recv(sd, rcvbuf, sizeof(rcvbuf), 0);
            cout<<rcvbuf;
            //bye
            bzero(rcvbuf, 1024);
            recv(sd, rcvbuf, sizeof(rcvbuf), 0);
            cout<<rcvbuf;
        }
    }
    
    
    close(sd);
    return 0;
}