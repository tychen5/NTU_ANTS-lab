#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "menu.h"
#include "user.h"
#include "../util/def.h"
#include "../packet/packet.h"
#include "../util/util.h"

void display_menu()
{
  printf("%c. List online user\n", MENU_LIST);
  printf("%c. Show personal information\n", MENU_INFO);
  printf("%c. Topup\n", MENU_TOPUP);
  printf("%c. Send money to your friend\n", MENU_GIFT);
  printf("%c. Mailbox\n", MENU_MAILBOX);
  printf("%c. Exit\n\n", MENU_EXIT);
}

void list(SSL* sslP)
{

  char path[] = PATH_LIST;

  PacketRequest* reqP = newReq(GET, path, NULL, 0, newMyCookie());
  sendReq(reqP, sslP);
  freeReq(reqP);

  PacketResponse* resP = recvRes(sslP);
  printf("STATUS CODE: %zu\n", resP->status);
  printf("%s\n", resP->content);

  freeRes(resP);
}

void info(SSL* sslP)
{

  char path[] = PATH_INFO;

  PacketRequest* reqP = newReq(GET, path, NULL, 0, newMyCookie());

  sendReq(reqP, sslP);
  freeReq(reqP);

  PacketResponse* resP = recvRes(sslP);
  printf("STATUS CODE: %zu\n", resP->status);
  printf("%s\n", resP->content);

  freeRes(resP);

}

void topup(SSL *sslP){
  
  char code[BUF_SIZE];
  size_t code_len;

  printf("how much do you want to top up?: ");
  code_len = my_read(code);

  char path[] = PATH_TOPUP;

  Param **paramsPP = malloc(1 * sizeof(Param *));
  
  paramsPP[0] = newParam("code", LEN("code"), code, code_len);

  PacketRequest* reqP = newReq(POST, path, paramsPP, 1, newMyCookie());
  sendReq(reqP, sslP);
  freeReq(reqP);

  PacketResponse *resP = recvRes(sslP);
  printf("STATUS CODE: %zu\n", resP->status);
  printf("%s\n", resP->content);

  freeRes(resP);
}

void gift(SSL *sslP){

  char friend[BUF_SIZE];
  size_t friend_len;
  
  printf("please enter the account of your friend: ");
  friend_len = my_read(friend);

  char money[BUF_SIZE];
  size_t money_len;
  
  printf("how much money do you want to give: ");
  money_len = my_read(money);

  char path[] = PATH_GIFT;

  Param** paramsPP = malloc(2 * sizeof(Param*));

  paramsPP[0] = newParam("friend", LEN("friend"), friend, friend_len);
  paramsPP[1] = newParam("money", LEN("money"), money, money_len);

  PacketRequest* reqP = newReq(POST, path, paramsPP, 2, newMyCookie());
  sendReq(reqP, sslP);
  freeReq(reqP);

  PacketResponse *resP = recvRes(sslP);
  printf("STATUS CODE: %zu\n", resP->status);
  printf("%s\n", resP->content);

  freeRes(resP);

}

void mailbox(SSL *sslP)
{

  char path[] = PATH_MAILBOX;

  PacketRequest *reqP = newReq(POST, path, NULL, 0, newMyCookie());
  sendReq(reqP, sslP);
  freeReq(reqP);

  PacketResponse *resP = recvRes(sslP);
  printf("STATUS CODE: %zu\n", resP->status);
  printf("%s\n", resP->content);

  freeRes(resP);
}