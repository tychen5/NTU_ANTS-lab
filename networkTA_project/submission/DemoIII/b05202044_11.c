#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../util/def.h"
#include "../packet/packet.h"
#include "user.h"
#include "process.h"

extern pthread_mutex_t lock;
extern OnlineUsers users;

size_t process(PacketRequest *reqP, SSL *sslP, User* userP)
{
  if (reqP == NULL)
  {
    sendNotFoundRes(sslP);
    return STAGE_CONNECT;
  }

  if (reqP == &req_discon)
  {
    return STAGE_DISCONNECT;
  }

  char *dir;
  dir = strtok(reqP->path, "/"); // replace the char to 0 if the char is '/'

  printf("dir: %s\n", dir);

  if (strcmp(dir, EXIT) == 0)
  {
    return STAGE_DISCONNECT;
  }
  else if (strcmp(dir, INFO) == 0)
  {
    info(reqP, sslP, userP);
  }
  else if (strcmp(dir, LIST) == 0)
  {
    list(reqP, sslP, userP);
  }
  else if (strcmp(dir, TOPUP) == 0)
  {
    topup(reqP, sslP, userP);
  }
  else if (strcmp(dir, GIFT) == 0)
  {
    gift(reqP, sslP, userP);
  }
  else if (strcmp(dir, MAILBOX) == 0)
  {
    mailbox(reqP, sslP, userP);
  }
  else if (strcmp(dir, LOGIN) == 0)
  {
    login(reqP, sslP, userP);
  }
  else if (strcmp(dir, REG) == 0)
  {
    reg(reqP, sslP);
  }
  else
  {
    sendNotFoundRes(sslP);
  }

  dir = strtok(NULL, "/"); // pass NULL to tell strtok to continue tokenizing the string you passed

  return STAGE_CONNECT;
}

void info(PacketRequest *reqP, SSL *sslP, User* userP)
{
  if (!checkCookie(reqP->cookieP, userP))
  {
    // bad cookie
    PacketResponse *resP = newRes(UNAUTHO, LEN("please login first\n"), "please login first\n");
    sendRes(resP, sslP);
    freeRes(resP);
    return;
  }

  //get balance
  char buf[BUF_SIZE];
  size_t balance = get_balance(reqP->cookieP->account);
  sprintf(buf, "balance: %zu\n", balance);

  //send response
  PacketResponse *resP = newRes(OK, LEN(buf), buf);
  sendRes(resP, sslP);
  freeRes(resP);
}

void list(PacketRequest *reqP, SSL *sslP, User* userP)
{
  if (!checkCookie(reqP->cookieP, userP))
  {
    // bad cookie
    PacketResponse *resP = newRes(UNAUTHO, LEN("please login first\n"), "please login first\n");
    sendRes(resP, sslP);
    freeRes(resP);
    return;
  }

  //read data from users
  pthread_mutex_lock(&lock);

  size_t buf_len = 0;
  for(size_t i=0; i!=THREAD; i++){
    if (users.users[i].cookieP){
      buf_len += users.users[i].cookieP->account_len;
      buf_len += 64; //magic number
    }
  }

  char buf[buf_len];
  sprintf(buf, "number of online users: %zu\n", users.users_n);

  for (size_t i = 0; i != THREAD; i++)
  {
    if(users.users[i].cookieP){
      strcat(buf, users.users[i].cookieP->account);
      strcat(buf, "\n");
    }
  }

  pthread_mutex_unlock(&lock);

  //send packet
  PacketResponse *resP = newRes(OK, LEN(buf), buf);
  sendRes(resP, sslP);
  freeRes(resP);

}

