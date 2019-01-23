#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <string>
#include <queue>
#include <sys/select.h>
#include <sys/time.h>
#include <vector>
#include <arpa/inet.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
using namespace std;
#define BLEN 600
pthread_mutex_t lock;
queue<int> activeSocketQ;
struct sockaddr_in allConnectSocket[50];
fd_set readfds;
fd_set backup;

struct onlineAccount{
  int cfd;
  int money;
  string name;
  string ip;
  string portnum;
};

vector<onlineAccount> onlineList;

struct Accounts{
  int money;
  string name;
};

struct SSLtoClient{
  int client_fd;
  SSL* ssl;
};

vector<SSLtoClient> SSL_Client;

class onlineAccountList{
public:
  onlineAccountList(){
    onlineList.clear();
    length = 0;
  }

  bool check(char* name){
    bool check = false;
    string checkName(name);
    for (int i = 0; i < length; i++) {
      if (onlineList[i].name == checkName) {
        check = true;
        break;
      }
    }
    return check;
  }

  void addOnlineAccount(int client_fd, char* name, char* ip, char* port,int money) {
    struct onlineAccount newOnline;
    string strName(name);
    string strIP(ip);
    string strPort(port);

    newOnline.cfd = client_fd;
    newOnline.money = money;
    newOnline.name = strName;
    newOnline.ip = strIP;
    newOnline.portnum = strPort;
    pthread_mutex_lock(&lock);
    onlineList.push_back(newOnline);
    pthread_mutex_unlock(&lock);
    length++;
  }

  void deleteOnlineAccount(int client_fd) {
    int index;
    pthread_mutex_lock(&lock);
    for(int i = 0; i < length; i++) {
      if (onlineList[i].cfd == client_fd) {
        index = i;
        break;
      }
    }
    onlineList.erase(onlineList.begin()+index);
    pthread_mutex_unlock(&lock);
    FD_CLR(client_fd,&backup);
    length--;
  }

  string getList(int client_fd) {
    char list_get[BLEN];
    const char* listname;
    const char* listip;
    const char* listport;
    int money = -1;
    pthread_mutex_lock(&lock);
    for (int i = 0; i < length; i++) {
      if (onlineList[i].cfd == client_fd) {
        money = onlineList[i].money;
        break;
      }
    }
    pthread_mutex_unlock(&lock);
    memset(list_get,'\0',strlen(list_get));
    sprintf(list_get, "AccountBalance: %d\nNumber of users: %d\n",money,length);

    pthread_mutex_lock(&lock);
    for (int i = 0; i < length; i++) {
      listname = onlineList[i].name.c_str();
      strcat(list_get, listname);
      strcat(list_get, "#");
      listip = onlineList[i].ip.c_str();
      strcat(list_get, listip);
      strcat(list_get, "#");
      listport = onlineList[i].portnum.c_str();
      strcat(list_get, listport);
      strcat(list_get, "\n");
    }
    pthread_mutex_unlock(&lock);

    string listSend(list_get);
    return listSend;
  }

  string checkOnline(char* name){ //when client request for trade
    string targetName(name);
    const char* tradeIP;
    const char* tradePort;
    bool find = false;
    char respTrade[BLEN] = "!";


    for (int i = 0; i < length; i++) {
      if (targetName == onlineList[i].name) {
        targetName = onlineList[i].name;
        tradeIP = onlineList[i].ip.c_str();
        tradePort = onlineList[i].portnum.c_str();
        find = true;
        break;
      }
    }
    if (find) {
      memset(respTrade,'\0',strlen(respTrade));
      strcat(respTrade, name);
      strcat(respTrade,"#");
      strcat(respTrade, tradeIP);
      strcat(respTrade,"#");
      strcat(respTrade, tradePort);
    }
    string checkOnlineAns(respTrade);
    return checkOnlineAns;
  }

  bool modifyMoney(int client_fd, int money)
  {
    bool modified = false;
    for (int i = 0; i < length; i++) {
      if (onlineList[i].cfd == client_fd) {
        onlineList[i].money = onlineList[i].money + money;
        modified = true;
        break;
      }
    }
    return modified;
  }

  char* getName(int client_fd){
    const char* name;
    for (int i = 0; i < length; i++) {
      if (onlineList[i].cfd == client_fd) {
        name = onlineList[i].name.c_str();
        break;
      }
    }
    char* reName = strdup(name);
    return reName;
  }

