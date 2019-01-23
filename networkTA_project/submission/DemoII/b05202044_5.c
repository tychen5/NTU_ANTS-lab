#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string.h>
#include "login.h"
#include "user.h"
#include "../util/def.h"
#include "../util/util.h"
#include "../packet/packet.h"

extern Cookie* g_cookieP;

void display_login()
{
  printf("%c. Login\n", LOGIN_LOGIN);
  printf("%c. Register\n", LOGIN_REGISTER);
  printf("%c. Exit\n\n", LOGIN_EXIT);
}

size_t login(SSL* sslP)
{
  //renew cookie
  freeCookie(g_cookieP);
  
  char account[BUF_SIZE];
  char password[BUF_SIZE];
  size_t account_len;
  size_t password_len;

  printf("account: ");
  account_len = my_read(account);

  printf("password: ");
  password_len = my_read(password);

  g_cookieP = newCookie(account_len, password_len, account, password);

  size_t port_len;
  char port[BUF_SIZE];

  printf("port: ");
  port_len = my_read(port);

  printf("\n");

  char path[] = PATH_LOGIN;

  Param** paramsPP = malloc(3 * sizeof(Param*));

  paramsPP[0] = newParam("account", LEN("account"), g_cookieP->account, g_cookieP->account_len);
  paramsPP[1] = newParam("password", LEN("password"), g_cookieP->password, g_cookieP->password_len);
  paramsPP[2] = newParam("port", LEN("port"), port, port_len);

  PacketRequest* reqP = newReq(POST, path, paramsPP, 3, newMyCookie());
  sendReq(reqP, sslP);
  freeReq(reqP);

  PacketResponse* resP = recvRes(sslP);
  printf("STATUS CODE: %zu\n", resP->status);
  printf("%s\n", resP->content);

  int stage;

  if(resP->status == OK){
    stage = STAGE_MENU;
  }
  else
  {
    stage = STAGE_LOGIN;
  }
  freeRes(resP);
  printf("\n");
  return stage;
}

void reg(SSL *sslP)
{
  char account[BUF_SIZE];
  char password[BUF_SIZE];

  printf("account: ");
  size_t account_len = my_read(account);

  printf("password: ");
  size_t password_len = my_read(password);

  printf("\n");

  char path[] = PATH_REG;

  Param** paramsPP = malloc(2 * sizeof(Param*));
  paramsPP[0] = newParam("account", LEN("account"), account, account_len);
  paramsPP[1] = newParam("password", LEN("password"), password, password_len);

  PacketRequest* reqP = newReq(POST, path, paramsPP, 2, newMyCookie());
  sendReq(reqP, sslP);
  freeReq(reqP);

  PacketResponse *resP = recvRes(sslP);
  printf("STATUS CODE: %zu\n", resP->status);
  printf("%s\n", resP->content);

  freeRes(resP);
}