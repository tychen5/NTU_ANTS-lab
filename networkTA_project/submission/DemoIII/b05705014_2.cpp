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
fd_set readfds;
fd_set backup;
queue<int> tradeSocketQ;
int server_sd;


struct serverSSL{
  int server_sd;
  SSL* server_SSL;
};

struct SSLtoTrade{
  int client_fd;
  SSL* ssl;
};

vector<SSLtoTrade> TradeSSL;

struct serverSSL server;

SSL_CTX* InitServerCTX(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc.
    SSL_load_error_strings();   // load all error messages */
    method = TLSv1_server_method();  // create new server-method instance
    ctx = SSL_CTX_new(method);   // create new context from method
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
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

SSL* findSSL(int client_fd)
{
  SSL* ssl;
  for (int i = 0; i < TradeSSL.size(); i++) {
    if (TradeSSL[i].client_fd == client_fd) {
      ssl = TradeSSL[i].ssl;
      break;
    }
  }
  return ssl;
}


void* listen(void* data)
{
  struct sockaddr_in addr;
  int sd;
  int max_fd = -1;
  int clientPort = *(int*) data;
  SSL_CTX *ctx;
  struct timeval tv;
  char cert[20] = "client_cert.pem";
  char key[20] = "client_key.pem";
  tv.tv_sec = 0;
  tv.tv_usec = 0;
//??  SSL_library_init();
  ctx = InitServerCTX();
  LoadCertificates(ctx, cert, key);


  sd = socket(AF_INET,SOCK_STREAM,0);

  max_fd = sd;
  FD_SET(sd,&backup);

  addr.sin_family = AF_INET;
  addr.sin_port = htons(clientPort);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    puts("Fail to bind");
    exit(0);
  }
  listen(sd, 10);

  while(true){
    if (tradeSocketQ.size() == 0) {
      int client_fd = -1;
      int check_st = -1;
      struct sockaddr_in client_addr;

      socklen_t client_addr_len = sizeof(client_addr);
      FD_ZERO(&readfds);
      readfds = backup;

      check_st = select(max_fd+1,&readfds,NULL,NULL,&tv);

      if (check_st > 0) {
        SSL* ssl;
        if (FD_ISSET(sd,&readfds)) {
          client_fd = accept(sd,(struct sockaddr*)&client_addr,&client_addr_len);
          ssl = SSL_new(ctx);
          SSL_set_fd(ssl, client_fd);
          if (SSL_accept(ssl) == -1){
            ERR_print_errors_fp(stderr);
          }

          SSL_write(ssl, "trade connection successful", strlen("trade connection successful"));
          FD_SET(client_fd,&backup);
          if (client_fd > max_fd) {
            max_fd = client_fd;
          }
          struct SSLtoTrade pair;
          pair.client_fd = client_fd;
          pair.ssl = ssl;
          TradeSSL.push_back(pair);
        }
        for (int i = 0; i <= max_fd; i++) {
          if (FD_ISSET(i,&readfds) && i != sd) {
            tradeSocketQ.push(i);
          }
        }
      }
    }
    else{
      int trade_fd = -1;
      char buf[BLEN];
      int n;
      trade_fd = tradeSocketQ.front();
      tradeSocketQ.pop();

      SSL* sslT = findSSL(trade_fd);

//      else {
        memset(buf,'\0',strlen(buf));
        n = SSL_read(sslT,buf,sizeof(buf));
        printf("%s\n", buf);
        puts("\ntrade!!!");
        SSL_write(sslT,"accept trade",strlen("accept trade"));

        char addMoney[11];
        SSL* serverssl;
        serverssl = server.server_SSL;

        memset(addMoney,'\0',strlen(addMoney));
        strcat(addMoney,"#");
        strcat(addMoney,buf);
        SSL_write(serverssl,addMoney,strlen(addMoney));
        memset(buf,'\0',strlen(buf));
        n = SSL_read(serverssl,buf,sizeof(buf));
        printf("%s\n", buf);
        puts("Enter the action you want to take.");
        puts("L for list, T for Trade, E for Exit?");
        close(trade_fd);

        FD_CLR(trade_fd,&backup);
//      }
      SSL_free(sslT);
    }
  }
}

SSL_CTX* InitCTX(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = TLSv1_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

void ShowCerts(SSL* ssl) {
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl); // get the server's certificate
    if ( cert != NULL ) {
        printf("---------------------------------------------\n");
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       // free the malloc'ed string
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);\
        free(line);       // free the malloc'ed string
        X509_free(cert);     // free the malloc'ed certificate copy */
        printf("---------------------------------------------\n");
    }
    else
        printf("No certificates.\n");
}


