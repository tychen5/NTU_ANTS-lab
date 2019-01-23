#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sstream>

using namespace std;
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;
#define BufferLength 1000    // length of buffer
#define BACKLOG 5
char buffer[BufferLength];
void* miniServer(void* data);
int n;
string temp;
int errno;
string portNumToSend;
SSL *ssl;
char CLIENT_CERT[BufferLength] = "./cacert.pem";
char CLIENT_PRI[BufferLength] = "./cakey.pem";
char portnum[BufferLength];
string clientName;

SSL_CTX* initCTX(void)
{
    const SSL_METHOD *ssl_method;
    SSL_CTX *ctx;
    
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ssl_method = TLSv1_client_method();
    ctx = SSL_CTX_new(ssl_method);
    if(ctx == NULL){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

SSL_CTX* initCTXServer(void)
{   
    const SSL_METHOD *ssl_method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ssl_method = TLSv1_server_method();
    ctx = SSL_CTX_new(ssl_method);
    if(ctx == NULL)
    {
        //ERR_print_errors_fp(stderr);
        cout << "iniError" << endl;
        abort();
    }
    return ctx;
}

void ShowCerts(SSL *ssl)
{
    X509 *cert;
    char *line;
    
    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        //line = " /C=TW/ST=Taiwan/L=Taipei/O=NTU/OU=IM/CN=mileszero/emailAddress=nightlight870213@gmail.com";
        printf("Subject: %s\n", line);
        free(line);       /* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        //line = " /C=TW/ST=Taiwan/L=Taipei/O=NTU/OU=IM/CN=mileszero/emailAddress=nightlight870213@gmail.com";
        printf("Issuer: %s\n", line);
        free(line);       /* free the malloc'ed string */
        X509_free(cert);     /* free the malloc'ed certificate copy */
    }
    else
        printf("No certificates.\n");
}

void LoadCertificatesServer(SSL_CTX* ctx)
{

    if (SSL_CTX_use_certificate_file(ctx, CLIENT_CERT, SSL_FILETYPE_PEM) <= 0)
    {
        //cout << "load1" << endl;
        ERR_print_errors_fp(stderr);
        abort();
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx, CLIENT_PRI, SSL_FILETYPE_PEM) <= 0)
    {
        //cout << "load1" << endl;
        ERR_print_errors_fp(stderr);
        abort();
    }
    
    if (!SSL_CTX_check_private_key(ctx))
    {
        ERR_print_errors_fp(stderr);
        cout << "Private key does not match the public certification." << endl;
        abort();
    }

    // SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    // SSL_CTX_set_verify_depth(ctx, 4);

}

void LoadCertificates(SSL_CTX* ctx)
{
    if(SSL_CTX_use_certificate_file(ctx, CLIENT_CERT, SSL_FILETYPE_PEM) <= 0)
    {
        //cout << "load1" << endl;
        ERR_print_errors_fp(stderr);
        abort();
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx, CLIENT_PRI, SSL_FILETYPE_PEM) <= 0)
    {
        //cout << "load1" << endl;
        ERR_print_errors_fp(stderr);
        abort();
    }
    
    if (!SSL_CTX_check_private_key(ctx))
    {
        //cout << "load1" << endl;
        ERR_print_errors_fp(stderr);
        cout << "--> Private key does not match the public certification." << endl;
        abort();
    }
}



void registerOption(char* buffer, int n, int mysocket)
{
	bzero(buffer, BufferLength);
	cout << " Enter the name you want to register: ";
	string registerName1;
	cin >> registerName1;
    cout << " Enter the balance of the register: ";
    string registerName2;
    cin >> registerName2;
    string registerName;
	registerName = "Register" + registerName1 + "@" + registerName2;
    SSL_write(ssl, registerName.c_str(), registerName.length());
	//send(ssl, registerName.c_str(), registerName.size());
	SSL_read(ssl, buffer, sizeof(buffer));
    cout << buffer << endl;
}

void loginOption(char* buffer, int n, int mysocket)
{
	memset((char *)buffer, 0, BufferLength);
	cout << " Enter username: ";
	string userName; 
	cin >> userName;
    clientName = userName;
	cout << " Enter the port number: ";

	cin >> portnum;
	userName = "login"+userName + "#" + portnum;
    SSL_write(ssl, userName.c_str(), userName.length());
	//send(mysocket, userName.c_str(), userName.size(), 0);
    memset((char *)buffer, 0, BufferLength);
    SSL_read(ssl, buffer, BufferLength);
	//n = recv(mysocket, buffer, BufferLength, 0);
    cout << buffer << endl;
}

