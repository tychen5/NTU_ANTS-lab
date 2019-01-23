#ifndef MODEL_HPP
#define MODEL_HPP

#include <string>
#include <vector>

namespace JPay {

class User {
 public:
  User() {}
  User(std::string name)
      : _name(name), _port(0), _ip(std::string()), _money(0) {}
  User(std::string name, unsigned short port)
      : _name(name), _port(port), _ip(std::string()), _money(0) {}
  User(std::string name, std::string ip, unsigned short port)
      : _name(name), _port(port), _ip(ip), _money(0) {}

  const std::string name() const { return _name; }
  const std::string ip() const { return _ip; }
  const unsigned short port() const { return _port; }
  const int money() const { return _money; }

  void name(std::string const& name) { _name = name; }
  void port(unsigned short const& port) { _port = port; }
  void ip(std::string const& ip) { _ip = ip; };
  void money(int const money) { _money = money; }

 private:
  std::string _name;
  std::string _ip;
  unsigned short _port;
  int _money;
};

class OnlineStatus {
 public:
  OnlineStatus(int money, std::vector<User> list)
      : _money(money), _list(list) {}

  const int money() const { return _money; }
  const std::vector<User> list() const { return _list; }
  const size_t num_online() const { return _list.size(); }

  const User& operator[](size_t idx) const { return _list.at(idx); }

 private:
  int _money;
  std::vector<User> _list;
};

}  // namespace JPay

#endif