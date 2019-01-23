#ifndef SERIALIZER_HPP
#define SERIALIZER_HPP

#include <string>

#include "./model.h"
#include "./protocol.h"
#include "./util.h"

namespace JPay {

static const char CRLF = '\n';

template <typename T>
class Serializer {
 public:
  virtual string encode(const T&) const = 0;
  virtual T decode(const string&) const = 0;
};

class UserSerializer : public Serializer<User> {
 public:
  // TODO
  string encode(const User& user) const override {}

  User decode(const string& raw) const override {
    vector<string> tokens = split(raw, "#");

    if (tokens.size() != 3) {
      // TODO error
    }

    string name = tokens.at(0);
    string ip = tokens.at(1);
    unsigned short port = stoi(tokens.at(2));

    return User(name, ip, port);
  }
};

class RegisterSerializer : public Serializer<User> {
 public:
  string encode(const User& user) const override {
    return "REGISTER#" + user.name() + "#" + to_string(user.money()) + CRLF;
  }

  // TODO
  User decode(const string& raw) const override { return User(); }
};

class ResponseStatusSerializer : public Serializer<ResponseStatus> {
 public:
  string encode(const ResponseStatus& status) const override {
    switch (status) {
      case OK:
        return "100 OK" + CRLF;
        break;
      case FAIL:
        return "210 FAIL" + CRLF;
        break;
      case AUTH_FAIL:
        return "220 FAIL" + CRLF;
        break;
    }
  }

  ResponseStatus decode(const string& raw) const override {
    if (raw.find("100 OK") != string::npos) {
      return ResponseStatus(OK);
    } else if (raw.find("210 FAIL") != string::npos) {
      return ResponseStatus(FAIL);
    } else if (raw.find("220 AUTH_FAIL") != string::npos) {
      return ResponseStatus(AUTH_FAIL);
    }
    return ResponseStatus(OK);
  }
};

class LoginSerializer : public Serializer<User> {
 public:
  string encode(const User& user) const override {
    return user.name() + "#" + to_string(user.port()) + CRLF;
  }

  User decode(const string& raw) const override {
    User user;
    size_t p = raw.find_first_of('#');
    user.name(raw.substr(0, p));
    user.port(stoi(raw.substr(p)));
    return user;
  }
};

class OnlineStatusSerializer : public Serializer<OnlineStatus> {
 public:
  // TODO
  string encode(const OnlineStatus& status) const override {}

  OnlineStatus decode(const string& raw) const override {
    UserSerializer serializer;

    vector<string> tokens = split(raw, "\n");
    if (tokens.size() < 3) {
      // TODO
    };

    int money = stoi(tokens.at(0));
    int num_online = stoi(tokens.at(1));
    vector<User> list;

    vector<string>::iterator it;
    for (it = tokens.begin() + 2; it != tokens.end(); ++it) {
      if ((*it).find('#') == string::npos) continue;
      list.push_back(serializer.decode(*it));
    }

    if (num_online != list.size()) {
      // TODO
    }
    return OnlineStatus(money, list);
  }
};

}  // namespace JPay

#endif