void listOption(char* buffer, int n, int mysocket)
{
	string msg;
	msg= "List";
    SSL_write(ssl, msg.c_str(), msg.length());
	//send(mysocket, msg.c_str(), msg.size(), 0);
	memset((char *)buffer, 0, BufferLength);
    SSL_read(ssl, buffer, BufferLength);
	//n = recv(mysocket, buffer, BufferLength, 0);
    cout << buffer << endl;
}

void quitOption(SSL_CTX *ctx, char* buffer, int n, int mysocket)
{
	string msg;
	msg= "Exit";
	SSL_write(ssl, msg.c_str(), msg.length());
	memset((char *)buffer, 0, BufferLength);
    SSL_read(ssl, buffer, sizeof(buffer));
	//n = recv(mysocket, buffer, BufferLength, 0);
	cout << "bye" << endl;
    close(mysocket);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    exit(0);
}

void giveMoneyOption(char* buffer, int n, int mysocket)
{
    string msg;
    string DestUser;
    string money;
    struct sockaddr_in connect_addr;
    char connectBuffer[BufferLength];
    
    bool requestSuccess;
    while (!requestSuccess)
    {
        cout << " Enter account port number you want to give: " ;
        cin >> DestUser;
        cout << " Enter the money you want to give: " ;
        cin >> money;
        msg = "FindAccountValid";
        msg = msg + DestUser + "&" + money;
        SSL_write(ssl, msg.c_str(), msg.length());
        memset((char *)buffer, 0, BufferLength);
        SSL_read(ssl, buffer, sizeof(buffer));
        if (buffer[0] == '0')
        {
            cout << " The account is not exist, please enter the valid account port number." << endl;
        }
        else
        {
            requestSuccess = true;
        }
    }

    int socketC2C = socket(PF_INET, SOCK_STREAM, 0);
    if(socketC2C < 0)
    {
        cout << "ERROR creating socket.\n\n";
    }

    memset((char *)buffer, 0, sizeof(buffer));
    connect_addr.sin_family = AF_INET;
    connect_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    connect_addr.sin_port = htons(stoi(DestUser));

    if(connect(socketC2C, (struct sockaddr*)&connect_addr, sizeof(connect_addr)) < 0)
    {
        cout << "ERROR Connecting. \n" << endl;
    }

    SSL_CTX *ctx1;
    SSL *ssl1;    // ssl1 use to connect between client to client
                  // ssl1 <---> ssl2
    ctx1 = initCTX();
    LoadCertificates(ctx1);
    ssl1 = SSL_new(ctx1);
    SSL_set_fd(ssl1, socketC2C);
    SSL_connect(ssl1);

    memset((char *)connectBuffer, 0, BufferLength);
    SSL_read(ssl1, connectBuffer, BufferLength);    // read from client2 *** (msgs)
    cout << connectBuffer << endl;

    string temp = clientName + "#" + money;

    SSL_write(ssl1, temp.c_str(), temp.length());    // write to client2 ^^^ (info, name + money)

    memset((char *)connectBuffer, 0, BufferLength);
    SSL_read(ssl1, connectBuffer, BufferLength);    // read from client2 %%% (msgs)
    cout << connectBuffer << endl;
    SSL_free(ssl1);

}


int main(int argc, char *argv[])
{

	// Wrong Input Format
	string fstSeg = argv[0];
	// if the argument number is less than 3, the input format is wrong
	if(argc < 3)    
	{
		// if the format is wrong, it will diaplay:
		// ERROR: Correct Inout Format: ./client hostname port
		cout << "ERROR: Correct Input Format: " + fstSeg + " hostname port\n";
		exit(0);
	}
	
    // creating socket
    int mysocket;
    mysocket = socket(AF_INET, SOCK_STREAM, 0);    // (IPv4, TCP, 0)
    if(mysocket < 0)    // if sd = -1, create error
    {
        cout << "ERROR creating socket.\n";
        exit(0);
    }

	// Address
	struct sockaddr_in suckit;
	memset((char *) &suckit, 0, sizeof(suckit));    // initialize to 0
	suckit.sin_addr.s_addr = inet_addr(argv[1]);    // convert hostname
	suckit.sin_family = AF_INET;    // comunnitation type
	suckit.sin_port = htons(atoi(argv[2]));    // port number

	

	

	// connecting
	if (connect(mysocket, (struct sockaddr*)&suckit, sizeof(suckit)) < 0)
	{
		cout << errno << " ERROR connecting.\n";
		exit(0);
	}

    // SSL
    SSL_CTX *ctx;
    SSL_library_init();
    ctx = initCTX();
    LoadCertificates(ctx);

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, mysocket);


    if(SSL_connect(ssl) <= 0)
        ERR_print_errors_fp(stderr);
    else
    {
        
        memset((char *)buffer, 0, BufferLength);
        ShowCerts(ssl);

        int n = SSL_read(ssl, buffer, sizeof(buffer));
        if (n < 0)
        {
            cout << "fail to receive.\n";
            exit(1);
        }
        cout << buffer << endl;

        // connected!
        
        while(true)
        {
        //two options to choose
            string opt; 
            cout<<"Enter 1 for Register, 2 for Login: ";
            cin>>opt;

            //Register
            if(opt=="1")
            {
                registerOption(buffer, n, mysocket);
            }

            //Login
            else if(opt=="2")
            {
                //LOGIN
                loginOption(buffer, n, mysocket);       

                string action;
                string msg;
                
                pthread_t tid;
                pthread_create(&tid, NULL, &miniServer, NULL);

                while(strcmp(temp.c_str(), "220 AUTH_FAIL\n")!=0)
                {
                    
                    
                    cout<<"Enter the number of actions you want to take.\n";
                    cout<<"3 to ask for the latest list, 4 to give money to other client, 5 to Exit:";
                    
                    cin>>action;
                    if(action=="3")
                    {
                        listOption(buffer, n, mysocket);
                    }
                    else if(action=="5")
                    {
                        quitOption(ctx, buffer, n, mysocket);
                    }
                    else if (action=="4")
                    {
                        giveMoneyOption(buffer, n, mysocket);
                        //cout << "111111111111111111111111111" << endl; 
                    }
                }
            }
        }
    }
    
	
    
}

