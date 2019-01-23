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

using namespace std;



typedef void (*fn)(void *);

struct sockaddr_in server_info, client_info, peer_info;
char ipAddr[INET_ADDRSTRLEN];

struct UserInfo {
  string _name;
  string _balance;
  string _ip;
  string _port;
  bool _online;
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
void sendBack(int, string);
void sendList(int, string);

void printErr(string err) {
  cerr << err << endl;
  exit(1);
}

bool is_number(const string& s) {
    string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

int main(int argc, char *argv[]) {
  if (argc != 2) { 
    printErr("Wrong input number!"); 
  }
  string ppp(argv[1]);
  if (!is_number(ppp)) { 
    printErr("Wrong input number!"); 
  }
  int port = atoi(argv[1]);

  int sd = 0;
  sd = socket(AF_INET , SOCK_STREAM , 0);
  if (sd == -1) {
    printErr("Fail to create a socket!");
  }
  
  bzero(&server_info, sizeof(server_info));
  server_info.sin_family = PF_INET;
  server_info.sin_addr.s_addr = INADDR_ANY;
  server_info.sin_port = htons(port);
  
  int err = bind(sd, (struct sockaddr *)&server_info, sizeof(server_info));
  if (err == -1) {
    printErr("Bind error!");
  }

  listen(sd, 8);
  cout << "Listening on :::" << port << endl;

  _threadpool *pool;
  pool = new _threadpool;

  pool->threads = new pthread_t[8];
  pthread_mutex_init(&(pool->mutex), NULL);
  pthread_cond_init(&(pool->q_empty), NULL);
  pthread_cond_init(&(pool->q_not_empty), NULL);

  for (int i = 0; i < 8 ; ++i) {
    if(pthread_create(&(pool->threads[i]), NULL, worker, pool) != 0) {
      printErr("Error during threadpool creation!");  
    }
  }
  
  int new_fd;
  pthread_mutex_init(&(user_lock), NULL);
  while (true) {
    socklen_t clientLen = sizeof(client_info);
    new_fd = accept(sd, (struct sockaddr*)&client_info, &clientLen);
    if (new_fd < 0) {
      printErr("Error during accepting!");
    }
    manager
  (pool, handler, (void *)&new_fd);
  }


  close(sd);
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
  int fd = *((int *)arg);
  bool isLogin = false;
  string userName = "";

  sendBack(fd, "connection accepted\n");

  while (true) {
    char recv_buf[1024];
    memset(recv_buf, 0, 1024);
    if (recv(fd, recv_buf, sizeof(recv_buf), 0) == 0) {
      // handle exit
      pthread_mutex_lock(&(user_lock));
      for(int i = 0; i < user_list.size(); i++){
        if(user_list[i]._name == userName){
          user_list[i]._online = false;
        }
      }
      pthread_mutex_unlock(&(user_lock));
      close(fd);
      return;
    }

    string input(recv_buf);
    size_t sign = input.find("#", 0);

    if (sign != string::npos && !isLogin) { // sign up or login
      string action = input.substr(0, sign);
      if (action == "REGISTER") {  
        size_t pos = input.find("#", sign + 1);  
        string name = input.substr(sign + 1,pos-(sign+1));
        string balance = input.substr(pos + 1);
        balance.erase(balance.end() - 1); // remove end of line
        
        pthread_mutex_lock(&(user_lock));
        int found = -1;

        for(int i = 0; i < user_list.size(); i++){
          if(user_list[i]._name == name){
            found = i;
          }
        }
        pthread_mutex_unlock(&(user_lock));

        if (found == -1) { // valid name
          UserInfo newUser;
          newUser._name = name;
          newUser._ip = "";
          newUser._port = "";
          newUser._balance = balance;
          newUser._online = false;

          pthread_mutex_lock(&(user_lock));
          user_list.push_back(newUser);
          pthread_mutex_unlock(&(user_lock));

          sendBack(fd, "100 OK\n");
        }
        else { sendBack(fd, "210 FAIL\n"); }
      }
      else {
        string name = action;
        pthread_mutex_lock(&(user_lock));
        int index = 0;
        for(int i = 0; i < user_list.size(); i++){
          if(user_list[i]._name == name){
            index = i;
          }
        }
        pthread_mutex_unlock(&(user_lock));

        if (index == -1) { sendBack(fd, "220 AUTH_FAIL\n"); }
        else {
          string port = input.substr(sign + 1);
          port.erase(port.end() - 1); // remove end of line

          pthread_mutex_lock(&(user_lock));
          int port_found = -1;
          for(int i = 0; i < user_list.size(); i++){
            if(user_list[i]._port == port && user_list[i]._online){
              port_found = i;
            }
          }
          pthread_mutex_unlock(&(user_lock));

          if (port_found != -1 || !is_number(port)) { 
            sendBack(fd, "220 AUTH_FAIL\n"); continue; }
          int p = stoi(port);
          if (p < 1024 || p > 65535) { sendBack(fd, "220 AUTH_FAIL\n"); continue; }
        
          socklen_t peerLen = sizeof(peer_info);
          getpeername(fd, (struct sockaddr *)&peer_info, &peerLen);

          pthread_mutex_lock(&(user_lock));
          userName = user_list[index]._name;
          user_list[index]._ip = inet_ntop(AF_INET, &peer_info.sin_addr, ipAddr, sizeof(ipAddr));
          user_list[index]._port = port;
          user_list[index]._online = true;
          pthread_mutex_unlock(&(user_lock));

          isLogin = true;

          pthread_mutex_lock(&(user_lock));
          sendList(fd, userName);
          pthread_mutex_unlock(&(user_lock));
        }
      }
    }
    else if (isLogin) {
      input.erase(input.end() - 1); // remove end of line
      if (input == "List") {
        pthread_mutex_lock(&(user_lock));
        sendList(fd, userName);
        pthread_mutex_unlock(&(user_lock));
      }
      else if (input == "Exit") {
        sendBack(fd, "Bye\n");
        pthread_mutex_lock(&(user_lock));
        
        for(int i = 0; i < user_list.size(); i++){
          if(user_list[i]._name == userName){
            user_list[i]._online = false;
          }
        }
        pthread_mutex_unlock(&(user_lock));
        break;
      }
      else { sendBack(fd, "please type the right option number!\n"); }
    }
    else { sendBack(fd, "220 AUTH_FAIL\n"); }
  }

  close(fd);
  return;
}

void sendBack(int fd, string ret) {
  send(fd, ret.c_str(), ret.length(), 0);
}

void sendList(int fd, string name) {
  int index = -1;
  for(int i = 0; i < user_list.size(); i++){
    if(user_list[i]._name == name){
      index = i;
    }
  }
  string l = "";
  string ret = user_list[index]._balance;
  int c = 0;
  for (unsigned i = 0; i < user_list.size(); ++i) {
    if (user_list[i]._online) {
      l += user_list[i]._name;
      l += '#';
      l += user_list[i]._ip;
      l += '#';
      l += user_list[i]._port;
      l += '\n';
      ++c;
    }
  }
  
  ret += "\nnumber of accounts online: ";
  ret += to_string(c);
  ret += '\n';
  ret += l;
  
  send(fd, ret.c_str(), ret.length(), 0);
  return;
}
