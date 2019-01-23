/*
Socket P2P APP - Client part
server IP:
	(1)140.112.107.194	port= 33120	
	(2)140.112.107.45		port= 32500 
	(#)140.112.106.45		port= 33220
*/ 
#ifdef __WIN32__
#	include <winsock2.h>
#else
#	include <sys/types.h> 
#	include <sys/socket.h>
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <regex>
using namespace std; 

// set globle variance/structure
const char IP[25] = "140.112.107.194";
const int Port = 33120;

const regex isName("^([a-zA-Z])([a-zA-Z0-9]{2,19})$");
const regex isIP("^([1-9][0-9]{0,2})(\\.)([1-9][0-9]{0,2})(\\.)([1-9][0-9]{0,2})(\\.)([1-9][0-9]{0,2})$");
const regex isPort("^([0-9]{1,5})$");
const regex isMoney("^([0-9]{1,5})$");
const int PortMax = 65535;

enum Status{
	Inter = -1,
   Login
};
int sd = -1; /* socket descriptor*/
char buff[1024] = "";

// SSL
SSL_CTX *sslctx;
SSL *cSSL;


// IO function
bool registering();
bool login();

// tool function
bool  checkChar(const char*, const regex);
bool  readRecv(char*, regex, regex);
void  clrCin(){
		cin.clear();
		cin.ignore(10000,'\n'); 	//clear all char in cin buffer 
}
void  clrRecv(int sd){
	/* read response*/
	recv(sd, buff, 100, 0);	// how many bytes have been read	
	memset(buff, '\0', sizeof(buff));		// clean the buff	
}


// start function
int  SOCK(const char* = IP, int = Port);	// given IP,port;						return sd
// Inter state function
bool REG(const char*);							// given name;							return successe
bool LOG_IN(const char*, int=97);			// given name, password, port;	return successe
bool EXIT();										// given sd;							return successe (close socket)
// Login state function
int  LIST();										// given nothing;						return account balance
int  SRCH(const char*);							// given name want to found;		return which port he/she is on (-1 if not online)
bool TRANS(const char*,int=0);				// given who, give how much;		return successe
bool LOG_OUT();									// given nothing; 					return successe (command=Exit)


	
int main(int argc, char *argv[]){
	// check if IP and port are given
	int port = -1;
	if(argc==1){
		sd = SOCK(IP, Port);
	}else{		
		// exception handleing
		if( !regex_match(argv[1],argv[1]+strlen(argv[1]),isIP) ){
			cout<<"Illegal IP addresse."<<endl;
			return 0;
		}
		
		if( !regex_match(argv[2],argv[2]+strlen(argv[2]),isPort) ){
			cout<<"Illegal port number."<<endl;
			return 0;
		}
		port = atoi(argv[2]);
		if( port<0 || port>PortMax){
			cout<<"Illegal port number."<<endl;
			return 0;
		}
		
		// connect
		sd = SOCK(argv[1], port);
	}	
	cout<<"connect to "<<argv[1]<<", port="<<port<<endl;
	if( sd < 0){
		cout<<"Connection fail, end process"<<endl;
		EXIT();
		return 0;
	}else{
		cout<<endl<<"Welcome to this P2P program."<<endl;
	}

	/* Initialize */
	Status status = Inter;
	
	// main loop
	while(true){
		if(status==Inter){
			//get user input
			cout<<endl<<"Please enter 1 for regester, 2 for login, 3 for exit: ";
			cin>>buff;
			cout<<endl;
			// handle exception
			if(strlen(buff)!=1){
				clrCin();
				continue;
			}			
			
			//do actions
			if(buff[0] == '1'){			
				registering();				// no matter reg succese or not, back to reg/login page
			}else if(buff[0] == '2'){
				if( login() ){				// if login succese, move on to next page
					status = Login;
				}
			}else if(buff[0] == '3'){
				if(EXIT()){				// EXIT(): close socket connect
					cout<<"Socket is closed. Exit program"<<endl;
					return 1;
				}else{
					cout<<"Fail to close socket. Please try again."<<endl;
				}			
			}
		}	// end status==Inter
	
		if(status==Login){
			//get user input
			cout<<endl<<"Please enter 1 for list online user, 2 for log out: ";
			cin>>buff;
			cout<<endl;
			// handle exception
			if(strlen(buff)!=1){
				clrCin();
				continue;
			}
			
			//do actions
			if(buff[0] == '1'){			
				if( LIST() < 0 ){
					cout<<"Server no replied."<<endl;	
				}
			}else if(buff[0] == '2'){
				if(LOG_OUT()){				// if log out, close the socket connect
					cout<<"logout"<<endl;
					status = Inter;
				}else{
					cout<<"Fail to log out."<<endl;
				}
			}
		}	// end status==Login
		
		// clean cin
		clrCin();
	}	// end while
}


