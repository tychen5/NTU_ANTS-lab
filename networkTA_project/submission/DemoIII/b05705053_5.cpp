#include "user.h"
using namespace std;


bool 
UserList::registered(string username)
{
    bool registered = false;
    pthread_mutex_lock(&this->mutex);
    for (vector<User>::iterator it = this->user.begin(); it != this->user.end(); it++)
    {
        if (it->getName() == username)
        {
            registered = true;
            break;
        }
    }
    pthread_mutex_unlock(&this->mutex);
    return registered;
}
bool
UserList::modify_money(string username, int deltaMoney)
{
    bool find = false;
    pthread_mutex_lock(&this->mutex);
    for (vector<User>::iterator it = this->user.begin(); it != this->user.end(); it++)
    {
        if (it->getName() == username)
        {
            find = true;
            it->setBalance(it->getBalance() + deltaMoney);
            break;
        }
    }
    pthread_mutex_unlock(&this->mutex);
    return find;
}
bool
UserList::login(string username, string password)
{
    bool login = false;
    pthread_mutex_lock(&this->mutex);
    for (vector<User>::iterator it = this->user.begin(); it != this->user.end(); it++)
    {
        if (it->getName() == username && it->getPassword() == password)
        {
            login = true;
            break;
        }
    }
    pthread_mutex_unlock(&this->mutex);
    return login;
}
int
UserList::get_balance(string username)
{
    int balance = 10000;
    pthread_mutex_lock(&this->mutex);
    for (vector<User>::iterator it = this->user.begin(); it != this->user.end(); it++)
    {
        if (it->getName() == username)
        {
            balance = it->getBalance();
            break;
        }
    }
    pthread_mutex_unlock(&this->mutex);
    return balance;
}
void
UserList::update_user(string username, string password, int balance)
{
    pthread_mutex_lock(&this->mutex);
    User *u = new User(username, password, balance);
    this->user.push_back(*u);
    pthread_mutex_unlock(&this->mutex);
}
bool
UserOnlineList::is_online(string username)
{
    bool is_online = false;
    pthread_mutex_lock(&this->mutex);
    for (vector<UserOnline>::iterator it = this->user_online.begin(); it != this->user_online.end(); it++)
    {
        if (it->getName() == username)
        {
            is_online = true;
            break;
        }
    }
    pthread_mutex_unlock(&this->mutex);
    return is_online;
}
string 
UserOnlineList::list()
{
    string msg;
    pthread_mutex_lock(&this->mutex);
    msg += "There is " + to_string(user_online.size()) + " users online\n";
    for (int i = 0; i < this->user_online.size(); i++)
    {
        UserOnline *u = &this->user_online[i];
        msg += u->getName();
        msg += "#";
        msg += u->getIp();
        msg += "#";
        msg += to_string(u->getPort());
        msg += "\n";
    }
    pthread_mutex_unlock(&this->mutex);
    return msg;
}
string
UserOnlineList::get_user_info(string username)
{
    string msg = "";
    pthread_mutex_lock(&this->mutex);
    for (vector<UserOnline>::iterator it = user_online.begin(); it < user_online.end(); it++)
    {
        if (it->getName() == username)
        {
            msg += it->getName() + "#" + it->getIp() + "#" + to_string(it->getPort());
            break;
        }
    }
    pthread_mutex_unlock(&this->mutex);
    return msg;
}

void
UserOnlineList::update_online_user(string username, string ip, int port)
{
    pthread_mutex_lock(&this->mutex);
    UserOnline* u = new UserOnline(username, ip, port);
    this->user_online.push_back(*u);
    pthread_mutex_unlock(&this->mutex);
}
void
UserOnlineList::update_offline_user(string username)
{
    pthread_mutex_lock(&this->mutex);
    for (vector<UserOnline>::iterator it = user_online.begin(); it < user_online.end(); it++)
    {
        if (it->getName() == username)
        {
            user_online.erase(it);
        }
    }
    pthread_mutex_unlock(&this->mutex);
}
