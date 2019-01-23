#ifndef SERVER_AUTH_HPP
#define SERVER_AUTH_HPP

namespace JPay {
class UserList {
 public:
  UserList() {}

  std::shared_ptr<User> login(std::string name, std::string addr,
                              unsigned short port) {
    for (const std::shared_ptr<User> &user : _userList) {
      if (user->name().compare(name) == 0 && user->ip().empty()) {
        user->ip(addr);
        user->port(port);
        return user;
      }
    }
    return NULL;
  }

  bool regist(std::string name, int money) {
    for (const std::shared_ptr<User> &user : _userList) {
      if (user->name().compare(name) == 0) {
        return false;
      }
    }
    std::shared_ptr<User> newUser = std::make_shared<User>(name);
    newUser->money(money);
    _userList.push_back(std::move(newUser));
    return true;
  }

  void transfer(std::string clientA, std::string clientB, int money) {
    for (const std::shared_ptr<User> &user : _userList) {
      if (user->name().compare(clientA) == 0) {
        user->money(user->money() - money);
      }
      if (user->name().compare(clientB) == 0) {
        user->money(user->money() + money);
      }
    }
  }

  bool logout(std::string name) {
    for (const std::shared_ptr<User> &user : _userList) {
      if (user->name().compare(name) == 0) {
        user->ip(std::string());
        user->port(0);
        return true;
      }
    }
    return false;
  }

  std::shared_ptr<User> getUser(std::string name) {
    for (const std::shared_ptr<User> &user : _userList) {
      if (user->name().compare(name) == 0) {
        return user;
      }
    }
    return NULL;
  }

  std::string status() {
    std::stringstream status_s;

    int count = 0;
    for (const std::shared_ptr<User> &user : _userList) {
      if (!user->ip().empty() && user->port() != 0) {
        status_s << user->name() << "#" << user->ip() << "#" << user->port()
                 << '\n';
        ++count;
      }
    }

    std::string status = status_s.str();
    status.insert(0, std::to_string(count) + "\n");

    return status;
  }

 private:
  std::list<std::shared_ptr<User>> _userList;
};
}  // namespace JPay

#endif