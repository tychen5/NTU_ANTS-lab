#include "server_comm.h"
#include <stdio.h>
#include <arpa/inet.h>//for htonl and sockaddr_in
#include "header.h"
#include <errno.h>//for errno
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>//for atoi
#include <unistd.h>//for read,write,close
#include <string.h>//for memset and memcpy
#include <stdbool.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>


#define LEN 1024
#define QUEUE_SIZE 10
#define POOL_SIZE 4

using namespace std;

void send_strC(int &sockfd, string &str){
	char req[LEN];
	memset(&req, 0, sizeof(req));
	strncpy(req, str.c_str(), sizeof(req));
	send(sockfd, req, strlen(req), 0);
}

int findHash(char array[]){
  for(int i=0; i<strlen(array); i++){
    if(array[i]=='#'){
        return i;
    }
  }
}

struct User2{
	string name;
  string IP;
	string port;
	string money;
};

void server_run(int connfd, void* regList, void* userList){
    printf("New Thread\n");
    string hello = "Connection accepted";
    send_strC(connfd, hello);
    bool stop = false;
		vector<User2> *onlineList = static_cast<vector<User2> *>(userList);
		vector<string> *regLs = static_cast<vector<string> *>(regList);

    while(!stop){
			/*receive REGISTER or Login*/
      char buf[LEN];
			memset(&buf, 0, sizeof(buf));
      int n = recv(connfd, buf, sizeof(buf), 0);
      buf[n-1] = '\0';
      char temp[LEN];
			char temp2[LEN];
			memset(&temp, 0, sizeof(temp));
			memset(&temp2, 0, sizeof(temp2));
      strncpy(temp, buf, findHash(buf));
			strncpy(temp2, buf+findHash(buf)+1, sizeof(temp2));
			string command = temp;
			string second = temp2;
			cout << "command: " << command << " second: " << second << endl;

      if(command == "REGISTER"){
        string regisOK = "100 OK\n";
				send_strC(connfd, regisOK);
				regLs->push_back(second);

      }else{
				if(find(regLs->begin(), regLs->end(), command) != regLs->end()){

					char amountC[LEN];
					char portC[LEN];
					memset(&amountC, 0, sizeof(amountC));
					memset(&portC, 0, sizeof(portC));
					strncpy(amountC, temp2, findHash(temp2));
					strncpy(portC, temp2+findHash(temp2)+1, sizeof(portC));


					struct User2 user;
					user.name = command;
					user.money = amountC;
					user.port = portC;

					onlineList->push_back(user);

					string in_money = "Initial Money: " + user.money + "\n";
					send_strC(connfd, in_money);

					while(true){
						char buf2[LEN];
						memset(&buf2, 0, sizeof(buf2));
						recv(connfd, buf2, sizeof(buf2), 0);

						if(strcmp(buf2, "List\n")==0){
							string onlineUser = "";
							for(int i = 0; i<(onlineList->size()); i++){
								onlineUser += onlineList->at(i).name + "#" + onlineList->at(i).IP + "#" + onlineList->at(i).port + "\n";

							}

							send_strC(connfd, onlineUser);

						}else if(strcmp(buf2, "Exit\n")==0){
							string bye = "Bye\n";
							send_strC(connfd, bye);
							stop = true;

						}

					}


				}else{
					string loginF = "220 AUTH_FAIL\n";
					send_strC(connfd, loginF);
				}


			}

    }


}
