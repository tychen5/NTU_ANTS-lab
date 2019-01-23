#include "../include/client_trans_server.h"
#include "../include/client.h"
#include "../include/pki.h"
#include "../include/protocol.h"

using namespace JPay;

void ClientTransServer::printPrompt(std::string content, bool newline) {
  std::cout << "[JPay Transfer] " << content;
  if (newline) std::cout << std::endl;
}

void ClientTransServer::handleClient(int client, SSL *ssl, std::string ip) {
  std::cout << std::endl;
  printPrompt("New Transfer Request from " + ip);
  std::cout << "[JPay] >> ";
  PKI pki = PKI();
  packed_send(ssl, pki.sharePUBKEY(), ip);
  std::string recv_data;
  if (!packed_recv(ssl, recv_data, ip))
    throw std::runtime_error("Error while recv message");
  RSA *rsa = PKI::loadPUBKEY((char *)recv_data.c_str());
  char *recvCipher = packed_recv(ssl);
  if (recvCipher == 0) throw std::runtime_error("Error while recv message");

  char *plainMsg = pki.decrypt(recvCipher, rsa);

  Client transClient(this->hostip, this->port);

  transClient.packed_send("Transfer\n", 9);
  Request pkirequest = Request(false, pki.sharePUBKEY());
  unique_ptr<Response> pkiresponse = transClient.send_request(pkirequest);
  RSA *centralRsa = PKI::loadPUBKEY((char *)pkiresponse->message().c_str());

  char *chiperMsg = pki.encrypt(plainMsg, centralRsa);
  transClient.packed_send(chiperMsg, RSA_size(centralRsa));
  transClient.packed_recv();

  packed_send(ssl, "OK", ip);
}