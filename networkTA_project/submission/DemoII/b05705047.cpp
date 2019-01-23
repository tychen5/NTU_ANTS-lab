
//
//  main.cpp
//  homework_part2
//
//  Created by KuoChris on 2018/12/23.
//  Copyright © 2018年 KuoChris. All rights reserved.
//

#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), connect(), recv() and send() */
#include <arpa/inet.h> /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h> /* for atoi() and exit() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */
#include <pthread.h> /* for POSIX threads */
#include <vector>  /* the use of vector */
#include <sstream>
#include <iostream>
#define MAXPENDING 20 /* Maximum connections at once */
using namespace std ;
class userData {
private:
    int counts ;
    vector <string> username ;
    vector <unsigned short> sin_port ;
    vector <struct in_addr> sin_addr ;
    vector <int> balance ;
public:
    userData()
    {
        this ->counts = 0;
    }
    void addData (string name,struct in_addr addrNum,unsigned short portNum, int amount)
    {
        username.push_back(name);
        sin_port.push_back(portNum);
        sin_addr.push_back(addrNum);
        balance.push_back(amount);
        counts ++ ;
    }
    void print()
    {
        cout << "Total user: " << counts << endl ;
        for(int i=0;i<counts;i++)
        {
            cout << username[i] << " " << sin_port[i] << " " << inet_ntoa(sin_addr[i]) << " " << balance[i] << endl ;
        }
    }
    int lookupUser(string tempname)
    {
        for(int i=0;i<counts;i++)
        {
            if(username[i] == tempname)
                return i ;
        }
        return -1 ;
    }
    void reset(string tempname) {
        for(int i=0;i<counts;i++)
        {
            if(username[i] == tempname) {
                sin_port[i] = 0 ;
                return;
            }
        }
        return;
        
    }
    void update (int who,struct in_addr addrNum,unsigned short portNum)
    {
        sin_addr[who] = addrNum ;
        sin_port[who] = portNum ;
    }
    void updateMoney (int who,int amount)
    { 
        balance[who] = amount ;
    }
    int activeUser(int clntSock,int whichUser)
    {
        int Balance = 0;
        int active = 0;
        Balance = balance[whichUser];
        for(int i=0;i<counts;i++)
            if(sin_port[i] > 0)
                active ++ ;
        string sendBalance = "<accountBalance> " + to_string(Balance) +"\n" ;
        if(send(clntSock,sendBalance.c_str(),sendBalance.length(),0) != sendBalance.length())
            cout << "send() error\n" ;
        string senduserAmount = "<number of accounts online> " + to_string(active) + "\n" ;
        if(send(clntSock,senduserAmount.c_str(),senduserAmount.length(),0) != senduserAmount.length())
            cout << "send() error\n" ;
        for(int i=0;i<counts;i++)
        {
            if(sin_port[i] > 0)
            {
                active ++ ;
                string p = to_string(sin_port[i]);
                string sendString = username[i] + "#" + inet_ntoa(sin_addr[i]) + "#" + p +"\n";
                if(send(clntSock,sendString.c_str(),sendString.length(),0 ) != sendString.length())
                    cout << "send() fail\n" ;
            }
        }
        return active ;
    }
};
void *ThreadMain(void *arg) ; /* Main program of a thread */
struct ThreadArgs
{ /* Structure of arguments to pass to client thread */
    int clntSock; /* socket descriptor for client */
    struct userInfo *users;
    int *counts ;
    class userData *accountPtr ;
};
struct userInfo
{
    char userName[30];
    unsigned short sin_port ;
    struct sockaddr_in user_addr;
    int checking ;
};

int CreateTCPServerSocket(unsigned short b) ;
int AcceptTCPConnection(int a);
void HandleTCPClient(int clntSocket,struct userInfo *users,int *counts,class userData *user);
string getRegister(int clntSocket,class userData *user) ;
string createAccount(int clntSocket,class userData *user,string a) ;
string command(int clntSocket,class userData *user,string a);
string money(int clntSocket, class userData *user,string a);

