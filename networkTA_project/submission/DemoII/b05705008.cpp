#include<stdio.h>
#include<string.h>   
#include<stdlib.h>    
#include<sys/socket.h>
#include<arpa/inet.h> 
#include<unistd.h>    
#include<pthread.h> 
#include <iostream>
using namespace std;

struct mypara
{
	int sd;
	int id;
	string str;
};

//an account for each client
struct account
{
	string name;
	string ip;
	string port;
	string balance;
	bool online;
};

account info[100];
int numon = 0; //nums of online client 
int i = -1;//register id

//the thread function
void *connection_handler(void *);
 
int main(int argc , char *argv[])
{
    int socket_desc , c;
    struct sockaddr_in server , client;
    struct mypara client_sock;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 9999 );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }

     
    //Listen
    listen(socket_desc , 10);
     
    //Accept and incoming connection
    cout << "Waiting...";
    c = sizeof(struct sockaddr_in);
 
    pthread_t thread_id[10];

    while( (client_sock.sd = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
    	i ++;
    	client_sock.id = i;
    	char str[INET_ADDRSTRLEN];
	struct sockaddr_in* sa = (struct sockaddr_in*)&client;
	struct in_addr hip = sa-> sin_addr;
	inet_ntop(AF_INET, &sa->sin_addr,str,INET_ADDRSTRLEN);
	client_sock.str = str;

        if( pthread_create( &thread_id[i] , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
    }
     
    if (client_sock.sd < 0)
    {
        return 1;
    }
     
    return 0;
}
 
/*
 * Handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
	struct mypara *re;
	re = (struct mypara *)socket_desc;
	int id = re->id;
	info[id].ip = re->str;
	

 	//Get the socket descriptor
  	int sock = *(int*)socket_desc;
    	int read_size;
	char client_message[2000] = {0};
	
	char s0[20] = {"connection accepted"};
	char s1[20] = {"100 OK"};
	char s2[20] = {"10000"};
	char s3[20] = {"Bye"};
	char f0[20] = {"210 FAIL"};
	char f1[20] = {"220 AUTH_FAIL"};
	
	string name;
	string port;
	string check;
	string balance = "0";//money

    //send "connection accepted" to client
	send(sock,s0,sizeof(s0),0);          
		
        while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 ){
		client_message[read_size] = '\0';
		//case register
		if(check.assign(client_message,0,9)=="REGISTER#"){
			//get name
			name.assign(client_message,9,read_size-8);
			bool exist = false;
			//name has been used
			for(int i=0;i<100;i++){
				if(info[i].name == name){
					//send 210 fail
					exist = true;
					write(sock , f0 , strlen(f0));
				}
			}
			//send 100OK
			if(!exist){
				info[id].name = name;
				write(sock , s1 , strlen(s1));
				memset(client_message,'\0',sizeof(client_message));
				read_size = recv(sock , client_message , 2000 , 0);
				info[id].balance.assign(client_message,0,read_size);//receive initial value
			}
		}
		else if(check.assign(client_message,0,4) =="List"){
				string tota = std::to_string(numon);
				string slist = "\r\nnumbers of account online: " + tota + '\n';
	
				for(int i = 0;i < 100;i ++){
					if(info[i].online){
						slist = slist + info[i].name + "#" + info[i].ip + "#" + info[i].port + '\n';
					}
				}
				//send the list
				const char *lis = slist.c_str();
				send(sock, lis, strlen(lis),0);
		}
		else if(check.assign(client_message,0,4) =="Exit"){
			numon--;
			info[id].online = false;
			cout << info[id].name << " is leaving" << '\n';
			send(sock , s3 , strlen(s3),0);			
			return 0;
		}
		//case log in
		else if(check.assign(client_message,0,9)!="REGISTER#" && check.assign(client_message,0,read_size).find("#")>=0){
			string sec;
			sec.assign(client_message,0,read_size);
			check.assign(sec,0,sec.find("#"));
			bool reg =false;
			for(int i = 0;i<100;i++){
				if(info[i].name == check){
					reg = true;
					break;
				}
			}
			if(reg){
				port.assign(client_message,name.length()+1,read_size-name.length());
				info[id].port = port;
				info[id].online = true;
				numon ++;
				send(sock , s2 , strlen(s2),0);//send 10000 to client
					
				string tota = std::to_string(numon);
				string slist = "\r\nnumbers of account online: " + tota + '\n';
	
				for(int i = 0;i < 100;i ++){
					if(info[i].online){
						slist = slist + info[i].name + "#" + info[i].ip + "#" + info[i].port + '\n';
					}
				}
				//send the list
				const char *lis = slist.c_str();
				send(sock, lis, strlen(lis),0);
			}
			else{
				write(sock, f1, strlen(f1));
			}
		}
		memset(client_message,'\0',sizeof(client_message));
	}

} 
