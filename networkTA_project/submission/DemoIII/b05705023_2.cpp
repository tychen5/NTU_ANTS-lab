#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <pthread.h>

#include "openssl/bio.h"
#include "openssl/err.h"
#include "openssl/ssl.h"
#include <iomanip>

using namespace std;

#define CERT_C "client.crt"
#define KEY_C "client.key"
#define BUFFER 10240


/*user structure*/
struct user{
    string name;
    string ip;
    string port;
};
/*data to be pass*/
struct data{
    string port;
    BIO *bio;
};

vector<user> add_to_list(const string& list, string& balance, string& userNumber) {
  size_t pos = list.find("\n");
  balance = list.substr(0, pos);
  size_t pos2 = pos + 1;
  pos = list.find("\n", pos2);
  userNumber = list.substr(pos2, pos - pos2);
  vector<user> userList;
  pos2 = list.find('#', ++pos);
  while (pos2 != string::npos) {
    user tmp;
    tmp.name = list.substr(pos, pos2 - pos);
    pos = pos2 + 1;
    pos2 = list.find('#', pos);
    tmp.ip = list.substr(pos, pos2 - pos);
    pos = pos2 + 1;
    pos2 = list.find("\n", pos);
    tmp.port = list.substr(pos, pos2 - pos);
    userList.push_back(tmp);
    pos = pos2 + 1;
    pos2 = list.find('#', pos);
  }
  return userList;
}

void printList(const vector<user>& l, const string& b, string num) {
  cout << "Your balance is: " << b << endl;
  cout << "The total users online is: " << num << endl;
  cout << left << setw(6) << "NO." 
      << left << setw(20) << "Name" 
      << left << setw(20) << "Ip"
      << left << setw(10) << "Port" << endl;
  for (int i = 0, n = stoi(num); i < n; ++i) {
    cout << left << setw(6) << i + 1
      << left << setw(20) << l[i].name
      << left << setw(20) << l[i].ip
      << left << setw(10) << l[i].port << endl;
  }
  cout << endl;
}
void send(BIO *bio, string msg) {
  if (BIO_write(bio, msg.c_str(), msg.length()) <= 0)
    cout << "Error during writing message.";
}
string recv(BIO *bio) {
  char b[BUFFER];
  memset(b, 0, BUFFER);
  if (BIO_read(bio, b, sizeof(b)) <= 0)
    cout << "Error during reading message";
  
  return string(b);
}

void *p2p(void *param){
    data *data_p2p = (data *)param;

    BIO *bio = BIO_new_accept(data_p2p->port.c_str());
    if(bio == NULL){
        cout << "Error setting up p2p connection" << endl;
        return 0;
    }

    BIO *connection;
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_server_method());
    if(ctx == NULL){
        cout << "Error loading ctx" << endl;
        return 0;
    } 
    if (!SSL_CTX_use_certificate_file(ctx, CERT_C, SSL_FILETYPE_PEM))
        cout << "Error loading certificate" << endl;
    if (!SSL_CTX_use_PrivateKey_file(ctx, KEY_C, SSL_FILETYPE_PEM))
        cout << "Error loading private key" << endl;
    if (!SSL_CTX_check_private_key(ctx))
        cout << "Error during checking private key" << endl;
    connection = BIO_new_ssl(ctx, 0);
    BIO_set_accept_bios(bio, connection);
    if (BIO_do_accept(bio) <= 0)
        cout << "Error during connection" << endl;
    while (true) {
        if (BIO_do_accept(bio) <= 0)
            cout << "Error during connection"<< endl;
        connection = BIO_pop(bio);
        string recvMsg = recv(connection);
        if (recvMsg == "quit") 
            break;
        send(data_p2p->bio, recvMsg);
  }
  SSL_CTX_free(ctx);
  BIO_free(bio);
  BIO_free(connection);
  pthread_exit(NULL);
}

