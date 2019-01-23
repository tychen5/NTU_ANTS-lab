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


#define CERT_S "server.crt"
#define KEY_S "server.key"

typedef void (*fn)(void *);

struct sockaddr_in server_info, client_info, peer_info;
char ipAddr[INET_ADDRSTRLEN];

struct UserInfo {
  string name;
  int balance;
  string ip;
  string port;
  bool online;
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

pthread_mutex_t user_lock; 
vector<UserInfo> user_list; 

void* worker(void*);
void manager(_threadpool *, fn, void *);
void handler(void *);
void sendList(BIO *, string);

void printErr(string err) {
  cerr << err << endl;
  exit(1);
}

void send(BIO *bio, string msg) {
  if (BIO_write(bio, msg.c_str(), msg.length()) <= 0)
    printErr("Error during writing message\n");
}

string recv(BIO *bio) {
  char b[1024];
  memset(b, 0, 1024);
  if (BIO_read(bio, b, sizeof(b)) <= 0) {
    return "-1";
  }
  
  
  cout << b << endl;
  return string(b);
}

bool is_number(const string& s) {
    string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

int main(int argc, char *argv[]) {
  SSL_load_error_strings();
  ERR_load_BIO_strings();
  OpenSSL_add_all_algorithms();
  SSL_library_init();
  
  _threadpool *pool;
  pool = new _threadpool;
  pool->threads = new pthread_t[10];
  pthread_mutex_init(&(pool->mutex), NULL);
  pthread_cond_init(&(pool->q_empty), NULL);
  pthread_cond_init(&(pool->q_not_empty), NULL);
  pthread_mutex_init(&(user_lock), NULL);

  for (int i = 0; i < 10 ; ++i) {
    if(pthread_create(&(pool->threads[i]), NULL, worker, pool) != 0) {
      cout<< "Error during threadpool creation!";  
    }
  }
  
  BIO *bio = BIO_new_accept(argv[1]);
  if (bio == NULL)
    cout << "Error during setting up connection";

  SSL_CTX *ctx = SSL_CTX_new(SSLv23_server_method());
  if (ctx == NULL)
    printErr("Error loading ctx");
  if (!SSL_CTX_use_certificate_file(ctx, CERT_S, SSL_FILETYPE_PEM))
    printErr("Error loading certificate");
  if (!SSL_CTX_use_PrivateKey_file(ctx, KEY_S, SSL_FILETYPE_PEM))
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
    manager(pool, handler, (void *)conn);
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

void manager(_threadpool *pool, fn fn, void *arg) {
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
      pthread_mutex_lock(&(user_lock));
      unsigned index = 0;
      for(unsigned i = 0; i < user_list.size(); i++){
        if(user_list[i].name == userName)
          index = i;
      }
      user_list[index].online = false;  
      pthread_mutex_unlock(&(user_lock));
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

        if (!is_number(b)) {
          send(bio, "210 FAIL\n");
          continue;
        }
        

        pthread_mutex_lock(&(user_lock));
        int found = 0;
        for(unsigned i = 0; i < user_list.size(); i++){
          if(user_list[i].name == name){
            found = i;
          }
        }

        pthread_mutex_unlock(&(user_lock));

        if (found == -1) { // valid name
          UserInfo newUser;
          newUser.name = name;
          newUser.ip = "";
          newUser.port = "";
          newUser.balance = stoi(b);
          newUser.online = false;

          pthread_mutex_lock(&(user_lock));
          user_list.push_back(newUser);
          pthread_mutex_unlock(&(user_lock));

          send(bio, "100 OK");
        }
        else { send(bio, "210 FAIL"); }
      }
      else {
        string name = action;
        pthread_mutex_lock(&(user_lock));

        int found = 0;
        for(unsigned i = 0; i < user_list.size(); i++){
          if(user_list[i].name == name){
            found = i;
          }
        }
        pthread_mutex_unlock(&(user_lock));

        if (found == -1) { send(bio, "220 AUTH_FAIL"); }
        else {
          string port = input.substr(poundSign + 1);
          //port.erase(port.end() - 1); // remove end of line

          pthread_mutex_lock(&(user_lock));
          int found = 0;
          for(unsigned i = 0; i < user_list.size(); i++){
            if(user_list[i].port == port){
              found = i;
            }
          }
          pthread_mutex_unlock(&(user_lock));

          if (found != -1 || !is_number(port)) { send(bio, "220 AUTH_FAIL"); continue; }
          int p = stoi(port);
          if (p < 1024 || p > 65535) { send(bio, "220 AUTH_FAIL"); continue; }
        
          int fd;
          BIO_get_fd(bio, &fd);
          struct sockaddr_in peer_info;
          socklen_t peerLen = sizeof(peer_info);
          getpeername(fd, (struct sockaddr *)&peer_info, &peerLen);
          char ipAddr[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &peer_info.sin_addr, ipAddr, sizeof(ipAddr));

          pthread_mutex_lock(&(user_lock));
          userName = user_list[found].name;
          user_list[found].ip = string(ipAddr);
          user_list[found].port = port;
          user_list[found].online = true;
          pthread_mutex_unlock(&(user_lock));

          isLogin = true;

          pthread_mutex_lock(&(user_lock));
          sendList(bio, userName);
          pthread_mutex_unlock(&(user_lock));
        }
      }
    }
    else if (poundSign != string::npos && isLogin) {
      string name1 = input.substr(0, poundSign);
      ++poundSign;
      auto poundSign2 = input.find("#", poundSign);
      string t = input.substr(poundSign, poundSign2 - poundSign);
      string name2 = input.substr(poundSign2 + 1);
      //name2.erase(name2.end() - 1); // remove end of line

      int tt = stoi(t);
      pthread_mutex_lock(&(user_lock));
      int n1 = 0, n2 = 0;
      for(unsigned i = 0; i < user_list.size(); i++){
        if(user_list[i].name == name1){
          n1 = i;
        }
        if(user_list[i].name == name2){
          n2 = i;
        }
      }
      if (user_list[n1].balance >= tt) {
        user_list[n1].balance -= tt;
        user_list[n2].balance += tt;
      }
      pthread_mutex_unlock(&(user_lock));
    }
    else if (isLogin) {
      //input.erase(input.end() - 1); // remove end of line
      if (input == "List") {
        pthread_mutex_lock(&(user_lock));
        sendList(bio, userName);
        pthread_mutex_unlock(&(user_lock));
      }
      else if (input == "Exit") {
        send(bio, "Bye\n");
        pthread_mutex_lock(&(user_lock));
        int found = 0;
        for(unsigned i = 0; i < user_list.size(); i++){
          if(user_list[i].name == userName){
            found = i;
          }
        }
        user_list[found].online = false;
        pthread_mutex_unlock(&(user_lock));
        break;
      }
      else { send(bio, "please type the right option number!"); }
    }
    else { send(bio, "220 AUTH_FAIL"); }
  }

  BIO_free(bio);
  return;
}

void sendList(BIO *bio, string name) {
  int index = -1;
  for(unsigned i = 0; i < user_list.size(); i++){
    if(user_list[i].name == name){
      index = i;
    }
  }
  string l = "";
  string ret = to_string(user_list[index].balance);
  int c = 0;
  for (unsigned i = 0; i < user_list.size(); ++i) {
    if (user_list[i].online) {
      l += user_list[i].name;
      l += '#';
      l += user_list[i].ip;
      l += '#';
      l += user_list[i].port;
      l += '\n';
      ++c;
    }
  }
  
  ret += "\nnumber of accounts online: ";
  ret += to_string(c);
  ret += '\n';
  ret += l;
  
  send(bio, ret);
  return;
}
