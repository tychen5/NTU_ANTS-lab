#include <string.h>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <deque>
#include "threadPool.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
using namespace std;

char userAccountRegistered[100][20];
char userOnline[100][50];
int balance[100];
int countRegistered = 0;
int countLogIn = 0;
int sockfd = 0;
int TOTALTHREAD = 10;

int lookForBalanceNum(char name[]);
char* list(char nowUser[]);
char* regis(char inputBuffer[]);
char* logIn(char inputBuffer[], char clientIP[]);
char* getNowUser(char inputBuffer[]);
int main(int argc , char *argv[])
{
	SSL_CTX *ctx;
  	char pwd[100];
  	char* temp;

  	SSL_library_init();
  	OpenSSL_add_all_algorithms();
  	SSL_load_error_strings();
  	ctx = SSL_CTX_new(SSLv23_server_method());
  	if (ctx == NULL)
  	{
    	ERR_print_errors_fp(stdout);
    	exit(1);
  	}

  	getcwd(pwd,100);
  	if(strlen(pwd)==1)
    	pwd[0]='\0';
  	if (SSL_CTX_use_certificate_file(ctx, temp=strcat(pwd,"/cert/servercert.pem"), SSL_FILETYPE_PEM) <= 0)
  	{
    	ERR_print_errors_fp(stdout);
    	exit(1);
  	}
  	
  	getcwd(pwd,100);
  	if(strlen(pwd)==1)
    	pwd[0]='\0';
  	if (SSL_CTX_use_PrivateKey_file(ctx, temp=strcat(pwd,"/cert/serverkey.pem"), SSL_FILETYPE_PEM) <= 0)
  	{
    	ERR_print_errors_fp(stdout);
    	exit(1);
  	}
  	
  	if (!SSL_CTX_check_private_key(ctx))
  	{
    	ERR_print_errors_fp(stdout);
    	exit(1);
  	}

	//------------------------------------------
	ThreadPool pool(TOTALTHREAD);
	sockfd = socket(AF_INET , SOCK_STREAM , 0);

	if (sockfd == -1){
		printf("Fail to create a socket.");
	}

	//socket的連線
	//struct sockaddr_in serverInfo,clientInfo;
	struct sockaddr_in serverInfo, clientInfo;
	socklen_t addrlen = sizeof(clientInfo);
	bzero(&serverInfo,sizeof(serverInfo));

	serverInfo.sin_family = PF_INET;
	serverInfo.sin_addr.s_addr = INADDR_ANY;
	serverInfo.sin_port = htons(8877);
	size_t bn = ::bind(sockfd,(struct sockaddr *)&serverInfo, sizeof(serverInfo));
	if (bn < 0) {
		cerr << "Bind error: " << errno << endl; exit(-1);
	}

	listen(sockfd,TOTALTHREAD);
	
	while(1){
		// struct sockaddr_in clientInfo;
		// socklen_t addrlen = sizeof(clientInfo);
		SSL *ssl;
		int forClientSockfd = 0;
		forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);
		ssl = SSL_new(ctx);
    	SSL_set_fd(ssl, forClientSockfd);
    	if (SSL_accept(ssl) == -1)
    	{
      		perror("accept");
      		close(forClientSockfd);
      		break;
    	}	
		pool.enqueue([&](int forClientSockfd, SSL* ssl){
		while(1){
			cout << "client" << forClientSockfd << "accepted" << endl;
			char clientIP[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(clientInfo.sin_addr), clientIP, INET_ADDRSTRLEN);
			char inputBuffer[256] = {'\0'};
			char nowUser[20]; 
			while(1){
				bzero(&inputBuffer, sizeof(inputBuffer));
				int r = SSL_read(ssl,inputBuffer,sizeof(inputBuffer));
				cout << "hi" << inputBuffer << endl;
				if (r == -1) {
					cerr << "recv error, errno: " << errno << endl; return;
				}
				if (r == 0){
					cout << "client" << forClientSockfd << "shut down" << endl;
					char tempName[20];
					char tempOnlineInfo[100];
					for (int i = 0; i < 100; i++){
						strcpy(tempOnlineInfo, userOnline[i]);
						int tempNamelen = 0;
						for (int j = 0; j < strlen(tempOnlineInfo); j++){
							if (tempOnlineInfo[j] == ' '){
								tempNamelen = j;
									break;
							}
							else{
								tempName[j] = tempOnlineInfo[j];
							}
						}
						tempName[tempNamelen] = '\0';
						if (strcmp(tempName, nowUser) == 0){
							cout << "delete" << endl;
							bzero(&userOnline[i],sizeof(userOnline[i]));
							break;
						}
						bzero(&tempName,sizeof(tempName));
						bzero(&tempOnlineInfo,sizeof(tempOnlineInfo));
					}
					countLogIn -= 1;
					return;
				}
				else if (r != 0){
					if (strcmp(inputBuffer, "List") == 0){
						cout << "Server receive command: List" << endl;
						SSL_write(ssl,list(nowUser),strlen(list(nowUser)));
						//send(forClientSockfd,list(nowUser),strlen(list(nowUser)),0);
					}
					// user register
					else if (strncmp(inputBuffer, "REGISTER#", 9) == 0){
						cout << "Server receive command: REGISTER" << endl;
						char goal[20] = {};
						strcpy(goal, regis(inputBuffer));
						SSL_write(ssl,goal,strlen(goal));
						//send(forClientSockfd,goal,strlen(goal),0);
						cout << "finishREGISSend" << endl;
					}
					// end register
					else if (strncmp(inputBuffer, "Exit", 4) == 0){
						cout << "Server receive command: Exit" << endl;
						char bye[] = {"Bye\n"};
						//cout << "now leaving : " << nowUser << endl;
						char tempName[20];
						char tempOnlineInfo[100];
						for (int i = 0; i < 100; i++){
							strcpy(tempOnlineInfo, userOnline[i]);
							int tempNamelen = 0;
							for (int j = 0; j < strlen(tempOnlineInfo); j++){
								if (tempOnlineInfo[j] == ' '){
									tempNamelen = j;
									break;
								}
								else{
									tempName[j] = tempOnlineInfo[j];
								}
							}
							tempName[tempNamelen] = '\0';
							if (strcmp(tempName, nowUser) == 0){
								bzero(&userOnline[i],sizeof(userOnline[i]));
								break;
							}
							bzero(&tempName,sizeof(tempName));
							bzero(&tempOnlineInfo,sizeof(tempOnlineInfo));
						}
						countLogIn -= 1;
						SSL_write(ssl,bye,strlen(bye));
						//send(forClientSockfd,bye,strlen(bye),0);
						return;
					}
					else if (strncmp(inputBuffer, "pay", 3) == 0){
						//bzero(&inputBuffer, sizeof(inputBuffer));
						cout << "enter pay" << endl;
						//r = recv(forClientSockfd,inputBuffer,sizeof(inputBuffer),0);
						cout << "inputBuffer : " << inputBuffer << endl;
						if (true){
							char payer[20] = {};
							int countPayer = 0;
							char payee[20];
							int countPayee = 0;
							char payAmount[20] = {};
							int countAmount = 0;
							int numOfSharp = 0;
							for (int i = 0; i < strlen(inputBuffer); i++){
								if (numOfSharp == 1 && inputBuffer[i] != '#'){
									payer[countPayer] = inputBuffer[i];
									countPayer++;
								}
								else if (numOfSharp == 2 && inputBuffer[i] != '#'){
									payAmount[countAmount] = inputBuffer[i];
									countAmount++;
								}
								else if (numOfSharp == 3){
									payee[countPayee] = inputBuffer[i];
									countPayee++;
								}
								if (strncmp(&inputBuffer[i], "#", 1) == 0){
									numOfSharp++;
								}
							}
							cout << "payer : " << payer << endl;
							cout << "payee : " << payee << endl;
							//cout << tempNamelen << endl;
							int record = -1;
							int record1 = -1;
							for (int i = 0; i < 100; i++){ //check whether the user is logging in now
								if (strncmp(userOnline[i], payee, countPayee) == 0){
									if (userOnline[i][countPayee] == ' '){
										record = i;
										break;
									}
								}
							}
							for (int i = 0; i < 100; i++){ //check whether the user is logging in now
								if (strncmp(userOnline[i], payer, countPayer) == 0){
									if (userOnline[i][countPayer] == ' '){
										record1 = i;
										break;
									}
								}
							}
							cout << "payAmount : " << payAmount << endl;
							cout << "balance : " << balance[record1] << endl;
							if (record != -1){
								if (atoi(payAmount) > balance[record1]){
									char hasNoMoney[] = {"noEnoughMoney"};
									SSL_write(ssl,hasNoMoney,strlen(hasNoMoney));
									//send(forClientSockfd,hasNoMoney,strlen(hasNoMoney),0);
								}
								else{
									char successPayFind[] = {"successPayFind"};
									strcat(successPayFind, "#");
									strcat(successPayFind, userOnline[record]);
									SSL_write(ssl,successPayFind,strlen(successPayFind));
									//send(forClientSockfd,successPayFind,strlen(successPayFind),0);
								}
							}
							else{
								cout << "fail pay!" << endl;
								char failPayFind[] = {"failPayFind"};
								SSL_write(ssl,failPayFind,strlen(failPayFind));
								//send(forClientSockfd,failPayFind,strlen(failPayFind),0);
							}
						}
					}
					else if (strncmp(inputBuffer, "2pay", 4) == 0){
						//bzero(&inputBuffer, sizeof(inputBuffer));
						//r = recv(forClientSockfd,inputBuffer,sizeof(inputBuffer),0);
						cout << "hi" << inputBuffer << endl;
						char payer[20] = {};
						int countPayer = 0;
						char payee[20] = {};
						int countPayee = 0;
						char payAmount[20] = {};
						int countAmount = 0;
						int numOfSharp = 0;
						for (int i = 0; i < strlen(inputBuffer); i++){
							if (numOfSharp == 2 && inputBuffer[i] != '#'){
								payer[countPayer] = inputBuffer[i];
								countPayer++;
							}
							else if (numOfSharp == 3 && inputBuffer[i] != '#'){
								payAmount[countAmount] = inputBuffer[i];
								countAmount++;
							}
							else if (numOfSharp == 4){
								payee[countPayee] = inputBuffer[i];
								countPayee++;
							}
							if (inputBuffer[i] == '#'){
								numOfSharp++;
							}
						}
						cout << payer << endl;
						int payerIndex = -1;
						int payeeIndex = -1;
						for (int i = 0; i < countRegistered; i++){
							if (strcmp(userAccountRegistered[i], payer) == 0)
								payerIndex = i;
						}
						for (int i = 0; i < countRegistered; i++){
							if (strcmp(userAccountRegistered[i], payee) == 0)
								payeeIndex = i;
						}
						cout << payerIndex << endl;
						balance[payerIndex] -= atoi(payAmount);
						balance[payeeIndex] += atoi(payAmount);
						cout << "success pay" << endl;
					}
					else{ // log
						cout << "Server receive command: log" << endl;
						char goal[20] = {};
						strcpy(nowUser, getNowUser(inputBuffer));
						strcpy(goal, logIn(inputBuffer, clientIP));
						SSL_write(ssl, goal, strlen(goal));
						//send(forClientSockfd, goal, strlen(goal), 0);
					}
				}
			}
		}
		}, forClientSockfd, ssl);
	}
	return 0;
}