/*setting up connection*/
BIO *connection_setting(string server){
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    SSL *ssl;
    /*connect with ssl*/
   BIO *bio = BIO_new_ssl_connect(ctx);
    BIO_set_conn_hostname(bio, server.c_str());
    BIO_get_ssl(bio, &ssl);
    if (!ssl){ 
        cout << "Error finding ssl pointer";
        return 0;
    }
    if (SSL_get_verify_result(ssl) != X509_V_OK){
        cout << "Error verifying ssl connection";
        return 0;
    }
    if (BIO_do_connect(bio) <= 0){ 
        cout << "Error connection failed\n";
        return 0;
    }
    if (BIO_do_handshake(bio) <= 0){
        cout << "Error handshake failed\n";
        return 0;
    }
    
    return bio;

}



///////////////////////////////////////////////////
int main(int argc, char *argv[]){
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    SSL_library_init();

    char serverInfo[BUFFER] = {};
    strcpy(serverInfo, argv[1]);
    strcat(serverInfo, ":");
    strcat(serverInfo, argv[2]);

    BIO *bio = connection_setting(serverInfo);
    cout << "+++++++++++++++++++++++++++++++++++++++" << endl;

    int login = 0;
    char action;
    string balance, usrName, number;
    string loginPort = "-1";
    vector<user> list;

    while(login ==0){
        cout << "Enter 'R' to register, 'L' to login: ";
        cin >> action;

        if(action == 'R' || action == 'r'){
            string name;
            cout << "Please enter your name: ";
            cin >> name;
            cout << "Please enter your starting balance: ";
            cin >> balance;
            send(bio, "REGISTER#" + name + "#" + balance);
            string recvMsg = recv(bio);
            if(recvMsg.substr(0, 8) == "210 FAIL"){
                cout << "Fail to register, try again!";
            }
            else if(recvMsg.substr(0, 6) == "100 OK"){
                cout << "Registered as " << name;
            }
        }
        else if(action == 'L' || action == 'l'){
            string name, port;
            cout << "Please enter your name and port number: ";
            cin >> name >> port;
            int port_i = stoi(port);
            if(port_i < BUFFER || port_i > 65535){
                cout << "Invalid port! (range: BUFFER~65535)";
                break;
            }

            send(bio, name + "#" + port);
            string recvMsg = recv(bio);
            if(recvMsg.substr(0, 13) == "220 AUTH_FAIL"){
                cout << "Fail to login, please try again!";
            }
            else{
                login = 1;
                usrName = name;
                loginPort = port;
                list = add_to_list(recvMsg, balance, number);
                printList(list, balance, number);
            }
        }

        pthread_t worker;
        if(login == 1){
            data *param = new data{loginPort, bio};
            if (pthread_create(&worker, NULL, p2p, (void *)param))
                cout << "Error during creating thread";
        }

        while(login == 1){
            cout << "What action do you want to take?" << endl;
            cout << "Enter 'L' to get user list, 'T' to transact, 'E' to exit.";
            cin >> action;
            if(action == 'L' || action == 'l'){
                send(bio, "List");
                string recvMsg = recv(bio);
                list = add_to_list(recvMsg, balance, number);
                printList(list, balance, number);
            }
            else if(action == 't' || action == 'T'){
                send(bio, "List");
                string recvMsg = recv(bio);
                list = add_to_list(recvMsg, balance, number);
                printList(list, balance, number);

                int userN = 0, money = 0;
                cout << "Please enter the user number you want to create transaction:";
                cin >> userN;
                cout << "Please enter the amount of money: ";
                cin >> money;
                if(money > stoi(balance)){
                    cout << "Transaction fail, you don't have that much money. ";
                    break;
                }
                --userN;
                BIO *p2p2 = connection_setting(list[userN].ip + ":" +list[userN].port);
                send(p2p2, usrName + "#" + to_string(money) + "#" + list[userN].name);
                BIO_free(p2p2);
            }
            else if(action == 'E' || action == 'e'){
                cout << "Bye" << endl;
                login = -1;
                send(bio, "Exit");
            }
        }

        if(loginPort != "-1"){
            BIO *tmp = connection_setting("localhost:" + loginPort);
            send(tmp, "quit");
            BIO_free(tmp);
         }
        pthread_join(worker, NULL);
        pthread_exit(NULL);
        BIO_free(bio);

        return 0;
    }
}

