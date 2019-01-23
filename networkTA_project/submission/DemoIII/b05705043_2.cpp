#include <iostream>
#include <iomanip>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <pthread.h>
#include "openssl/bio.h"
#include "openssl/err.h"
#include "openssl/ssl.h"

using namespace std;

#define C_CERT "client.crt"
#define C_KEY  "client.key"
#define BLEN 1024

void printErr(string err) {
  cerr << err << endl;
  exit(1);
}

struct userInfo {
  string _name;
  string _ip;
  string _port;
};
struct transData {
  string _p;
  BIO *bio;
};

void *p2p(void *param);
vector<userInfo> listParser(const string&, string&, string&);
void printList(const vector<userInfo>&, const string&, string);
void printWel(const string& name);

void send(BIO *bio, string msg) {
  if (BIO_write(bio, msg.c_str(), msg.length()) <= 0)
    printErr("Error during writing message\n");
}

string recv(BIO *bio) {
  char b[BLEN];
  memset(b, 0, BLEN);
  if (BIO_read(bio, b, sizeof(b)) <= 0)
    printErr("Error during reading message\n");
  
  return string(b);
}

bool is_number(const string& s) {
  string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

BIO *setup_conn(string server) {
  SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
  SSL *ssl;

  BIO *bio = BIO_new_ssl_connect(ctx);
  BIO_get_ssl(bio, &ssl);
  BIO_set_conn_hostname(bio, server.c_str());

  if (!ssl)
    printErr("Error during getting ssl connection");
  if (SSL_get_verify_result(ssl) != X509_V_OK)
    printErr("Error during verifying ssl connection");
  if (!bio)
    printErr("Error during setting up bio");
  if (BIO_do_connect(bio) <= 0)
    printErr("Error during bio connection");
  if (BIO_do_handshake(bio) <= 0)
    printErr("Error during bio handshaking");
  return bio;
}

int main(int argc, char *argv[]) {
  if (argc < 3)
    printErr("Usage: " + string(argv[0]) + "<serverHost> <serverPort>\n");

  // init
  SSL_load_error_strings();
  ERR_load_BIO_strings();
  OpenSSL_add_all_algorithms();
  SSL_library_init();

  // setup bio
  char serverName[BLEN] = {};
  strcpy(serverName, argv[1]);
  strcat(serverName, ":");
  strcat(serverName, argv[2]);

  BIO *bio = setup_conn(serverName);
  
  // receive the welcome message
  string wel = recv(bio);
  cout << wel << endl;

  cout << "==================================================\n";
  cout << "= " << left << setw(47) 
      << "Welcome To Secure P2P Micropayment System" << "=\n";
  cout << "==================================================\n" << endl;

  int login = 0;
  string input, loginName, bal, userNum;
  string loginPort = "-1";
  vector<userInfo> userList;
  // before login
  while (login == 0) {
    cout << "What action do you want to take?\n"
        << "(R)egister (S)ign-in (E)xit: ";
    cin >> input; 
    
    switch (input[0]) {
      case 'r':
      case 'R': {
        string name, bal;
        cout << "Please enter your username: ";
        cin >> name;

        // handle the illegal username
        if (name.find("#", 0) != string::npos) {
          cout << "illegal username!\n" << endl;
          break;
        }

        cout << "Please enter your starting balance: ";
        cin >> bal;
        
        // handle the illegal balance 
        if (!is_number(bal)) {
          cout << "the balance is not a number!\n" << endl;
          break;
        }
        
        send(bio, "REGISTER#" + name + "#" + bal + "\n");
        string ret = recv(bio);
        if (ret.substr(0, 6) == "100 OK") {
          cout << "Successful!" << endl << endl;
        }
        else if (ret.substr(0, 8) == "210 FAIL") {
          cout << "Fail!" << endl << endl;
        }
        break;
      }
      case 's':
      case 'S': {
        string name, p;
        cout << "Please enter your username: ";
        cin >> name;
        // handle the illegal username
        if (name.find("#", 0) != string::npos) {
          cout << "illegal username!\n" << endl;
          break;
        }

        cout << "Please enter the port you want to login: ";
        cin >> p;
        
        // handle the illegal port
        if (!is_number(p)) {
          cout << "the port is not a number!\n" << endl;
          break;
        }
        int pp = stoi(p);
        if (pp > 65535 || pp < 1024) {
          cout << "the port is out of range! (1024~65535)\n" << endl;
          break;
        }

        send(bio, name + "#" + p + "\n");
        string ret = recv(bio);
        if (ret.substr(0, 13) == "220 AUTH_FAIL")
          cout << "Login Failed, please enter again!\n" << endl; 
        else { // login seccessfully
          login = 1;
          loginName = name;
          loginPort = p;
          userList = listParser(ret, bal, userNum);
          printWel(name);
          printList(userList, bal, userNum);
        }
        break;
      }
      case 'e':
      case 'E': {
        string e;
        cout << "Are you sure to exit? [y/N]\n";
        cin >> e;
        if (e[0] == 'Y' || e[0] == 'y') {
          cout << "Bye~" << endl;
          login = -1;
          break;
        }
        else {
          break;
        }
      }
      default:
        cout << "the action " << input << " is not legal, please enter again!\n" << endl;
    }
  }
  
  pthread_t worker;
  if (login == 1) {
    transData *param = new transData{loginPort, bio};
    if (pthread_create(&worker, NULL, p2p, (void *)param))
        printErr("Error during creating thread");
  }

  while (login == 1) {
    cout << "What action do you want to take?\n"
        << "(L)ist (T)ransaction (E)xit: ";
    cin >> input;
    cout << endl;
    switch (input[0]) {
      case 'l':
      case 'L': {
        send(bio, "List\n");
        string ret = recv(bio);
        // update user list
        userList = listParser(ret, bal, userNum);
        printList(userList, bal, userNum);
        break;
      }
      case 't':
      case 'T': {
        // update list before transaction
        send(bio, "List\n");
        string ret = recv(bio);
        // update user list
        userList = listParser(ret, bal, userNum);
        printList(userList, bal, userNum);

        int u;
        int t;
        cout << "Please enter the user number(1~" << userList.size() <<  "): ";
        cin >> u;
        while (u < 1 || u > int(userList.size())) {
          cout << "number " << u << " is out of range(1~" << userList.size() << "), "
              << "please enter again! ";
          cout << endl;
          cin >> u;
        }
        cout << "Please enter the transaction amount: ";
        cin >> t;
        cout << endl;
        if (t < 0) {
          cout << "transaction amount must be positive!" << endl;
          cout << endl;
          break;
        }
        if (t > stoi(bal)) {
          cout << "Insufficient balance!" << endl;
          cout << endl;
          break;
        }
        --u;
        BIO *p2pb = setup_conn(userList[u]._ip + ":" + userList[u]._port);
        send(p2pb, loginName + "#" + to_string(t) + "#" + userList[u]._name + "\n");
        BIO_free(p2pb);
        break;
      }
      case 'e':
      case 'E': {
        string e;
        cout << "Are you sure to exit? [y/N]\n";
        cin >> e;
        if (e[0] == 'Y' || e[0] == 'y') {
          cout << "Bye~" << endl;
          login = -1;
          send(bio, "Exit\n");
          break;
        }
        else {
          break;
        }
      }
      default:
        cout << "the action " << input << " is not legal, please enter again!\n" << endl;
    }
  }

  if (loginPort != "-1") {
    BIO *tmp = setup_conn("localhost:" + loginPort);
    send(tmp, "quit");
    BIO_free(tmp);
  }
  pthread_join(worker, NULL);
  pthread_exit(NULL);
  BIO_free(bio);

  return 0;
}

void *p2p(void *param) {
  transData *data = (transData *)param;
  
  BIO *bio = BIO_new_accept(data->_p.c_str());
  if (bio == NULL)
    printErr("Error setting up p2p connection\n");

  BIO *conn;
  SSL_CTX *ctx = SSL_CTX_new(SSLv23_server_method());
  if (ctx == NULL)
    printErr("Error loading ctx\n");
  if (!SSL_CTX_use_certificate_file(ctx, C_CERT, SSL_FILETYPE_PEM))
    printErr("Error loading certificate\n");
  if (!SSL_CTX_use_PrivateKey_file(ctx, C_KEY, SSL_FILETYPE_PEM))
    printErr("Error loading private key\n");
  if (!SSL_CTX_check_private_key(ctx))
    printErr("Error during checking private key\n");
  conn = BIO_new_ssl(ctx, 0);
  BIO_set_accept_bios(bio, conn);
  
  if (BIO_do_accept(bio) <= 0)
    printErr("Error during connection");
  while (true) {
    if (BIO_do_accept(bio) <= 0)
      printErr("Error during connection");
    conn = BIO_pop(bio);
    string ret = recv(conn);
    if (ret == "quit") break;
    send(data->bio, ret);
  }

  SSL_CTX_free(ctx);
  BIO_free(bio);
  BIO_free(conn);
  pthread_exit(NULL);
}

vector<userInfo> listParser(const string& list, string& bal, string& userNum) {
  auto pos = list.find("\n");
  bal = list.substr(0, pos);
  auto pos2 = pos + 1;
  pos = list.find("\n", pos2);
  userNum = list.substr(pos2, pos - pos2);
  vector<userInfo> userList;
  pos2 = list.find('#', ++pos);
  while (pos2 != string::npos) {
    userInfo tmp;
    tmp._name = list.substr(pos, pos2 - pos);
    pos = pos2 + 1;
    pos2 = list.find('#', pos);
    tmp._ip = list.substr(pos, pos2 - pos);
    pos = pos2 + 1;
    pos2 = list.find("\n", pos);
    tmp._port = list.substr(pos, pos2 - pos);
    userList.push_back(tmp);
    pos = pos2 + 1;
    pos2 = list.find('#', pos);
  }
  return userList;
}

void printList(const vector<userInfo>& l, const string& b, string num) {
  cout << "Your balance is: " << b << endl;
  cout << "The total users online is: " << num << endl;
  cout << left << setw(6) << "NO" 
      << left << setw(20) << "Username" 
      << left << setw(20) << "Ip"
      << left << setw(10) << "Port" << endl;
  for (int i = 0, n = stoi(num); i < n; ++i) {
    cout << left << setw(6) << i + 1
      << left << setw(20) << l[i]._name
      << left << setw(20) << l[i]._ip
      << left << setw(10) << l[i]._port << endl;
  }
  cout << endl;
}

void printWel(const string& name) {
  cout << endl
        << "==================================================\n"
        << "= Welcome Back "
        << left << setw(33) << name
        << " =\n"
        << "==================================================\n"
        << endl;
}
