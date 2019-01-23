#ifndef CLIENT_TRANS_SERVER
#define CLIENT_TRANS_SERVER

#include <string>
#include "./base_server.h"

namespace JPay {

class ClientTransServer : public Server {
 public:
  using Server::Server;
  void handleClient(int client, SSL *ssl, std::string ip) override;
  void printPrompt(std::string content = "", bool newline = true);
  void setCentralServer(std::string hostip, unsigned short port) {
    this->hostip = hostip;
    this->port = port;
  }

 private:
  std::string hostip;
  unsigned short port;
};

}  // namespace JPay
#endif