  int getLength(){
    return length;
  }

private:
  int length;
};


class accountList
{
public:
  accountList(){
    length = 0;
  }

  void addAccount(char* name,int money){
    string newName(name);
    account[length].name = newName;
    account[length].money = money;
    length++;
  }

  bool findName(char* name){
    string targetName(name);
    bool find = false;
    if (length > 0) {
      for (int i = 0; i < length; i++) {
        if (account[i].name == targetName) {
          find = true;
          break;
        }
      }
    }
    return find;
  }
  int getMoney(char* name){
    string targetName(name);
    bool find = false;
    int money;
    for (int i = 0; i < length; i++) {
      if (account[i].name == targetName) {
        find = true;
        money = account[i].money;
        break;
      }
    }
    return money;
  }

  bool changeMoney(char* name, int money){
    bool changed = false;
    string changedName(name);
    for (int i = 0; i < length; i++) {
      if (account[i].name == changedName) {
        account[i].money = account[i].money + money;
        changed = true;
        break;
      }
    }
    return changed;
  }

private:
  Accounts account[BLEN];
  int length;
};

accountList allAccounts;
onlineAccountList allOnline;

SSL* findSSL(int client_fd)
{
  SSL* ssl;
  for (int i = 0; i < SSL_Client.size(); i++) {
    if (SSL_Client[i].client_fd == client_fd) {
      ssl = SSL_Client[i].ssl;
      break;
    }
  }
  return ssl;
}

void* logic(void* data){
  int client_fd = -1;
  while (true) {
    pthread_mutex_lock(&lock);
    if (activeSocketQ.size() > 0) {
      client_fd = activeSocketQ.front();
      activeSocketQ.pop();
    }
    pthread_mutex_unlock(&lock);
    if (client_fd > 0) {
      char buffer[BLEN]="";
      char* hashpos;
      int n;
      SSL* ssl = findSSL(client_fd);

      memset(buffer,'\0',strlen(buffer));
      n = SSL_read(ssl,buffer,BLEN);
      /*register*/
      if (strstr(buffer,"REGISTER#") != NULL) {
        char* tempName;
        char* tempMoney;
        int int_tempMoney;
        tempName = strtok(buffer,"#");
        tempName = strtok(NULL,"#");
        tempMoney = strtok(NULL,"#");
        int_tempMoney = atoi(tempMoney);

        if (allAccounts.findName(tempName) == false) {
          allAccounts.addAccount(tempName,int_tempMoney);
          SSL_write(ssl, "100 OK", strlen("100 OK"));
          puts("register successful");
        }
        else{
          SSL_write(ssl, "210 FAIL", strlen("210 FAIL"));
          puts("register failed");
        }
      }
      /*list*/
      else if (strcmp(buffer,"List") == 0) {
        string list;
        const char* send;
        list = allOnline.getList(client_fd);
        send = list.c_str();
        SSL_write(ssl,send,strlen(send));
        puts("list");
      }
      /*exit*/
      else if (strcmp(buffer,"Exit") == 0) {
        SSL_write(ssl, "Bye\n", strlen("Bye\n"));
        allOnline.deleteOnlineAccount(client_fd);
        for (int i = 0; i < SSL_Client.size(); i++) {
          if (SSL_Client[i].client_fd == client_fd) {
            SSL_Client.erase(SSL_Client.begin()+i);
            break;
          }
        }
        SSL_free(ssl);
        close(client_fd);
        puts("exit");
      }
      else if (strstr(buffer,"Trade#") != NULL) {
        puts("trade");
        char* tradeName;
        const char* resp;
        string check;
        tradeName = strtok(buffer,"#");
        tradeName = strtok(NULL,"#");
        check = allOnline.checkOnline(tradeName);
        resp = check.c_str();

        if (resp[0] == '!') {
          SSL_write(ssl,"230 NO_MATCH",strlen("230 NO_MATCH"));
        }
        else{
          SSL_write(ssl,resp,strlen(resp));
        }
      }
      else if (buffer[0] == '#') {
        puts("change money");
        int modifyNum;
        modifyNum = atoi(strtok(buffer,"#"));
        allOnline.modifyMoney(client_fd,modifyNum);
        allAccounts.changeMoney(allOnline.getName(client_fd),modifyNum);
        SSL_write(ssl,"trade done",strlen("trade done"));
      }
      /*login*/
      else if ((hashpos = strstr(buffer,"#")) != NULL) {
        char* port;
        char* name;
        char* ip;
        const char* send;
        string list_login;
        int onlinemoney;

        name = strtok(buffer,"#");
        port = strtok(NULL,"#");
        ip = inet_ntoa(allConnectSocket[client_fd].sin_addr);
        if (allAccounts.findName(name) == true && !allOnline.check(name)) {
          onlinemoney = allAccounts.getMoney(name);
          allOnline.addOnlineAccount(client_fd,name,ip,port,onlinemoney);
          list_login = allOnline.getList(client_fd);
          send = list_login.c_str();
          SSL_write(ssl, send,strlen(send));
          puts("login successful");
        }
        else{
          SSL_write(ssl,"220 AUTH_FAIL",strlen("220 AUTH_FAIL"));
          puts("login failed");
        }
      }
    }//if
    client_fd = -1;
  }//while
}

