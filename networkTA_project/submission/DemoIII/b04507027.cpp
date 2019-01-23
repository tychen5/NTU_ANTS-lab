/* Socket P2P APP - Server part */ 
#include <sys/types.h> 
#include <sys/socket.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <iostream>
//#include <stdio.h>
#include <errno.h>
#include <regex>
#include <thread>
#include <pthread.h>
using namespace std; 

// set globle variance/structure
const char IP[25] = "127.28.0.1";
const int Port = 59927;
int sd = -1;
//
char buff[4096] = "";
char recvBuff[128] = "";
pthread_t threads[50];
//
const regex isIP("^([1-9][0-9]{0,2})(\\.)([1-9][0-9]{0,2})(\\.)([1-9][0-9]{0,2})(\\.)([1-9][0-9]{0,2})$");
const regex isPort("^([0-9]{1,5})$");
const int PortMax = 65535;
int tn = 0;

const regex REG_("REGISTER#.+");
const regex LOGIN_("(.*)#(.*)");
const regex LIST_("List\n");
const regex LOGOUT_("Exit\n");
const regex ASK_PAY_("(.*)#(.*)#(.*)");
const regex CHECK_PAY_("CHECK#(.*)#(.*)#(.*)");
enum Status{
	Inter = -1,
   Login
};

class RecordUser;
class User;
int nameCheck(char*);
int now_nameCheck(char*);

// socket command function
bool Start(int, char* = 0, char* = 0);
int  SOCK(const char* = IP, int = Port);
bool REG(char*,User*);						// given name;							return successe
bool LOG_IN(char*, int,User*);			// given name, port;	return successe
bool LIST(User*);								// given nothing;						return account balance
bool LOG_OUT(User*);							// given nothing; 					return successe (command=Exit)
bool ASK_PAY(User*, char*, int=0);			//
bool CHECK_PAY(User*, char*, int=0);		//
bool recvHandle(char*,User*);

class RecordUser{
	public:
	static RecordUser* allRecord[100];
	static int totalRecord;
	char name[21];
	int money = 0;
	int id = -1;
	RecordUser(char* name_, int money_){
		for(int i=0; name_[i]!='\0' && i<25; i++){
			name[i] = name_[i];
		}
		money = money_;
		id = RecordUser::totalRecord;
		cout<<"Add to RecordUser["<<id<<"]: "<<name<<", money=$"<<money<<endl;
		RecordUser::totalRecord += 1;
	}
};
int RecordUser::totalRecord = 0;
RecordUser* RecordUser::allRecord[100] = {};

class User{
	public:
	static int totalUser;
	static User* allUser[50];

	//normal
	char name[21] = "Visiter";
	int money = 0;	
	int stat = Inter;
	int id = -1;
	int rid = -1;
	char p[100] = "";
	//socket
	int sockID = -1;
	struct sockaddr_in user_addr = {'\0'};		//sockaddr*
	int portN = -1;
	char strAddr[20] = "";
	
	User(int sock_id,sockaddr_in addr){
		sockID = sock_id;
		user_addr = addr;
		getAddr();
		getPort();
		cout<<"create a new Visiter user @ "<<strAddr<<", port="<<portN<<endl;
		id = User::totalUser;
		User::totalUser += 1; 
	}
	
	~User(){	
		// change place	
		int t = User::totalUser;
		int i = id;
		for(i; i < t-1; i++){
			User::allUser[i] = User::allUser[i+1];
		}
		i = id;
		for(i; i < t-1; i++){
			threads[i] = threads[i+1];
		}
		User::allUser[t] = 0;
		User::totalUser -= 1;
		cout<<"has delete user["<< id <<"]"<<endl;
	 }
	
