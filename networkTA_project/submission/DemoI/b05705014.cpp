#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <string>

#define BLEN 4096
char username[50];
int portnum;

using namespace std;

int main(int argc, char* argv[]) {
	struct sockaddr_in addr;
	int sd;
	int n;
	int buflen;
	int msg1;
	int msg2;
	char buf[BLEN];
	char* bptr;
	char req[BLEN];
	char portnum_c[10];
	char loginans_c[3];
	char list[5] = "List";
	char Exit[5] = "Exit";

	/*host IP*/
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	addr.sin_port = htons(atoi(argv[2]));

	/*allocate socket*/
	sd = socket(AF_INET,SOCK_STREAM,0);

	/*connection*/
	if (connect(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		puts("Fail to connect");
		exit(0);
	} 
	else
	{
		memset(buf,'\0',strlen(buf));
		n = recv(sd,buf,sizeof(buf),0);
		printf("%s\n", buf);
	}

	/*register or login*/
	while (true)
	{
		printf("Enter 1 for Register, 2 for login: ");
		scanf("%d", &msg1);
		if (msg1 == 1)
		{
			printf("Enter the name you want to register: ");
			scanf("%s",username);
			memset(req,'\0',strlen(req));
			strcat(req,"REGISTER#");
			strcat(req,username);
			// strcat(req,"\n");
			memset(buf,'\0',strlen(buf));

			send(sd,req,strlen(req),0);
			n = recv(sd,buf,sizeof(buf),0);

			printf("%s\n", buf);

		}
		else if (msg1 == 2)
		{
			printf("Enter your name: ");
			scanf("%s",username);
			printf("Enter your port number: ");
			scanf("%d",&portnum);
			while (portnum > 65535 || portnum < 1024)
			{
				printf("port must be between 1024 and 65535.\n");
				printf("Enter your port number: ");
				scanf("%d",&portnum);
			}
			sprintf(portnum_c,"%d",portnum);
			memset(req,'\0',strlen(req));
			strcat(req,username);
			strcat(req,"#");
			strcat(req,portnum_c);
			// strcat(req,"\n");
			memset(buf,'\0',strlen(buf));

			send(sd,req,strlen(req),0);

			memset(buf,'\0',strlen(buf));
			n = recv(sd,buf,sizeof(buf),0);
			printf("%s\n",buf);
			memset(buf,'\0',strlen(buf));
			n = recv(sd,buf,sizeof(buf),0);
			printf("%s\n", buf);

			if (strncmp(buf,"220 AUTH_FAIL",13)) {
				break;
			}
		}
		else
		{
			printf("Wrong command, please try again\n");
		}
	}
	/*list or exit*/
	while (true) {
		printf("Enter the action you want to take.\n1 to ask for the lastest list, 8 to Exit: ");
		scanf("%d",&msg2);

		if (msg2 == 1)
		{
			memset(req,'\0',strlen(req));
			strcat(req,list);

			memset(buf,'\0',strlen(buf));

			n = recv(sd,buf,sizeof(buf),0);
			printf("%s\n",buf);
			memset(buf,'\0',strlen(buf));
			n = recv(sd,buf,sizeof(buf),0);

			printf("%s\n",buf);
		}
		else if (msg2 == 8)
		{
			memset(req,'\0',strlen(req));
			strcat(req,Exit);

			memset(buf,'\0',strlen(buf));

			send(sd,req,strlen(req),0);
			n = recv(sd,buf,sizeof(buf),0);
			printf("%s\n",buf);

			close(sd);
			exit(0);
		}
		else
		{
			printf("Wrong command, please try again\n");
		}
	}
	return 0;
}
