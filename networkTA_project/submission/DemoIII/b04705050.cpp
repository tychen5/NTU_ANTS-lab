#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<string>
#include <iostream>
#include <string.h>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream> 
#include <map>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <unistd.h>
using namespace std;


#define MAXBUF 1024

void *task1(void *);
static int newsockfd;
map<string, string> mapIP_Port;
vector <string> ip;
map<string, int> mapBalance;
map<string, int> mapThread;
map <string,string> usersOnline;
char Info[100] = {};
char ip_address[100];

SSL_CTX *ctx;
map<int , SSL*> sock_ssl;
map<string, SSL*> mapSSL;
SSL* server_ssl ;

//Init server instance and context
SSL_CTX* InitServerCTX(void)
{   
    const SSL_METHOD *method;
    SSL_CTX *fctx;
 
     // & register all cryptos, etc. 
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



void send_msg(int sockfd, string msg , SSL* fssl){  //0 for success
    //if (send(sockfd,msg.c_str(),msg.size(),0)<0){
    if (SSL_write(fssl, msg.c_str(), msg.size()) <0 ){ // encrypt & send message 
        cout<<"Error: Sending msg."<<endl;
    }
}

string rec_msg(int sockfd , SSL* fssl){
    char buf[MAXBUF];
    char *bptr=buf;
    int buflen=MAXBUF;
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
    //載入certificate    
    SSL* ssl;
    SSL_library_init(); //init SSL library
    ctx = InitServerCTX();  //initialize SSL 
    LoadCertificates(ctx, "cert.pem", "key.pem"); // load certs and key

    //socket的建立
    int sockfd = 0;
    struct sockaddr cli_addr;
    socklen_t clilen;
    pthread_t threadA[3];
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket的連線
    struct sockaddr_in serverInfo,clientInfo;
    int addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));
    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(8700);
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sockfd,5);

    ssl = SSL_new(ctx);        // get new SSL state with context 
    SSL_set_fd(ssl, sockfd);  // 連接用户的 socket 加入到 SSL
    sock_ssl[sockfd] = ssl;
    server_ssl = ssl;

    int noThread = 0;
    while(noThread < 3){
        cout<<"Listening"<<endl;
        SSL* c_ssl;
        newsockfd = accept(sockfd, &cli_addr, &clilen);
        if (newsockfd < 0){
            cout<<"Cannot connect."<<endl;
        }
        else{
            cout<<"Successfully connected. "<<endl;
            struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&cli_addr;
            struct in_addr ipAddr = pV4Addr->sin_addr;
            inet_ntop(AF_INET, &ipAddr, ip_address, 100);
            c_ssl = SSL_new(ctx);//產生新的ssl
            SSL_set_fd(c_ssl, newsockfd);//連接socket
            if ( SSL_accept(c_ssl) < 0){ //SSL連接失敗
                cout<<"SSL Acception Failed!"<<endl;
                SSL_free(c_ssl);
                continue;
            }
            sock_ssl[newsockfd] = c_ssl;
              
        }
        pthread_create(&threadA[noThread], NULL, task1, NULL); 
        noThread++;
    }
    for(int i = 0; i < 3; i++){
        pthread_join(threadA[i], NULL);
    }
    return 0;
}

