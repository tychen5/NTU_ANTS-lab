/* Socket P2P APP - Client part */ 
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <regex>
using namespace std; 

// set globle variance/structure
char IP[25] = "127.28.0.1";
int Port = 59927;
int Money = 0;
char Name[21] = "";

char Recv_IP[25] = "";
int Recv_Port = 0;
char Recv_Name[21] = "";

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
bool payment();

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
bool PAYED();
// Inter state function
bool REG(const char*);							// given name;							return successe
bool LOG_IN(const char*, int=97);			// given name, password, port;	return successe
bool EXIT();										// given sd;							return successe (close socket)
// Login state function
int  LIST();										// given nothing;						return account balance
bool ASK_PAY(const char*, int=0);				// given who, give how much;		return success
bool DO_PAY(int, const char*, int);				// given money, ip, port;		return success
bool RECV_PAY(int, const char*, int, bool=false);			// given money, ip, port;		return success
bool CHECK_PAY(const char*, int=0);				// given who, give how much;		return success
bool LOG_OUT();									// given nothing; 					return successe (command=Exit)


	
int main(int argc, char *argv[]){	
	// check if IP and port are given
	int port = -1;
	if(argc==1){
		cout<<"Use setting: IP = "<<IP<<", port = "<<Port<<endl;
		sd = SOCK(IP, Port);
	}else{	
		cout<<"connect to "<<argv[1]<<", port="<<port<<endl;	
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
		
		//change setting
		int i=0;
		//IP
		for(i; argv[1][i]!='\0'; i++){
			IP[i] = argv[1][i];
		}
		IP[i] = '\0';
		//Port
		i = 0;
		char pn[6] = "";
		for(i; argv[2][i]!='\0' && i<6; i++){
			pn[i] = argv[2][i];
		}
		Port = atoi(pn);
	}	

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
		/* read response from other client*/
		memset(buff, '\0', sizeof(buff));		// clean the buff
		regex rx("PAY#");						/* check if succese or not */
		
		int n = 0;									// how many bytes have been read
		char* bptr = buff;						// start of the buff
		int buflen = sizeof(buff);				// space leave in buff
		if(( n = recv(sd, bptr, buflen, 0) ) > 0){
			while( ( n = recv(sd, bptr, buflen, 0) ) > 0 && buflen>n+3) { 	// receive by byte and put them in buff
				cout<<"read: "<<buff<<endl;		
				if( regex_search(bptr,bptr+n,rx) ){
					cout<<"someone what to pay me!"<<endl;
				}
				bptr += n; buflen -= n;
			}	
		}
		
		
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
			cout<<endl<<"Please enter 1 for list online user, 2 for log out, 3 for micropayment: ";
			cin>>buff;
			cout<<endl;
			// handle exception
			if(strlen(buff)!=1){
				clrCin();
				continue;
			}
			
			//do actions
			if(buff[0] == '1'){	
				int recv_m = LIST();	
				if( recv_m < 0 ){
					cout<<"Server no replied."<<endl;	
				}else{
					Money = recv_m;
				}
			}else if(buff[0] == '2'){
				if(LOG_OUT()){				// if log out, close the socket connect
					cout<<"logout"<<endl;
					status = Inter;
				}else{
					cout<<"Fail to log out."<<endl;
				}
			}else if(buff[0] == '3'){
				if( payment() ){				// micropayment
					cout<<"Micropayment done."<<endl;
				}else{
					cout<<"Fail to do micropayment."<<endl;
				}
			}
		}	// end status==Login
		
		// clean cin
		clrCin();
	}	// end while
}


