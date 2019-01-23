#include <cstring>
#include <iostream>
#include <memory>

#include "./include/client.h"
#include "./include/model.h"

#include "./include/client_ui.h"

using namespace std;
using namespace JPay;

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printPrompt("Wrong Given Parameter");
    printPrompt("help: ./JPay_client <server ip> <server port>");
    exit(1);
  }

  Client client(argv[1], atoi(argv[2]));
  User user = User("", 0);

  printPrompt("Welcom To JPay");
  printHelpMsg();

  bool exit = false;

  while (!exit) {
    printPrompt("What do you want to do now? " + user.name() +
                " (Enter 6 for help)");
    string input;
    printPrompt(">> ", false);
    cin >> input;
    if (strcmp(input.c_str(), "1") == 0) {
      login(client, user);
    } else if (strcmp(input.c_str(), "2") == 0) {
      regist(client, user);
    } else if (strcmp(input.c_str(), "3") == 0) {
      list(client);
    } else if (strcmp(input.c_str(), "4") == 0) {
      logout(client, user);
    } else if (strcmp(input.c_str(), "5") == 0) {
      exit = true;
    } else if (strcmp(input.c_str(), "6") == 0) {
      printHelpMsg();
    } else {
    }
  }

  // Cmd cmd = Cmd(client, user);
  // cmd.registCommand(unique_ptr<LoginCommand>(new LoginCommand('1')));
  // cmd.registCommand(unique_ptr<RegistCommand>(new RegistCommand('2')));
  // cmd.registCommand(unique_ptr<ListCommand>(new ListCommand('3')));
  // cmd.registCommand(unique_ptr<ExitCommand>(new ExitCommand('4')));

  // cmd.run();
}