void login(PacketRequest *reqP, SSL *sslP, User* userP)
{
  //index
  int account_i = -1;
  int password_i = -1;
  int port_i = -1;

  if(userP->cookieP){
    //login already
    PacketResponse *resP = newRes(BAD_REQ, LEN("you have login already!\n"), "you have login already!\n");
    sendRes(resP, sslP);
    freeRes(resP);
    return;
  }

  for (size_t i = 0; i != reqP->params_len; i++){
    if (strcmp("account", reqP->paramsPP[i]->var) == 0)
    {
      regex_t regex;
      regcomp(&regex, "[a-zA-Z0-9]*", 0);
      if (regexec(&regex, reqP->paramsPP[i]->val, 0, NULL, 0))
      {
        //bad account
        PacketResponse *resP = newRes(BAD_REQ, LEN("account contains invalid alphbet!\n"), "account contains invalid alphbet!\n");
        sendRes(resP, sslP);
        freeRes(resP);
        return;
      }

      account_i = i;
    }

    if (strcmp("password", reqP->paramsPP[i]->var) == 0)
    {
      regex_t regex;
      regcomp(&regex, "[a-zA-Z0-9]*", 0);
      if (regexec(&regex, reqP->paramsPP[i]->val, 0, NULL, 0))
      {
        //bad password
        PacketResponse *resP = newRes(BAD_REQ, LEN("password contains invalid alphbet!\n"), "password contains invalid alphbet!\n");
        sendRes(resP, sslP);
        freeRes(resP);
        return;
      }

      password_i = i;
    }

    if (strcmp("port", reqP->paramsPP[i]->var) == 0)
    {
      regex_t regex;
      regcomp(&regex, "[0-9]*", 0);
      if (regexec(&regex, reqP->paramsPP[i]->val, 0, NULL, 0))
      {
        //bad port
        PacketResponse *resP = newRes(BAD_REQ, LEN("port contains invalid alphbet!\n"), "port contains invalid alphbet!\n");
        sendRes(resP, sslP);
        freeRes(resP);
        return;
      }

      port_i = i;
    }
  }

  if(account_i == -1 || password_i == -1 || port_i == -1){
    //bab params
    PacketResponse *resP = newRes(BAD_REQ, LEN("please fill the form!"), "please fill the form!");
    sendRes(resP, sslP);
    freeRes(resP);
    return;
  }

  //check account
  if (!is_account_valid(reqP->paramsPP[account_i]->val))
  {
    //the account does not exist!
    PacketResponse *resP = newRes(BAD_REQ, LEN("the account does not exist\n"), "the account does not exist\n");
    sendRes(resP, sslP);
    freeRes(resP);
    return;
  }

  //check password
  char base[] = "db/client/";
  char dir[reqP->paramsPP[password_i]->val_len + strlen(base) + 32];
  sprintf(dir, "%s%s/pass", base, reqP->paramsPP[account_i]->val);

  FILE *fileP = fopen(dir, "r");
  if (!fileP)
  {
    fprintf(stderr, "cannot open %s", dir);
  }

  fseek(fileP, 0, SEEK_END);
  size_t fsize = ftell(fileP);
  fseek(fileP, 0, SEEK_SET);

  char password[fsize];
  fread(password, fsize, sizeof(char), fileP);
  fclose(fileP);

  if(strcmp(password, reqP->paramsPP[password_i]->val) != 0){
    // password is not correct!
    PacketResponse *resP = newRes(BAD_REQ, LEN("password is not correct\n"), "password is not correct\n");
    sendRes(resP, sslP);
    freeRes(resP);
    return;    
  }

  //update users
  addUser(
    reqP->paramsPP[account_i]->val, 
    reqP->paramsPP[account_i]->val_len, 
    reqP->paramsPP[password_i]->val,
    reqP->paramsPP[password_i]->val_len,
    atoi(reqP->paramsPP[port_i]->val), userP);

  //send packet
  PacketResponse *resP = newRes(OK, LEN("SUCCESS\n"), "SUCCESS\n");
  sendRes(resP, sslP);
  freeRes(resP);

}

