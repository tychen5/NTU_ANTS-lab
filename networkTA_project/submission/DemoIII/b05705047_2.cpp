// Client side C/C++ program to demonstrate Socket programming 
#include <iostream>
#include <pthread.h>    /* for POSIX threads */
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

using namespace std;

#include "openssl_rsa.h"

#define PORT 33120
#define MAXPENDING 20    /* Maximum outstanding connection requests */
#define RCVBUFSIZE 4096   /* Size of receive buffer */

struct User {
	string name;
	int balance;
	unsigned short port;
};

struct ThreadArgs {
   struct User *user;
   int listenSock;   
   int servSock;
   RSA *pubRSA;
   RSA *priRSA;
   string *pubStrKey;
};

bool IsNumber(const std::string& s);
void DieWithError(string errorMessage);
void printSockInfo(int sock); 
int splitString(string input, vector<string> &output);
int peerSendMessage(string myName, string msg, string addr, string port, RSA *priKey); 
int Type2Send (int fd, struct User *user, RSA *pri, RSA *pub);
void Receive2Print (int fd);
int CreateTCPServerSocket(unsigned short port);
int AcceptTCPConnection(int servSock);
void *ThreadMain(void *threadArgs);
string rsaGenKey(RSA &pri, RSA &pub); /* return public key string */
void rsaTesting(RSA *pri, RSA *pub);
string privateEncrypt(int fd, char* msg, RSA *pri, RSA *pub);
RSA * pubKey2RSA(string name, string key);

int main(int argc, char const *argv[]) 
{ 
    struct sockaddr_in address; 
	struct User user;
	//memset(&user, '\0', sizeof(user));
    int sock = 0, valread, result=0; 
	int listenSock=0;
    struct sockaddr_in serv_addr; 
	pthread_t threadID;  /* Thread ID from pthread_create()*/
	struct ThreadArgs *threadArgs; /* Pointer to argument structure for thread */
    char input[200];
    char buffer[1024] = {0}; 
	RSA priKey;
	RSA pubKey;
	string pubStrKey=rsaGenKey(priKey, pubKey);
	cout << "Public Key length:" << pubStrKey.length() << endl;
	//rsaTesting(&priKey, &pubKey);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    memset(&serv_addr, '\0', sizeof(serv_addr)); 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(atoi(argv[2])); 


   // printf("%s, %s\n", argv[1], argv[2]);
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return -1; 
    } 

    Receive2Print(sock);
	
//	cout << "main server sock: " << sock << endl;

    while((result=Type2Send(sock, &user, &priKey, &pubKey)) >= 0)
    {
	    if (result==2) /* ---- send msg to peer, not to server ----*/
			continue;
        Receive2Print(sock);
        if (result == 99)
           break;
		else if (result == 1) {
		    /* Send public key to server */	
			sleep(1);
			cout << "send:" << endl;
			cout << pubStrKey << endl;
   			if (send(sock , pubStrKey.c_str() , pubStrKey.length(), 0) != pubStrKey.length()) 
            	DieWithError("send() failed");
			listenSock = CreateTCPServerSocket(user.port);

      		if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs))) == NULL)
         		DieWithError("threadArgs failed");

      		threadArgs -> user = &user;
      		threadArgs -> listenSock = listenSock;
      		threadArgs -> servSock = sock;
			threadArgs -> priRSA=&priKey;
			threadArgs -> pubRSA=&pubKey;
			threadArgs -> pubStrKey=&pubStrKey;
//edit
cout << "after threadArgs" << endl;
      		/* Create client thread */
      		if (pthread_create (&threadID, NULL, ThreadMain, (void *) threadArgs) != 0)
         		DieWithError("pthread_create failed");
		}
    }
    close(sock);
    return 0;
}

