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

//split a c string
char** split(char* str, int n) 
{
	char** ptr; 
	ptr = malloc(sizeof(char*)*3);
	for(int i=0 ; i<3 ; ++i)
	{
		ptr[i] = malloc(sizeof(char)*64);
		memset(ptr[i],0, sizeof(ptr[i]));
	}
	int index = 0;
	char first[64], second[64];
	int part = 0;
	for (int i = 0; i < n; ++i)
	{
		ptr[part][i-index-part] = str[i];
		if (str[i] == '#'){
			index = i;
			ptr[part][i] = '\0';
			part ++;
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
	accountData[accountNum].accountBalance = 1000;
	accountData[accountNum].online = 0;
	accountNum++;
	return 1; //register successfully
}

//login
int login(char name[], int port)
{
	for (int i = 0; i < accountNum; ++i)
		if(strcmp(name, accountData[i].name)==0) //compare two strings, if equals return 0
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
						
			/*printf("%s", accountData[i].name);
			printf("#");
			printf("%s", clntName);
			printf("#");
			printf("%d\r\n", accountData[i].port);*/
			
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

			if(strcmp(first, "REGISTER") == 0) //register
			{
				char reply[20];
				if(regist(second))
					strcpy(reply, "100 OK\r\n");	
				else
					strcpy(reply, "210 FAIL\r\n");
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
			else //login
			{
				index = login(first, atoi(second));
				if (index==-1) //account doesn't exist
				{
					char reply[16];
					strcpy(reply, "220 AUTH_FAIL\r\n");
					send(clientfd, reply, strlen(reply), 0);
				}
				else if (index == -2) //port occupied
				{
					char reply[16];
					strcpy(reply, "230 OCCUPIED\r\n");
					send(clientfd, reply, strlen(reply), 0);
				}
				else //successfully login
				{
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
