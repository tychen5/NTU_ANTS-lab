#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

struct Account
{
	char name[1024];
	int port;
	int accountBalance;
	int online;//login or not
};

struct Account accountData[1024]; //store the account infromation
int accountNum; //total current account number
int TOTALTHREAD = 10;
int sd;

int getAccount(char name[]){
	for(int i = 0; accountNum; ++i){
		if(strcmp(name, accountData[i].name) == 0){
			return accountData[i].accountBalance;
		}
	}
	return -1;
}

//split a c string
char** split(char* str, int n) 
{
	char** ptr; 
	ptr = malloc(sizeof(char*)*4);
	for(int i=0 ; i<4 ; ++i)
	{
		ptr[i] = malloc(sizeof(char)*64);
		memset(ptr[i],0, sizeof(ptr[i]));
	}
	int index = 0;
	char first[64], second[64];
	int part = 0;
	int j=0;
	for (int i = 0; i < n; ++i)
	{
		
		if (str[i] == '#'){
			index = i;
			ptr[part][j] = '\0';
			part ++;
			j = 0;
		}
		else{
			ptr[part][j] = str[i];
			j++;
		}
	}
	return ptr;
}

//register
int regist(char name[])
{
	for (int i = 0; i < accountNum; ++i)
		if(strcmp(name, accountData[i].name)==0)
			return 0; //register failed
	strcpy(accountData[accountNum].name, name);
	accountData[accountNum].port = -1;
	//accountData[accountNum].accountBalance = 1000;
	accountData[accountNum].online = 0;
	return 1; //register successfully
}

//enter money
int moneyyy(char name[])
{
	accountData[accountNum].accountBalance = atoi(name);
	accountNum++;
	return 1; //successfully
}

//login
int login(char name[], int port)
{
	for (int i = 0; i < accountNum; ++i)
		if(strcmp(name, accountData[i].name) == 0) //compare two strings, if equals return 0
		{
			if (accountData[i].online == 1) //already login
			{
				return -2; 
			}
			accountData[i].port = port;
			accountData[i].online = 1;
			return i; //i >= 0 indicates login succesfully
		}
	return -1;
}

//receive "LIST" and send online list back to the client
char* getOnlineList(char clntName[])
{
	char* list;
	list = malloc(sizeof(char)*4096);
	
	for (int i = 0; i < accountNum; ++i)
	{
		if (accountData[i].online == 1)
		{
			
			char casti[1024];
						
			sprintf(casti,"%d",accountData[i].port);//casting ,from int to string
	
			strcat(list, accountData[i].name);
			strcat(list, "#");
			strcat(list, clntName);
			strcat(list, "#");
			strcat(list, casti);
			strcat(list, "\n");

		}
	}
	return list;
}

int Transaction(char name[], char peerName[], char amount[]){
	int judge = 0;
	int s = -1;
	int r = -1;
	if(strcmp(name,peerName) == 0)
		return -1;

	for (int i = 0; i < accountNum; ++i){
		if(strcmp(name, accountData[i].name) == 0){
			if((accountData[i].online == 1) && (accountData[i].accountBalance >= atoi(amount)) ){
				s=i;
				judge++;
				break;
			}
		}
	}

	for (int i = 0; i < accountNum; ++i){
		if(strcmp(peerName, accountData[i].name) == 0){
			if(accountData[i].online == 1){
				r=i;
				judge++;
				break;
			}
		}
	}
	
	if(judge == 2){
		accountData[s].accountBalance = accountData[s].accountBalance - atoi(amount);
		printf("%d\n", accountData[s].accountBalance);
		accountData[r].accountBalance = accountData[r].accountBalance + atoi(amount);
		printf("%d\n", accountData[r].accountBalance);
		return 0;
	}
	else
		return -1;

	

}