bool IsNumber(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

void DieWithError(string errorMessage)
{
    cout << errorMessage << endl;
    exit(1);
}

void printSockInfo(int sock) {
    /*----------- get peer address and port number ----------*/
    struct sockaddr_in peer;
    socklen_t peer_len;
    peer_len = sizeof(peer);
    if (getpeername(sock, (struct sockaddr *)&peer, (socklen_t *)&peer_len) == -1)
       DieWithError("getpeername() failed");

    printf("Peer IP: %s, Port:%d \n", inet_ntoa(peer.sin_addr), (int) ntohs(peer.sin_port));
}

int splitString(string input, vector<string> &output)
{   
    vector<string> splitCommand;
    char* pch;
    int cnt=0; 
    char* temp = ( char * ) malloc( input.length() );
    strncpy(temp, input.c_str(), input.length());
    
    pch = strtok(temp, "<># \n");
    while (pch !=NULL) { 
        output.push_back(pch);
        cnt++;
        pch = strtok(NULL, "<># \n");
    }
//	for (int i=0; i<cnt; i++)
//		cout << i << ":" << output[i] << " ";
//	cout << endl;	
    return cnt;
}

int peerSendMessage(string myName, string msg, string addr, string port, RSA *priKey) {
    struct sockaddr_in address; 
    int sock = 0, valread, result=0; 
	int listenSock=0;
    struct sockaddr_in serv_addr; 
    //char buffer[1024] = {0}; 

	cout << msg << " to " << addr << " " << port << endl;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)  {
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
    memset(&serv_addr, '\0', sizeof(serv_addr)); 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(atoi(port.c_str())); 

    printf("%s, %s\n", addr.c_str(), port.c_str());
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, addr.c_str(), &serv_addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return -1; 
    } 
//edit
 	send(sock , myName.c_str(), myName.length(), 0); 

    Receive2Print(sock);

    /*--------  To receive peer pubKey ------ */
	char echoBuffer[RCVBUFSIZE];
	memset(echoBuffer, '\0', sizeof(echoBuffer));
	int recvMsgSize=-1;
	string peerPubKey;
	RSA *peerPubRSA=NULL;
    if ((recvMsgSize = recv(sock, echoBuffer, RCVBUFSIZE, 0)) < 0)
         DieWithError("recv() failed");
    if(strstr(echoBuffer, "RSA") != NULL) {
		peerPubKey=echoBuffer; //edit
		cout << peerPubKey << endl;
		peerPubRSA=pubKey2RSA("peerClient", echoBuffer);
	}
	/* -------- as peer client ------- */

	/* -------- encrypted with myself private key ------- */
    unsigned char initEncrypt[4096];
    memset(initEncrypt, '\0', sizeof(initEncrypt));
    int initEncrypt_length = RSA_private_encrypt(msg.length()+1, (unsigned char*)msg.c_str(), (unsigned char*)initEncrypt, priKey, RSA_PKCS1_PADDING);
    if(initEncrypt_length == -1) {
        LOG("An error occurred in private_encrypt() method");
    }
	cout << "Initial enc bytes:" << initEncrypt_length << endl;

    /* -------- encrypted with peer public key --------editor */
    unsigned char encrypted[20480];

    for(int i=0;i<initEncrypt_length;i+=64) {
        memset(encrypted, '\0', sizeof(encrypted));
		int encSize=0;
		if (initEncrypt_length-i>64)
		    encSize=64;
		else 
		    encSize=initEncrypt_length-i;

        int encryptedLength = RSA_public_encrypt(encSize, (unsigned char*)&initEncrypt[i], (unsigned char*)encrypted, peerPubRSA, RSA_PKCS1_OAEP_PADDING);
    //    int encryptedLength = RSA_public_encrypt(msg.length()+1, (unsigned char*)msg.c_str(), (unsigned char*)encrypted, peerPubRSA, RSA_PKCS1_OAEP_PADDING);
        if(encryptedLength == -1) {
            LOG("An error occurred in public_encrypt() method");
                ERR_load_ERR_strings();
                ERR_load_crypto_strings();
                unsigned long ulErr = ERR_get_error();
                char szErrMsg[1024] = {0};
                char *pTmp = NULL;
                pTmp = ERR_error_string(ulErr,szErrMsg);
                string errcode(szErrMsg);
                cout << errcode << endl;
        }
    
        if(encryptedLength > 0) {
       		int bytes=send(sock , encrypted, encryptedLength, 0); 
    		cout << "Peer send bytes:" << bytes << " enc types:" << encryptedLength << endl;
        }
    	else
    	    cout << "No transaction command sent out" << endl;
    }
   	//send(sock , msg.c_str(), msg.length(), 0); 
	close(sock);
	return 0;
}

int Type2Send (int fd, struct User *user, RSA *pri, RSA *pub)
{
    char input[200];
	int resultCode=0;
    memset(input, '\0', sizeof(input));
    //scanf("%s" , input) ; 
	cin.getline(input, sizeof(input));
	input[strlen(input)]='\n';
	/*-------- send to remote at first --------*/
	char *sendstring = ( char * ) malloc( strlen(input) );	
	strncpy(sendstring, input, strlen(input));
   	//send(fd , sendstring , strlen(sendstring) , 0); 
//	sleep(1);

    /*-------- process input string ----------*/
	vector<string> splitCommand;
	int cnt=splitString(input, splitCommand);

    if (splitCommand[0].compare("Stop") == 0)
         return -1;
	else if(splitCommand[0].compare("REGISTER") == 0) {
		user->name = splitCommand[1];
        if (cnt>=3)
			user->balance = atoi(splitCommand[2].c_str());
		else
			user->balance = 0;
		resultCode=0;
	}
	else if(splitCommand[0].compare("Connect") == 0) {
	//edit
	    string msg="<" + splitCommand[3] + ">#<" + splitCommand[4] + ">#<" +splitCommand[5] + ">"; 
//		cout << "Send to peer with myname:" << user->name << endl;
		peerSendMessage(user->name, msg, splitCommand[1], splitCommand[2], pri); 
		return 2;
	}
	else if(splitCommand[0].compare(user->name) == 0) {
        if(IsNumber(splitCommand[1].c_str()) != true)
            cout << "Port number format error" << endl;
		else {
			//user->port = atoi(portNum);
			user->port = atoi(splitCommand[1].c_str());
			//cout << "input port: " << user->port << endl;
			resultCode=1; /* return 1 to fork listen process */
		}
	}
	//edit
	//else if(splitCommand[0].compare("TryTest") == 0) {
		//char encString[]="<J>#<90000>#<M>";
      //  string enc=privateEncrypt(fd, encString, pri, pub);
		//return 0;
    //}
    if (strstr(input,"Exit") != NULL) 
         resultCode=99;
   	send(fd , sendstring , strlen(sendstring) , 0); 
	sleep(1);
    return resultCode;
}