bool registering(){
	// get name
	char name[35] = "";
	cout<<endl<<"Which name would you like to use?  ";
	cin>>name;
	int i=0;
	// check name
	while( !regex_match(name, name+strlen(name), isName) ){
		cout<<"Account name should start with an English letter, use only letters and numbers, and have a length between 3~20."<<endl;
		cout<<endl<<"Please choose a another name. Which name would you like to use?  ";
		cin>>name;
	}
	for(i; name[i]!='\0' && name[i]!='\n' && i<22; i++){
		continue;
	}
	name[i] = '#';
	i += 1;	
	// set money
	cout<<endl<<"How much would you want to save in your new account now?   $ ";
	char pn[6] = "10000";
	int money = 0;
	memset(buff, '\0', sizeof(buff));		// clean the buff
	cin>>buff;
	// handleing exception
	if( !regex_match(buff,buff+strlen(buff),isMoney) ){
		cout<<"Money needs to be an interger between 0 ~ 99999. Useing default value as initail money."<<endl;
		int j=0;		
		for(i,j; pn[j]!='\0' && pn[j]!='\0' && i<6; i++,j++){
			name[i] = pn[j]; 
		}
		money = 10000;
	}else{
		int j=0;		
		for(i,j; buff[j]!='\0' && buff[j]!='\n' && j<6; i++,j++){
			name[i] = buff[j];
		}
		money = atoi(buff);
	}	
	
	cout<<"Registering: "<<name<<" with $"<<money<<"."<<endl<<endl;	
	
	// socket connect
	bool ans = REG(name); 
	cout<<buff;
	
	// check if REG is succese
	if( !ans ){
		cout<<"The name \""<<name<<"\" is used by another user. Please change another name."<<endl;
		return false;
	}else{
		cout<<"\""<<name<<"\" is registered successfully."<<endl;			
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
		cout<<buff<<endl<<"This account does not exist. Do you mistake uppercase and lowercase letters?"<<endl;
		return false;
	}		
}

bool payment(){
	//name
	char name[21] = "";
	cout<<"Please enter the name who you want to send the money:  ";
	cin>>name;
	while( !regex_match(name,name+strlen(name),isName) ){
		cout<<"This is not a legal user name. Please enter your name:  ";
		cin>>name;	
	}

	//money
	int money = 0;
	cout<<"Please enter the amount of money you want to transform to "<<name<<":  $";
	cin>>buff;
	clrCin();
	// exception handleing
	bool ok = false;
	while( !ok ){
		if( !regex_match(buff,buff+strlen(buff),isMoney)){
			cout<<"Is this a number? Is so, it's too large or small."<<endl;
			cout<<"Please enter the amount of money again: $";
			cin>>buff;
		}else{
			money = atoi(buff);
			if( money<0){
				cout<<"Can't accept negative number."<<endl;
				cout<<"Please enter the amount of money again: $";
				cin>>buff;
			}else if( money > Money ){
				cout<<"You don't have enough money to do the payment."<<endl;
				cout<<"Please enter the amount of money again: $";
				cin>>buff;
			}else{
				ok = true;
			}
		}
	}	

	// check if  success
	if( !ASK_PAY(name,money) ){
		cout<<name<<" doesn't online."<<endl;
		return false;
	}else{
		cout<<"Try to connect with "<<name<<". Please wait for his/her respoense..."<<endl;
		if( DO_PAY(money,Recv_IP,Recv_Port) ){
			cout<<"Successfully send $"<<money<<" to "<<name<<"."<<endl;
			return true;		
		}else{
			cout<<name<<" doesn't replies you. Maybe he/she has a bad internet."<<endl;
			return false;
		}
	}			
}