void reg(PacketRequest *reqP, SSL *sslP)
{
  //index
  int account_i = -1;
  int password_i = -1;

  for (size_t i = 0; i != reqP->params_len; i++)
  {
    if (strcmp("account", reqP->paramsPP[i]->var) == 0)
    {
      regex_t regex;
      regcomp(&regex, "[a-zA-Z0-9]*", 0);
      if (regexec(&regex, reqP->paramsPP[i]->val, 0, NULL, 0))
      {
        //bad account
        PacketResponse *resP = newRes(BAD_REQ, LEN("account contains invalid alphbet!\n"), "account contains invalid alphbet!\n");
        sendRes(resP, sslP);
        freeRes(resP);
        return;
      }

      account_i = i;
    }

    if (strcmp("password", reqP->paramsPP[i]->var) == 0)
    {
      regex_t regex;
      regcomp(&regex, "[a-zA-Z0-9]*", 0);
      if (regexec(&regex, reqP->paramsPP[i]->val, 0, NULL, 0))
      {
        //bad password
        PacketResponse *resP = newRes(BAD_REQ, LEN("password contains invalid alphbet!\n"), "password contains invalid alphbet!\n");
        sendRes(resP, sslP);
        freeRes(resP);
        return;
      }

      password_i = i;
    }
  }

  if (account_i == -1 || password_i == -1){
    //bab params
    PacketResponse *resP = newRes(BAD_REQ, LEN("please fill the form!"), "please fill the form!");
    sendRes(resP, sslP);
    freeRes(resP);
    return;
  }

  char base[] = "db/client/";
  char dir[reqP->paramsPP[account_i]->val_len + strlen(base) + 10];

  strcpy(dir, base);
  strcat(dir, reqP->paramsPP[account_i]->val);

  struct stat st;
  if (stat(dir, &st) == 0)
  {
    //the account is used already!
    PacketResponse *resP = newRes(BAD_REQ, LEN("the account is used already!\n"), "the account is used already!\n");
    sendRes(resP, sslP);
    freeRes(resP);
    return;
  }

  size_t x = mkdir(dir, 0777);

  //write password
  strcpy(dir, base);
  strcat(dir, reqP->paramsPP[account_i]->val);
  strcat(dir, "/pass");

  FILE *fileP = fopen(dir, "w");
  if (!fileP)
  {
    fprintf(stderr, "cannot open %s", dir);
  }
  fwrite(reqP->paramsPP[password_i]->val, sizeof(char), reqP->paramsPP[password_i]->val_len, fileP);
  fclose(fileP);

  //write balance
  set_balance(reqP->paramsPP[account_i]->val, 0);

  //create mailbox
  strcpy(dir, base);
  strcat(dir, reqP->paramsPP[account_i]->val);
  strcat(dir, "/mailbox");
  size_t mails_len = 0;

  fileP = fopen(dir, "w");
  if (!fileP)
  {
    fprintf(stderr, "cannot open %s", dir);
  }
  fwrite(&mails_len, sizeof(size_t), 1, fileP);
  fclose(fileP);

  //send packet
  PacketResponse *resP = newRes(OK, LEN("SUCCESS\n"), "SUCCESS\n");
  sendRes(resP, sslP);
  freeRes(resP);
}


void topup(PacketRequest *reqP, SSL *sslP, User* userP){

  if (!checkCookie(reqP->cookieP, userP))
  {
    // bad cookie
    PacketResponse *resP = newRes(UNAUTHO, LEN("please login first\n"), "please login first\n");
    sendRes(resP, sslP);
    freeRes(resP);
    return;
  }

  //index
  int code_i = -1;

  for (size_t i = 0; i != reqP->params_len; i++)
  {
    if (strcmp("code", reqP->paramsPP[i]->var) == 0)
    {
      regex_t regex;
      regcomp(&regex, "[0-9]*", 0);
      if (regexec(&regex, reqP->paramsPP[i]->val, 0, NULL, 0))
      {
        //bad code
        PacketResponse *resP = newRes(BAD_REQ, LEN("code contains invalid alphbet!\n"), "code contains invalid alphbet!\n");
        sendRes(resP, sslP);
        freeRes(resP);
        return;
      }

      code_i = i;
    }
  }

  //bab params
  if(code_i == -1){
    PacketResponse *resP = newRes(BAD_REQ, LEN("please fill the form!"), "please fill the form!");
    sendRes(resP, sslP);
    freeRes(resP);
    return;
  }

  //update balance
  size_t value = atoi(reqP->paramsPP[code_i]->val);
  size_t balance = get_balance(reqP->cookieP->account);
  set_balance(reqP->cookieP->account, balance + value);

  //send packet
  PacketResponse *resP = newRes(OK, LEN("SUCCESS\n"), "SUCCESS\n");
  sendRes(resP, sslP);
  freeRes(resP);

}

