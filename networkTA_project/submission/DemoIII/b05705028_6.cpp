#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "../include/client.h"
#include "../include/protocol.h"
#include "../include/ssl.h"

using namespace std;
using namespace JPay;

Client::Client(string hostip, unsigned short port) {
  sockfd = 0;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  init_openssl();

  if (sockfd == -1) {
    throw runtime_error("Fail to create a socket");
  }

  struct sockaddr_in info;

  info.sin_family = AF_INET;
  info.sin_addr.s_addr = inet_addr(hostip.c_str());
  info.sin_port = htons(port);

  if (connect(sockfd, (struct sockaddr*)&info, sizeof(info)) == -1) {
    throw runtime_error("Connection error");
  }

  const SSL_METHOD* meth = SSLv23_client_method();
  ctx = SSL_CTX_new(meth);
  ssl = SSL_new(ctx);
  SSL_set_fd(ssl, sockfd);

  if (SSL_connect(ssl) <= 0) {
    ERR_print_errors_fp(stderr);
  }

  pki = PKI();
}

Client::~Client() { close(sockfd); }

unique_ptr<Response> Client::send_request(Request& request) {
  string content = request.message();
  int result = SSL_write(ssl, content.c_str(), content.length());

  if (result <= 0) {
    int error = SSL_get_error(ssl, result);
    const char* error_reason = ERR_reason_error_string(error);
    request.success(false);
    request.message(string(error_reason));
    return NULL;
  }

  char buffer[1024];
  unique_ptr<Response> response(new Response(true, ""));
  result = SSL_read(ssl, buffer, sizeof(buffer));
  if (result <= 0) {
    int error = SSL_get_error(ssl, result);
    const char* error_reason = ERR_reason_error_string(error);
    response->success(false);
    response->message(string(error_reason));
    return move(response);
  }

  response->message(string(buffer, buffer + result));

  return move(response);
}