bool recv_pay(){
	/* read response (may come in many pieces) */
	memset(buff, '\0', sizeof(buff));		// clean the buff
	regex rx("PAY#");						/* check if succese or not */	
	bool isPAY = false;
	int n = 0;									// how many bytes have been read
	char* bptr = buff;						// start of the buff
	int buflen = sizeof(buff);				// space leave in buff
	while( ( n = recv(sd, bptr, buflen, 0) ) > 0 && buflen>n+3) { 	// receive by byte and put them in buff
		if( regex_search(bptr,bptr+n,rx) ){		
			isPAY = true;
			break;
		}		
		bptr += n; buflen -= n;
	}		
	
	if(!isPAY){
		return false;
	}
	
	// get data
	int i=4, j=0;
	//name	
	j = 0;
	for(i,j; buff[i]!='#'; i++,j++){
		Recv_Name[j] = buff[i];
	}
	i += 1;
	//ip
	j = 0;
	for(i,j; buff[i]!='#'; i++,j++){
		Recv_IP[j] = buff[i];
	}
	i += 1;
	//port
	j = 0;
	char pn[6] = "";
	for(i,j; buff[i]!='#'; i++,j++){
		pn[j] = buff[i];
	}
	Recv_Port = atoi(pn);
	i += 1;
	//money
	j = 0;
	char pn_[6] = "";
	for(i,j; buff[i]!='\0'; i++,j++){
		pn_[j] = buff[i];
	}
	int money = atoi(pn);
	cout<<Recv_Name<<"@"<<Recv_IP<<":"<<Recv_Port<<" want to give you $"<<money<<endl;
	
	//ask
	memset(buff, '\0', sizeof(buff));		// clean the buff
	cout<<"Do you want to recieve? Y/N: ";
	cin>>buff;
	clrCin();
	while( strlen(buff)!=1 || (buff[0]!='Y' && buff[0]!='N' && buff[0]!='y' && buff[0]!='n') ){
		cout<<"Do you want to recieve? Y/N: ";
		cin>>buff;
		clrCin();
	}
	if(buff[0]!='Y' || buff[0]!='y'){
		RECV_PAY(money, Recv_IP, Recv_Port, true);
	}else{
		RECV_PAY(money, Recv_IP, Recv_Port, false);
	}
	return true;	
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
	regex rx2("200 FAIL");						/* check if succese or not */
	
	int n = 0;									// how many bytes have been read
	char* bptr = buff;						// start of the buff
	int buflen = sizeof(buff);				// space leave in buff
	while( ( n = recv(sd, bptr, buflen, 0) ) > 0 && buflen>n+3) { 	// receive by byte and put them in buff
//		cout<<"read: "<<buff<<endl;		
		if( regex_search(bptr,bptr+n,rx) ){
			return true;
		}else if( regex_search(bptr,bptr+n,rx2) ){
			return false;
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
	regex rx2("220 AUTH_FAIL");				/* check if fail*/
	
	int n = 0;									// how many bytes have been read
	char* bptr = buff;						// start of the buff
	int buflen = sizeof(buff);				// space leave in buff
	while( ( n = recv(sd, bptr, buflen, 0) ) > 0 && buflen>n+3) { 	// receive by byte and put them in buff
		if( regex_search(bptr,bptr+n,rx2) ){
			return false;
		}
		bptr += n; buflen -= n;
	}
	
	// refresh
	Money = atoi(buff);
//	cout<<"Money: $"<<Money<<endl;
	for(int i=0; name[i]!='\0' && i<25; i++){
		Name[i] = name[i];
	}

	return true;
}

bool LOG_OUT(){
	/* Send request */ 
	char req[10] = "Exit\n";
	cout<<"sending request:"<<req<<endl;	
	send(sd, req, sizeof(req), 0 ); 
	
	/* read response (may come in many pieces) */
	memset(buff, '\0', sizeof(buff));		// clean the buff
	regex rx("Bye");						/* check if succese*/
	regex rx2("210 FAIL");				/* check if fail*/
	
	int n = 0;									// how many bytes have been read
	char* bptr = buff;						// start of the buff
	int buflen = sizeof(buff);				// space leave in buff
	while( ( n = recv(sd, bptr, buflen, 0) ) > 0 && buflen>n+3) { 	// receive by byte and put them in buff
		if( regex_search(bptr,bptr+n,rx2) ){
			return false;
		}
		else if(regex_search(bptr,bptr+n,rx)){
			return true;
		}
		bptr += n; buflen -= n;
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
	
	/* read response */	
	memset(buff, '\0', sizeof(buff));		// clean the buff
	regex rx2("220 AUTH_FAIL");				/* check if fail*/	
	int n = 0;									// how many bytes have been read
	char* bptr = buff;						// start of the buff
	int buflen = sizeof(buff);				// space leave in buff
	while( ( n = recv(sd, bptr, buflen, 0) ) > 0 && buflen>n+3) { 	// receive by byte and put them in buff
		if( regex_search(bptr,bptr+n,rx2) ){
			return false;
		}
		bptr += n; buflen -= n;
	}
	
	// refresh
	Money = atoi(buff);
	//	cout<<"Money: $"<<Money<<endl;
	cout<<"Money in account: "<<buff<<endl;	
	return Money;
}

bool ASK_PAY(const char* name,int money){				// given who, give how much;		return successe
	/* Send request to server*/ 
	//my name
	char req[100] = "";
	int i=0;
	for(i; i<25 && Name[i]!='\0' ;i++){
		req[i] = Name[i];
	}
	req[i] = '#';
	i += 1;
	//money
	int j=0;
	char pn[6] = "";
	sprintf(pn, "%d", money);
	for(i,j; j<6 && pn[j]!='\0'; i++,j++){
		req[i] = pn[j];
	}
	req[i] = '#';
	i += 1;
	//to whom
	j=0;
	for(i,j; j<25 && name[j]!='\0' ;i++,j++){
		req[i] = name[j];
	}	
	req[i] = '\n';			// end messege
	req[i+1] = '\0';		// end messege
	cout<<"sending request:"<<req<<endl;	
	send(sd, req, sizeof(req), 0 ); 
	
	
	/* read response (may come in many pieces) */
	memset(buff, '\0', sizeof(buff));		// clean the buff
	regex rx2("220 AUTH_FAIL");						/* check if succese or not */	
	int n = 0;									// how many bytes have been read
	char* bptr = buff;						// start of the buff
	int buflen = sizeof(buff);				// space leave in buff
	while( ( n = recv(sd, bptr, buflen, 0) ) > 0 && buflen>n+3) { 	// receive by byte and put them in buff
		if( regex_search(bptr,bptr+n,rx2) ){
			return false;
		}		
		bptr += n; buflen -= n;
	}	
	
	// get recver IP & port
	i=0;
	j=0;
	for(i; buff[i]!='#'; i++){
		continue;
	}
	i += 1;
	//IP
	char ip[15] = "";
	j = 0;
	for(i,j; buff[i]!='#'; i++,j++){
		ip[j] = buff[i];
	}
	i += 1;
	//port
	char pn2[15] = "";
	j = 0;
	for(i,j; buff[i]!='\0' && buff[i]!='\n'; i++,j++){
		pn2[j] = buff[i];
	}
	int port = atoi(pn2);
	cout<<"get data of client: "<<name<<"@"<<ip<<":"<<port<<"."<<endl;		
	
	return true;
};

bool DO_PAY(int money, const char* recvIP, int recvPort){
	/* connect to another client */
	int recv_sockID = SOCK(recvIP,recvPort);
	
	// send request
	int i = 4, j=0;
	char req[100] = "PAY#";
	//name
	j = 0;
	for(i,j; Name[j]!='\0'; i++,j++){
		req[i] = Name[j];
	}
	req[i] = '#';
	i += 1;
	// myIP
	j = 0;
	for(i,j; IP[j]!='\0'; i++,j++){
		req[i] = IP[j];
	}
	req[i] = '#';
	i += 1;
	// my port
	j = 0;
	char pn[6] = "";
	sprintf(pn, "%d", Port);
	for(i,j; j<6 && pn[j]!='\0'; i++,j++){
		req[i] = pn[j];
	}
	req[i] = '#';
	i += 1;
	// money
	j = 0;
	char pn_[6] = "";
	sprintf(pn_, "%d", money);
	for(i,j; j<6 && pn_[j]!='\0'; i++,j++){
		req[i] = pn_[j];
	}
	req[i] = '\0';
	//send
	cout<<"sending to client: "<<req<<endl;	
	send(recv_sockID, req, sizeof(req), 0 ); 
	
	
	/* read response (may come in many pieces) */
	memset(buff, '\0', sizeof(buff));		// clean the buff
	regex rx("100 OK");						/* check if succese or not */
	regex rx2("200 FAIL");						/* check if succese or not */
	
	int n = 0;									// how many bytes have been read
	char* bptr = buff;						// start of the buff
	int buflen = sizeof(buff);				// space leave in buff
	while( ( n = recv( recv_sockID, bptr, buflen, 0) ) > 0 && buflen>n+3) { 	// receive by byte and put them in buff
		cout<<"read: "<<buff<<endl;		
		if( regex_search(bptr,bptr+n,rx) ){
			shutdown(recv_sockID,SHUT_RDWR);		// close both side of socket
			return true;
		}else if( regex_search(bptr,bptr+n,rx2) ){
			shutdown(recv_sockID,SHUT_RDWR);		// close both side of socket
			return false;
		}		
		bptr += n; buflen -= n;
	}	
	
	shutdown(recv_sockID,SHUT_RDWR);		// close both side of socket
	return false;
}

bool RECV_PAY(int money, const char* ip, int port, bool acept){
	//connect
	int send_sockID = SOCK(ip,port);
	if(send_sockID<0){
		return false;
	}
	
	if(!acept){		//refuse
		char req[20] = "200 FAIL\n"; 
		cout<<"sending to client:"<<req<<endl;	
		send(sd, req, sizeof(req), 0 ); 
	}else{	//acept
		Money += money;
		char req[20] = "100 OK\n"; 
		cout<<"sending to client:"<<req<<endl;	
		send(sd, req, sizeof(req), 0 ); 
	}
	
	return true;
}

bool CHECK_PAY(const char* name,int money){				// given who, give how much;		return successe
	/* Send request */ 
	char req[100] = "CHECK#";
	int i=6, j=0;
	//name
	for(i,j; j<25 && Name[j]!='\0' ;j++){
		req[i] = Name[j];
	}
	req[i] = '#';
	i += 1;
	//money
	j=0;
	char pn[6] = "";
	sprintf(pn, "%d", money);
	for(i,j; j<6 && pn[j]!='\0'; i++,j++){
		req[i] = pn[j];
	}
	req[i] = '#';
	i += 1;
	// to whom
	j=0;
	for(i,j; j<25 && name[j]!='\0' ;i++,j++){
		req[i] = name[j];
	}	
	req[i] = '\n';			// end messege
	req[i+1] = '\0';		// end messege
	cout<<"sending request:"<<req<<endl;	
	send(sd, req, sizeof(req), 0 ); 
	
	/* read response (may come in many pieces) */
	memset(buff, '\0', sizeof(buff));		// clean the buff
	regex rx("100 OK");						/* check if succese or not */
	regex rx2("200 FAIL");						/* check if succese or not */
	
	int n = 0;									// how many bytes have been read
	char* bptr = buff;						// start of the buff
	int buflen = sizeof(buff);				// space leave in buff
	while( ( n = recv(sd, bptr, buflen, 0) ) > 0 && buflen>n+3) { 	// receive by byte and put them in buff
		cout<<"read: "<<buff<<endl;		
		if( regex_search(bptr,bptr+n,rx) ){
			return true;
		}else if( regex_search(bptr,bptr+n,rx2) ){
			return false;
		}		
		bptr += n; buflen -= n;
	}
	
	return false;
};

