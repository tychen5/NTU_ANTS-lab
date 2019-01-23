#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <netdb.h>
#include <string>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <vector>
#include "server_comm.h"
#include "threadpool.h"
//#include "header.h"

#define LEN 1024
#define PORT 8072

#define QUEUE_SIZE 10
#define POOL_SIZE 4

using namespace std;

bool stop;

struct user {
  string name;
  string IP;
	string port;
};

bool print_recv(int &sockfd){
	char buf[LEN];
	recv(sockfd, buf, sizeof(buf), 0);
	if(strcmp(buf, "210 FAIL\n")==0 || strcmp(buf, "220 AUTH_FAIL\n")==0){
		return false;
	}else{
		printf("%s\n",buf);
		return true;
	}

}

void send_str(int &sockfd, string &str){
	char req[LEN];
	strncpy(req, str.c_str(), sizeof(req));
	send(sockfd, req, strlen(req), 0);
}


int main()
{
    /* server socket set up */
		int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);//(IPv4, TCP, protocal)
    if (sockfd == -1){
    	cout << "Fail to create a socket.\n";
    }

    /* socket connect */
    struct sockaddr_in server, client;
		int addrlen = sizeof(client);
		bzero(&server,sizeof(server));
		server.sin_family = AF_INET; //IPv4
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);
		bind(sockfd, (struct sockaddr *)&server, sizeof(server));
		listen(sockfd, QUEUE_SIZE);

		/* worker pool */
		threadpool_t *tp = threadpool_create(POOL_SIZE, QUEUE_SIZE, 0);

	 	stop = false;
		int connfd;
    vector<string>* regList = new vector<string>();
		vector<user>* userList = new vector<user>();
		while(!stop){
        connfd = accept(sockfd, (struct sockaddr*)NULL ,NULL); // accept awaiting request
        if(connfd!=-1){
						printf("Thread Add\n");
            threadpool_add(tp, server_run, connfd, (vector<string>*)regList, (vector<user>*)userList, 0);

        }else{
            //sleep for 0.5 seconds
						printf("Sleep\n");
            usleep(500000);
        }

    }

		close(sockfd);
    threadpool_destroy(tp,0);

    return 0;
}
