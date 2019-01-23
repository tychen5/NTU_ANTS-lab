#ifndef PACKET_H
#define PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../util/def.h"

#define GET 0
#define POST 1

#define OK 200
#define BAD_REQ 400
#define UNAUTHO 401
#define FORBIDEN 403
#define NOT_FOUND 404
#define TOO_BUSY 503

#define REG "reg"
#define LOGIN "login"
#define LIST "list"
#define INFO "info"
#define TOPUP "topup"
#define GIFT "gift"
#define MAILBOX "mailbox"
#define EXIT "exit"

#define PATH_REG "/" REG
#define PATH_LOGIN "/" LOGIN
#define PATH_LIST "/" LIST
#define PATH_INFO "/" INFO
#define PATH_TOPUP "/" TOPUP
#define PATH_GIFT "/" GIFT
#define PATH_MAILBOX "/" MAILBOX
#define PATH_EXIT "/" EXIT

typedef struct
{
  size_t var_len;
  size_t val_len;

  char* var;
  char* val;
} Param;

typedef struct
{
  size_t account_len;
  size_t password_len;

  char* account;
  char* password;
} Cookie;

typedef struct
{
  size_t method;

  size_t path_len;
  size_t params_len;

  char* path;
  Param** paramsPP;
  Cookie* cookieP;
} PacketRequest;

PacketRequest req_discon;

typedef struct
{
  size_t status;
  size_t content_len;
  
  char* content;
} PacketResponse;

PacketResponse res_discon;

// function

void sendReq(PacketRequest *reqP, SSL* sslp);
void sendRes(PacketResponse *resP, SSL* sslp);
void sendNotFoundRes(SSL* sslp);
PacketResponse *recvRes(SSL* sslp);
PacketRequest *recvReq(SSL* sslp);

Cookie *newCookie(size_t account_len, size_t password_len, char *account, char *password);
Cookie *newEmptyCookie();
Param *newParam(char *var, size_t var_len, char *val, size_t val_len);
PacketRequest* newReq(size_t method, char *path, Param **paramsPP, size_t params_len, Cookie* cookieP);
PacketResponse* newRes(size_t status, size_t content_len, char *content);
PacketResponse *newHelloRes();

void freeReq(PacketRequest* reqP);
void freeRes(PacketResponse* resP);
void freeCookie(Cookie* cookieP);

#endif