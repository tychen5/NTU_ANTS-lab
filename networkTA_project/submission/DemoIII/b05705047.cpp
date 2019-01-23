#include <iostream>
#include <pthread.h>    /* for POSIX threads */
#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <vector>     /* for close() */
#include <sstream>
#include <fstream>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

using namespace std;

#include "openssl_rsa.h"

#define MAXPENDING 20  /* Maximum outstanding connection requests */
#define RCVBUFSIZE 4096   /* Size of receive buffer */

template <typename T>
  std::string NumberToString ( T Number )
  {
     std::ostringstream ss;
     ss << Number;
     return ss.str();
  }

void *ThreadMain(void *arg);           /* Main program of a thread */
void DieWithError(string errorMessage); /* Error handling function */
int CreateTCPServerSocket(unsigned short port);
int AcceptTCPConnection(int servSock);
void HandleTCPClient(int clntSocket, class UserData *acct);  /* TCP client handling function */
string GetRegister(int clntSocket, class UserData *acct);
string CreateAccount(int clntSocket, string name, class UserData *acct);
string getCommand(int clntSocket, int index, int userAIndex, class UserData *acct);

int transactionCheck(string command, class UserData *acct);
bool IsNumber(const std::string& s);
int splitString(string input, vector<string> &output);
RSA * pubKey2RSA(string name, string key);
string privateEncrypt(char* msg, RSA *pri, RSA *pub); /* use of private key encryption */

struct ThreadArgs      /* Structure of arguments to pass to client thread */
{
   class UserData *accountPtr;
   int clntSock;        
};

class UserData {
private:
   int userCnt;
   vector<string> userName;
   vector<unsigned short> sin_port;
   vector<struct in_addr> sin_addr;
   vector<int> checking;
   vector<string> pubKey;
   vector<RSA *> rsa;
public:
   UserData() 
   {
      this->userCnt=0;
   }
   void addUser(string name, unsigned short port, struct in_addr addr, int amount) 
   {
      userName.push_back(name);
      sin_port.push_back(port);
      sin_addr.push_back(addr);
      checking.push_back(amount);
	  pubKey.push_back("");
	  rsa.push_back(NULL);
      userCnt++;
   }
   int lookupUser(string lookupName) 
   {
      for(int i=0;i<userCnt; i++) 
      {
         if (userName[i].compare(lookupName) == 0)
            return i;
      }
      return -1;
   }
   RSA * returnPubRSA(int index) 
   {
      return rsa[index];
   }
   string returnPubKey(int index) 
   {
      return pubKey[index];
   }
   string returnName(int index) 
   {
      return userName[index];
   }
   int checkUserActive(string name) 
   {
      for(int i=0;i<userCnt; i++) 
      {
         if (userName[i].compare(name) == 0) 
         {
             if (sin_port[i] > 0)
                 return i;
            else
                 return -1;
         }
      }
      return -1;
   }
   void updateInet(int i, unsigned short port, struct in_addr addr) 
   {
      sin_port[i]=port;
      sin_addr[i]=addr;
      return;
   }
   void updatePubKey(int i, string key) 
   {
   	  pubKey[i]=key;
	    if (key.empty()) 
      {
	  	   rsa[i]=NULL;
         cout << userName[i] << "'s key reset" << endl;
      }   
 	    else 
      {
         rsa[i]=pubKey2RSA(userName[i], key); 
    	   cout << userName[i] << "'s Key:" << endl;
		     cout << pubKey[i] << endl; 
    	   if(rsa[i] == NULL)
              printf( "Failed to create RSA");
         else 
              cout << userName[i] << "'s pubkey saved" << endl;
	  }
	  return;
   }
   int listActiveUser(int sockID, int index)
    {
      int balance=0, active=0;
      balance=checking[index];

      for(int i=0;i<userCnt; i++) 
      {
         if (sin_port[i] > 0)
            active++;
      }

      string b=NumberToString(balance);
      string sendBalance="<AccountBalance> :" + b + "\n";
      if (send(sockID, sendBalance.c_str(), sendBalance.length(), 0) != sendBalance.length())
      	 DieWithError("send() failed");

      string a=NumberToString(active);
      string sendActive="<Number of accounts online> :" + a + "\n";
      if (send(sockID, sendActive.c_str(), sendActive.length(), 0) != sendActive.length())
      	 DieWithError("send() failed");

      for(int i=0;i<userCnt; i++) 
      {
         if (sin_port[i] > 0) 
         {
            string p=NumberToString(sin_port[i]);
            string c=NumberToString(checking[i]);
            string sendString=userName[i] + " " + inet_ntoa(sin_addr[i]) + " " + p + " " + c + "\n";
            cout << sendString;
        	if (send(sockID, sendString.c_str(), sendString.length(), 0) != sendString.length())
          		DieWithError("send() failed");
         }
      }
      return active;
   }
   int tranSact(string user1, string user2, int amount) 
   {
   	  int u1=-1, u2=-1;
	  cout << "Transaction - user1:" << user1 << " User2:" << user2 << " Amount:" << amount << endl;
	  if (user1.compare(user2) == 0)
	     return -1; 
      for(int i=0;i<userCnt; i++) 
      {
         if (userName[i].compare(user1) == 0 && sin_port[i] > 0 )
            u1=i;
		     else if (userName[i].compare(user2) == 0 && sin_port[i] > 0) 	
		        u2=i;
	       if (u1 >=0 && u2 >=0) 
        {
		 	      checking[u1]-=amount;
		 	      checking[u2]+=amount;
			      return 0;
		    }
      }
   	  return -1;
   }
   void print() 
   {
      cout << "Total Users: " << userCnt << endl;
      for(int i=0;i<userCnt; i++)
         cout << userName[i] << " " << sin_port[i] << " " << inet_ntoa(sin_addr[i]) << " " << checking[i] << endl;
   }
};

