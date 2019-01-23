#ifndef BASE_SERVER_HPP
#define BASE_SERVER_HPP

#include <openssl/ssl.h>
#include <unistd.h>
#include <algorithm>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

#include "./model.h"
#include "./pki.h"
#include "./server_auth.h"
#include "./ssl.h"
#include "./threadpool.h"

namespace JPay {

class Server {
 public:
  Server(size_t num_worker, const char *cert_path, const char *key_path)
      : _pki(),
        shouldStop(false),
        _pool(std::unique_ptr<ThreadPool>(new ThreadPool(num_worker))) {
    init_openssl();
    ctx = create_context();
    configure_context(ctx, cert_path, key_path);
  }

  ~Server() {
    close(sockfd);
    SSL_CTX_free(ctx);
    cleanup_openssl();
    std::cout << "Server closed" << std::endl;
  }

  static bool isPortAvailable(unsigned short port);
  void listen(unsigned short port);
  virtual void handleClient(int client, SSL *ssl, std::string ip) = 0;
  void stop() { shouldStop = true; }

 protected:
  int sockfd;
  SSL_CTX *ctx;
  PKI _pki;
  bool shouldStop;

  std::unique_ptr<ThreadPool> _pool;
  UserList _userList;
  std::mutex _userListMutex;

  const std::string currentDateTime() const;
  const bool packed_send(SSL *ssl, const char *content,
                         std::string client_ip) const;
  const bool packed_recv(SSL *ssl, std::string &recv_data,
                         std::string client_ip) const;
  char *packed_recv(SSL *ssl) const;
};

}  // namespace JPay

#endif