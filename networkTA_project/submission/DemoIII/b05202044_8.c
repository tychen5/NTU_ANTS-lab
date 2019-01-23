#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "packet.h"
#include "../util/rdwrn.h"

/***
 * recieve Request
 ***/

PacketRequest *recvReq(SSL* sslp)
{

  DEBUG("recvReq");

  size_t method;
  if(SSL_read(sslp, (unsigned char *)&(method), sizeof(size_t)) == 0){
    return &req_discon;
  }
  DEBUG_ZU(method);

  size_t path_len;
  SSL_read(sslp, (unsigned char *)&(path_len), sizeof(size_t));
  DEBUG_ZU(path_len);
  if(path_len > BUF_SIZE){
    return NULL;
  }

  char path[path_len];
  SSL_read(sslp, (unsigned char *)(path), path_len);
  DEBUG_S(path);

  size_t params_len;
  SSL_read(sslp, (unsigned char *)&(params_len), sizeof(size_t));
  DEBUG_ZU(params_len);
  if (params_len > BUF_SIZE)
  {
    return NULL;
  }

  Param **paramsPP = malloc(params_len * sizeof(Param*));
  for (size_t i = 0; i != params_len; i++)
  {

    size_t var_len;
    SSL_read(sslp, (unsigned char *)&(var_len), sizeof(size_t));
    DEBUG_ZU(var_len);
    if (var_len > BUF_SIZE)
    {
      return NULL;
    }

    char var[var_len];
    SSL_read(sslp, (unsigned char *)(var), var_len);
    DEBUG_S(var);

    size_t val_len;
    SSL_read(sslp, (unsigned char *)&(val_len), sizeof(size_t));
    DEBUG_ZU(val_len);
    if (val_len > BUF_SIZE)
    {
      return NULL;
    }

    char val[val_len];
    SSL_read(sslp, (unsigned char *)(val), val_len);
    DEBUG_S(val);

    paramsPP[i] = newParam(var, var_len, val, val_len);

  }

  size_t account_len;
  SSL_read(sslp, (unsigned char *)&(account_len), sizeof(size_t));
  DEBUG_ZU(account_len);
  if (account_len > BUF_SIZE)
  {
    return NULL;
  }

  char account[account_len];
  SSL_read(sslp, (unsigned char *)(account), account_len);
  DEBUG_S(account);

  size_t password_len;
  SSL_read(sslp, (unsigned char *)&(password_len), sizeof(size_t));
  DEBUG_ZU(password_len);
  if (password_len > BUF_SIZE)
  {
    return NULL;
  }

  char password[password_len];
  SSL_read(sslp, (unsigned char *)(password), password_len);
  DEBUG_S(password);

  Cookie *cookieP = newCookie(account_len, password_len, account, password);

  DEBUG("");

  return newReq(method, path, paramsPP, params_len, cookieP);
}

/***
 * recieve Responce
 ***/

PacketResponse *recvRes(SSL* sslp)
{

  DEBUG("recvRes");

  size_t status;
  SSL_read(sslp, (unsigned char *)&(status), sizeof(size_t));
  DEBUG_ZU(status);

  size_t content_len;
  SSL_read(sslp, (unsigned char *)&(content_len), sizeof(size_t));
  DEBUG_ZU(content_len);
  if (content_len > BUF_SIZE)
  {
    return NULL;
  }

  char content[content_len];
  SSL_read(sslp, (unsigned char *)(content), content_len);
  DEBUG_S(content);

  DEBUG("");

  return newRes(status, content_len, content);
}

/***
 * send Request
 ***/

void sendReq(PacketRequest *reqP, SSL* sslp)
{

  DEBUG("sendReq");

  SSL_write(sslp, (unsigned char *)&(reqP->method), sizeof(size_t));
  DEBUG_ZU(reqP->method);

  SSL_write(sslp, (unsigned char *)&(reqP->path_len), sizeof(size_t));
  DEBUG_ZU(reqP->path_len);

  SSL_write(sslp, (unsigned char *)(reqP->path), reqP->path_len);
  DEBUG_S(reqP->path);

  SSL_write(sslp, (unsigned char *)&(reqP->params_len), sizeof(size_t));
  for (size_t i = 0; i != reqP->params_len; i++)
  {
    SSL_write(sslp, (unsigned char *)&(reqP->paramsPP[i]->var_len), sizeof(size_t));
    DEBUG_ZU(reqP->paramsPP[i]->var_len);

    SSL_write(sslp, (unsigned char *)(reqP->paramsPP[i]->var), reqP->paramsPP[i]->var_len);
    DEBUG_S(reqP->paramsPP[i]->var);

    SSL_write(sslp, (unsigned char *)&(reqP->paramsPP[i]->val_len), sizeof(size_t));
    DEBUG_ZU(reqP->paramsPP[i]->val_len);

    SSL_write(sslp, (unsigned char *)(reqP->paramsPP[i]->val), reqP->paramsPP[i]->val_len);
    DEBUG_S(reqP->paramsPP[i]->val);
  }

  SSL_write(sslp, (unsigned char *)&(reqP->cookieP->account_len), sizeof(size_t));
  DEBUG_ZU(reqP->cookieP->account_len);

  SSL_write(sslp, (unsigned char *)(reqP->cookieP->account), reqP->cookieP->account_len);
  DEBUG_S(reqP->cookieP->account);

  SSL_write(sslp, (unsigned char *)&(reqP->cookieP->password_len), sizeof(size_t));
  DEBUG_ZU(reqP->cookieP->password_len);

  SSL_write(sslp, (unsigned char *)(reqP->cookieP->password), reqP->cookieP->password_len);
  DEBUG_S(reqP->cookieP->password);

  DEBUG("");
}