int main(int argc, char *argv[]) {
    int servSock; /* Socket descriptor for server */
    int clntSock; /* Socket descriptor for client */
    unsigned short echoServPort; /* Server port */
    pthread_t threadID; /* Thread ID from pthread_create()*/
    struct ThreadArgs *threadArgs; /* Pointer to argument structure for thread */
    struct userInfo users[100] ;
    class userData accounts ;
    int userCounts = 0 ;
    if (argc != 2)
    { /* Test for correct number of arguments */
        fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]);
        exit(1);
    }
    echoServPort = atoi(argv[1]); /* First arg: local port */
    servSock = CreateTCPServerSocket(echoServPort);
    for (;;)
    {   /* Run forever */
        clntSock = AcceptTCPConnection(servSock);
        /* Create separate memory for client argument */
        if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs))) == NULL)
            perror("Unable allocate resource");
        threadArgs -> clntSock = clntSock;
        threadArgs -> users = users;
        threadArgs -> counts = &userCounts ;
        threadArgs-> accountPtr = &accounts ;
        /* Create client thread */
        if (pthread_create (&threadID, NULL, ThreadMain, (void *) threadArgs) != 0)
            perror("Unable Create thread ");
    }
    return 0 ;
    /* NOT REACHED */
}

void *ThreadMain(void *threadArgs)
{
    int clntSock; /* Socket descriptor for client connection */
    struct userInfo *users ;
    int *counts ;
    class userData *acct;
    pthread_detach(pthread_self()); /* Guarantees that thread resources are deallocated upon return */
    /* Extract socket file descriptor from argument */
    clntSock = ((struct ThreadArgs *) threadArgs) -> clntSock;
    users = ((struct ThreadArgs *) threadArgs) -> users ;
    counts = ((struct ThreadArgs *) threadArgs) -> counts ;
    acct = ((struct ThreadArgs *) threadArgs) -> accountPtr ;
    free(threadArgs); /* Deallocate memory for argument */
    HandleTCPClient(clntSock,users,counts,acct);
    return (NULL);
}

int CreateTCPServerSocket(unsigned short port)
{
    int sock;                        /* socket to create */
    struct sockaddr_in echoServAddr; /* Local address */
    
    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        perror("socket() failed");
    
    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(port);              /* Local port */
    
    /* Bind to the local address */
    if (::bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        perror("bind() failed");
    
    /* Mark the socket so it will listen for incoming connections */
    if (listen(sock, MAXPENDING) < 0)
        perror("listen() failed");
    
    return sock;
}

int AcceptTCPConnection(int servSock)
{
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int clntLen;            /* Length of client address data structure */
    /* Set the size of the in-out parameter */
    clntLen = sizeof(echoClntAddr);
    /* Wait for a client to connect */
    if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr,
                           &clntLen)) < 0)
        perror("accept() failed");
    if (send(clntSock,"Connection Accepted\n",sizeof("Connection Accepted\n"),0) != sizeof("Connection Accepted\n"))
        perror ("Connection Denied");
    /* clntSock is connected to a client! */
    printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));
    
    return clntSock;
}

