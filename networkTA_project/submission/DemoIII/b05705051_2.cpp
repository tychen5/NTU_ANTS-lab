#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <resolv.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <thread>
#include <openssl/ssl.h>
#include <openssl/err.h>

using namespace std;

int sockfd = 0;
char globalPort[10] = {};
SSL *ssl;
void paySocket();
void ShowCerts(SSL * ssl)
{
  X509 *cert;
  char *line;
 
  cert = SSL_get_peer_certificate(ssl);
  if (cert != NULL) {
    printf("Digital certificate information:\n");
    line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
    printf("Certificate: %s\n", line);
    free(line);
    line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
    printf("Issuer: %s\n", line);
    free(line);
    X509_free(cert);
  }
  else
    printf("No certificate information！\n");
}

int main(int argc , char *argv[])
{
	SSL_CTX *ctx;
  	SSL_library_init();
  	OpenSSL_add_all_algorithms();
  	SSL_load_error_strings();
  	ctx = SSL_CTX_new(SSLv23_client_method());

  	if (ctx == NULL)
  	{
    	ERR_print_errors_fp(stdout);
    	exit(1);
  	}
    //---------------------------------------------------

	//create a socket
	char* IP = argv[1];
	char* port = argv[2];
	sockfd = socket(AF_INET , SOCK_STREAM , 0);

	if (sockfd == -1){
		cout << "Fail to create a socket.";
	}

	//the connection of the socket

	struct sockaddr_in info;
	bzero(&info,sizeof(info));
	info.sin_family = PF_INET;
 
	//localhost test 140.112.107.194 port 33120
	//info.sin_addr.s_addr = inet_addr("127.0.0.1");
	info.sin_addr.s_addr = inet_addr(IP);
	//info.sin_port = htons(8700);
	info.sin_port = htons(atoi(port));

	int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
	if(err==-1){ // if connection error
		cout << "Connection error\n"; exit(-1);
	}
	else{
		cout << "connection accepted\n";
	}

	//--------------------------------
  	ssl = SSL_new(ctx);
  	SSL_set_fd(ssl, sockfd);
  	if (SSL_connect(ssl) == -1)
    	ERR_print_errors_fp(stderr);
  	else
  	{
    	printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
    	ShowCerts(ssl);
  	}
	//--------------------------------

	char receiveMessage[500] = {};
	char regisSuccess[20] = {"100 OK\n"};
	char regisFail[20] = {"210 FAIL\n"};
	char logInFail[20] = {"220 AUTH_FAIL\n"};
	bool logInSuccess = false;
	char globalUserName[20] = {};
	char command[10] = {};
	
	//Send a message to server
	bool leaveServer = false;
	while(1){
		cout << "1 for register. 2 for log in. 3 for exit.\n";
		bzero(&command, sizeof(command));
		bzero(&receiveMessage, sizeof(receiveMessage));
		cin >> command;
		if (atoi(command) == 1){
			// register
			bzero(&command, sizeof(command));
			cout << "please enter your name.\n";
			char username[100] = {};
			cin >> username;
			char registerMessage[100] = {"REGISTER#"};
			strcat(registerMessage, username);
			SSL_write(ssl,registerMessage,strlen(registerMessage));
			SSL_read(ssl,receiveMessage,sizeof(receiveMessage));
			//send(sockfd,registerMessage,strlen(registerMessage),0);
			//recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
			if (strcmp(receiveMessage, regisFail) == 0){
				cout << "you have registered before.\n";
			}
			else if (strcmp(receiveMessage, regisSuccess) == 0){
				cout << "register successfully!!.\n";
			}
		}
		else if (atoi(command) == 2){
			// log in
			bzero(&command, sizeof(command));
			cout << "please enter your name.\n";
			char username[100] = {};
			cin >> username;
			cout << "please enter your port number.\n";
			cout << "your port number must >= 1024 and <= 65536.\n";
			char portNumber[10] = {};
			bool portRight = false;
			while(!portRight){
				bzero(&portNumber, sizeof(portNumber));
				cin >> portNumber;
				if (atoi(portNumber) >= 1024 && atoi(portNumber) <= 65536){
					portRight = true;
				}
				else{
					// port number out of range
					cout << "your port number is out of range!!\n";
					cout << "please type it again!!\n";
				}
			}
			char logInMessage[100] = {};
			strcat(logInMessage, username);
			strcat(logInMessage, "#");
			strcat(logInMessage, portNumber);
			strcat(globalUserName, username);
			strcat(globalPort, portNumber);
			SSL_write(ssl, logInMessage, strlen(logInMessage));
			SSL_read(ssl,receiveMessage,sizeof(receiveMessage));
			//send(sockfd,logInMessage,strlen(logInMessage),0);
			//recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
			if (strcmp(receiveMessage, logInFail) == 0){
				// 220 AUTH_FAIL
				cout << "this account has not be registered or its logging in or the port is occupied.\n";
				bzero(&globalUserName, sizeof(globalUserName));
				bzero(&globalPort, sizeof(globalPort));
			}
			else{
				cout << "log in successfully!!\n";
				cout << receiveMessage << "\n";
				logInSuccess = true;
			}
		}
		else if (atoi(command) == 3){
			// exit the client can close the client
			bzero(&command, sizeof(command));
			cout << "exit the client!!\n";
			break;
		}
		else{
			// you are doing something wrong
			bzero(&command, sizeof(command));
			cout << "you are doing wrong!!\n";
		}
		if (logInSuccess){ // log in success
			thread myThread(paySocket);
			//myThread.join();
			cout << "now things you can do are as follows:\n";
			char receiveMessage2[500] = {};
			while(1){ // the loop while logging in success
				bzero(&receiveMessage2, sizeof(receiveMessage2));
				cout << "1 for list the users online. 2 for exit. 3 for pay amount to others\n";
				cin >> command;
				if (atoi(command) == 1){ // list
					bzero(&command, sizeof(command));
					char list[5] = {"List"};
					SSL_write(ssl,list,strlen(list));
					SSL_read(ssl,receiveMessage2,sizeof(receiveMessage2));
					//send(sockfd,list,strlen(list),0);
					//recv(sockfd,receiveMessage2,sizeof(receiveMessage2),0);
					cout << receiveMessage2 << "\n";
				}
				else if (atoi(command) == 2){ // exit
					bzero(&command, sizeof(command));
					logInSuccess = false;
					char exit[5] = {"Exit"};
					SSL_write(ssl,exit,strlen(exit));
					SSL_read(ssl,receiveMessage2,sizeof(receiveMessage2));
					//send(sockfd,exit,strlen(exit),0);
					//recv(sockfd,receiveMessage2,sizeof(receiveMessage2),0);
					cout << receiveMessage2 << "\n";
					cout << "now you leave the server!!\n";
					leaveServer = true;
					break;
				}
				else if (atoi(command) == 3){ // pay cash to others
					bzero(&command, sizeof(command));
					char payCash[100] = {};
					char payAmount[10] = {};
					char payee[20] = {};
					cout << "please enter the user you want to pay to." << endl;
					cin >> payee;
					cout << "please enter the amount you want to pay." << endl;
					cin >> payAmount;
					if (strcmp(payee, globalUserName) == 0){
						cout << "you can't pay cash to yourself in this system." << endl;
					}
					else{
						char pay[4] = {"pay"};
						strcat(payCash, pay);
						strcat(payCash, "#");
						strcat(payCash, globalUserName);
						strcat(payCash, "#");
						strcat(payCash, payAmount);
						strcat(payCash, "#");
						strcat(payCash, payee);
						SSL_write(ssl,payCash,strlen(payCash));
						SSL_read(ssl,receiveMessage2,sizeof(receiveMessage2));
						//send(sockfd,payCash,strlen(payCash),0);
						//recv(sockfd,receiveMessage2,sizeof(receiveMessage2),0);
						if (strncmp(receiveMessage2, "successPayFind", 14) != 0){
							if (strcmp(receiveMessage2, "failPayFind") == 0)
								cout << "this payee has not registered." << endl;
							else if (strcmp(receiveMessage2, "noEnoughMoney") == 0)
								cout << "you don't have enough money." << endl;
						}
						else{
							cout << "successfully find the IP and Port" << endl;
							//bzero(&receiveMessage2, sizeof(receiveMessage2));
							//recv(sockfd,receiveMessage2,sizeof(receiveMessage2),0);
							cout << receiveMessage2 << endl;
							int countSpace = 0;
							char targetIP[20] = {};
							int countTarIP = 0;
							char targetPort[10] = {};
							int countTarPort = 0;
							for (int i = 0; i < strlen(receiveMessage2); i++){
								if (countSpace == 1){
									targetIP[countTarIP] = receiveMessage2[i];
									countTarIP++;
								}
								else if (countSpace == 2){
									targetPort[countTarPort] = receiveMessage2[i];
									countTarPort++;
								}
								if (receiveMessage2[i] == ' '){
									countSpace++;
								}
							}
							//-----------
							SSL *ssl3;
							SSL_CTX *ctx3;

  							SSL_library_init();
  							OpenSSL_add_all_algorithms();
  							SSL_load_error_strings();
  							ctx3 = SSL_CTX_new(SSLv23_client_method());

  							if (ctx3 == NULL)
  							{
    							ERR_print_errors_fp(stdout);
    							exit(1);
  							}
							//-----------
							int sockfd3 = 0;
							sockfd3 = socket(AF_INET , SOCK_STREAM , 0);
							if (sockfd3 == -1){
								cout << "Fail to create a socket.";
							}
							struct sockaddr_in info2;
							bzero(&info,sizeof(info2));
							info2.sin_family = PF_INET;
 
							info2.sin_addr.s_addr = inet_addr(targetIP);
							info2.sin_port = htons(atoi(targetPort));

							int err = connect(sockfd3,(struct sockaddr *)&info2,sizeof(info2));
							//--------------------------------
  							ssl3 = SSL_new(ctx3);
  							SSL_set_fd(ssl3, sockfd3);
  							if (SSL_connect(ssl3) == -1)
    							ERR_print_errors_fp(stderr);
  							else
  							{
    							//printf("Connected with %s encryption\n", SSL_get_cipher(ssl3));
    							ShowCerts(ssl3);
  							}
							//--------------------------------
							if(err==-1){ // if connection error
								cout << "fail pament! please try again!\n";
								close(sockfd3);
							}
							else{
								cout << "successful payment!\n";
								SSL_write(ssl3, payCash, strlen(payCash));
								//send(sockfd3, payCash, strlen(payCash),0);
								close(sockfd3);
							}
						}
					}
				}
			}
		}
		if (leaveServer){
			break;
		}
	}
	close(sockfd);
	return 0;
}
void paySocket(){
	SSL_CTX *ctx2;
  	char pwd2[100];
  	char* temp2;

  	SSL_library_init();
  	OpenSSL_add_all_algorithms();
  	SSL_load_error_strings();
  	ctx2 = SSL_CTX_new(SSLv23_server_method());
  	if (ctx2 == NULL)
  	{
    	ERR_print_errors_fp(stdout);
    	exit(1);
  	}

  	getcwd(pwd2,100);
  	if(strlen(pwd2)==1)
    	pwd2[0]='\0';
  	if (SSL_CTX_use_certificate_file(ctx2, temp2=strcat(pwd2,"/cert/servercert.pem"), SSL_FILETYPE_PEM) <= 0)
  	{
    	ERR_print_errors_fp(stdout);
    	exit(1);
  	}
  	
  	getcwd(pwd2,100);
  	if(strlen(pwd2)==1)
    	pwd2[0]='\0';
  	if (SSL_CTX_use_PrivateKey_file(ctx2, temp2=strcat(pwd2,"/cert/serverkey.pem"), SSL_FILETYPE_PEM) <= 0)
  	{
    	ERR_print_errors_fp(stdout);
    	exit(1);
  	}
  	
  	if (!SSL_CTX_check_private_key(ctx2))
  	{
    	ERR_print_errors_fp(stdout);
    	exit(1);
  	}
	//------------------------------------
	int sockfd2 = 0;
	sockfd2 = socket(AF_INET , SOCK_STREAM , 0);
	if (sockfd2 == -1){
		printf("Fail to create a socket.");
	}
	struct sockaddr_in clientSelfInfo, clientTargetInfo;
	socklen_t addrlen = sizeof(clientTargetInfo);
	bzero(&clientSelfInfo,sizeof(clientSelfInfo));

	clientSelfInfo.sin_family = PF_INET;
	clientSelfInfo.sin_addr.s_addr = INADDR_ANY;
	clientSelfInfo.sin_port = htons(atoi(globalPort));
	size_t bn = ::bind(sockfd2,(struct sockaddr *)&clientSelfInfo, sizeof(clientSelfInfo));
	if (bn < 0) {
		cerr << "Bind error: " << errno << endl; exit(-1);
	}
	listen(sockfd2,5);
	while(1){
		SSL *ssl2;
		int forClientTargetSockfd = 0;
		forClientTargetSockfd = accept(sockfd2,(struct sockaddr*) &clientTargetInfo, &addrlen);
		//cout << "connected" << endl;
		ssl2 = SSL_new(ctx2);
    	SSL_set_fd(ssl2, forClientTargetSockfd);
    	if (SSL_accept(ssl2) == -1)
    	{
      		perror("accept");
      		close(forClientTargetSockfd);
      		break;
    	}	
		char inputBuffer[256] = {'\0'};
		int r = SSL_read(ssl2,inputBuffer,sizeof(inputBuffer));
		cout << "inputBuffer : " << inputBuffer << endl;
		if (r == -1) {
			cerr << "recv error, errno: " << errno << endl;
		}       
		if (r != 0){
			cout << "successfully send to server." << endl;
			char wantToSend[100] = {};
			strcat(wantToSend, "2pay");
			strcat(wantToSend, "#");
			strcat(wantToSend, inputBuffer);
			SSL_write(ssl, wantToSend, strlen(wantToSend));
			//send(sockfd, wantToSend, strlen(wantToSend), 0);
		}
	}
}