void* miniServer(void* data)
{
    int myNewsocket;
    int newSocket;
    string portNumToSend(portnum);    // the port num who give me money
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    char bufferServer[BufferLength];
    char bufferClient[BufferLength];

    // socket create
    
    myNewsocket = socket(PF_INET, SOCK_STREAM, 0);    // (IPv4, TCP, 0)
    if(myNewsocket < 0)    // if sd = -1, create error
    {
        cout << "ERROR creating socket.\n";
        exit(1);
    }
    
    // socket initial
    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(stoi(portNumToSend));
 
    // binding
    int bind2 = bind(myNewsocket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(bind2 < 0)
    {
        cout << "Error binding.";
        exit(1);
    }
    
    socklen_t clientLen = sizeof(client_addr);
    
    listen(myNewsocket, BACKLOG);
    
    //SSL
    SSL_CTX *ctx2;
    SSL_library_init();
    SSL *ssl2;
    
    //someone connect me
    while((newSocket = accept(myNewsocket, (struct sockaddr *)&client_addr, &clientLen)))
    {
        if(newSocket >= 0)
        {
            pthread_mutex_lock(&mutex_lock);
            
            ctx2 = initCTXServer();
            LoadCertificatesServer(ctx2);
            ssl2 = SSL_new(ctx2);
            SSL_set_fd(ssl2, newSocket);
            SSL_accept(ssl2);
            
            memset((char *)bufferServer, 0, BufferLength);
            string msg = "Already Connected with another client.\n";
            SSL_write(ssl2, msg.c_str(), msg.length());     // write to client1 *** (msgs)
            
            pthread_mutex_unlock(&mutex_lock);
            
            string nameFrom, moneyFrom;
            memset((char *)bufferServer, 0, BufferLength);
            SSL_read(ssl2, bufferServer, BufferLength);    // read from client 1 ^^^ (info, name + money)
            string temp(bufferServer);
            int pos = temp.find("#");
            nameFrom = temp.substr(0, pos);
            moneyFrom = temp.substr(pos + 1);

            // cout << "\nNew connection." << endl;
            cout << "\n New connection from client: " << nameFrom << endl;
            
            cout << "                    gives you: " << moneyFrom << " dollars." << endl;
            
            string money(bufferServer);
            string msg2 = "Transfer" + nameFrom + "&" + moneyFrom;
            SSL_write(ssl, msg2.c_str(), msg2.length());   

            
            memset((char *)bufferServer, 0, BufferLength);
            SSL_read(ssl, bufferServer, BufferLength);   // ssl connect with server
            string t(bufferServer);
            int pos1 = t.find("#");
            string balanceNow = t.substr(0, pos1);
            string b = t.substr(pos1+1);
            cout << "Your Current Balance: " << balanceNow << endl;
            cout << b << endl;
            SSL_write(ssl2, b.c_str(), b.length());  // ssl2 connect with client
                                                                  // write to client1 (msgs) %%%
            
            cerr<<"Enter the number of actions you want to take.\n";
            cerr<<"3 to ask for the latest list, 4 to give money to other client, 5 to Exit:";
            //break;
        }
        
    }
    
    // close server socket
    close(myNewsocket);
    SSL_CTX_free(ctx2);
    
    return 0;
    
}







