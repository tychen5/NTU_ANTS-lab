#include <iostream>
#include <memory>

#include "./include/model.h"
#include "./include/protocol.h"
#include "./include/serializer.h"
#include "./include/server.h"
#include "./include/threadpool.h"

using namespace std;
using namespace JPay;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cout << "Wrong Given Parameter" << endl;
    cout << "help: ./JPay_server <server port>" << endl;
    exit(1);
  }

  Server server(15);
  server.listen(atoi(argv[1]));

  cout << "Exit" << endl;

  return 0;
}