int main(int argc, char *argv[]) 
{  
   int servSock;  /* Socket descriptor for server */
   int clntSock;  /* Socket descriptor for client */
   unsigned short servPort;  /* Server port */
   pthread_t threadID;  /* Thread ID from pthread_create()*/
   struct ThreadArgs *threadArgs; /* Pointer to argument structure for thread */
   class UserData accounts;
 
   if (argc != 2) {     /* Test for correct number of arguments */
      fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
      exit(1);
   }
   servPort = atoi(argv[1]); /* First arg: local port */

   servSock = CreateTCPServerSocket(servPort); 

   for (;;)   /* Run forever */
   { 
      clntSock = AcceptTCPConnection(servSock);
      /* Create separate memory for client argument and pass in the data to thread*/
      if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs))) == NULL) 
         DieWithError("threadArgs failed");
      threadArgs -> clntSock = clntSock;
      threadArgs -> accountPtr = &accounts;
      /* Create client thread */
      if (pthread_create (&threadID, NULL, ThreadMain, (void *) threadArgs) != 0) 
         DieWithError("pthread_create failed");
   }
   /* NOT REACHED */
   close(servSock);
}

int CreateTCPServerSocket(unsigned short port)
{
    int sock;                        /* socket to create */
    struct sockaddr_in echoServAddr; /* Local address */

    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");
      
    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(port);              /* Local port */

    /* Bind to the local address */
    if (::bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");

    /* Mark the socket so it will listen for incoming connections */
    if (listen(sock, MAXPENDING) < 0)
        DieWithError("listen() failed");

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
        DieWithError("accept() failed");
    /* clntSock is connected to a client! */
    return clntSock;
}

void *ThreadMain(void *threadArgs) 
{
   int clntSock; /* Socket descriptor for client connection */ 
   class UserData *acct;

   pthread_detach(pthread_self()); /* Guarantees that thread resources are deallocated upon return */

   /* Extract socket file descriptor from argument */
   clntSock = ((struct ThreadArgs *) threadArgs) -> clntSock; 
   acct = ((struct ThreadArgs *) threadArgs) -> accountPtr;

   free(threadArgs); /* Deallocate memory for argument */
   HandleTCPClient(clntSock, acct); 
   return (NULL);
}

void HandleTCPClient(int clntSocket, class UserData *acct)
{
    if (send(clntSocket, "Connection Accepted\n", sizeof("Connection Accepted\n"), 0) != sizeof("Connection Accepted\n"))
        DieWithError("send() failed");

    /* ------------- Register ----------- */
    string tempname=GetRegister(clntSocket, acct);

    if(tempname.compare("Quit") == 0 || tempname.compare("Break") == 0) 
    {
       close(clntSocket);
       return;
    }

    /* ------------- Create Account or update port ----------- */
    string portNumber=CreateAccount(clntSocket, tempname, acct);
    if(portNumber.compare("Break") == 0) 
    {
       close(clntSocket);
       return;
    }

    /* ------------- get List or Bye or transaction command ----------- */
    string command="";
  int userAIndex=-1;
    while(command.compare("Exit") != 0 && command.compare("Break") !=0 )
    {
      string userA;                /*------ for transaction user 1 ------*/
      string userB=tempname;       /*------ for transaction user 2 ------*/
      int nameIndex=acct->lookupUser(tempname);
      
      command=getCommand(clntSocket, nameIndex, userAIndex, acct);
      /*------------- print out the command for self debuging--------------*/
      cout << "Commandlength:" << command.length() << endl;
      cout << command << endl;

       if (command.compare("List") == 0)  
       {
          int j=acct->listActiveUser(clntSocket, nameIndex);
          cout << "Total active user: " << j << endl;
       }
       else if (command.compare("Exit") == 0) 
       {
          if (send(clntSocket, "Bye\n", sizeof("Bye\n"), 0) != sizeof("Bye\n"))
             DieWithError("send() failed");
          break;
       }
       else if (command.length() == 0)
       {
          if (send(clntSocket, "Command Error\n", sizeof("Command Error\n"), 0) != sizeof("Command Error\n"))
              DieWithError("send() failed");
        continue;
       }
       else if (command.find("Encrypted_Transaction_Request")!= std::string::npos) 
       {
          vector<string> splitCommand;
          int commandCnt=0;
          commandCnt=splitString(command, splitCommand);
          cout << "A:" << splitCommand[1] << " B:" << splitCommand[2] << endl;
          if(splitCommand[2]==userB) 
          {
              userAIndex=acct->checkUserActive(splitCommand[1]);
              cout << "Transaction authorized" << endl;
          }
       }
       else 
       {
          if (transactionCheck(command, acct) < 0)
              if (send(clntSocket, "Command Error\n", sizeof("Command Error\n"), 0) != sizeof("Command Error\n"))
                 DieWithError("send() failed");
       }
    }
    /* ---------- Reset port and addr to 0 after user disconnect   */
    int nameIndex=acct->lookupUser(tempname);
    struct in_addr tempAddr;
    tempAddr.s_addr=0;
    acct->updateInet(nameIndex, 0, tempAddr); /* reset port number */
    string resetKey="";
    acct->updatePubKey(nameIndex, resetKey); /* reset public key */
    cout << tempname << " disconnected. Port # and IP addr are reset" << endl;

    close(clntSocket);    /* Close client socket */
    return;
}

string GetRegister(int clntSocket, class UserData *acct)
{
    char echoBuffer[RCVBUFSIZE];        /* Buffer for echo string */
    ssize_t recvMsgSize;                    /* Size of received message */

    /*----------- get peer address and port number ----------*/
	  struct sockaddr_in peer;
	  socklen_t peer_len;
	  peer_len = sizeof(peer);
	  if (getpeername(clntSocket, (struct sockaddr *)&peer, (socklen_t *)&peer_len) == -1)
	     DieWithError("getpeername() failed"); 
   	printf("Client IP: %s, Port:%d \n", inet_ntoa(peer.sin_addr), (int) ntohs(peer.sin_port));
//   	printf("Peer's port is: %d\n", (int) ntohs(peer.sin_port));
    /*---------  Receive message from client---------- */
    memset(echoBuffer, '\0', sizeof(echoBuffer));
    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");

    /* Send received string and receive again until end of transmission */
    while (recvMsgSize > 0)      /* zero indicates end of transmission */
    {
        /* print out the received message for debug */
        printf("Register received: %s", echoBuffer);
    	if (strstr(echoBuffer, "REGISTER#<") != NULL) 
      {
			  char* tempname = strtok(echoBuffer, "#<>");
			  tempname = strtok(NULL, "#<>");
        char* deposit = strtok (NULL, " #><");
			  cout << endl;
        cout << "name :" << tempname << " deposit: " << deposit << endl;

        int nameIndex=acct->lookupUser(tempname);
        if (nameIndex >= 0)
            cout << tempname << " exists\n" << endl; 
        else  
        {
            if (deposit == NULL)
                acct->addUser(tempname, 0, peer.sin_addr, 0);
            else
                acct->addUser(tempname, 0, peer.sin_addr, atoi(deposit));
        }
        if (send(clntSocket, "100 OK\n", sizeof("100 Ok\n"), 0) != sizeof("100 Ok\n"))
           	DieWithError("send() failed");
			      return tempname;
		   }
		   else if (strstr(echoBuffer, "Exit") != NULL)
       {
        	if (send(clntSocket, "Bye\n", sizeof("Bye\n"), 0) != sizeof("Bye\n"))
           		DieWithError("send() failed");
			    close(clntSocket);
			    return "Quit";
		   }
		   else 
       {
         cout << "Register Format error, please retype" << endl;
         if (send(clntSocket, "210 Failed\n", sizeof("210 Failed\n"), 0) != sizeof("210 Failed\n"))
            	DieWithError("send() failed");
		   }
        memset(echoBuffer, '\0', sizeof(echoBuffer));
        /* See if there is more data to receive */
        if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
            DieWithError("recv() failed");
    }
	return "Break";
}

string CreateAccount(int clntSocket, string name, class UserData *acct)
{
  char echoBuffer[RCVBUFSIZE];        /* Buffer for echo string */
  ssize_t recvMsgSize;                    /* Size of received message */
  /* get peer address and port number */

	struct sockaddr_in peer;
	socklen_t peer_len;
	peer_len = sizeof(peer);
	if (getpeername(clntSocket, (struct sockaddr *)&peer, (socklen_t *)&peer_len) == -1)
	   DieWithError("getpeername() failed"); 

  /* Receive message from client */
  memset(echoBuffer, '\0', sizeof(echoBuffer));
  if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
      DieWithError("recv() failed");

  /* Send received string and receive again until end of transmission */
  while (recvMsgSize > 0)      /* zero indicates end of transmission */
    {
      /* print out the received message for debug */
        printf("Account Received: %s", echoBuffer);
      /* Check the name equal to first input name or not, if yes, store the  port number */
    	if (strstr(echoBuffer, name.c_str()) != NULL) 
      {
			   char* port = strtok(echoBuffer, ">");
			   port = strtok(NULL, "<");
			   char* portNum = strtok(NULL, ">");
      /*    check port number is integer or not  */
          if(IsNumber(portNum) != true) 
          {
              cout << "Port number format error" << endl;
     	        if (send(clntSocket, "Port Error\n", sizeof("Port Error\n"), 0) != sizeof("Port Error\n"))
        		     DieWithError("send() failed");
              memset(echoBuffer, '\0', sizeof(echoBuffer));
              if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
                    DieWithError("recv() failed");
                 continue;
          }
      /*  lookup user name and udpate his ip address/port number  If not find user, reply "Auth Fail" */
          int nameIndex=acct->lookupUser(name);
          if (nameIndex >= 0) 
          {
              acct->updateInet(nameIndex, atoi(portNum), peer.sin_addr);
               //cout << name << " " << inet_ntoa(peer.sin_addr) << " " << portNum << " updated" << "\n\n";
              cout << "Account updated" << endl;
              int j=acct->listActiveUser(clntSocket, nameIndex);
			        /*---    to receive client public key and update ----*/
    		      memset(echoBuffer, '\0', sizeof(echoBuffer));
  			      if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
      			     DieWithError("recv() failed");
              if(strstr(echoBuffer, "RSA") != NULL)
					       acct->updatePubKey(nameIndex, echoBuffer);	
          }
          else  
          {
        	    if (send(clntSocket, "220 Auth_Fail\n", sizeof("220 Auth_Fail\n"), 0) != sizeof("220 Auth_Fail\n"))
            	    DieWithError("send() failed");
          }
			     return portNum;
		      }
		      else if (strstr(echoBuffer, "Exit") != NULL) 
          {
        	    if (send(clntSocket, "Bye\n", sizeof("Bye\n"), 0) != sizeof("Bye\n"))
           		   DieWithError("send() failed");
			        close(clntSocket);
			        return "Quit";
		  }
		  else 
        if (send(clntSocket, "220 Auth_Fail\n", sizeof("220 Auth_Fail\n"), 0) != sizeof("220 Auth_Fail\n"))
            DieWithError("send() failed");
        /* See if there is more data to receive */
        memset(echoBuffer, '\0', sizeof(echoBuffer));
        if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
            DieWithError("recv() failed");
    }
	return "Break";
}
string getCommand(int clntSocket, int index, int userAIndex, class UserData *acct)
{
    char echoBuffer[RCVBUFSIZE];        /* Buffer for echo string */
    ssize_t recvMsgSize;                    /* Size of received message */

    /* Receive message from client */
    memset(echoBuffer, '\0', sizeof(echoBuffer));
    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");
    
    if (recvMsgSize == 0)
    return "Break";

    if (recvMsgSize <= 5) 
    {
       char *newline = strchr( echoBuffer, '\n' );
       if ( newline )
          *newline = 0;
       return echoBuffer;
    }   
    else if ( recvMsgSize > 5 ) 
    { 
       if (strstr(echoBuffer, "Encrypted_Transaction_Request") != NULL) 
       {
          return echoBuffer;
       }
       RSA *pub=NULL;
       char decrypt[recvMsgSize];
       memset(decrypt, '\0', sizeof(decrypt));
    
       pub=acct->returnPubRSA(userAIndex);
       if (pub==NULL)
          cout << "RSA pub is NULL" << endl;
    
       int decrypt_length = RSA_public_decrypt(recvMsgSize, (unsigned char*)echoBuffer, (unsigned char*)decrypt, pub, RSA_PKCS1_PADDING);
       if(decrypt_length == -1) 
       {
         LOG("An error occurred in public_decrypt() method");
         ERR_load_ERR_strings();
         ERR_load_crypto_strings();
         unsigned long ulErr = ERR_get_error();
         char szErrMsg[1024] = {0};
         char *pTmp = NULL;
         pTmp = ERR_error_string(ulErr,szErrMsg);
         string errcode(szErrMsg);
         cout << errcode << endl;
         return echoBuffer;
       }
       cout << "Decrypted Transaction:" << decrypt << endl;
       return decrypt;
    }
    return "Break";
}

void DieWithError(string errorMessage)
{
    cout << errorMessage << endl;
    exit(1);
}

bool IsNumber(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

int splitString(string input, vector<string> &output)
{
	vector<string> splitCommand;
	char* pch;
	int cnt=0;
  char* temp = ( char * ) malloc( input.length() );	
	strcpy(temp, input.c_str()); 
  pch = strtok(temp, "<>#");
  while (pch !=NULL) 
  { 
	    output.push_back(pch);
      cnt++;
      pch = strtok(NULL, "<>#");
  }
  return cnt;
}

int transactionCheck(string command, class UserData *acct)
{
	vector<string> splitCommand;
  int commandCnt=0;
	commandCnt=splitString(command, splitCommand);
	if(IsNumber(splitCommand[1]) != true) 
  {
    cout << "Dollar format error" << endl;
		return -1;
	}
	if (acct->tranSact(splitCommand[0], splitCommand[2], atoi(splitCommand[1].c_str())) >=0) 
  {
	  cout << "Transaction successfull" << endl;
		return 0;
	}
	else 
  {
	  cout << "Transaction fail" << endl;
		return -1;
	}
}

RSA * pubKey2RSA(string name, string key)
{
  /* create a file to input the pubKey string format and turns into RSA format */
    RSA *rsa=NULL;
    string filename=name + "_pubKey.PEM";
    ofstream out(filename);
    out << key;
    out.close();
    FILE *fp = NULL;
    fp = fopen(filename.c_str(), "rb");
    PEM_read_RSAPublicKey(fp, &rsa, NULL, NULL);
    remove(filename.c_str());
    if(rsa == NULL) 
    {
        printf( "Failed to create RSA\n");
        return NULL;
    }
    else
        return rsa;
}

string privateEncrypt(char* msg, RSA *pri, RSA *pub) 
{
    char *encryptPri = NULL;
    encryptPri = (char*)malloc(RSA_size(pri));
    char *encryptPub = NULL;
    encryptPub = (char*)malloc(RSA_size(pub));

    int encrypt_length = RSA_private_encrypt(strlen(msg) + 1, (unsigned char*)msg, (unsigned char*)encryptPri, pri, RSA_PKCS1_PADDING);
    if(encrypt_length == -1) 
       LOG("An error occurred in private_encrypt() method");

    cout << "AfterPri:" << encryptPri << endl;

    char *decrypt=NULL;
    decrypt=(char *)malloc(encrypt_length);

    int decrypt_length = RSA_public_decrypt(encrypt_length, (unsigned char*)encryptPri, (unsigned char*)decrypt, pub, RSA_PKCS1_PADDING);
    if(decrypt_length == -1)
        LOG("An error occurred in private_decrypt() method");

    cout << "decrypted:" << decrypt << endl;
    return encryptPri;
}