	char* print(){
		int i=0, j=0;
		//name
		for(i; name[i]!='\0' && i<25; i++){
			p[i] = name[i];
		}
		p[i] = '#';
		i += 1;
		//addr
		j = 0;
		for(i,j; strAddr[j]!='\0' && j<25; i++,j++){
			p[i] = strAddr[j];
		}
		p[i] = '#';
		i += 1;
		//port
		char pn[6] = "";
		sprintf(pn, "%d", portN);
		for(i,j; j<6 && pn[j]!='\0'; i++,j++){
			p[i] = pn[j];
		}
		p[i] = '\n';
		
		return p;
	}
	
	bool reg(char* name_, int money_){
		//check if anyone has used the name_
		int rid = nameCheck(name_);
		if(rid!=-1){	//not record yet
			return false;
		}
		//change data
		for(int i=0; i<20; i++){
			name[i] = '\0';
		}
		for(int i=0; name_[i]!='\0' && name_[i]!='\n' && i<20; i++){
			name[i] = name_[i];
		}
		money = money_;
		cout<<"Visiter@"<<strAddr<<":"<<portN<<" has REG# as "<<name<<" with money="<<money<<endl;
		int r = RecordUser::totalRecord;
		rid = r;
		RecordUser::allRecord[rid] = new RecordUser(name,money);
		return true;
	}
	
	bool login(char* name_, int port){
		int find_rid = nameCheck(name_);
		if(find_rid==-1){	//not record yet
			return false;
		}
		
		//set record
		rid = find_rid;
		//change name
		for(int i=0; name[i]!='\0' && i<25; i++){
			name[i] = '\0';
		}
		for(int i=0; name_[i]!='\0' && i<25; i++){
			name[i] = name_[i];
		}
		//change money
		money = RecordUser::allRecord[rid]->money;		
		
		//reset
		memset(&user_addr, '0', sizeof(user_addr));
		// user_addr.sin_family = PF_INET;
		//	user_addr.sin_addr.s_addr = inet_addr(strAddr); 		// IP address	
	   user_addr.sin_port = htons(port); 					// port number	
	   //
	   getPort();
	   stat = Login;
	   cout<<"Visiter@"<<strAddr<<":"<<portN<<" has LOGIN as "<<name<<" with money=$"<<money<<endl;
	   cout<<print()<<endl;
	   return true;
	}
	bool logout(){
		// change record
		RecordUser::allRecord[rid]->money = money;
		// reset		
		for(int i=0; name[i]!='\0' && i<25; i++){
			name[i] = '\0';
		}
		char default_name[25] = "Visiter";
		for(int i=0; default_name[i]!='\0' && i<25; i++){
			name[i] = default_name[i];
		}
		money = 0;
		rid = -1;
		stat = Inter;
		cout<<"Visiter@"<<strAddr<<":"<<portN<<" has LOGOUT."<<endl;
		return true;
	}
	bool payTo(int give, char* recver_name){
		int recv_rid = nameCheck(recver_name);
		if(recv_rid==-1){	//not record yet
			return false;
		}
		RecordUser* recver = RecordUser::allRecord[recv_rid];
	
		// self --
		money -= give;
		RecordUser::allRecord[rid]->money = money;
		// recver ++
		RecordUser::allRecord[recv_rid]->money += give;		
		int nid = now_nameCheck(recver_name);
		if(nid!=-1){	//exist user
			User::allUser[nid]->money += give;
		}
		// end
		cout<< name <<"@"<<strAddr<<":"<<portN<<" has give $"<<give<<" to ";
		cout<< recver->name <<"."<<endl;
	   return true;
	}
	
	char* getAddr(){
		sprintf(strAddr, "%s", inet_ntoa(user_addr.sin_addr));	
		return strAddr;
	}
	int getPort(){
		char s[6] = "";
		sprintf(s, "%d", (int) ntohs(user_addr.sin_port));
		int i = atoi(s);
		portN = i;
		return i;
	}
	
	private:
};
int User::totalUser = 0;
User* User::allUser[50] = {};