#define RCVBUFSIZE 32 /* Size of receive buffer */
void HandleTCPClient(int clntSocket,struct userInfo *users,int *counts,class userData *user)
{
    string tempName = getRegister(clntSocket,user);
    if (strstr(tempName.c_str(), "Quit") != NULL)
    {
        user->reset(tempName);
        close(clntSocket);
        return;
    }
    string returnStatus=createAccount(clntSocket,user,tempName);
    if (strstr(returnStatus.c_str(), "Quit") != NULL)
    {
        user->reset(tempName);
        close(clntSocket);
        return;
    }
    returnStatus=command(clntSocket,user,tempName);
    if (strstr(returnStatus.c_str(), "Quit") != NULL)
    {
        user->reset(tempName);
        close(clntSocket);
        return;
    }
    close(clntSocket); /* Close client socket */
}
string getRegister(int clntSocket,class userData *user)
{
    char echoBuffer[RCVBUFSIZE]; /* Buffer for echo string */
    ssize_t recvMsgSize; /* Size of received message */
    string a ;
    /* get peer Ip and port num */
    struct in_addr tempaddr ;
    tempaddr.s_addr = 0;
    struct sockaddr_in peer;
    socklen_t peer_len;
    memset(echoBuffer, '\0', sizeof(echoBuffer));
    peer_len = sizeof(peer);
    if (getpeername(clntSocket, (struct sockaddr *)&peer, (socklen_t *)&peer_len) == -1)
       cout << "getpeername() failed" << endl ;
    /* Receive message from client */
    if ((recvMsgSize= recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        perror("recv() failed");
    /* Send received string and receive again until end of transmission */
    while (recvMsgSize> 0) /* zero indicates end of transmission */
    {
        if( strstr(echoBuffer,"Exit") != NULL) /* If exit then jump */
        {
            if (send (clntSocket,"Bye\n", sizeof("Bye\n"),0)!= sizeof("Bye\n"))
                perror ("Wrong");
            return "Quit" ;
        }
        if (strstr(echoBuffer,"REGISTER#<") != NULL )
        {
            char* tempName = strtok(echoBuffer,"<");
            tempName = strtok (NULL,">") ;
			char* deposit = strtok (NULL, " ><");
			cout << "name :" << tempName << " deposit: " << deposit << endl;
			if (deposit == NULL)
                user->addData(tempName,tempaddr,0,0);
            else 
                user->addData(tempName,tempaddr,0,atoi(deposit));
            if (send (clntSocket,"100 OK\n", sizeof("100 OK\n"),0)!= sizeof("100 OK\n"))
                perror ("Wrong");
            /*user->addData(tempName,tempaddr,0,atoi(deposit));*/
            user->print();
            a = tempName;
            cout << "The ID is " << tempName << endl;
            return a;
        }
        else
        {
            if (send (clntSocket,"210 FAIL\n", sizeof("210 FAIL\n"),0)!= sizeof("210 FAIL\n"))
                perror ("Wrong");
        }
        /* See if there is more data to receive */
        memset(echoBuffer, '\0', sizeof(echoBuffer));
        if ((recvMsgSize= recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0) /* to let the pipe know there's nothing to send */
            perror("recv() failed");
    }
    return "Break" ;
}
string createAccount (int clntSocket,class userData *user,string a)
{
    char echoBuffer[RCVBUFSIZE];
    ssize_t recvMsgSize;
    int whichUser = 0 ;
    struct sockaddr_in peer;
    socklen_t peer_len;
    memset(echoBuffer, '\0', sizeof(echoBuffer));
    peer_len = sizeof(peer);
    if (getpeername(clntSocket, (struct sockaddr *)&peer, (socklen_t *)&peer_len) == -1)
        cout << "getpeername() failed" << endl ;
    if ((recvMsgSize= recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        perror("recv() failed");
    while (recvMsgSize> 0)
        {
            if( strstr(echoBuffer,"Exit") != NULL) /* If exit then jump */
            {
                if (send (clntSocket,"Bye\n", sizeof("Bye\n"),0)!= sizeof("Bye\n"))
                    perror ("Wrong");
                return "Quit" ;
            }
            else if(strstr(echoBuffer,a.c_str()) != NULL) // change string into char for compare
            {
                whichUser = user->lookupUser(a);
                char *port = strtok(echoBuffer,">");
                port = strtok(NULL,"<");
                char *portNum = strtok(NULL,">");
                user->update(whichUser, peer.sin_addr,atoi(portNum));
                //user->print();
                user->activeUser(clntSocket,whichUser);
                return "fine";
            }
            else
            {
                if (send (clntSocket,"220 AUTH_FAIL\n", sizeof("220 AUTH_FAIL\n"),0)!= sizeof("220 AUTH_FAIL\n"))
                    perror ("Wrong");
            }
            memset(echoBuffer, '\0', sizeof(echoBuffer));
            if ((recvMsgSize= recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
                perror("recv() failed");
        }
    return "Quit";
}
string command(int clntSocket,class userData *user,string a)
{
    char echoBuffer[RCVBUFSIZE];
    ssize_t recvMsgSize;
    int whichUser = 0;
    if ((recvMsgSize= recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        perror("recv() failed");
    while (recvMsgSize > 0)
    {
        if( strstr(echoBuffer,"Exit") != NULL) /* If exit then jump */
        {
            if (send (clntSocket,"Bye\n", sizeof("Bye\n"),0)!= sizeof("Bye\n"))
                perror ("Wrong");
            return "Quit" ;
        }
        else if(strstr(echoBuffer,"List\n") != NULL)
        {
            whichUser = user->lookupUser(a);
            user->activeUser(clntSocket,whichUser);
        }
        memset(echoBuffer, '\0', sizeof(echoBuffer));
        if ((recvMsgSize= recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
            perror("recv() failed");
    }
    return "Quit";
}