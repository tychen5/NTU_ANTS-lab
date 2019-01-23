#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <string>

using namespace std;

namespace JPay {

class Protocol {
 public:
  Protocol() : _success(false), _message("") {}
  Protocol(bool success, string message)
      : _success(success), _message(message) {}

  const bool success() const { return _success; }
  const string message() const { return _message; }
  const string rstrip() const {
    return _message.substr(0, _message.length() - 1);
  }

  void success(const bool success) { _success = success; }
  void message(const string message) { _message = message; }

 protected:
  // whether send success
  bool _success;
  string _message;
};

class Request : public Protocol {
 public:
  Request() : Protocol() {}
  Request(bool success, string message) : Protocol(success, message) {}
};

class Response : public Protocol {
 public:
  Response() : Protocol() {}
  Response(bool success, string message) : Protocol(success, message) {}
};

enum ResponseStatus { OK = 100, FAIL = 210, AUTH_FAIL = 220 };

}  // namespace JPay

#endif