void *task1 (void *arg)
{
    int thread_socket = newsockfd;
    string thread_ip = ip_address;
    int payer_thread = 0;
    SSL* c_ssl;
    c_ssl = sock_ssl[thread_socket];
    cout << "Thread No: " << pthread_self() << endl;
    bool loop = false;
    bool registered = false;
    bool login = false;
    string inputBuffer = "";
    char message[] = {"Hi from server.\n"};
    string name = "";
    string inputString (inputBuffer);
    map<string, string>::iterator iter;
    map<string, int>::iterator it;
    string payee_name = "";
    char serverMessage[] = {"\nServer Listening.\n"};
    while(!loop)
    {    
        if (registered == false && login == false){//first connection
            send_msg(thread_socket, message, c_ssl);
            
        }
        inputBuffer = rec_msg(thread_socket, c_ssl);
        string input_str(inputBuffer);
        string delimiter = "#";
        string command = input_str.substr(0, input_str.find(delimiter));
        cout<<"Recieve from client " <<inputBuffer<<endl;
        if (command == "REGISTER"){//REGISTER#name
            char registerResponse[100] = {"100 OK"};
            send_msg(thread_socket, registerResponse, c_ssl);
            name = input_str.erase(0,9);
            registered = true;
        }
        else if(command == "Exit"){
            
            send(thread_socket,"Bye \n",100,0);
            //remove user from online list
            iter = mapIP_Port.find(name);
            mapIP_Port.erase(iter);
            usersOnline.erase(name);
            mapThread.erase(name);
            mapSSL.erase(name);
            
            //update online user list
            stringstream ss;
            for ( iter = usersOnline.begin(); iter != usersOnline.end(); iter++ ){
                ss << iter -> first << "#" << mapIP_Port[iter->first] << "\n";
                cout << ss.str();
            }
            
            string userInfo = ss.str();
            char usersOnline_s[10];
            sprintf(usersOnline_s, "%1ld\n", usersOnline.size());
            stringstream online_ss;
            online_ss << "number of users online: " << usersOnline_s << userInfo; 
            string onlineInfo = online_ss.str();
            sprintf(Info, "%s", onlineInfo.c_str());
            break;
        }     
        else {
            
            if (login == false){
                if (command == name && registered == true){//name#11111#balance

                    
                    string login_input = input_str.erase(0,name.length()+1);
                    string port = login_input.substr(0, login_input.find(delimiter));
                    string input_balance = login_input.erase(0,port.length()+1);
                    cout<<"user: "<<name<<" wants to save "<<input_balance<<" (port: "<<port<<endl;
                    int balance = std::stoi(input_balance);//balance

                    stringstream ip_ss;
                    ip_ss << thread_ip << "#" << port;
                    string ip_port = ip_ss.str();
                    //find user 
                    it = mapBalance.find(name);
                    if (it != mapBalance.end()){//if the user has registered before
                        //update the account balance 
                        mapBalance[name] += balance;
                        usersOnline.insert(pair<string, string>(name, name));
                        mapIP_Port.insert(pair<string, string>(name, ip_port));
                        mapSSL.insert(pair<string, SSL*>(name, c_ssl));
                        mapThread.insert(pair<string, int>(name, thread_socket));
                    }
                    else{
                        usersOnline.insert(pair<string, string>(name, name));
                        
                        mapIP_Port.insert(pair<string, string>(name, ip_port));//"name" = "IP#port"
                        mapBalance.insert(pair<string, int>(name, balance));//"name" = "10000"
                        mapSSL.insert(pair<string, SSL*>(name, c_ssl));
                        mapThread.insert(pair<string, int>(name, thread_socket));
                    }
                    //send account balance
                    stringstream ss;
                    string newbalance = std::to_string(mapBalance[name]);
                    ss << newbalance <<"\n";
                    char userbalance[100] = {};
                    string balance_ss = ss.str();
                    sprintf(userbalance, "%s", balance_ss.c_str()); 
                    send_msg(thread_socket, userbalance, c_ssl);
                    
                    
                    //send list of users 
                    stringstream user_ss;
                    for ( iter = usersOnline.begin(); iter != usersOnline.end(); iter++ ){
                        user_ss << iter -> first << "#" << mapIP_Port[iter->first] << "\n";
                        cout << "ss.str" << user_ss.str()<<"ss.str";
                    }
                    
                    string userInfo = user_ss.str();
                    char usersOnline_s[10];
                    sprintf(usersOnline_s, "%1ld\n", usersOnline.size());//num of users
                    stringstream online_ss;
                    online_ss << "number of users online: " << usersOnline_s << userInfo; 
                    string onlineInfo = online_ss.str();
                    sprintf(Info, "%s", onlineInfo.c_str());
                    send_msg(thread_socket, Info, c_ssl);
                    cout<<"online info: "<<Info<<endl;
                    
                    login = true;
                }
                else{
                    cout<<"User not correctly registered..."<<endl;
                    char registerResponse[100] = {};
                    send_msg(thread_socket, registerResponse, c_ssl);  
                }  
            }
            else{
                if (command == "List"){
                    
                    send_msg(thread_socket, Info, c_ssl);
                    
                }
                else if(command == "Transfer"){
                    string substr1 = input_str.erase(0,9);//name#amount
                    string payee_name = substr1.substr(0, substr1.find(delimiter));
                    string input_balance = substr1.erase(0,payee_name.length()+1);
                    int balance = std::stoi(input_balance);
                    mapBalance[name] -= balance;
                    
                    mapBalance[payee_name] += balance;
                    stringstream payer_ss;
                    payer_ss<<mapIP_Port[payee_name]<<"\n"<<"Your Account Balance: "<<mapBalance[name]<<"\n\n";
                    payer_thread = mapThread[payee_name];
                    //send IP and port to Client A
                    char payee[100] = {};
                    // sprintf(payee_ip, "%s", mapIP_Port[payee_name].c_str());
                    string payer_s = payer_ss.str();
                    sprintf(payee, "%s", payer_s.c_str());
                    send_msg(thread_socket, payee, c_ssl);
                    //send TransferRequest#Client A to payee
                    stringstream transfer_ss;
                    

                    transfer_ss << "Your Account balance: " << mapBalance[payee_name]<<"\n\n";
                    string transfer_str = transfer_ss.str();
                    char transfer_msg[100] = {};
                    sprintf(transfer_msg, "%s", transfer_str.c_str());
                    cout<<"threadsock"<<thread_socket<<endl;
                    send_msg(payer_thread, transfer_msg, mapSSL[payee_name]);  
                    cout<<"transfer_msg: "<<payer_thread<<" "<<transfer_msg;

                }
                else if(command == "Trans"){
                    cout<<"waiting";
                }
                else{
                    cout<<"Invalid input"<<endl;
                }
            }
        }
    }
    send(thread_socket,"Bye \n",sizeof("Bye \n"),0);
    cout << "\nClosing thread and conn" << endl;
    close(thread_socket);
    return 0;
    
}