//Init server instance and context
SSL_CTX* InitServerCTX(void) {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc.
    SSL_load_error_strings();   // load all error messages */
    method = TLSv1_server_method();  // create new server-method instance
    ctx = SSL_CTX_new(method);   // create new context from method
    if ( ctx == NULL ) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

//Load the certificate
void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile) {
    //set the local certificate from CertFile
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //set the private key from KeyFile
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //verify private key
    if ( !SSL_CTX_check_private_key(ctx) ) {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

int isRoot() {
    if (getuid() != 0)
        return 0;
    else
        return 1;
}

int main(int argc, char * argv[]){
  struct sockaddr_in addr;
  int sd;
  int max_fd = -1;
  struct timeval tv;
  char cert[20] = "server_cert.pem";
  char key[20] = "server_key.pem";
  SSL_CTX *ctx;
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  SSL_library_init(); //init SSL library
  ctx = InitServerCTX();
  LoadCertificates(ctx, cert, key); // load certs and key

  /*socket*/
  sd = socket(AF_INET,SOCK_STREAM,0);
  FD_SET(sd,&backup);
  max_fd = sd;
  /*server*/
  memset(&addr,0,sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(argv[1]));
  addr.sin_addr.s_addr = INADDR_ANY;
  /*bind*/
  if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    puts("Fail to bind");
    exit(0);
  }
  /*lisen*/
  listen(sd, 50);
  /*client*/

  pthread_t pthreads[50];
  pthread_mutex_init(&lock, NULL);
  for (int i = 0; i < 50; i++) {
    pthread_create(&pthreads[i],NULL,&logic,NULL);
  }


  while (true) {
    sleep(1);
    if (activeSocketQ.size() == 0) {

      int client_fd = -1;
      int check_st = -1;
      struct sockaddr_in client_addr;
      socklen_t client_addr_len = sizeof(client_addr);
      FD_ZERO(&readfds);
      readfds = backup;
      check_st = select(max_fd+1,&readfds,NULL,NULL,&tv);
      if (check_st > 0) {
        if (FD_ISSET(sd,&readfds)) {
          SSL *ssl;
          client_fd = accept(sd,(struct sockaddr*)&client_addr,&client_addr_len);

          ssl = SSL_new(ctx);
          SSL_set_fd(ssl, client_fd);
          if (SSL_accept(ssl) == -1) {
            ERR_print_errors_fp(stderr);
          }
          puts("accept");
          SSL_write(ssl, "connection successful", strlen("connection successful"));
          allConnectSocket[client_fd] = client_addr;
          FD_SET(client_fd,&backup);
          if (client_fd > max_fd) {
            max_fd = client_fd;
          }
          struct SSLtoClient pair;
          pair.client_fd = client_fd;
          pair.ssl = ssl;
          SSL_Client.push_back(pair);
        }
        for (int i = 0; i <= max_fd; i++) {

          if (FD_ISSET(i,&readfds) && i != sd) {
            pthread_mutex_lock(&lock);
            activeSocketQ.push(i);
            pthread_mutex_unlock(&lock);
          }//if
        }//for
      }//if
    }//if
  }//while
  SSL_CTX_free(ctx);
  return 0;
}
