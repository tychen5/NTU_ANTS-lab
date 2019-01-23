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
using namespace std;

#define MAXBUF 1024

SSL_CTX *ctx;
SSL_CTX *sctx;
SSL* c_ssl;
SSL* trade_ssl;
SSL* cs_c_ssl;

SSL_CTX* InitCTX(void){   
    
    const SSL_METHOD *method;
    SSL_CTX *fctx;
 
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = TLSv1_client_method();  /* Create new method instance */
    fctx = SSL_CTX_new(method);   /* Create new context */
    if ( fctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return fctx;
}

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
    //if (send(sockfd,msg.c_str(),msg.size(),0)<0)
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
    //ssl initialize
    SSL_library_init();
    ctx = InitCTX();
    sctx = InitServerCTX();
    c_ssl = SSL_new(ctx);

    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket的連線
    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr("127.0.0.1");//server ip address
    info.sin_port = htons(8700);
    
    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        cout<<"Connection error"<<"\n";
        return 0;
    }
    else{
        SSL_set_fd(c_ssl, sockfd);      // set connection socket to SSL state
        if ( SSL_connect(c_ssl) < 0 ){
            cout<<"SSL connection error!!!"<<endl;
            return 1;
        }
        cout<<"successfully connected"<<"\n";
    }

    //Send connection message to server
    string connectMessage="";
    connectMessage = rec_msg(sockfd, c_ssl);

    char message[100] = {}; 
    char name[100] = {};
    char port[10] = {};
    char balance[100] = {};
    bool Login = false;
    bool Register = false;
    
    while(Login == false){
        cout<<"Enter 1 for register, 2 for login: ";
        cin>>message;
        string s(message);
        if (s.compare("1") == 0){
            cout<<"Enter the name you want to register: ";
            cin>>name;
            string registerResponse="";
            char registerMessage[100] = {"REGISTER#"};
            strcat(registerMessage, name);
            send_msg(sockfd, registerMessage, c_ssl);
            registerResponse = rec_msg(sockfd, c_ssl);
            cout << registerResponse <<endl;
            cout<<"\n";
            Register = true;
        }
        else if (s.compare("2") == 0){
            if (Register == false){
                cout<<"You need to register first."<<"\n"<<"\n";
            }
            else{
                char loginName[100] = {};
                cout<<"Enter your name: ";
                cin>>loginName;
                cout<<"Enter the port number: ";
                cin>>port;
                if (std::stoi(port) < 1024 || std::stoi(port) > 65535){
                    cout<<"Please enter a port number between 1024 and 65535."<<"\n"<<"\n";
                }
                else{
                    cout<<"How much would you like to save? ";
                    cin>>balance;
                    string str_name(name);
                    string str_loginName(loginName);
                    char loginMessage[100] = {"#"};
                    strcat(loginName, loginMessage);
                    strcat(loginName, port);
                    strcat(loginName,"#");
                    strcat(loginName,balance);
                    send_msg(sockfd, loginName, c_ssl);
                    
                    string loginResponse1 = "";
                    string loginResponse2= "";
                    
                    if (str_name == str_loginName){

                        loginResponse1 = rec_msg(sockfd, c_ssl);
                        loginResponse2 = rec_msg(sockfd, c_ssl);
                        cout << loginResponse1 <<endl;
                        cout << loginResponse2 <<endl;
                        cout<<"\n"<<"\n";
                        Login = true;
                        break;
                    }
                    else{
                        cout<<"You didn't register that name."<<"\n";
                        loginResponse1 = rec_msg(sockfd, c_ssl);
                        cout << loginResponse1 << endl;
                        cout<<"\n";
                    } 
                }
            }
        }
        else{
            cout<<"Invalid input."<<"\n"<<"\n";
        }
    }
    
    while (Login == true){
        //if server msg = TransferRequest#Client A, open socket and be server
        string input_amount = "";


        //else server msg = server listening
        cout<<"Enter the action you want to take. 1 to ask for latest list, 5 to transfer money, 6 to wait for transaction, 8 to exit: ";
        

        memset(message, 0, 100);
        cin>>message;
        


        string s2(message);
        if (s2.compare("1") == 0){
            char listMessage[100] = {"List"};
            string listResponse1 = "";
            char listResponse2[100] = {};
            send_msg(sockfd, listMessage, c_ssl);
            listResponse1= rec_msg(sockfd, c_ssl);
            cout << listResponse1 <<endl;
            cout<<"\n";
        }
        else if(s2.compare("5") == 0){
            //get ip and port from server
            char payeeAccount[100] = {};
            cout << "Enter the payee's account: ";
            cin>> payeeAccount;
            cout << "Enter the transaction amount: ";
            cin >> input_amount;
            stringstream ss;
            ss << "Transfer#" << payeeAccount << "#" << input_amount;
            string transfer_ss = ss.str();
            char transfer_msg[100] = {};
            sprintf(transfer_msg, "%s", transfer_ss.c_str());
            send_msg(sockfd, transfer_msg, c_ssl);
            string transferResponse = "";
            transferResponse = rec_msg(sockfd, c_ssl);
            cout <<transferResponse << endl;


        }
        else if (s2.compare("6")==0){
            char TransMessage[100] = {"Trans"};
            send_msg(sockfd, TransMessage, c_ssl);
            string serverMessage = "";
            serverMessage = rec_msg(sockfd, c_ssl);
            cout << serverMessage << endl;
            string server_str(serverMessage);
            string delimiter = "#";
            string command = server_str.substr(0, server_str.find(delimiter));
            
            //open socket
            // int sock2 = 0;

            // sock2 = socket(AF_INET , SOCK_STREAM , 0);
            // struct sockaddr_in info;
            // bzero(&info,sizeof(info));
            // info.sin_family = PF_INET;
            // info.sin_addr.s_addr = inet_addr("127.0.0.1");
            // info.sin_port = htons(8800);
            // connect(sock2,(struct sockaddr *)&info,sizeof(info));

            // int payment;
            // cout << "Enter how much you want to transfer: ";
            // cin >> payment;
            // //if client B accepts then transfer succeeds
            // //close connection with B
            // //update the account balance with server
            // connect(sockfd,(struct sockaddr *)&info,sizeof(info));
            // get former account balance
            // former - payee
        }
        else if(s2.compare("8") == 0){
            char exitMessage[100] = {"Exit"};
            string exitResponse = "";
            send_msg(sockfd, exitMessage, c_ssl);
            exitResponse = rec_msg(sockfd, c_ssl);
            //cout<<"Bye"<<"\n";
            cout << exitResponse <<endl;
            Login = false;
            break;
        }
        else{
            cout<<"Invalid input."<<"\n"<<"\n";
        }
        
        
              
        
    }
    
    printf("client close Socket\n");
    close(sockfd); 
    return 0;
}
