#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <list>
#include "openssl/bio.h"
#include "openssl/err.h"
#include "openssl/ssl.h"

using namespace std;

#define BLEN 10240
#define THREAD_NUM 10
#define DEFAULT_BALANCE 10000
#define S_CERT "server.crt"
#define S_KEY "server.key"

typedef void (*fn)(void *);


struct UserInfo {
  string _name;
  int _balance;
  string _ip;
  string _port;
  bool _isOnline;
};

typedef struct task_st {
    void (*function) (void *);
    void *arg;
}task_t;

typedef struct _threadpool_st {
  pthread_mutex_t mutex;
  pthread_cond_t q_empty;
  pthread_cond_t q_not_empty;
  pthread_t *threads;
  list<task_t *> task;
}_threadpool;

pthread_mutex_t table_lock; 
vector<UserInfo> user_list; 

void* worker(void*);
void dispatch(_threadpool *, fn, void *);
void handler(void *);
int findUser(string);
int findIp(string);
int findPort(string);
void sendList(BIO *, string);

void printErr(string err) {
  cerr << err << endl;
  exit(1);
}

bool is_number(const string& s) {
    string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

void printTable() {
  cout << "=== List ===" << endl;
  for (unsigned i = 0; i < user_list.size(); ++i) {
    cout << "Name: " << user_list[i]._name << endl;
    cout << "Balance: " << user_list[i]._balance << endl;
    cout << "IP: " << user_list[i]._ip << endl;
    cout << "Port: " << user_list[i]._port << endl;
    if (user_list[i]._isOnline) { cout << "online" << endl; }
    else { cout << "offline" << endl; }
    cout << endl;
  }
}

void send(BIO *bio, string msg) {
  if (BIO_write(bio, msg.c_str(), msg.length()) <= 0)
    printErr("Error during writing message\n");
}

string recv(BIO *bio) {
  char b[BLEN];
  memset(b, 0, BLEN);
  if (BIO_read(bio, b, sizeof(b)) <= 0) {
    return "-1";
  }
  
  
  cout << b << endl;
  return string(b);
}

int main(int argc, char *argv[]) {
  SSL_load_error_strings();
  ERR_load_BIO_strings();
  OpenSSL_add_all_algorithms();
  SSL_library_init();

  if (argc < 2) 
    printErr("Usage: " + string(argv[0]) + " <serverPort>");

  _threadpool *pool;
  pool = new _threadpool;

  pool->threads = new pthread_t[THREAD_NUM];
  pthread_mutex_init(&(pool->mutex), NULL);
  pthread_cond_init(&(pool->q_empty), NULL);
  pthread_cond_init(&(pool->q_not_empty), NULL);
  pthread_mutex_init(&(table_lock), NULL);

  for (int i = 0; i < THREAD_NUM ; ++i) {
    if(pthread_create(&(pool->threads[i]), NULL, worker, pool) != 0) {
      printErr("Error during threadpool creation!");  
    }
  }
  
  BIO *bio = BIO_new_accept(argv[1]);
  if (bio == NULL)
    printErr("Error during setting up connection");

  SSL_CTX *ctx = SSL_CTX_new(SSLv23_server_method());
  if (ctx == NULL)
    printErr("Error loading ctx");
  if (!SSL_CTX_use_certificate_file(ctx, S_CERT, SSL_FILETYPE_PEM))
    printErr("Error loading certificate");
  if (!SSL_CTX_use_PrivateKey_file(ctx, S_KEY, SSL_FILETYPE_PEM))
    printErr("Error loading private key");
  if (!SSL_CTX_check_private_key(ctx))
    printErr("Error checking private key");
  BIO *conn = BIO_new_ssl(ctx, 0);
  BIO_set_accept_bios(bio, conn);

  if (BIO_do_accept(bio) <= 0) 
    printErr("Error connection");
  while (true) {
    if (BIO_do_accept(bio) <= 0)
      printErr("Error connection");
    conn = BIO_pop(bio);
    dispatch(pool, handler, (void *)conn);
  }

  SSL_CTX_free(ctx);
  BIO_free(bio);
  BIO_free(conn);
  pthread_exit(NULL);
  return 0;
}

void* worker(void* param) {
  _threadpool *pool = (_threadpool *)param;
  task_t *current;   

  pthread_mutex_lock(&(pool->mutex));

  while (1) {
    while(pool->task.size() == 0) {
        pthread_cond_wait(&(pool->q_not_empty), &(pool->mutex));
    }
  
    current = pool->task.front();
    pool->task.pop_front();
  
    pthread_mutex_unlock(&(pool->mutex));

    (current->function) (current->arg);
  
    delete current;
  }
}

void dispatch(_threadpool *pool, fn fn, void *arg) {
  task_t *cur_task = new task_t;

  cur_task->function = fn;
  cur_task->arg = arg;

  pthread_mutex_lock(&(pool->mutex));
  
  pool->task.push_back(cur_task);
  pthread_cond_signal(&(pool->q_not_empty));

  pthread_mutex_unlock(&(pool->mutex));
}

void handler(void *arg) {
  BIO *bio = (BIO *)arg;
  bool isLogin = false;
  string userName = "";

  send(bio, "connection accepted\n");

  while (true) {
    string input = recv(bio);
    if (input == "-1") {
      // handle exit
      pthread_mutex_lock(&(table_lock));
      user_list[findUser(userName)]._isOnline = false;  
      pthread_mutex_unlock(&(table_lock));
      BIO_free(bio);
      return;
    }

    auto poundSign = input.find("#", 0);

    if (poundSign != string::npos && !isLogin) { // sign up or login
      string action = input.substr(0, poundSign);
      if (action == "REGISTER") {
        auto poundSign2 = input.find("#", poundSign + 1);
        if (poundSign2 == string::npos) {
          send(bio, "210 FAIL\n");
          continue;
        }
        string name = input.substr(poundSign + 1, poundSign2 - poundSign - 1);
        string b = input.substr(poundSign2 + 1);
        b.erase(b.end() - 1); // remove end of line

        if (!is_number(b)) {
          send(bio, "210 FAIL\n");
          continue;
        }
        

        pthread_mutex_lock(&(table_lock));
        int isFind = findUser(name);
        pthread_mutex_unlock(&(table_lock));

        if (isFind == -1) { // valid name
          UserInfo newUser;
          newUser._name = name;
          newUser._ip = "";
          newUser._port = "";
          newUser._balance = stoi(b);
          newUser._isOnline = false;

          pthread_mutex_lock(&(table_lock));
          user_list.push_back(newUser);
          pthread_mutex_unlock(&(table_lock));

          send(bio, "100 OK\n");
        }
        else { send(bio, "210 FAIL\n"); }
      }
      else {
        string name = action;
        pthread_mutex_lock(&(table_lock));
        int index = findUser(name);
        pthread_mutex_unlock(&(table_lock));

        if (index == -1) { send(bio, "220 AUTH_FAIL\n"); }
        else {
          string port = input.substr(poundSign + 1);
          port.erase(port.end() - 1); // remove end of line

          pthread_mutex_lock(&(table_lock));
          int isFind = findPort(port);
          pthread_mutex_unlock(&(table_lock));

          if (isFind != -1 || !is_number(port)) { send(bio, "220 AUTH_FAIL\n"); continue; }
          int p = stoi(port);
          if (p < 1024 || p > 65535) { send(bio, "220 AUTH_FAIL\n"); continue; }
        
          int fd;
          BIO_get_fd(bio, &fd);
          struct sockaddr_in peer_info;
          socklen_t peerLen = sizeof(peer_info);
          getpeername(fd, (struct sockaddr *)&peer_info, &peerLen);
          char ipAddr[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &peer_info.sin_addr, ipAddr, sizeof(ipAddr));

          pthread_mutex_lock(&(table_lock));
          userName = user_list[index]._name;
          user_list[index]._ip = string(ipAddr);
          user_list[index]._port = port;
          user_list[index]._isOnline = true;
          pthread_mutex_unlock(&(table_lock));

          isLogin = true;

          pthread_mutex_lock(&(table_lock));
          sendList(bio, userName);
          pthread_mutex_unlock(&(table_lock));
        }
      }
    }
    else if (poundSign != string::npos && isLogin) {
      string name1 = input.substr(0, poundSign);
      ++poundSign;
      auto poundSign2 = input.find("#", poundSign);
      string t = input.substr(poundSign, poundSign2 - poundSign);
      string name2 = input.substr(poundSign2 + 1);
      name2.erase(name2.end() - 1); // remove end of line

      int tt = stoi(t);
      pthread_mutex_lock(&(table_lock));
      int i = findUser(name1);
      int j = findUser(name2);
      if (user_list[i]._balance >= tt) {
        user_list[i]._balance -= tt;
        user_list[j]._balance += tt;
      }
      pthread_mutex_unlock(&(table_lock));
    }
    else if (isLogin) {
      input.erase(input.end() - 1); // remove end of line
      if (input == "List") {
        pthread_mutex_lock(&(table_lock));
        sendList(bio, userName);
        pthread_mutex_unlock(&(table_lock));
      }
      else if (input == "Exit") {
        send(bio, "Bye\n");
        pthread_mutex_lock(&(table_lock));
        user_list[findUser(userName)]._isOnline = false;
        pthread_mutex_unlock(&(table_lock));
        break;
      }
      else { send(bio, "please type the right option number!\n"); }
    }
    else { send(bio, "220 AUTH_FAIL\n"); }
  }

  BIO_free(bio);
  return;
}

int findUser(string name) {
  for (unsigned i = 0; i < user_list.size(); ++i) {
    if (user_list[i]._name == name) { return i; }
  }
  return -1;
}

int findIp(string ip) {
  for (unsigned i = 0; i < user_list.size(); ++i) {
    if (user_list[i]._ip == ip && user_list[i]._isOnline) { return i; }
  }
  return -1;
}

int findPort(string port) {
  for (unsigned i = 0; i < user_list.size(); ++i) {
    if (user_list[i]._port == port && user_list[i]._isOnline) { return i; }
  }
  return -1;
}

void sendList(BIO *bio, string name) {
  int index = findUser(name);
  string l = "";
  string ret = to_string(user_list[index]._balance);
  int c = 0;
  for (unsigned i = 0; i < user_list.size(); ++i) {
    if (user_list[i]._isOnline) {
      l += user_list[i]._name;
      l += '#';
      l += user_list[i]._ip;
      l += '#';
      l += user_list[i]._port;
      l += '\n';
      ++c;
    }
  }
  
  ret += "\n";
  ret += to_string(c);
  ret += '\n';
  ret += l;
  
  send(bio, ret);
  return;
}