bool registering(){
	// get name
	char name[25] = "";
	cout<<endl<<"Which name would you like to use?  ";
	cin>>name;
	// check name
	while( !regex_match(name, name+strlen(name), isName) ){
		cout<<"Account name should start with an English letter, use only letters and numbers, and have a length between 3~20."<<endl;
		cout<<endl<<"Please choose a another name. Which name would you like to use?  ";
		cin>>name;
	}	
	
	cout<<"Registering the name \""<<name<<"\"."<<endl<<endl;	
	
	// socket connect
	bool ans = REG(name); 
	cout<<buff;
	
	// check if REG is succese
	if( !ans ){
		cout<<"The name \""<<name<<"\" is used by another user. Please change another name."<<endl;
		return false;
	}else{
		cout<<"\""<<name<<"\" is registered successfully."<<endl;			
		
		// set money
		cout<<endl<<"How much would you want to save in your new account now?   $ ";
		int money = 10000;
		cin>>buff;
		// handleing exception
		if( !regex_match(buff,buff+strlen(buff),isMoney) ){
			cout<<"Money needs to be an interger in 0 ~ 99999. Useing default value as initail money."<<endl;
		}else{
			money = atoi(buff);
		}	
		cout<<endl<<"Initail money: "<<money<<endl;
		
		return true;
	}		
}

bool login(){
	//name
	char name[21] = "";
	cout<<"Please enter your name:  ";
	cin>>name;
	while( !regex_match(name,name+strlen(name),isName) ){
		cout<<"This is not a legal user name. Please enter your name:  ";
		cin>>name;	
	}

	//port
	int port = -1;
	cout<<"Please enter the port you want to use:  ";
	cin>>buff;
	clrCin();
	// exception handleing
	if( !regex_match(buff,buff+strlen(buff),isPort)){
		cout<<"Useing default port."<<endl;
		port = 97;
	}else{
		port = atoi(buff);
		if( port<0 || port>PortMax){
			cout<<"Useing default port."<<endl;
			port = 97;
		}		
	}

	//exist user?
	if( LOG_IN(name,port) ){
		cout<<"Welcome, "<<name<<"!"<<endl;
		cout<<"Money in account: "<<buff<<endl;	
		return true;
	}else{
		cout<<buff<<"This account does not exist. Do you mistake uppercase and lowercase letters?"<<endl;
		return false;
	}		
}


int SOCK(const char* IP, int port){
	// create an endpoint for communication; int socket(int domain, int type, int protocol); 
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd==-1){
		cout<<"Fail to create socket: "<<strerror(errno)<<endl;
		return -1;
	}
	
	//set connecting parameter 
	struct sockaddr_in serv_addr = {0};					// struct sockaddr *server
	memset(&serv_addr, '0', sizeof(serv_addr));
   
   serv_addr.sin_family = PF_INET;
   serv_addr.sin_addr.s_addr = inet_addr(IP); 		// IP address	
   serv_addr.sin_port = htons(port); 					// port number

	// connect to sd
	int rd = connect(sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr) );	
	if(rd==-1){
		cout<<"Fail to connect to socket:"<<strerror(errno)<<endl;
		return -1;
	}
	
	return sd;
}