void gift(PacketRequest *reqP, SSL *sslP, User *userP){
  
  if (!checkCookie(reqP->cookieP, userP))
  {
    // bad cookie
    PacketResponse *resP = newRes(UNAUTHO, LEN("please login first\n"), "please login first\n");
    sendRes(resP, sslP);
    freeRes(resP);
    return;
  }

  //index
  int friend_i = -1;
  int money_i = -1;

  for (size_t i = 0; i != reqP->params_len; i++)
  {
    if (strcmp("friend", reqP->paramsPP[i]->var) == 0)
    {
      regex_t regex;
      regcomp(&regex, "[a-zA-Z0-9]*", 0);
      if (regexec(&regex, reqP->paramsPP[i]->val, 0, NULL, 0))
      {
        //bad friend
        PacketResponse *resP = newRes(BAD_REQ, LEN("friend contains invalid alphbet!\n"), "friend contains invalid alphbet!\n");
        sendRes(resP, sslP);
        freeRes(resP);
        return;
      }

      friend_i = i;
    }

    if (strcmp("money", reqP->paramsPP[i]->var) == 0)
    {
      regex_t regex;
      regcomp(&regex, "[0-9]*", 0);
      if (regexec(&regex, reqP->paramsPP[i]->val, 0, NULL, 0))
      {
        //bad money
        PacketResponse *resP = newRes(BAD_REQ, LEN("money contains invalid alphbet!\n"), "money contains invalid alphbet!\n");
        sendRes(resP, sslP);
        freeRes(resP);
        return;
      }

      money_i = i;
    }
  }

  //bab params
  if (friend_i == -1 || money_i == -1)
  {
    PacketResponse *resP = newRes(BAD_REQ, LEN("please fill the form!"), "please fill the form!");
    sendRes(resP, sslP);
    freeRes(resP);
    return;
  }

  //check if the user is valid
  if (!is_account_valid(reqP->cookieP->account))
  {
    //the account does not exist!
    PacketResponse *resP = newRes(BAD_REQ, LEN("the account does not exist\n"), "the account does not exist\n");
    sendRes(resP, sslP);
    freeRes(resP);
    return;
  }

  //check if balance is enough
  size_t money = atoi(reqP->paramsPP[money_i]->val);
  if(get_balance(reqP->cookieP->account) < money){
    //balance is not enough!
    PacketResponse *resP = newRes(BAD_REQ, LEN("you dont have enough money!\n"), "you dont have enough money!\n");
    sendRes(resP, sslP);
    freeRes(resP);
    return;
  }

  //send mail
  char mail[reqP->cookieP->account_len + reqP->paramsPP[money_i]->val_len + 128];
  sprintf(mail, "%s gives you %zu coins\n", reqP->cookieP->account, money);

  char base[] = "db/client/";
  char dir[reqP->paramsPP[friend_i]->val_len + LEN(base) + 32];
  sprintf(dir, "%s%s/mailbox", base, reqP->paramsPP[friend_i]->val);

  FILE *fileP = fopen(dir, "r");
  if (!fileP)
  {
    fprintf(stderr, "cannot open %s", dir);
  }
  size_t mails_len;
  fread(&mails_len, sizeof(size_t), 1, fileP);

  mails_len += 1;

  fileP = fopen(dir, "r+");
  if (!fileP)
  {
    fprintf(stderr, "cannot open %s", dir);
  }

  fwrite(&mails_len, sizeof(size_t), 1, fileP);

  fseek(fileP, 0, SEEK_END);

  size_t mail_len = LEN(mail);
  fwrite(&mail_len, sizeof(size_t), 1, fileP);
  fwrite(mail, sizeof(char), LEN(mail), fileP);


  fclose(fileP);

  //update balance
  set_balance(reqP->cookieP->account, (get_balance(reqP->cookieP->account) - money));
  set_balance(reqP->paramsPP[friend_i]->val, (get_balance(reqP->paramsPP[friend_i]->val) + money));

  //send packet
  PacketResponse *resP = newRes(OK, LEN("SUCCESS\n"), "SUCCESS\n");
  sendRes(resP, sslP);
  freeRes(resP);

}

