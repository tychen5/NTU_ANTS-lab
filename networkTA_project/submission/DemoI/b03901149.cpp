#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
using namespace std;

int main(int argc,char *argv[])
{
	if(argc != 3)
	{printf("Error Input Format.\n");exit(-1);}

	int sockfd = 0;
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1)
	{
		printf("Fail to create a socket.\n");
		exit(-1);
	}
	
	struct sockaddr_in clientInfo;
	bzero(&clientInfo,sizeof(clientInfo));
	clientInfo.sin_family = PF_INET;
	clientInfo.sin_addr.s_addr = inet_addr(argv[1]);
	clientInfo.sin_port = htons(atoi(argv[2]));

	char receiveMessage[100000] = {};
	if(connect(sockfd,(struct sockaddr*)&clientInfo,sizeof(clientInfo)) < 0)
	{
		printf("Connection Error.\n");
		exit(-1);
	}
	recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
	printf("%s\n",receiveMessage);
	memset(&receiveMessage,0,sizeof(receiveMessage));

	char a;
	while(1)
	{
		bool exit = false;
		char choose1[100] = {};
		printf("\n");
		printf("************************************************\n");
		printf("* Please Type the Usage (1-3):                 *\n");
		printf("* 1: Register   2: Login    3: End Program     *\n");
		printf("************************************************\n");
		printf("> ");
		a = fgets(choose1,sizeof(choose1),stdin)[0];
		if(int(a) < 32)
		{
			memset(&choose1,0,sizeof(choose1));
			continue;
		}
		if(int(choose1[0])<49 || int(choose1[0])>51 || int(choose1[1])>32)
		{
			printf("Invalid Input! You need to type 1~3.\n");
			memset(&choose1,0,sizeof(choose1));
			continue;
		}

		int mode = atoi(choose1);
		//printf("Execute Mode: %d\n",mode);
		if(mode == 3)
		{
			printf("Exit The Program ...\n");
			break;
		}

		//printf("Your Message: %s",choose1);

		// register
		if(mode == 1)
		{
			char userID[1000] = {};
			while(1)
			{
				printf("\n");
				printf("************************************************\n");
				printf("*                   Register                   *\n");
				printf("* Please Input the ID or type EXIT to quit.    *\n");
				printf("************************************************\n");
				printf("Register> ");
				a = fgets(userID,sizeof(userID),stdin)[0];
				if(int(a) < 32)
				{
					memset(&userID,0,sizeof(userID));
					continue;
				}
				if(strcmp(userID,"EXIT\n") == 0)
					break;
				char RegData[1000] = {};
				string RegDataString = "REGISTER#";
				for(int i=0;int(userID[i])>31;i++)
					RegDataString += userID[i];
				RegDataString += '\n';
				for(int i=0;i<int(RegDataString.length());i++)
					RegData[i] = RegDataString[i];
				
				send(sockfd,RegData,sizeof(RegData),0);
				printf("Send: %s\n",RegData);
				recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
				printf("Receive Message:\n");
				printf("%s\n",receiveMessage);
				if(receiveMessage[0] == '2' && receiveMessage[1] == '1' && receiveMessage[2] == '0')
					printf("The ID already exist!\n");
				memset(&RegData,0,sizeof(RegData));
				memset(&userID,0,sizeof(userID));
				memset(&receiveMessage,0,sizeof(receiveMessage));
				break;
			}
		}

		// login
		if(mode == 2)
		{
			char userID[1000] = {};
			while(1)
			{
				printf("\n");
				printf("************************************************\n");
				printf("*                     Login                    *\n");
				printf("*Please Input the ID, or type EXIT to quit.    *\n");
				printf("************************************************\n");
				printf("Login> ");
				a = fgets(userID,sizeof(userID),stdin)[0];
				if(int(a) < 32)
				{
					memset(&userID,0,sizeof(userID));
					continue;
				}
				if(strcmp(userID,"EXIT\n") == 0)
					break;
				char LoginData[1000] = {};
				string LoginDataString = "";
				for(int i=0;int(userID[i])>31;i++)
					LoginDataString += userID[i];
				LoginDataString += '#';
				LoginDataString += argv[2];
				LoginDataString += '\n';
				for(int i=0;i<int(LoginDataString.length());i++)
					LoginData[i] = LoginDataString[i];


				send(sockfd,LoginData,sizeof(LoginData),0);
				printf("Send: %s\n",LoginData);
				recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
				printf("Receive Message:\n");
				printf("%s\n",receiveMessage);


				// check successful or not
				bool flag = true;
				if(receiveMessage[0] == '2')
				{
					printf("The account is not exist!\n");
					memset(&userID,0,sizeof(userID));
					memset(&receiveMessage,0,sizeof(receiveMessage));
					continue;
				}
				recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
				printf("%s\n",receiveMessage);
				printf("Login Successfully.\n");

				memset(&LoginData,0,sizeof(LoginData));
				memset(&receiveMessage,0,sizeof(receiveMessage));
				// login successful
				while(flag)
				{
					flag = true;
					char choose2[100] = {};
					printf("\n");
					printf("************************************************\n");
					printf("* Please Type the Usage (1-3):                 *\n");
					printf("* 1: List   2: Payment    3: Logout            *\n");
					printf("************************************************\n");
					for(int j=0;int(userID[j])>31;j++)
						printf("%c",userID[j]);
					printf("> ");
					a = fgets(choose2,sizeof(choose2),stdin)[0];
					if(int(a) < 32)
					{
						memset(&choose2,0,sizeof(choose2));
						continue;
					}
					if(int(choose2[0])<49 || int(choose2[0])>51 || int(choose2[1])>32)
					{
						printf("Invalid Input! You need to type 1~3.\n");
						memset(&choose2,0,sizeof(choose2));
						continue;
					}

					int usage = atoi(choose2);
					if(usage == 1)
					{
						char text[] = {"List\n"};
						printf("Send: %s\n",text);
						send(sockfd,text,sizeof(text),0);
						recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
						printf("Receive Message:\n");
						printf("%s\n",receiveMessage);
						/*recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
						printf("%s\n",receiveMessage);*/
						memset(&receiveMessage,0,sizeof(receiveMessage));
						memset(&text,0,sizeof(text));
					}
					if(usage == 2)
					{
						char target[1000] = {};
						printf("\n");
						printf("************************************************\n");
						printf("*                   Payment                    *\n");
						printf("* Input Amount and Payee Seperate with Space.  *\n");
						printf("* E.g. 1000 Jimmy                              *\n");
						printf("* Or type EXIT to quit.                        *\n");
						printf("************************************************\n");
						printf("Payment> ");
						a = fgets(target,sizeof(target),stdin)[0];
						if(int(a) < 32)
						{
							memset(&target,0,sizeof(target));
							continue;
						}
						if(strcmp(target,"EXIT\n") == 0)
							break;
						char PayData[1000] = {};
						string PayDataString = "";
						for(int i=0;int(userID[i])>31;i++)
							PayDataString += userID[i];
						PayDataString += '#';
						for(int i=0;int(target[i])>31;i++)
						{
							if(target[i] == ' ')
								PayDataString += '#';
							else
								PayDataString += target[i];
						}
						PayDataString += '\n';
						for(int i=0;i<int(PayDataString.length());i++)
							PayData[i] = PayDataString[i];

						send(sockfd,PayData,sizeof(PayData),0);
						printf("Send: %s\n",PayData);
						recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
						printf("Receive Message:\n");
						printf("%s\n",receiveMessage);
						/*memset(&receiveMessage,0,sizeof(receiveMessage));
						recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
						printf("Receive Message:\n");
						printf("%s\n",receiveMessage);*/
						memset(&target,0,sizeof(target));
						memset(&PayData,0,sizeof(PayData));
						memset(&receiveMessage,0,sizeof(receiveMessage));
					}
					if(usage == 3)
					{
						char text[] = {"Exit\n"};
						printf("Send: %s\n",text);
						send(sockfd,text,sizeof(text),0);
						recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
						/*memset(&receiveMessage,0,sizeof(receiveMessage));
						recv(sockfd,receiveMessage,sizeof(receiveMessage),0);*/
						printf("Receive Message:\n");
						printf("%s\n",receiveMessage);
						memset(&receiveMessage,0,sizeof(receiveMessage));
						memset(&text,0,sizeof(text));
						flag = false;
						exit = true;
					}
				}
				if(!flag)
					break;
				memset(&userID,0,sizeof(userID));
			}
		}
		if(exit)
			break;
	}
	shutdown(sockfd,0);

	return 0;
}
