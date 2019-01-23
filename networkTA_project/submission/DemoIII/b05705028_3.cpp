#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#include "../include/base_server.h"
#include "../include/model.h"
#include "../include/ssl.h"

using namespace JPay;

bool Server::isPortAvailable(unsigned short port) {
  int tmpsockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (tmpsockfd == -1) {
    char *errmsg = strerror(errno);
    std::cerr << errmsg << std::endl;
    close(tmpsockfd);
    return false;
  }

  struct sockaddr_in info;

  info.sin_family = AF_INET;
  info.sin_addr.s_addr = INADDR_ANY;
  info.sin_port = htons(port);

  if (bind(tmpsockfd, (struct sockaddr *)&info, sizeof(info)) < 0) {
    char *errmsg = strerror(errno);
    std::cerr << errmsg << std::endl;
    close(tmpsockfd);
    return false;
  }

  close(tmpsockfd);
  return true;
}

void Server::listen(unsigned short port) {
  sockfd = 0;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd == -1) {
    char *errmsg = strerror(errno);
    throw std::runtime_error(errmsg);
  }

  struct sockaddr_in info;

  info.sin_family = AF_INET;
  info.sin_addr.s_addr = INADDR_ANY;
  info.sin_port = htons(port);

  if (bind(sockfd, (struct sockaddr *)&info, sizeof(info)) < 0) {
    char *errmsg = strerror(errno);
    throw std::runtime_error(errmsg);
  }
  if (::listen(sockfd, 10) < 0) {
    char *errmsg = strerror(errno);
    throw std::runtime_error(errmsg);
  }

  while (!shouldStop) {
    try {
      int client;
      struct sockaddr_in clientaddress;
      int address_len = sizeof(clientaddress);
      client = accept(sockfd, (struct sockaddr *)&clientaddress,
                      (socklen_t *)&address_len);
      if (client < 0) {
        char *errmsg = strerror(errno);
        throw std::runtime_error(errmsg);
      }

      SSL *ssl;
      ssl = SSL_new(ctx);
      SSL_set_fd(ssl, client);

      if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
      }

      char client_ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(clientaddress.sin_addr), client_ip, INET_ADDRSTRLEN);
      if (client_ip == NULL) {
        char *errmsg = strerror(errno);
        throw std::runtime_error(errmsg);
      }

      _pool->enqueue(
          std::bind(&Server::handleClient, this, client, ssl, client_ip));

    } catch (const std::exception &ex) {
      std::cerr << ex.what() << std::endl;
    } catch (const std::string &ex) {
      std::cerr << ex << std::endl;
    } catch (...) {
      std::cerr << "Unknown Error" << std::endl;
    }
  }
}

const std::string Server::currentDateTime() const {
  time_t now = time(0);
  struct tm tstruct;
  char buf[80];
  tstruct = *localtime(&now);
  // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
  // for more information about date/time format
  strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

  return buf;
}

const bool Server::packed_send(SSL *ssl, const char *content,
                               std::string client_ip) const {
  size_t content_size = strlen(content);

  int result = SSL_write(ssl, content, content_size);
  if (result <= 0) {
    int error = SSL_get_error(ssl, result);
    const char *error_reason = ERR_reason_error_string(error);
    std::cerr << "[" << currentDateTime() << "] "
              << "(" << client_ip << ") " << error_reason << std::endl;
    return false;
  }
  return true;
}

const bool Server::packed_recv(SSL *ssl, std::string &recv_data,
                               std::string client_ip) const {
  char buffer[1024] = "\0";
  int result = SSL_read(ssl, buffer, sizeof(buffer));

  if (result <= 0) {
    int error = SSL_get_error(ssl, result);
    const char *error_reason = ERR_reason_error_string(error);
    std::cerr << "[" << currentDateTime() << "] "
              << "(" << client_ip << ") " << error_reason << std::endl;
    return false;
  }

  recv_data = std::string(buffer);

  return true;
}

char *Server::packed_recv(SSL *ssl) const {
  char buffer[1024] = "\0";
  int recv = SSL_read(ssl, buffer, sizeof(buffer));

  if (recv <= 0) {
    int error = SSL_get_error(ssl, recv);
    const char *error_reason = ERR_reason_error_string(error);
    std::cerr << error_reason << std::endl;
    return 0;
  }

  char *recvMsg = new char[recv];
  memcpy(recvMsg, buffer, recv);

  return recvMsg;
}