//process works in a thread
void* function(void* tid)
{
	int threadID = *(int*)(tid);

	while(1)
	{
		int clientfd; //client descriptor
		struct sockaddr_in client_addr;
		int addrlen = sizeof(client_addr);
		clientfd = accept(sd, (struct sockaddr*)&client_addr, &addrlen);

		char clntName[64];
		inet_ntop(AF_INET,&client_addr.sin_addr.s_addr,clntName,sizeof(clntName));

		//connection established
		printf("connection from %s established\n", clntName);
		int index = -1;

		char msg[10];
		strcpy(msg, "Hello!");

		send(clientfd, msg, strlen(msg), 0);

		while(1)
		{
			char buf[1024];
			memset(buf, 0, sizeof(buf));
			int n = recv(clientfd, buf, sizeof(buf), 0);
			//buf[strlen(buf)-2] = '\0'; //get rid of CR and LF
			printf("%s\n", buf); //print out the message from client
			if(n == 0) //offline
				break;

			//split the message from client
			char* first = split(buf, n)[0];
			char* second = split(buf, n)[1];
			char* third = split(buf, n)[2];
			char* fourth = split(buf, n)[3];
			printf("%s\n", first);
			printf("%s\n", second);
			printf("%s\n", third);
			printf("%s\n", fourth);

			if(strcmp(first, "REGISTER") == 0) //register
			{
				char reply[20];
				if(regist(second))
					strcpy(reply, "100 OK\r\n");	
				else
					strcpy(reply, "210 FAIL\r\n");
				send(clientfd, reply, strlen(reply), 0);

			}
			
			else if(strcmp(first, "accountBalance") == 0){
				char reply[20];
				if(moneyyy(second))
					strcpy(reply, second);
				send(clientfd, reply, strlen(reply), 0);
				
			}
			
			else if(strcmp(first, "List")==0) //get the list
			{
				char* reply = getOnlineList(clntName);
				send(clientfd, reply, strlen(reply), 0);
			}
			else if(strcmp(first, "Exit")==0) //exit
			{
				char reply[8];
				strcpy(reply, "Bye\r\n");
				accountData[index].online = 0;
				send(clientfd, reply, strlen(reply), 0);
			}
			
			else if(strcmp(first, "Account")==0){
				char reply[30] = {};
				//strcat(reply, "Account Balance = ");
				//strcat(reply, itoa(accountData[currentNum].accountBalance));
				int B;
				B = getAccount(second);
				snprintf(reply, 30, "%s%d\r\n", "Account Balance = ", B);
				send(clientfd, reply, strlen(reply), 0);
			}


			else if(strcmp(first, "Tran") == 0){
				char reply[30];
				if(Transaction(second, third, fourth) == 0){
					strcpy(reply, "Transaction Success\r\n");
				}
				else{
					strcpy(reply, "Transaction Failed\r\n");
				}
				send(clientfd, reply, strlen(reply), 0);
			}
			///////////////
			else //login
			{
				index = login(first, atoi(second));
				if (index==-1) //account doesn't exist
				{
					char reply[16];
					strcpy(reply, "220 AUTH_FAIL\r\n");
					send(clientfd, reply, strlen(reply), 0);
				}
				else if (index == -2) //port occupied//already login
				{
					char reply[16];
					strcpy(reply, "230 OCCUPIED\r\n");
					send(clientfd, reply, strlen(reply), 0);
				}
				else //successfully login
				{
					char aaa[16];
					strcpy(aaa, "login\r\n");
					send(clientfd, aaa, strlen(aaa), 0);
					int onlineNum = 0;
					for (int i = 0; i < accountNum; ++i)
						if(accountData[i].online == 1)
							onlineNum ++;
					char reply[4096];
					snprintf(reply, 4096, "%d\r\n%d\r\n", accountData[index].accountBalance, onlineNum); //int to string
					
					strcat(reply, getOnlineList(clntName));
					send(clientfd, reply, strlen(reply), 0);

					
				}
			}
		}		
		printf("one session terminated\n");
	}
		
}


int main(int argc, char const *argv[])
{
	/*create socket*/
	struct sockaddr_in dest;
	dest.sin_port = htons(2016);	//port num
	dest.sin_addr.s_addr = INADDR_ANY;
	dest.sin_family = AF_INET;

	sd = socket(AF_INET, SOCK_STREAM, 0);//create socket, socket(domain, type, protocol)
	if (sd == -1){
        printf("Fail to create a socket.");
    }
	
	/* user connects to the server */
	int b = -1;
	while(b < 0) //bind to the port
	{
		b = bind(sd, (struct sockaddr*)&dest, sizeof(dest));
	}
	
	listen(sd, 10); //listen to the port, 10 user at most


	/* create threads */
	pthread_t tid[TOTALTHREAD];       //thread identifier
   	pthread_attr_t attr; 	//set of attributes for the thread


	for (int i = 0; i < TOTALTHREAD; ++i) 
	{
		pthread_attr_init(&attr);
		pthread_create(&tid[i], &attr, function, (void*)(tid));
	}
	
	pthread_join(tid[0], NULL);
	
	return 0;
}
