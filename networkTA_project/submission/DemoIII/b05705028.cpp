#include <signal.h>
#include <iostream>
#include <memory>

#include "./include/central_server.h"
#include "./include/model.h"
#include "./include/protocol.h"
#include "./include/serializer.h"
#include "./include/threadpool.h"

using namespace std;
using namespace JPay;

CentralServer *server;

void my_handler(int s) {
  delete server;
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cout << "Wrong Given Parameter" << endl;
    cout << "help: ./JPay_server <server port>" << endl;
    exit(1);
  }

  server = new CentralServer(15, "cert.pem", "key.pem");

  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = my_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);

  std::cout << "Server start" << std::endl;
  std::cout << "Wait for connection..." << std::endl;
  server->listen(atoi(argv[1]));

  return 0;
}