bool REG(const char* name){
	/* Send request */ 
	char req[128] = "REGISTER#";
	int i=0;
	for(i; i<100 && name[i]!='\0' ;i++){
		req[9+i] = name[i];
	}
	req[9+i] = '\n';
	cout<<"sending request:"<<req<<endl;	
	send(sd, req, sizeof(req), 0 ); 
	
	/* read response (may come in many pieces) */
	memset(buff, '\0', sizeof(buff));		// clean the buff
	regex rx("100 OK");						/* check if succese or not */
	
	int n = 0;									// how many bytes have been read
	char* bptr = buff;						// start of the buff
	int buflen = sizeof(buff);				// space leave in buff
	while( ( n = recv(sd, bptr, buflen, 0) ) > 0 && buflen>n+3) { 	// receive by byte and put them in buff
//		cout<<"read: "<<buff<<endl;		
		if( regex_search(bptr,bptr+n,rx) ){
			return true;
		}			
		bptr += n; buflen -= n;
	}

	return false;
}

bool LOG_IN(const char* name, int port){
	/* Send request */ 
	char req[45] = "";
	int i=0;
	for(i; i<20 && name[i]!='\0' ;i++){
		req[i] = name[i];
	}
	req[i] = '#';
	i += 1;

	int j=0;
	char pn[6] = "";
	sprintf(pn, "%d", port);
	for(i,j; j<6 && pn[j]!='\0'; i++,j++){
		req[i] = pn[j];
	}
	req[i] = '\n';			// end messege
	req[i+1] = '\0';		// end messege
	cout<<"sending request:"<<req<<endl;	
	send(sd, req, sizeof(req), 0 ); 
	
	
	/* read response (may come in many pieces) */
	memset(buff, '\0', sizeof(buff));		// clean the buff
	regex rx("(.*)#(.*)#(.*)\n");				/* check if succese*/
	regex rx2("220 AUTH_FAIL");				/* check if fail*/
	
	int n = 0;									// how many bytes have been read
	char* bptr = buff;						// start of the buff
	int buflen = sizeof(buff);				// space leave in buff
	while( ( n = recv(sd, bptr, buflen, 0) ) > 0 && buflen>n+3) { 	// receive by byte and put them in buff
//		cout<<endl<<"read: "<<buff<<endl;
		if( regex_search(bptr,bptr+n,rx) ){
			return true;
			break;
		}else if(regex_search(bptr,bptr+n,rx2)){
			return false;
		}
		bptr += n; buflen -= n;
	}
		
	return false;
}

bool LOG_OUT(){
	/* Send request */ 
	char req[10] = "Exit\n";
	cout<<"sending request:"<<req<<endl;	
	send(sd, req, sizeof(req), 0 ); 
	
	/* read response (may come in many pieces) */
	memset(buff, '\0', sizeof(buff));		// clean the buff	
	int n = recv(sd, buff, 20, 0);	// how many bytes have been read
	
	/* check if succese or not */
	regex rx("Bye");				/* check if succese or not */
	if( regex_search(buff,buff+20,rx) ){
		return true;
	}else{
		return false;
	}
}

bool EXIT(){
	shutdown(sd,SHUT_RDWR);		// close both side of socket
	return true;
}

int LIST(){
	char req[10] = "List\n";
	cout<<"sending request:"<<req<<endl;	
	send(sd, req, sizeof(req), 0 ); 
	
	/* read response (may come in many pieces) */	
	int money=0;
	int n = 0;									// how many bytes have been read
	
	// get money
	memset(buff, '\0', sizeof(buff));		// clean the buff	
	n = recv(sd, buff, 50, 0);
	if(n > 0){
		money = atoi(buff);
		cout<<"Money in account: "<<money<<endl;	
	}else{
		cout<<"Fail to get account balance."<<endl;
		memset(buff, '\0', sizeof(buff));
		return -1;
	}
	
	// get online data
	memset(buff, '\0', sizeof(buff));		// clean the buff	
	n = recv(sd, buff, sizeof(buff), 0);
	if(n > 0){
		cout<<buff<<endl;
	}else{
		cout<<"Fail to get data of online users."<<endl;
		memset(buff, '\0', sizeof(buff));
		return -1;
	}
	
	memset(buff, '\0', sizeof(buff));		// clean the buff	
	return money;
}