/***
 * send Response
 ***/

void sendRes(PacketResponse *resP, SSL* sslp)
{

  DEBUG("sendRes");

  SSL_write(sslp, (unsigned char *)&(resP->status), sizeof(size_t));
  DEBUG_ZU(resP->status);

  SSL_write(sslp, (unsigned char *)&(resP->content_len), sizeof(size_t));
  DEBUG_ZU(resP->content_len);

  SSL_write(sslp, (unsigned char *)(resP->content), resP->content_len);
  DEBUG_S(resP->content);

  DEBUG("");
}

void sendNotFoundRes(SSL* sslp){
    
  PacketResponse *resP = newRes(NOT_FOUND, LEN("Page Not Found!\n"), "Page Not Found!\n");
  sendRes(resP, sslp);

}

/***
 * new cookie
 ***/

Cookie *newCookie(size_t account_len, size_t password_len, char *account, char *password)
{

  Cookie *cookieP = (Cookie *)malloc(sizeof(Cookie));

  cookieP->account_len = account_len;
  cookieP->password_len = password_len;

  cookieP->account = malloc(cookieP->account_len * sizeof(char));
  cookieP->password = malloc(cookieP->password_len * sizeof(char));

  strcpy(cookieP->account, account);
  strcpy(cookieP->password, password);

  return cookieP;
}

Cookie *newEmptyCookie()
{
  return newCookie(LEN(""), LEN(""), "", "");
}

/***
 * new Param
 ***/

Param *newParam(char *var, size_t var_len, char *val, size_t val_len)
{

  Param *paramP = (Param *)malloc(sizeof(Param));

  paramP->var_len = var_len;
  paramP->val_len = val_len;

  paramP->var = malloc(paramP->var_len * sizeof(char));
  paramP->val = malloc(paramP->val_len * sizeof(char));

  strcpy(paramP->var, var);
  strcpy(paramP->val, val);

  return paramP;
}

/***
 * new Response
 ***/

PacketResponse *newRes(size_t status, size_t content_len, char *content)
{
  PacketResponse *resP = (PacketResponse *)malloc(sizeof(PacketResponse));

  resP->status = status;
  resP->content_len = content_len;
  resP->content = malloc(resP->content_len * sizeof(char));
  strcpy(resP->content, content);

  return resP;
}

PacketResponse *newHelloRes()
{

  return newRes(OK, LEN("HELLO"), "HELLO");
}

/***
 * new Request
 ***/

PacketRequest *newReq(size_t method, char *path, Param **paramsPP, size_t params_len, Cookie *cookieP)
{
  PacketRequest *reqP = (PacketRequest *)malloc(sizeof(PacketRequest));

  reqP->method = method;
  reqP->path_len = LEN(path);
  reqP->params_len = params_len;
  reqP->path = malloc(reqP->path_len * sizeof(char));
  strcpy(reqP->path, path);
  reqP->paramsPP = paramsPP;
  reqP->cookieP = cookieP;

  return reqP;
}

/***
 * free
 ***/

void freeReq(PacketRequest *reqP)
{
  free(reqP->path);

  for (size_t i = 0; i != reqP->params_len; i++)
  {
    free(reqP->paramsPP[i]->val);
    free(reqP->paramsPP[i]->var);
    free(reqP->paramsPP[i]);
  }

  free(reqP->paramsPP);

  freeCookie(reqP->cookieP);

  free(reqP);
}

void freeRes(PacketResponse *resP)
{
  free(resP->content);
  free(resP);
}

void freeCookie(Cookie* cookieP){
  free(cookieP->account);
  free(cookieP->password);
  free(cookieP);
}

/***
 * send quit
 ***/

