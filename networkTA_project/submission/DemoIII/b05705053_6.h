#ifndef USER_H
#define USER_H

#include <string>
#include <vector>
#include <pthread.h>

using namespace std;

class UserOnline
{
  private:
    string name;
    string ip;
    int port;

  public:
    UserOnline(string name, string ip, int port) : name(name), ip(ip), port(port) {}
    string getName() { return this->name; }
    string getIp() { return this->ip; }
    int getPort() { return this->port; }
};

class User
{
  private:
    string name;
    string password;
    int balance;

  public:
    User(string name, string password, int balance) : name(name), password(password), balance(balance) {}
    string getName() { return this->name; }
    string getPassword() { return this->password; }
    int getBalance() { return this->balance; }
    void setBalance(int balance) { this->balance = balance; }
};
class UserList
{
  private:
    vector<User> user;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  public:
    void update_user(string username, string password, int balance);
    bool registered(string username);
    bool modify_money(string username, int deltaMoney);
    int get_balance(string username);
    bool login(string username , string password);
};
class UserOnlineList
{
  private:
    vector<UserOnline> user_online;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  public:
    string list();
    bool is_online(string username);
    string get_user_info(string username);
    void update_online_user(string username , string ip, int port);
    void update_offline_user(string username);
};
#endif //USER_H