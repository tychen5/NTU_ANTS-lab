#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;

int main(int argc , char *argv[])
{
	//建立socket
    int sd = 0;
    sd = socket(AF_INET , SOCK_STREAM , 0);

    if (sd == -1){
        cout << "Fail to create a socket.";
    }
   //socket連線
    struct sockaddr_in info;
    int addrlen = sizeof(info);
    bzero(&info,sizeof(info));
    
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = INADDR_ANY;
    info.sin_port = htons(8900);
    bind(sd,(struct sockaddr *)&info, sizeof(info));
    listen(sd,10);

    //傳訊 
	char name[120] = {0};
	int choosenum=0;
	char portnum[120] = {0};
	int login=0;
	char actnum[120] = {0};
    char rec[120] = {0};
    cin >> choosenum;
	do{
		if(choosenum==1)
		{//Register
			name[120]= {0};
			cin >> name;
    			send(sd,name,strlen(name),0);
    			recv(sd,rec,strlen(rec),0);
    			cout << rec;
		}
		else if(choosenum==2)
		{//Login
			name[120] = {0};
			portnum[120]={0};
			cin >> name >> portnum;
    			send(sd,name,strlen(name),0);
    			send(sd,portnum,strlen(portnum),0);
    			recv(sd,rec,strlen(rec),0);
    			cout << rec;
			login = 1;
		}	
	}while(login!=1);
	
	do{//Action
		actnum[120] = {0};
		cin >> actnum;
    		send(sd,actnum,strlen(actnum),0);
    		recv(sd,rec,strlen(rec),0);
    		cout << rec;
	}while (actnum!="8");
		
    cout << "Bye"<< "\n";
    close(sd);
    return 0;
}
