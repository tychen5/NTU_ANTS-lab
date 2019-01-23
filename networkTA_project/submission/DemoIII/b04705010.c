#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdbool.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

const int maxN = 5;
const char t[]="\n";
const char b[]=":";
const char s[]="#";
bool login = false;
bool trade = false;
char usr[32] = {};
int usrPort;
int usrMoney = 0;

struct usrList
{
  char accountName[32];
  char *ipAddr;
  char portN[6];
};
struct usrList usrlist[maxN];

void updateList(char list[100])
{
  char *bal = NULL, *trash = NULL, *num = NULL;
  bal = strtok(list,t);
  trash = strtok(NULL,b);
  num = strtok(NULL,t);
  int usrN = atoi(num);
  for(int n = 0; n < maxN; n++)
  {
    if (n < usrN)
    {
      strcpy(usrlist[n].accountName, strtok(NULL, s));
      usrlist[n].ipAddr = strtok(NULL, s);
      strcpy(usrlist[n].portN, strtok(NULL, t));
    }
    else
    {
      memset(usrlist[n].accountName, 0, sizeof(usrlist[n].accountName));
      memset(usrlist[n].portN, 0, sizeof(usrlist[n].portN));
      usrlist[n].ipAddr = 0;
      usrlist[n].ipAddr = NULL;
    }
  }
  return;
}

void ShowCerts(SSL* fssl , bool isPrint){
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(fssl); // get the server's certificate
    if ( cert != NULL && isPrint )
    {
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        free(line);       // free the malloc'ed string
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        free(line);       // free the malloc'ed string
        X509_free(cert);     // free the malloc'ed certificate copy */
    }
    else if (cert == NULL)
        printf("No certificates.\n");
}

