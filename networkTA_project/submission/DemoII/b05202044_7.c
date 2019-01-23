#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../packet/packet.h"

extern Cookie *g_cookieP;

Cookie *newMyCookie()
{
  return newCookie(g_cookieP->account_len, g_cookieP->password_len, g_cookieP->account, g_cookieP->password);
}

void sendExitReq(SSL* sslP)
{
  PacketRequest *reqP = newReq(POST, PATH_EXIT, NULL, 0, newMyCookie());
  sendReq(reqP, sslP);
  freeReq(reqP);
}