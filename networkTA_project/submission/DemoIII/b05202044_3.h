#ifndef LOGIN_H
#define LOGIN_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdio.h>
#include "../util/def.h"

#define LOGIN_LOGIN '0'
#define LOGIN_REGISTER '1'
#define LOGIN_EXIT '2'

void display_login();
size_t login(SSL *sslP);
void reg(SSL *sslP);

#endif