int lookForBalanceNum(char name[]){
	for (int i = 0; i < countRegistered; i++){
		if (strncmp(name, userAccountRegistered[i], strlen(name)) == 0){
			return(balance[i]);
		}
	}
	return 0;
}
char* list(char nowUser[]){
	char* listReturn = new char[1000];
	char buffer[10] = {};
	int userBalance = lookForBalanceNum(nowUser);
	sprintf(buffer, "%d", userBalance);
	strncat(listReturn, buffer, strlen(buffer));
	strcat(listReturn, "\n");
	for (int i = 0; i < 100; i++){
		if (strcmp(userOnline[i], "") != 0)
			strcat(listReturn, userOnline[i]);
	}
	return listReturn;
}
char* regis(char inputBuffer[]){
	char temp[20];
	for (int i = 9; i < strlen(inputBuffer); i++){
		temp[i - 9] = inputBuffer[i];
	}
	temp[strlen(inputBuffer) - 9] = '\0';
	bool hasRegistered = false;
	for (int i = 0; i < countRegistered; i++){
		if (strcmp(userAccountRegistered[i], temp) == 0){
			hasRegistered = true;
		}
	}
	if (!hasRegistered){
		strcpy(userAccountRegistered[countRegistered], temp);
		balance[countRegistered] = 10000;
		countRegistered += 1;
	}
	char* successRegis = new char[20];
	char* failRegis = new char[20];
	strcpy(successRegis, "100 OK\n");
	strcpy(failRegis, "210 FAIL\n");
	if (!hasRegistered){
		cout << "sucRegis" << endl;
		return successRegis;
	}
	else{
		cout << "failRegis" << endl;
		return failRegis;
	}
}
char* logIn(char inputBuffer[], char clientIP[]){
	char* failLogIn = new char[20];
	strcpy(failLogIn, "220 AUTH_FAIL\n");
	int flag = 0;
	bool afterSharp = false;
	char temp1[20];
	char clientPort[10];
	int lengthName = 0;
	int lengthPort = 0;
	char tempUserOnline[1000];
	for (int i = 0; i < strlen(inputBuffer); i++){
		if (strncmp(&inputBuffer[i], "#", 1) == 0){
			afterSharp = true;
			continue;
		}
		if (!afterSharp){
			temp1[i] = inputBuffer[i];
			lengthName += 1;
		}
		else{
			clientPort[lengthPort] = inputBuffer[i];
			lengthPort += 1;
		}
	}
	clientPort[lengthPort] = '\0';
	for (int i = 0; i < countRegistered; i++){ //check whether the user registered
		if (strncmp(userAccountRegistered[i], temp1, strlen(userAccountRegistered[i])) == 0){
			flag = 1;
		}
	}
	for (int i = 0; i < 100; i++){ //check whether the user is logging in now
		if (strncmp(userOnline[i], temp1, lengthName) == 0){
			cout << "there are 220_AUTH_FAIL generated" << endl;
			flag = 0;
			break;
		}
	}
	for (int i = 0; i < 100; i++){
		int countSpace = 0;
		char tempPort[10] = {};
		int countTempPort = 0;
		for (int j = 0; j < strlen(userOnline[i]); j++){
			if (countSpace == 2 && userOnline[i][j] != '\n'){
				tempPort[countTempPort] = userOnline[i][j];
				countTempPort++;
			}
			if (userOnline[i][j] == ' '){
				countSpace++;
			}
		}
		//cout << "tempPort : " << tempPort << endl;
		tempPort[countTempPort] = '\0';
		if (strcmp(clientPort, tempPort) == 0){
			flag = 0;
			break;
		}
	}
	temp1[lengthName] = '\0';
	if (flag == 1){
		cout << temp1 << endl;
		bzero(&tempUserOnline,sizeof(tempUserOnline));
		strcat(tempUserOnline, temp1);
		strcat(tempUserOnline, " ");
		strcat(tempUserOnline, clientIP);
		strcat(tempUserOnline, " ");
		strcat(tempUserOnline, clientPort);
		strcat(tempUserOnline, "\n");
		cout << tempUserOnline << endl;
		for (int i = 0; i < 100; i++){
			if (strcmp(userOnline[i], "") == 0){
				cout << "success" << endl;
				strcat(userOnline[i], tempUserOnline);
				countLogIn += 1;
				break;
			}
		}
		//strcat(userOnline[countLogIn], tempUserOnline);
		char* listReturn = new char[1000];
		char buffer[10] = {};
		int userBalance = lookForBalanceNum(temp1);
		sprintf(buffer, "%d", userBalance);
		strncat(listReturn, buffer, strlen(buffer));
		strcat(listReturn, "\n");
		for (int i = 0; i < 100; i++){
			if (strcmp(userOnline[i], "") != 0){
				strcat(listReturn, userOnline[i]);
			}
		}
		return listReturn;
	}
	else
		return failLogIn;
}
char* getNowUser(char inputBuffer[]){
	char* temp1 = new char[20];
	int lengthName = 0;
	for (int i = 0; i < strlen(inputBuffer); i++){
		if (strncmp(&inputBuffer[i], "#", 1) == 0){
			break;
		}
		else{
			temp1[i] = inputBuffer[i];
			lengthName += 1;
		}
	}
	temp1[lengthName] = '\0';
	return temp1;
}