#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <cstdlib>
#include <map>
#include <list>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdlib.h>  


using namespace std;


#define MaxThread 5

#define BLEN 102400

static int conn_FD;
static char str[INET_ADDRSTRLEN];
static list<string> register_user;
static list<string> login_user;
static map<string, int> deposit_db;
void *task1(void *);
static int noThread;
SSL_CTX *ctx;
map<int , SSL*> sock_ssl;
SSL* server_ssl ;

//Init server instance and context
SSL_CTX* InitServerCTX(void)
{   
    const SSL_METHOD *method;
    SSL_CTX *fctx;
 
    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc. 
    SSL_load_error_strings();   // load all error messages */
    method = TLSv1_server_method();  // create new server-method instance 
    fctx = SSL_CTX_new(method);   // create new context from method
    if ( fctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return fctx;
}

//Load the certificate 
void LoadCertificates(SSL_CTX* fctx, string CertFile, string KeyFile)
{
    //set the local certificate from CertFile
    if ( SSL_CTX_use_certificate_file(fctx, CertFile.c_str(), SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //set the private key from KeyFile
    if ( SSL_CTX_use_PrivateKey_file(fctx, KeyFile.c_str(), SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //verify private key 
    if ( !SSL_CTX_check_private_key(fctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

bool send_msg(int sockfd, string msg , SSL* fssl){	//0 for success
	//if (send(sockfd,msg.c_str(),msg.size(),0)<0){
	if (SSL_write(fssl, msg.c_str(), msg.size()) <0 ){ // encrypt & send message 
		//cout<<msg<<endl;
		cout<<"Error: Sending msg."<<endl;
		return 1;
	}
	return 0;
}

string recv_msg(int sockfd , SSL* fssl){
	char buf[BLEN];
	char *bptr=buf;
	int buflen=BLEN;
	int n;

	memset(buf, '\0', sizeof(buf)); 

	n = SSL_read(fssl, bptr, buflen);
	if ( n < 0 ){
		cout<<"Recieving Error!"<<endl;
		return "error";
	}
    string ans (buf);
	return ans;
}



int main(int argc , char *argv[])

{

    SSL* ssl;
    
    SSL_library_init(); //init SSL library
	ctx = InitServerCTX();  //initialize SSL 
    LoadCertificates(ctx, "mycert.pem", "mykey.pem"); // load certs and key

    //socket的建立
    int port = atoi(argv[1]);
    pthread_t threadA[3];
    int sock_FD = 0,client_sock_FD = 0;
    sock_FD = socket(PF_INET , SOCK_STREAM , 0);

    if (sock_FD == -1){
        printf("Fail to create a socket.");
    }
    else{
        string res = "Server: socket created" + to_string(sock_FD);
        cout<< res << endl;
    }
    //socket的連線
    struct sockaddr_in serverInfo,clientInfo;
    int client_IP;
    socklen_t addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(port);
    bind(sock_FD,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sock_FD, 10);
    noThread = 0;

    ssl = SSL_new(ctx);              // get new SSL state with context 
    SSL_set_fd(ssl, sock_FD);      // set connection socket to SSL state
    sock_ssl[sock_FD] = ssl;
    server_ssl = ssl;
    
    while (noThread <= MaxThread)
    {
        SSL* c_ssl;
        cout<<noThread<<endl;
        cout << "Listening No."<< noThread << endl;
        //this is where client connects. svr will hang in this mode until client conn
        conn_FD = accept(sock_FD ,(struct sockaddr*) &clientInfo, &addrlen);
        if (conn_FD < 0)
        {
            cerr << "Cannot accept connection" << endl;
            return 0;
        }
        else
        {   
            cout << "Connection successful" << endl;
            inet_ntop(AF_INET, &(clientInfo.sin_addr), str, INET_ADDRSTRLEN);
            c_ssl = SSL_new(ctx);
            SSL_set_fd(c_ssl, conn_FD);      // set connection socket to SSL state

            if ( SSL_accept(c_ssl) < 0){ //do SSL-protocol accept
                cout<<"SSL Acception Failed!"<<endl;
                SSL_free(c_ssl);
                continue;
    		}
            sock_ssl[conn_FD] = c_ssl;
        }
        pthread_create(&threadA[noThread], NULL, task1, NULL); 
        
    }
    for(int i = 0; i < MaxThread; i++)
    {
        pthread_join(threadA[i], NULL);
    }
    cout<< "server shut down"<<endl;
    return 0;
}

void *task1 (void *dummyPt)
{   

    noThread++;
    int local_fd = conn_FD;
    printf("%s\n", str); 
    string ip = str + 0;
    ip = "#" + ip;
    cout << "Thread No: " << pthread_self() << endl;

    SSL* c_ssl;
    c_ssl = sock_ssl[local_fd];

    char greet[] = {"Connection accepted\n"};
    //send(local_fd, greet, strlen(greet), 0);
    send_msg(local_fd, greet, c_ssl);
    char receive[300] = {};
    bzero(receive, sizeof(receive));
    bool loop = false;
    bool login = false;
    list<string>::iterator login_name;
    string deposit_name ;
    while(!loop)
    {   
        bzero(&receive, sizeof(receive));
        string temp = "";
        while(temp.length()==0){

            //recv(local_fd, receive, sizeof(receive), 0);
            temp = recv_msg(local_fd, c_ssl);
        }

        cout<<temp<<endl;

        if(temp == "Exit"){
            login_user.erase(login_name);
            loop = true;
            login = false;
        }
        else if(temp.find("#") != std::string::npos){
            if (temp.find("REGISTER#") != std::string::npos){
                int index = temp.find("#");
                string user = temp.substr(index +1 );
                user.append(ip);
                list<string>::iterator it = find(register_user.begin(), register_user.end(), user);
                if (register_user.end() == it ){
                    register_user.push_front(user);
                    cout<<"new user registered: " << user<<endl;
                    //send(local_fd, "100 OK\n", strlen("100 OK\n"), 0);
                    send_msg(local_fd, "100 OK\n", c_ssl);
                }
                else{
                    cout<<"user have been registered: " << user<<endl;
                    send_msg(local_fd, "210 FAIL\n", c_ssl);
                    //send(local_fd, "210 FAIL\n", strlen("210 FAIL\n"), 0);
                }
            }
            else{
                int index = temp.find("#");
                string name = temp.substr(0, index);
                name.append(ip);
                string port = temp.substr(index + 1);
                cout<< "user "<< name << " is trying to login to server, its available port "<<port<<endl; 
                list<string>::iterator it = find(register_user.begin(), register_user.end(), name);
                if (register_user.end() == it ){
                    cout<<"user name "<< name<<" is not registered " <<endl;
                    send_msg(local_fd, "220 AUTH_FAIL\n", c_ssl);
                    //send(local_fd, "220 AUTH_FAIL\n", strlen("220 AUTH_FAIL\n"), 0);
                }
                else{
                    login = true;
                    string login_info;
                    deposit_name = name;
                    login_info = name + "#" +port;
                    login_user.push_front(login_info);
                    login_name = find(login_user.begin(), login_user.end(), login_info);
                    list<string>::iterator it = login_user.begin();

                    map<string, int>::iterator iter;
                    iter = deposit_db.find(deposit_name);
                    if(iter == deposit_db.end())   
                        deposit_db [deposit_name] = 10000;


                    char send_mes[10000] = {};
                    for(int i=0;i<login_user.size();i++){
                        cout<<*it<<endl;
                        strcat(send_mes, (*it).c_str());
                        if(i <= login_user.size()-1){
                            strcat(send_mes, "\n");
                        }
                        advance(it, 1);
                    }
                    cout<<"user " <<name<<" succeessfully login"<<endl;
                    char str [100];
                    sprintf(str, "%d", deposit_db [deposit_name]);
                    strcat(str, "\n");
                    send_msg(local_fd, str, c_ssl);
                    send_msg(local_fd, send_mes, c_ssl);
                    //send(local_fd, str, strlen(str), 0);
                    //send(local_fd, send_mes, strlen(send_mes), 0);
                }
            }
        }
        else if (temp.find("Depo")!= std::string::npos ){
            char ans [100] = "Your account is ";
            char str [100];
            sprintf(str, "%d", deposit_db [deposit_name]);
            strcat(str, "\n");
            strcat(ans, str);
            //send(local_fd, ans, strlen(ans), 0);
            send_msg(local_fd, ans, c_ssl);
            bzero(&receive, sizeof(receive));
            //recv(local_fd, receive, 10000, 0);
            string temp_rec = recv_msg(local_fd, c_ssl);
            int amount = atoi(temp_rec.c_str()) ;
            cout<<"User "<<deposit_name<< " deposit "<<amount<<endl;
            deposit_db [deposit_name] += amount;
        }        
        else if(temp.find("List") != std::string::npos && login){
            list<string>::iterator it = login_user.begin();
            char send_mes[10000] = {};
            for(int i=0;i<login_user.size();i++){
                cout<<*it<<endl;
                strcat(send_mes, (*it).c_str());
                if(i <= login_user.size()-1){
                    strcat(send_mes, "\n");
                }
                advance(it, 1);
            }
            //send(local_fd, send_mes, strlen(send_mes), 0);
            send_msg(local_fd, send_mes, c_ssl);
        }
        else if (temp.find("Check_account") != std::string::npos && login){
            char ans [100] = "Your account is ";
            char str [100];
            sprintf(str, "%d", deposit_db [deposit_name]);
            strcat(str, "\n");
            strcat(ans, str);
            //send(local_fd, ans, strlen(ans), 0);
            send_msg(local_fd, ans, c_ssl);
            cout<<"User "<<deposit_name << " deposit "<<deposit_db [deposit_name]<<endl;
        }
        else{
            continue;
        }
    }
    //send(local_fd, "closing", strlen("closing"), 0);
    send_msg(local_fd, "closing", c_ssl);
    cout << "\nClosing thread and connection" << endl;
    close(local_fd);
    noThread --;
    pthread_exit(0);
    return 0;
}