#ifndef USER_H
#define USER_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../packet/packet.h"

Cookie *newMyCookie();
void sendExitReq(SSL* sslP);

#endif