// theard function
void* inThread(void* this_user){
	User* user = (User*)this_user;
	cout<<"in the thread: get user @ "<<user<<endl<<endl;
	int newSocket = user->sockID;	
	// main action
	int nBytes = 1;
   /*loop while connection is live*/
   while(nBytes!=0){
   	memset(buff, '\0', sizeof(buff));		// clean the buff
   	
		nBytes = recv(newSocket,recvBuff,128,0);
		
		if(nBytes==0){
			cout<<"client "<<user->name<<"@"<<user->strAddr<<":"<<user->portN<<" close connection"<<endl;
			delete user;
			cout<<"now have "<<User::totalUser<<" users."<<endl;
			break;
		}
		
		cout<<endl<<"recv: "<<recvBuff<<endl;
		recvHandle(recvBuff, user);
	}
	cout<<"thread end."<<endl;
}
	
	
	
int main(int argc, char *argv[]){	
	// open socket
	if(argc==1){
		Start(argc);
	}else{
		Start(argc, argv[1], argv[2]);
	}
	
	//pre record
	char nameA[25] = "Alice";
	RecordUser::allRecord[0] = new RecordUser(nameA,500);
	char nameB[25] = "Bob";
	RecordUser::allRecord[1] = new RecordUser(nameB,1000);
	
	//accept
	struct sockaddr_in clientAddr;
	socklen_t addr_size = sizeof(clientAddr);
	while(true){
		//accept & creat new user
		int newSocket = accept(sd, (struct sockaddr *) &clientAddr, &addr_size);
		int uid = User::totalUser;
		User::allUser[uid] = new User(newSocket,clientAddr);	
		int rc = pthread_create(&threads[User::totalUser-1], NULL, inThread, (void*)User::allUser[uid]);
		if(rc!=0){
			cout<<"pthread_create fail!"<<endl;
		}		   
	}
	
	pthread_exit(NULL);
	return 0;
}