int main(int argc, char* argv[]) {
  struct sockaddr_in addr;
  int n;
  char buf[BLEN];
  char* bptr;
  char req[BLEN];
  char list[5] = "List";
  char Exit[5] = "Exit";
  char Trade[6] = "Trade";
  SSL_CTX* ctx;
  SSL* ssl;

  SSL_library_init();
  ctx = InitCTX();
  /*host IP*/
  memset(&addr,0,sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(argv[1]);
  addr.sin_port = htons(atoi(argv[2]));
  /*allocate socket*/
  server_sd = socket(AF_INET,SOCK_STREAM,0);
  /*connection*/
  if (connect(server_sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    puts("Fail to connect");
    exit(0);
  }
  else {
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, server_sd);
    if (SSL_connect(ssl) == -1) {
      ERR_print_errors_fp(stderr);
    }
    server.server_SSL = ssl;
    memset(buf,'\0',strlen(buf));
    n = SSL_read(ssl,buf,sizeof(buf));
    printf("%s\n", buf);
  }

  int clientPort;
  /*register or login*/
  ShowCerts(ssl);
  while (true) {
    char get_1[15];

    puts("Enter the action you want to take.");
    printf("R for Register, L for login?");
    scanf("%s", get_1);
    if (strcmp(get_1,"R") == 0) { //register
      char username[50];
      char money[10];

      printf("Enter the name you want to register: ");
      scanf("%s",username);
      printf("Enter the amount of money: ");
      scanf("%s",money);
      memset(req,'\0',strlen(req));
      strcat(req,"REGISTER#");
      strcat(req,username);
      strcat(req,"#");
      strcat(req,money);
      memset(buf,'\0',strlen(buf));
      SSL_write(ssl,req,strlen(req));
      n = SSL_read(ssl,buf,sizeof(buf));
      printf("%s\n", buf);
    }
    else if (strcmp(get_1,"L") == 0) { //login
      char username[50];
      int portnum;
      char portnum_c[10];

      printf("Enter your name: ");
      scanf("%s",username);
      printf("Enter your port number: ");
      scanf("%d",&portnum);
      clientPort = portnum;
      sprintf(portnum_c,"%d",portnum);
      memset(req,'\0',strlen(req));
      strcat(req,username);
      strcat(req,"#");
      strcat(req,portnum_c);
      memset(buf,'\0',strlen(buf));
      SSL_write(ssl,req,strlen(req));
      n = SSL_read(ssl,buf,sizeof(buf));
      printf("%s\n",buf);
      if (buf[0] == 'A') {
        printf("login successfully!\n");
        break;
      }
    }
  }

  pthread_t toConnect;
  pthread_create(&toConnect,NULL,&listen,&clientPort);

  while (true) {
    char get_2[15];
    puts("Enter the action you want to take.");
    puts("L for list, T for Trade, E for Exit?");
    scanf("%s",get_2);

    if (strcmp(get_2,"L") == 0) {
      memset(req,'\0',strlen(req));
      strcat(req,list);

      memset(buf,'\0',strlen(buf));
      SSL_write(ssl,req,strlen(req));
      n = SSL_read(ssl,buf,sizeof(buf));

      printf("%s\n",buf);
    }
    else if (strcmp(get_2,"T") == 0) {
      char tradeName[50];
      char payMoney[10];
      printf("Enter the name you want to trade: ");
      scanf("%s",tradeName);

      memset(req,'\0',strlen(req));
      strcat(req,Trade);
      strcat(req,"#");
      strcat(req,tradeName);

      memset(buf,'\0',strlen(buf));
      SSL_write(ssl,req,strlen(req));
      n = SSL_read(ssl,buf,sizeof(buf));
      if (buf[0] == '2') {
        printf("%s\n", buf);
      }
      else {
        SSL* sslT;
        puts("found");
        const char* tradePort;
        const char* tradeName;
        const char* tradeIP;
        tradeName = strtok(buf,"#");
        tradeIP = strtok(NULL,"#");
        tradePort = strtok(NULL,"#");
        struct sockaddr_in trade_addr;
        int trade_sd;
        memset(&trade_addr,0,sizeof(trade_addr));
        trade_addr.sin_family = AF_INET;
        trade_addr.sin_addr.s_addr = inet_addr(tradeIP);
        trade_addr.sin_port = htons(atoi(tradePort));
        trade_sd = socket(AF_INET,SOCK_STREAM,0);

        if (connect(trade_sd, (struct sockaddr *)&trade_addr, sizeof(trade_addr)) < 0) {
          puts("Fail to connect");
          exit(0);
        }
        else{
          sslT = SSL_new(ctx);
          SSL_set_fd(sslT,trade_sd);

          if (SSL_connect(sslT) == -1) {
            ERR_print_errors_fp(stderr);
          }

          memset(buf,'\0',strlen(buf));
          n = SSL_read(sslT,buf,sizeof(buf));

          printf("%s\n", buf);
          printf("Enter the amount you want to pay: ");
          scanf("%s", payMoney);
          SSL_write(sslT,payMoney,strlen(payMoney));
          memset(buf,'\0',strlen(buf));
          n = SSL_read(sslT,buf,sizeof(buf));

          printf("%s\n", buf);

          char minusMoney[11];
          memset(minusMoney,'\0',strlen(minusMoney));
          strcat(minusMoney,"#-");
          strcat(minusMoney,payMoney);
          SSL_write(ssl,minusMoney,strlen(minusMoney));
          memset(buf,'\0',strlen(buf));
          n = SSL_read(ssl,buf,sizeof(buf));
          printf("%s\n", buf);
          SSL_free(sslT);
        }
        close(trade_sd);

      }
    }
    else if (strcmp(get_2,"E") == 0) {
      memset(req,'\0',strlen(req));
      strcat(req,Exit);

      memset(buf,'\0',strlen(buf));
      SSL_write(ssl,req,strlen(req));
      n = SSL_read(ssl,buf,sizeof(buf));
      printf("%s\n",buf);

      SSL_free(ssl);

      close(server_sd);
      exit(0);
    }
  }
  SSL_CTX_free(ctx);
  return 0;
}
