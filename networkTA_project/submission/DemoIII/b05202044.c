#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "login.h"
#include "menu.h"
#include "user.h"
#include "../util/rdwrn.h"
#include "../util/def.h"
#include "../packet/packet.h"

Cookie *g_cookieP;

int newSockfd(const char *hostname, int port);
SSL_CTX *newCtx(void);
void getConfirm(SSL* sslP);

int main(int argc, char** argv)
{
  g_cookieP = newCookie(LEN(""), LEN(""), "", "");

  SSL_CTX *ctx;
  SSL *sslP;
  char *hostname, *portnum;

  if (argc != 3)
  {
    printf("usage: %s [hostname] [port]\n", argv[0]);
    exit(0);
  }

  ctx = newCtx();
  int sockfd = newSockfd(argv[1], atoi(argv[2]));
  sslP = SSL_new(ctx);
  SSL_set_fd(sslP, sockfd); //attach sockfd to sslP
  
  //check security connection
  if (SSL_connect(sslP) == -1){
    fprintf(stderr, "Error - ssl connect failed!\n");
    exit(1);
  } 

  //check if a certificate is valid
  if (SSL_get_verify_result(sslP) != X509_V_OK)
  {
    fprintf(stderr, "Error - verificate failed\n");
    exit(1);
  }

  getConfirm(sslP);

  char input;
  char buf[BUF_SIZE];

  size_t stage = STAGE_LOGIN;

  //login
  while (stage == STAGE_LOGIN)
  {

    display_login();

    printf("_> ");
    fgets(buf, BUF_SIZE, stdin);

    buf[strcspn(buf, "\n")] = 0;
    input = buf[0]; //takes only the first character
    if (strlen(buf) > 1)
      input = 'x'; // set invalid if input more 1 char

    switch (input)
    {
    case LOGIN_LOGIN:
      stage = login(sslP);
      break;

    case LOGIN_REGISTER:
      reg(sslP);
      break;

    case LOGIN_EXIT:
      stage = STAGE_EXIT;
      break;

    default:
      printf("Invalid choice...!\n");
      break;
    }
  }

  //menu
  while (stage == STAGE_MENU)
  {

    display_menu();

    printf("_> ");
    fgets(buf, BUF_SIZE, stdin);
    printf("\n");

    buf[strcspn(buf, "\n")] = 0;
    input = buf[0]; //takes only the first character
    if (strlen(buf) > 1)
      input = 'x'; // set invalid if input more 1 char

    switch (input)
    {
    case MENU_LIST:
      list(sslP);
      break;

    case MENU_INFO:
      info(sslP);
      break;

    case MENU_TOPUP:
      topup(sslP);
      break;

    case MENU_GIFT:
      gift(sslP);
      break;

    case MENU_MAILBOX:
      mailbox(sslP);
      break;

    case MENU_EXIT:
      stage = STAGE_EXIT;
      break;

    default:
      printf("Invalid choice...!\n");
      break;
    }
  }

  sendExitReq(sslP);
  SSL_free(sslP); /* release connection state */
  close(sockfd);     /* close socket */
  freeCookie(g_cookieP);
  SSL_CTX_free(ctx); /* release context */

  exit(EXIT_SUCCESS);
}

int newSockfd(const char *hostname, int port)
{
  int sd;
  struct sockaddr_in serv_addr;

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    fprintf(stderr, "Error - could not create socket");
    exit(1);
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = inet_addr(hostname);

  if (connect(sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
  {
    fprintf(stderr, "Error - connect failed");
    exit(1);
  }
  else
    printf("Connected to server...\n");

  return sd;
}

SSL_CTX *newCtx(void)
{
  SSL_CTX *ctx;

  OpenSSL_add_all_algorithms(); /* Load cryptos, et.al. */
  SSL_load_error_strings();     /* Bring in and register error messages */
  SSL_library_init();
  ctx = SSL_CTX_new(SSLv23_client_method()); /* Create new context */
  if (ctx == NULL)
  {
    ERR_print_errors_fp(stderr);
    abort();
  }

  //loading a trust store
  SSL_CTX_set_verify_depth(ctx, 4);
  const long flags = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
  SSL_CTX_set_options(ctx, flags);
  printf("load trust store...\n");
  if (!SSL_CTX_load_verify_locations(ctx, "root.crt", NULL))
  {
    fprintf(stderr, "Error - load failed\n");
  }

  return ctx;
}

void getConfirm(SSL* sslP){
  char buf[BUF_SIZE];
  size_t len;

  SSL_read(sslP, (unsigned char *)&len, sizeof(size_t));
  SSL_read(sslP, (unsigned char *)buf, len);

  printf("%s\n\n", buf);

  if (strcmp(buf, "server is bu5y, please wait for a second!\n\n") == 0)
  {
    getConfirm(sslP);
  }
}