void LoadCertificates(SSL_CTX* fctx, char* CertFile, char* KeyFile)
{
    //set the local certificate from CertFile
    if ( SSL_CTX_use_certificate_file(fctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //set the private key from KeyFile
    if ( SSL_CTX_use_PrivateKey_file(fctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
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

int main(int argc, char *argv[])
{
  struct sockaddr_in server;
  int sock;
  int n, prt;
  char receiveMessage[100] = {};
  char test[] = "Exit";
  const char s[]="#";

  char *IPAddr = argv[1];
  prt = atoi(argv[2]);

  //build up socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  server.sin_family = AF_INET;
  server.sin_port = htons(prt);
  inet_pton(AF_INET, IPAddr, &server.sin_addr.s_addr);

  socklen_t len = sizeof(server);
  //connect TCP
  int err = connect(sock, (struct sockaddr *)&server, len);
  recv(sock,receiveMessage,sizeof(receiveMessage),0);
  printf("%s\n",receiveMessage);

  char ret[100] = {};
  char mes[100] = {};
  char temp[100] = {};
  char *foo = NULL;
  char *bar = NULL;
  char *baz = NULL;
  //main
  //connection success
  if(err != -1){
    while(1){
      foo = NULL;
      bar = NULL;
      baz = NULL;
      //reset all string array
      memset(ret, 0, sizeof(ret));
      memset(mes, 0, sizeof(mes));
      memset(temp, 0, sizeof(temp));

      scanf("%s", mes);

      strcpy(temp, mes);
      foo = strtok(temp,s);
      bar = strtok(NULL,s);
      baz = strtok(NULL,s);

      if(login)
      {
        if(strcmp(mes, "List") == 0)
        {
          send(sock, mes, sizeof(mes), 0);
          recv(sock, ret, sizeof(ret), 0);
          printf("%s", ret);
          updateList(ret);
        }
        else if(strcmp(mes, "Exit") == 0)
        {
          send(sock, mes, sizeof(mes), 0);
          recv(sock, ret, sizeof(ret), 0);
          printf("%s", ret);
          login = false;
          break;
        }
        //Payer mode
        else if(strcmp(mes, "Pay") == 0)
        {
          printf("[INFO] Paying mode. Type \"Quit\" to exit.\n");

          char payment[100] = {};
          scanf("%s", payment);

          if(!strcmp(payment, "Quit"))
            continue;
          strcpy(temp, payment);
          foo = strtok(temp,s);
          bar = strtok(NULL,s);
          baz = strtok(NULL,s);

          char payeeIP[100] = {};
          int payeePrt = -1;
          for(int n = 0; n < maxN; n++)
          {
            if(strcmp(usrlist[n].accountName, baz)==0)
            {
              strcpy(payeeIP, usrlist[n].ipAddr);
              payeePrt = atoi(usrlist[n].portN);
            }
          }
          if(payeePrt != -1)
          {
            struct sockaddr_in payer;
            int paymentSock;
            SSL_CTX *ctx;
            SSL *ssl;

            SSL_library_init();
            OpenSSL_add_all_algorithms();
            SSL_load_error_strings();
            ctx = SSL_CTX_new(SSLv23_client_method());
            if (ctx == NULL)
            {
              ERR_print_errors_fp(stdout);
              exit(1);
            }

            LoadCertificates(ctx, "cert.pem", "key.pem");

            paymentSock = socket(AF_INET, SOCK_STREAM, 0);
            payer.sin_family = AF_INET;
            payer.sin_port = htons(payeePrt);
            inet_pton(AF_INET, payeeIP, &payer.sin_addr.s_addr);

            socklen_t len = sizeof(payer);
            int err_ = connect(paymentSock, (struct sockaddr *)&payer, len);
            char connectionMes[100] = {};
            char confirmMes[100] = {};

            ssl = SSL_new(ctx);
            SSL_set_fd(ssl, paymentSock);

            if (SSL_connect(ssl) == -1)
              ERR_print_errors_fp(stderr);
            else
            {
              printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
              ShowCerts(ssl, false);
            }

            if(err_ != -1)
            {
              SSL_read(ssl, connectionMes, strlen(connectionMes));
              printf("%s", connectionMes);
              SSL_write(ssl, payment, strlen(payment));
              SSL_read(ssl, confirmMes, strlen(confirmMes));
              printf("%s", confirmMes);
            }
          }
          else
            printf("[INFO] Not a reachable payee.\n");

        }
        //payee mode
        else if(strcmp(mes, "Trade") == 0)
        {
          printf("[INFO] Trading mode. Type anything to continue or \"Quit\" to exit.\n");

          char quit[100] = {};
          scanf("%s", quit);
          if(!strcmp(quit, "Quit"))
            continue;
          int hostSocket, clientSocket;
          struct sockaddr_in hostAddr;
          struct sockaddr_in hostStorage;
          socklen_t addr_size;
          SSL_CTX *ctx;
          SSL *ssl;

          SSL_library_init();
          OpenSSL_add_all_algorithms();
          SSL_load_error_strings();
          ctx = SSL_CTX_new(SSLv23_client_method());
          if (ctx == NULL)
          {
            ERR_print_errors_fp(stdout);
            exit(1);
          }

          LoadCertificates(ctx, "cert.pem", "key.pem");

          hostSocket = socket(AF_INET, SOCK_STREAM, 0);
          hostAddr.sin_family = AF_INET;
          hostAddr.sin_port = htons(usrPort);
          hostAddr.sin_addr.s_addr = INADDR_ANY;
          memset(hostAddr.sin_zero, '\0', sizeof hostAddr.sin_zero);
          bind(hostSocket, (struct sockaddr *)&hostAddr, sizeof(hostAddr));
          if(listen(hostSocket, 1) == 0)
            printf("[INFO] Listening......\n");
          else
            printf("[INFO] Error\n");

          addr_size = sizeof hostStorage;
          clientSocket = accept(hostSocket, (struct sockaddr *)&hostStorage, &addr_size);

          ssl = SSL_new(ctx);
          SSL_set_fd(ssl, clientSocket);
          if (SSL_accept(ssl) == -1)
          {
            perror("accept");
            close(clientSocket);
            break;
          }

          char ca[100] = "[INFO] Connection accepted\n";
          printf("%s", ca);
          SSL_write(ssl, ca, strlen(ca));
          SSL_read(ssl, ret, strlen(ret));

          strcpy(temp, ret);
          foo = strtok(temp,s);
          bar = strtok(NULL,s);
          baz = strtok(NULL,s);
          char mes_pay[100] = "[INFO] Payment accept.\n";
          char mes_wrong[100] = "[INFO] Payee wrong.\n";
          if(strcmp(baz, usr) == 0)
          {
            SSL_write(ssl, mes_pay, strlen(mes_pay));
            send(sock, ret, sizeof(ret), 0);
            printf("%s", mes_pay);
          }
          else
          {
            SSL_write(ssl, mes_wrong, strlen(mes_wrong));
            printf("%s", mes_wrong);
          }

          SSL_shutdown(ssl);
          SSL_free(ssl);
          close(clientSocket);
        }
        else
        {
          printf("[INFO] Invalid request.\n");
        }
      }
      else
      {
        if(strcmp(foo, "REGISTER") == 0)
        {
          send(sock, mes, sizeof(mes), 0);
          recv(sock, ret, sizeof(ret), 0);
          printf("%s", ret);
          if (strcmp(ret, "100 OK\n") == 0)
            strcpy(usr, bar);
        }
        else if(strcmp(foo, usr)==0)
        {
          usrPort = atoi(bar);
          send(sock, mes, sizeof(mes), 0);
          recv(sock, ret, sizeof(ret), 0);
          printf("%s", ret);
          if (strcmp(ret, "220 AUTH_FAIL\n"))
          {
            login = true;
            updateList(ret);
          }
        }
        else{
          printf("[INFO] Invalid request.\n");
        }
      }
    }
  }
  //connection fail
  else{
    printf("[INFO] connection error");
  }

  //close TCP connection
  close(sock);

  return 0;
}
