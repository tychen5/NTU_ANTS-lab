#ifndef MENU_H
#define MENU_H

#define MENU_LIST '0'
#define MENU_INFO '1'
#define MENU_TOPUP '2'
#define MENU_GIFT '3'
#define MENU_MAILBOX '4'
#define MENU_EXIT '5'

#include <openssl/ssl.h>
#include <openssl/err.h>

void display_menu();
void list(SSL *sslP);
void info(SSL *sslP);
void topup(SSL *sslP);
void gift(SSL *sslP);
void mailbox(SSL *sslP);

#endif
