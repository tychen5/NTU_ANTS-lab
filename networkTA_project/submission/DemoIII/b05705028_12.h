#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <openssl/ssl.h>
#include <memory>
#include <string>

#include "./pki.h"
#include "./protocol.h"

using namespace std;

namespace JPay {

class Client {
 public:
  Client(string hostip, unsigned short port);
  ~Client();

  unique_ptr<Response> send_request(Request &request);

  const bool packed_send(const char *content, size_t content_size) const {
    int result = SSL_write(ssl, content, content_size);

    if (result <= 0) {
      int error = SSL_get_error(ssl, result);
      const char *error_reason = ERR_reason_error_string(error);
      std::cerr << error_reason << std::endl;
      return false;
    }
    return true;
  }

  const char *packed_recv() const {
    char *buffer = new char[1024];
    int result = SSL_read(ssl, buffer, sizeof(buffer));

    if (result <= 0) {
      int error = SSL_get_error(ssl, result);
      const char *error_reason = ERR_reason_error_string(error);
      std::cerr << error_reason << std::endl;
      return NULL;
    }

    return buffer;
  }

 private:
  int sockfd;
  SSL_CTX *ctx;
  SSL *ssl;
  PKI pki;
};

}  // namespace JPay

#endif