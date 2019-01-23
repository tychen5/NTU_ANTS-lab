#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../packet/packet.h"
#include "user.h"

size_t process(PacketRequest* reqP, SSL* sslP, User* userP);
void info(PacketRequest* reqP, SSL* sslP, User* userP);
void list(PacketRequest* reqP, SSL* sslP, User* userP);
void login(PacketRequest* reqP, SSL* sslP, User* userP);
void reg(PacketRequest* reqP, SSL* sslP);
void topup(PacketRequest *reqP, SSL *sslP, User *userP);
void gift(PacketRequest *reqP, SSL *sslP, User *userP);
void mailbox(PacketRequest *reqP, SSL *sslP, User *userP);
bool checkCookie(Cookie* cookieP, User* userP);
void addUser(char* account, size_t account_len, char* password, size_t password_len, size_t port, User* userP);
size_t get_balance(char* account);
void set_balance(char *account, size_t balance);
bool is_account_valid(char *account);