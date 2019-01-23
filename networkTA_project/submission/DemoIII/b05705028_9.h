#ifndef CENTRAL_SERVER_HPP
#define CENTRAL_SERVER_HPP

#include "./base_server.h"

namespace JPay {

class CentralServer : public Server {
 public:
  using Server::Server;
  void handleClient(int client, SSL *ssl, std::string ip) override;
};

}  // namespace JPay

#endif