void Receive2Print (int fd)
{
    int valread=0;
    char buffer[1024];
    memset(buffer, '\0', sizeof(buffer)); 
    valread = recv(fd, buffer, sizeof(buffer), 0);
    if (valread > 0)
        printf("%s\n", buffer);
   return;
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
   
//    printf("Welcome client: %s\n", inet_ntoa(echoClntAddr.sin_addr));

    return clntSock;
}

void *ThreadMain(void *threadArgs)
{
   int listenSock; /* Socket descriptor for client connection */
   int servSock; /* Socket descriptor for client connection */
   struct User *user;
   char buffer[RCVBUFSIZE]={0};
   ssize_t msgSize=0;

   /* Extract socket file descriptor from argument */
   listenSock = ((struct ThreadArgs *) threadArgs) -> listenSock;
   servSock = ((struct ThreadArgs *) threadArgs) -> servSock;
   //cout << "ThreadMain serverSock: " << servSock << endl;
   user = ((struct ThreadArgs *) threadArgs) -> user;
   string pubStrKey=*(((struct ThreadArgs *) threadArgs) -> pubStrKey);
   RSA *pubRSA= ((struct ThreadArgs *) threadArgs) -> pubRSA;
   RSA *priRSA= ((struct ThreadArgs *) threadArgs) -> priRSA;

   pthread_detach(pthread_self()); /* Guarantees that thread resources are deallocated upon return */

   free(threadArgs); /* Deallocate memory for argument */

   cout << "Ready for accept TCP" << endl;

   for (;;) {
       int clntSock=AcceptTCPConnection(listenSock);

       string peerName;
       if(recv(clntSock, buffer, RCVBUFSIZE, 0) > 0) 
	       peerName=buffer;
       cout << "Welcome:" << peerName << " from ";
       string initStr="Encrypted_Transaction_Request#" + peerName + "#" + user->name;
       if (send(servSock, initStr.c_str(), initStr.length(), 0) != initStr.length())
            DieWithError("send() failed");
    
       printSockInfo(clntSock);
   //edit client as server
       if (send(clntSock, "Connecting Peer Accepted\n", sizeof("Connecting Peer Accepted\n"), 0) != sizeof("Connecting Peer Accepted\n"))
            DieWithError("send() failed");

        /*------- Send public key to peer client -------*/
         sleep(1);
         cout << "PubKey to peer client:" << endl;
	     cout << pubStrKey << endl;
         send(clntSock , pubStrKey.c_str() , pubStrKey.length(), 0);

   		memset(buffer, '\0', sizeof(buffer));
    	while ((msgSize = recv(clntSock, buffer, RCVBUFSIZE, 0)) > 0) {
			char decrypt[msgSize];
   			memset(decrypt, '\0', msgSize);
			int totalResult=0;
        	int result = RSA_private_decrypt(msgSize, (unsigned char*)buffer, (unsigned char *)decrypt, priRSA, RSA_PKCS1_OAEP_PADDING);
			totalResult+=result;
   			memset(buffer, '\0', sizeof(buffer));
			if ((msgSize = recv(clntSock, buffer, RCVBUFSIZE, 0)) > 0)
        		result = RSA_private_decrypt(msgSize, (unsigned char*)buffer, (unsigned char *)&decrypt[totalResult], priRSA, RSA_PKCS1_OAEP_PADDING);
			totalResult+=result;
        	cout << "Decrypted length:" << totalResult << endl;
       		if (send(servSock, decrypt, totalResult, 0) != totalResult)
            	DieWithError("send() failed");
       		if (send(clntSock, "Any more transaction?\n", sizeof("Any more transaction?\n"), 0) != sizeof("Any more transaction?\n"))
            	DieWithError("send() failed");
    		memset(buffer, '\0', sizeof(buffer));
    	}
    	close(clntSock);
	}

   //pthread_detach(pthread_self()); /* Guarantees that thread resources are deallocated upon return */

  // free(threadArgs); /* Deallocate memory for argument */
   return (NULL);
}
string rsaGenKey(RSA &pri, RSA &pub) {
    RSA *private_key;
    RSA *public_key;

    char *encrypt = NULL;
    char *decrypt = NULL;

    char private_key_pem[] = "client_private_key";
    char public_key_pem[]  = "client_public_key";

    LOG(KEY_LENGTH);
    LOG(PUBLIC_EXPONENT);

    RSA *keypair = RSA_generate_key(KEY_LENGTH, PUBLIC_EXPONENT, NULL, NULL);
    LOG("Generate key has been created.");

    private_key = create_RSA(keypair, PRIVATE_KEY_PEM, private_key_pem);
    LOG("Private key pem file has been created.");

    public_key = create_RSA(keypair, PUBLIC_KEY_PEM, public_key_pem);
    LOG("Public key pem file has been created.");;

	pri=*private_key;
	pub=*public_key;

    ifstream ifsPri(private_key_pem);
    string priStrKey( (std::istreambuf_iterator<char>(ifsPri) ),
                       (std::istreambuf_iterator<char>()    ) );
    cout << priStrKey << endl;

    ifstream ifsPub(public_key_pem);
    string pubStrKey( (std::istreambuf_iterator<char>(ifsPub) ),
                       (std::istreambuf_iterator<char>()    ) );
    cout << pubStrKey << endl;
    remove("client_private_key");
    remove("client_public_key");

    return pubStrKey;
}
void rsaTesting(RSA *pri, RSA *pub) {
    char message[KEY_LENGTH / 8] = "OpenSSL_RSA demo testing";
	char *encrypt = NULL;
    char *decrypt = NULL;

 	encrypt = (char*)malloc(RSA_size(pub));
    int encrypt_length = public_encrypt(strlen(message) + 1, (unsigned char*)message, (unsigned char*)encrypt, pub, RSA_PKCS1_OAEP_PADDING);
    if(encrypt_length == -1) {
        LOG("An error occurred in public_encrypt() method");
    }

	cout << message << endl;
	cout << encrypt << endl;

	decrypt = (char *)malloc(encrypt_length);
    int decrypt_length = private_decrypt(encrypt_length, (unsigned char*)encrypt, (unsigned char*)decrypt, pri, RSA_PKCS1_OAEP_PADDING);
    if(decrypt_length == -1) {
        LOG("An error occurred in private_decrypt() method");
    }

	cout << "decryted: " << decrypt << endl;
	return;
}

