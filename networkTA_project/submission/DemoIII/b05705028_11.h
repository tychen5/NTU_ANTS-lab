#ifndef CLIENT_UI_HPP
#define CLIENT_UI_HPP

#include "./client.h"
#include "./client_trans_server.h"
#include "./model.h"
#include "./protocol.h"
#include "./serializer.h"

#include <sstream>
#include <thread>

namespace JPay {

thread* t1;

void printPrompt(string content = "", bool newline = true) {
  cout << "[JPay] " << content;
  if (newline) cout << endl;
}

void printHelpMsg() {
  cout << ">> Enter 1 to Login" << endl;
  cout << ">> Enter 2 to Regist New Account" << endl;
  cout << ">> Enter 3 to List Online Status" << endl;
  cout << ">> Enter 4 to Transfer" << endl;
  cout << ">> Enter 5 to Logout" << endl;
  cout << ">> Enter 6 to Exit Program" << endl;
  cout << endl;
}

void login(Client& client, User& user, ClientTransServer* transServer) {
  string username;
  int port;
  printPrompt("Enter Username >> ", false);
  cin >> username;
  printPrompt("Enter Port >> ", false);
  cin >> port;

  if (port < 1024 || port > 66530 || !Server::isPortAvailable(port)) {
    printPrompt("The input port is invalid. Try Again...");
    return login(client, user, transServer);
  }

  t1 = new thread(&ClientTransServer::listen, transServer, port);

  Request request =
      Request(false, LoginSerializer().encode(User(username, port)));
  unique_ptr<Response> response = client.send_request(request);
  ResponseStatus status = ResponseStatusSerializer().decode(response->rstrip());
  if (status == ResponseStatus::OK) {
    user.name(username);
    user.port(port);

    printPrompt("Login Successfully");
    OnlineStatus online = OnlineStatusSerializer().decode(response->rstrip());
    size_t num_online = online.num_online();
    cout << "Your account balance is " << online.money() << endl;
    cout << "There are " << num_online << " users online:" << endl;
    for (size_t i = 0; i < num_online; ++i) {
      cout << "\t" << online[i].name() << " " << online[i].ip() << endl;
    }
  } else {
    printPrompt("Login fail. May be you are not yet regist");
  }
}

void regist(Client& client, User& user) {
  if (strcmp(user.name().c_str(), "") != 0) {
    printPrompt("You have already login");
    return;
  }
  string username;
  int money;
  printPrompt("Enter Username >> ", false);
  cin >> username;
  printPrompt("Enter Init Money >> ", false);
  cin >> money;

  user.name(username);
  user.money(money);

  Request request = Request(false, RegisterSerializer().encode(user));
  unique_ptr<Response> response = client.send_request(request);
  ResponseStatus status = ResponseStatusSerializer().decode(response->rstrip());

  if (status == ResponseStatus::OK) {
    printPrompt("Regist successfully!!");
  } else if (status == ResponseStatus::FAIL) {
    printPrompt("Regist fail. Maybe this username is already used");
    return regist(client, user);
  }
}

void listStatus(Client& client) {
  Request request = Request(false, "List\n");
  unique_ptr<Response> response = client.send_request(request);
  ResponseStatus status = ResponseStatusSerializer().decode(response->rstrip());
  if (status == ResponseStatus::OK) {
    OnlineStatus online = OnlineStatusSerializer().decode(response->rstrip());
    size_t num_online = online.num_online();
    cout << "Your account balance is " << online.money() << endl;
    cout << "There are " << num_online << " users online:" << endl;
    for (size_t i = 0; i < num_online; ++i) {
      cout << "\t" << online[i].name() << " " << online[i].ip() << endl;
    }
  } else {
    printPrompt("You are not yet login");
  }
}

void transfer(Client& client, User& user) {
  Request request = Request(false, "List\n");
  unique_ptr<Response> response = client.send_request(request);
  ResponseStatus status = ResponseStatusSerializer().decode(response->rstrip());
  if (status == ResponseStatus::OK) {
    OnlineStatus online = OnlineStatusSerializer().decode(response->rstrip());
    size_t num_online = online.num_online();
    if (num_online - 1 == 0) {
      cout << "There are no other user online now. Please wait..." << endl;
    } else {
      vector<User> userList;
      cout << "Your account balance is " << online.money() << endl;
      cout << "There are " << num_online - 1 << " users online:" << endl;
      int userIdx = 1;
      for (size_t i = 0; i < num_online; ++i) {
        if (online[i].name().compare(user.name()) != 0) {
          cout << " (" << userIdx << ")"
               << "\t" << online[i].name() << " " << online[i].ip() << endl;
          ++userIdx;
          userList.push_back(online[i]);
        }
      }
      printPrompt("Chose which one you want to transfer >> ", false);
      cin >> userIdx;
      --userIdx;
      int transMoney;
      printPrompt("Enter how much money to transfer >> ", false);
      cin >> transMoney;
      printPrompt("Progressing... ");
      Client transClient(userList[userIdx].ip(), userList[userIdx].port());
      PKI pki = PKI();
      Request pkirequest = Request(false, pki.sharePUBKEY());
      unique_ptr<Response> pkiresponse = transClient.send_request(pkirequest);

      stringstream transMsg;
      transMsg << user.name() << "#" << transMoney << "#"
               << userList[userIdx].name();
      RSA* rsa = PKI::loadPUBKEY((char*)pkiresponse->message().c_str());
      char* chiperMsg = pki.encrypt(transMsg.str().c_str(), rsa);

      transClient.packed_send(chiperMsg, RSA_size(rsa));
      transClient.packed_recv();
      printPrompt("Transfer finished... ");
    }

  } else {
    printPrompt("You are not yet login");
  }
}

void logout(Client& client, User& user) {
  Request request = Request(false, "Exit\n");
  unique_ptr<Response> response = client.send_request(request);
  ResponseStatus status = ResponseStatusSerializer().decode(response->rstrip());
  if (status == ResponseStatus::AUTH_FAIL) {
    printPrompt("You are not yet login");
    return;
  }
  printPrompt("Loging out.. Bye");
  user.name("");
  user.port(0);

  exit(0);
}

}  // namespace JPay

#endif