int SOCK(const char* IP_, int port){
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
   serv_addr.sin_addr.s_addr = inet_addr(IP_); 		// IP address	
   serv_addr.sin_port = htons(port); 					// port number
	
	//bind
	int a = bind(sd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if(a==-1){
		cout<<"Fail to bind socket: "<<strerror(errno)<<endl;
		return -1;
	}

	//listen
	if(listen(sd,10)!=0){
		cout<<"listen false";
		return -1;
	}

	return sd;
}


bool REG(char* name, int money, User* user){
	cout<<"try to REG "<<name<<endl;
	bool ok = user->reg(name,money);
	//change port & name
	if(ok){
		send(user->sockID,&"100 OK\n",10,0);		
		cout<<"REG success"<<endl;	
		
	}else{
		send(user->sockID,&"200 FAIL\n",10,0);	
		cout<<"REG fail"<<endl;
	}	
	return ok;
}

bool LOG_IN(char* name, int port, User* user){
	bool ok = user->login(name,port);
	if(ok){
		LIST(user);
		cout<<"LOGIN success."<<endl;	
	}else{
		send(user->sockID,&"220 AUTH_FAIL\n",15,0);
		cout<<"LOGIN fail."<<endl;	
	}
	return ok;
}

bool LOG_OUT(User* user){
	bool ok = user->logout();
	if(ok){
		send(user->sockID,&"Bye\n",5,0);
		cout<<"LOGOUT success."<<endl;	
	}else{
		send(user->sockID,&"210 FAIL\n",15,0);
		cout<<"LOGOUT fail."<<endl;	
	}
	return ok;
}

bool LIST(User* user){
	memset(buff, '\0', sizeof(buff));		// clean the buff
	int i = 0, j = 0, k=0;
	
	//money
	char m[10] = "";
	sprintf(m, "%d", user->money);
	for(i; m[i]!='\0' && i<10; i++){
		buff[i] = m[i];
		j += 1;
	}
	buff[j] = '\n';
	j += 1;
	
	//number of users
	char num[10] = "";
	i = 0;
	sprintf(num, "%d", User::totalUser);
	for(i; num[i]!='\0' && i<10; i++){
		buff[j] = num[i];
		j += 1;
	}
	buff[j] = '\n';
	j += 1;
	
	//for all users
	i = 0;
	int n = User::totalUser;
	for(i; i<n && j<sizeof(buff)-1; i++){
		//get name	
		char* name = User::allUser[i]->name;
		cout<<"exist user ("<<i<<"): "<<name<<", write from ["<<j<<"]"<<endl;
		k = 0;
		for(k; k<20 && name[k]!='\0'; k++){
			buff[j] = name[k];
			j += 1;
		}
		buff[j] = '#';
		j += 1;
		
		//get user addr
		char* s = User::allUser[i]->strAddr;
		k = 0;
		for(k; k<50 && s[k]!='\0'; k++){
			buff[j] = s[k];			
			j += 1;
		}
		buff[j] = '#';
		j += 1;
		
		//get port
		char m[6] = "";
		k = 0;
		sprintf(m, "%d", User::allUser[i]->portN);
		for(k; m[i]!='\0' && m[i]!='\n' && k<5; k++){
			cout<<m[k]<<endl;
			buff[j] = m[k];
			j += 1;
		}		
		buff[j] = '\n';
		j += 1;
	}
	buff[j] = '\0';
	
	cout<<"return LIST (size="<<sizeof(buff)<<"):"<<endl<<buff<<endl;	
	send(user->sockID,buff,sizeof(buff),0);
	return true;
}

bool ASK_PAY(User* user, char* recver, int money){	//send recver data
	int where = now_nameCheck(recver);
	if(where==-1){		// no these recver
		send(user->sockID,&"220 AUTH_FAIL\n",20,0);
		cout<<"PAY fail: recver doesn't online.'"<<endl;
		return false;	
	}else{
		User* recv_ = User::allUser[where];
		send(user->sockID, recv_->print(), 100,0);
		cout<<"Send data: "<<recv_->print()<<endl;	
	}
	return true;
}

bool CHECK_PAY(User* user, char* recver, int money){	// change data
	bool ok = user->payTo(money, recver);
	if(ok){
		send(user->sockID,&"100 OK\n",15,0);
		cout<<"Pay recorded success."<<endl;	
	}else{
		send(user->sockID,&"200 FAIL\n",15,0);
		cout<<"Pay recorded fail."<<endl;	
	}
	return ok;
}


bool recvHandle(char* ask, User* user){
	smatch sm;
		
	//REG
	if(regex_search(ask,ask+128,REG_)){
		int i=9;
		//name
		char name[25] = "";
		for(i; ask[i]!='#'; i++){
			name[i-9] = ask[i];
		}
		i += 1;
		// $
		int j=0;
		char pn[6] = "";
		for(i,j; ask[i]!='\0' && ask[i]!='\n'; i++,j++){
			pn[j] = ask[i];
		}
		int money = atoi(pn);		
		//do
		cout<<"server reg: "<<name<<", $"<<money<<endl;
		REG(name,money,user);
	}
	
	//payment: ask
	else if(regex_search(ask,ask+128,ASK_PAY_)){
		int i=0;
		//name
		char name[25] = "";
		for(i; ask[i]!='#'; i++){
			name[i] = ask[i];
		}		
		//money
		char p[7] = "";
		i += 1;
		int n=i;
		for(i; ask[i]!='#'; i++){
			p[i-n] = ask[i];
		}
		int money = atoi(p);				
		//recv_name	
		i += 1;
		n=i;	
		char recv_name[25] = "";
		for(i; ask[i]!='\0' && ask[i]!='\n'; i++){
			recv_name[i-n] = ask[i];
		}		
		//do
		cout<<"server payment: from "<<name<<" to "<<recv_name<<", $"<<money<<endl;
		ASK_PAY(user, recv_name, money);
	}
	
	//payment: check
	else if(regex_search(ask,ask+128,CHECK_PAY_)){
		int i=6;
		//name
		char name[25] = "";
		for(i; ask[i]!='#'; i++){
			name[i] = ask[i];
		}		
		//money
		char p[7] = "";
		i += 1;
		int n=i;
		for(i; ask[i]!='#'; i++){
			p[i-n] = ask[i];
		}
		int money = atoi(p);				
		//recv_name	
		i += 1;
		n=i;	
		char recv_name[25] = "";
		for(i; ask[i]!='\0' && ask[i]!='\n'; i++){
			recv_name[i-n] = ask[i];
		}		
		//do
		cout<<"server payment check: has transmit $"<<money<<" from "<<name<<" to "<<recv_name<<endl;
		CHECK_PAY(user, recv_name, money);
	}
	
	//LOGIN
	else if(regex_search(ask,ask+128,LOGIN_)){
		int i=0;
		//name
		char name[25] = "";
		for(i; ask[i]!='#'; i++){
			name[i] = ask[i];
		}
		//port
		char p[7] = "";
		i += 1;
		int n=i;
		for(i; ask[i]!='\0' && ask[i]!='\n'; i++){
			p[i-n] = ask[i];
		}
		int port = atoi(p);
		//do
		cout<<"server login: "<<name<<"@"<<port<<endl;
		LOG_IN(name,port,user);
	}
	
	//LIST
	else if(regex_search(ask,ask+128,LIST_)){
		LIST(user);
	}
	
	//LOGOUT
	else if(regex_search(ask,ask+128,LOGOUT_)){
		LOG_OUT(user);
		//do not close thread
	}
	
	//exception
	else{
		cout<<"Not a command."<<endl;
		char ff[10] = "200 FAIL";
		send(user->sockID,ff,sizeof(ff),0);
		return false;
	}
}

int nameCheck(char* name_){
	int i=0;
	for(i; i<RecordUser::totalRecord; i++){
		char* na = RecordUser::allRecord[i]->name;
		bool same = true;
		for(int c=0; na[c]!='\0' && c<25; c++){
			if(name_[c]!=na[c]){
				same = false;
				break;
			}
		}
		if( same ){	//totally the same
			cout<<"match record ("<<i<<"): "<<na<<"."<<endl;
			return i;
		}
		cout<<"not match record ("<<i<<"): "<<na<<"."<<endl;
	}
	cout<<"not match any record."<<endl;
	return -1;
}
int now_nameCheck(char* name_){
	int i=0;
	for(i; i<User::totalUser; i++){
		char* na = User::allUser[i]->name;
		bool same = true;
		for(int c=0; na[c]!='\0' && c<25; c++){
			if(name_[c]!=na[c]){
				same = false;
				break;
			}
		}
		if( same ){	//totally the same
			cout<<"match record ("<<i<<"): "<<na<<"."<<endl;
			return i;
		}
	}
	cout<<"not match any record."<<endl;
	return -1;
}

bool Start(int n, char* ip, char* port){
	
	if(n==1){
		cout<<"Use setting: IP = "<<IP<<", port = "<<Port<<endl;
		sd = SOCK(IP, Port);
	}else{	
		// exception handleing
		if( !regex_match(ip,ip+strlen(ip),isIP) ){
			cout<<"Illegal IP addresse."<<endl;
			return false;
		}
		
		if( !regex_match(port,port+strlen(port),isPort) ){
			cout<<"Illegal port number."<<endl;
			return false;
		}
		int p = atoi(port);
		if( p<0 || p>PortMax){
			cout<<"Illegal port number."<<endl;
			return false;
		}	
		
		// connect
		cout<<"Open server at "<<ip<<", port="<<p<<endl;
		sd = SOCK(ip, p);
	}	
	
	//check socket	
	if( sd < 0){
		cout<<"Socket fail, end process"<<endl;
		shutdown(sd,SHUT_RDWR);
		return false;
	}
	
	cout<<endl<<"Welcome to this P2P program."<<endl;
	return true;
}