string privateEncrypt(int fd, char* msg, RSA *pri, RSA *pub) {
//edit
	char initialTransact[]="Encrypted_Transaction_Request#M#J";
	send(fd, initialTransact, strlen(initialTransact), 0);
	sleep(1);

	char *encryptPri = NULL;
 	encryptPri = (char*)malloc(RSA_size(pri));
	char *encryptPub = NULL;
 	encryptPub = (char*)malloc(RSA_size(pub));

    int encrypt_length = RSA_private_encrypt(strlen(msg)+1, (unsigned char*)msg, (unsigned char*)encryptPri, pri, RSA_PKCS1_PADDING);
    if(encrypt_length == -1) {
       LOG("An error occurred in private_encrypt() method");
    }

	send(fd, encryptPri, encrypt_length, 0);
	sleep(1);

    char *decrypt=NULL;
    decrypt=(char *)malloc(encrypt_length);

    int decrypt_length = RSA_public_decrypt(encrypt_length, (unsigned char*)encryptPri, (unsigned char*)decrypt, pub, RSA_PKCS1_PADDING);
    if(decrypt_length == -1) {
        LOG("An error occurred in private_decrypt() method");
    }

    cout << "decrypted:" << decrypt << endl;
    return encryptPri;
}
RSA * pubKey2RSA(string name, string key)
{
    RSA *rsa=NULL;
    string filename=name + "_pubKey.PEM";
    ofstream out(filename);
    out << key;
    out.close();
      FILE *fp = NULL;
      fp = fopen(filename.c_str(), "rb");
    PEM_read_RSAPublicKey(fp, &rsa, NULL, NULL);
    remove(filename.c_str());
    if(rsa == NULL) {
        printf( "Failed to create RSA\n");
        return NULL;
    }
    else {
            ERR_load_ERR_strings();
            ERR_load_crypto_strings();
            unsigned long ulErr = ERR_get_error();
            char szErrMsg[1024] = {0};
            char *pTmp = NULL;
            pTmp = ERR_error_string(ulErr,szErrMsg);
            string errcode(szErrMsg);
            cout << errcode << endl;

        return rsa;
    }
}