void mailbox(PacketRequest *reqP, SSL *sslP, User *userP){

  if (!checkCookie(reqP->cookieP, userP))
  {
    // bad cookie
    PacketResponse *resP = newRes(UNAUTHO, LEN("please login first\n"), "please login first\n");
    sendRes(resP, sslP);
    freeRes(resP);
    return;
  }

  //index
  char** mails;
  size_t mail_len;
  size_t mails_len;
  size_t total_len = 0;

  // check mail
  char base[] = "db/client/";
  char dir[reqP->cookieP->account_len + LEN(base) + 32];
  sprintf(dir, "%s%s/mailbox", base, reqP->cookieP->account);

  FILE *fileP = fopen(dir, "r");
  if (!fileP)
  {
    fprintf(stderr, "cannot open %s", dir);
  }

  fread(&mails_len, sizeof(size_t), 1, fileP);

  mails = malloc(sizeof(char*) * mails_len);

  for (size_t i = 0; i != mails_len; i++)
  {
    fread(&mail_len, sizeof(size_t), 1, fileP);
    total_len += mail_len;
    mails[i] = malloc(mail_len * sizeof(char));
    fread(mails[i], sizeof(char), mail_len, fileP);
  }
  fclose(fileP);

  char buf[total_len + 32 * mails_len];
  buf[0] = 0;

  char idx_str[32];
  for (size_t i = 0; i != mails_len; i++)
  {
    sprintf(idx_str, "%zu. ", i);
    strcat(buf, idx_str);
    strcat(buf, mails[i]);
  }

  //send packet
  PacketResponse *resP = newRes(OK, LEN(buf), buf);
  sendRes(resP, sslP);
  freeRes(resP);

  for(size_t i=0; i!=mails_len; i++){
    free(mails[i]);
  }

  free(mails);

}


bool checkCookie(Cookie* cookieP, User* userP){

  bool flag = true;
  
  pthread_mutex_lock(&lock);
  if (!userP->cookieP){
    flag = false;
  }
  else if (cookieP->account_len != userP->cookieP->account_len)
  {
    flag = false;
  }  
  else if(cookieP->password_len != userP->cookieP->password_len){
    flag = false;
  }
  else if (strcmp(cookieP->account, userP->cookieP->account) != 0)
  {
    flag = false;
  }
  else if (strcmp(cookieP->password, userP->cookieP->password) != 0)
  {
    flag = false;
  }
  pthread_mutex_unlock(&lock);

  return flag;
}


void addUser(char* account, size_t account_len, char* password, size_t password_len, size_t port, User* userP)
{

  // update users
  pthread_mutex_lock(&lock);
  userP->cookieP = newCookie(account_len, password_len, account, password);
  userP->port = port;
  users.users_n += 1;
  pthread_mutex_unlock(&lock);

}

size_t get_balance(char* account){

  size_t balance;

  char base[] = "db/client/";
  char dir[strlen(account) + strlen(base) + 32];
  sprintf(dir, "%s%s/balance", base, account);

  FILE *fileP = fopen(dir, "r");
  if (!fileP)
  {
    fprintf(stderr, "cannot open %s", dir);
  }

  fread(&balance, sizeof(size_t), 1, fileP);
  fclose(fileP);

  return balance;

}

void set_balance(char* account, size_t balance){

  char base[] = "db/client/";
  char dir[strlen(account) + strlen(base) + 32];
  sprintf(dir, "%s%s/balance", base, account);

  FILE *fileP = fopen(dir, "w");
  if (!fileP)
  {
    fprintf(stderr, "cannot open %s", dir);
  }
  fwrite(&balance, sizeof(size_t), 1, fileP);
  fclose(fileP);

}

bool is_account_valid(char* account){

  char base[] = "db/client/";
  char dir[strlen(account) + strlen(base) + 32];
  sprintf(dir, "%s%s", base, account);

  struct stat st;
  if (stat(dir, &st) == -1)
  {
    return false;
  }
  else{
    return true;
  }

}
