#include "../include/central_server.h"
#include "../include/pki.h"

using namespace JPay;

void CentralServer::handleClient(int client, SSL *ssl, std::string ip) {
  std::cout << "[" << currentDateTime() << "] "
            << "New Connection from " << ip << std::endl;

  std::shared_ptr<User> user = NULL;
  while (true) {
    try {
      std::string recv_data;
      if (!packed_recv(ssl, recv_data, ip))
        throw std::runtime_error("Error while recv message");

      recv_data.erase(std::remove(recv_data.begin(), recv_data.end(), '\n'),
                      recv_data.end());
      // std::cout << "+++" << recv_data.size() << "+++" << std::endl;

      if (recv_data.find("REGISTER#") != std::string::npos) {
        size_t first_pound = recv_data.find('#');
        size_t second_pound = recv_data.find('#', first_pound + 1);
        if (first_pound == std::string::npos ||
            second_pound == std::string::npos) {
          std::cout << recv_data << " FOUND ERROR" << std::endl;
          char msg[] = "210 FAIL\n";
          if (!packed_send(ssl, msg, ip))
            throw std::runtime_error("Error while send message");

          continue;
        }
        std::string username =
            recv_data.substr(first_pound + 1, second_pound - first_pound - 1);
        std::string money = recv_data.substr(second_pound + 1);
        if (username.empty() || money.empty()) {
          char msg[] = "210 FAIL\n";
          if (!packed_send(ssl, msg, ip))
            throw std::runtime_error("Error while send message");

          continue;
        }
        std::unique_lock<std::mutex> lock(this->_userListMutex);
        bool success = _userList.regist(username, std::stoi(money));
        lock.unlock();
        if (success) {
          if (!packed_send(ssl, "100 OK\n", ip))
            throw std::runtime_error("Error while send message");
        } else {
          if (!packed_send(ssl, "210 FAIL\n", ip))
            throw std::runtime_error("Error while send message");
        }
      } else if (recv_data.compare("List") == 0) {
        if (user) {
          std::unique_lock<std::mutex> lock(this->_userListMutex);
          std::string status = _userList.status();
          lock.unlock();
          status.insert(0, std::to_string(user->money()) + "\n");
          if (!packed_send(ssl, status.c_str(), ip))
            throw std::runtime_error("Error while send message");
        } else {
          if (!packed_send(ssl, "220 AUTH_FAIL\n", ip))
            throw std::runtime_error("Error while send message");
        }
      } else if (recv_data.compare("Transfer") == 0) {
        std::cout << "New Transfer !!" << std::endl;
        PKI pki = PKI();
        packed_send(ssl, pki.sharePUBKEY(), ip);
        std::string recv_data;
        if (!packed_recv(ssl, recv_data, ip))
          throw std::runtime_error("Error while recv message");
        RSA *rsa = PKI::loadPUBKEY((char *)recv_data.c_str());
        char *recvCipher = packed_recv(ssl);
        if (recvCipher == 0)
          throw std::runtime_error("Error while recv message");
        char *plainMsg = pki.decrypt(recvCipher, rsa);
        std::string transfer = std::string(plainMsg);

        size_t first_pound = transfer.find('#');
        size_t second_pound = transfer.find('#', first_pound + 1);

        std::string clientA = transfer.substr(0, first_pound);
        std::string moneyS =
            transfer.substr(first_pound + 1, second_pound - first_pound - 1);

        std::string clientB = transfer.substr(second_pound + 1);

        std::cout << clientA << " transfer " << moneyS << " dollars to "
                  << clientB << std::endl;

        int money = std::stoi(moneyS);
        _userList.transfer(clientA, clientB, money);

        packed_send(ssl, "OK", ip);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client);
        return;
      } else if (recv_data.find("#") != std::string::npos) {
        size_t split = recv_data.find('#');
        std::string username = recv_data.substr(0, split);
        std::string port_s = recv_data.substr(split + 1);
        if (username.empty() || port_s.empty()) {
          if (!packed_send(ssl, "WTF YOU ARE DOING ?????\n", ip))
            throw std::runtime_error("Error while send message");
          continue;
        }

        if (std::all_of(port_s.begin(), port_s.end(), ::isdigit)) {
          int port = std::stoi(port_s);
          if (port < 1024 || port > 65535) {
            if (!packed_send(
                    ssl,
                    "WTF YOU ARE DOING WITH THIS OUT OF RANGE PORT ?????\n",
                    ip))
              throw std::runtime_error("Error while send message");
            continue;
          }
          std::unique_lock<std::mutex> lock(this->_userListMutex);
          user = _userList.login(username, ip, port);
          lock.unlock();
          if (user) {
            std::cout << "[" << currentDateTime() << "] "
                      << "User Login " << username << std::endl;
            std::unique_lock<std::mutex> lock(this->_userListMutex);
            std::string status = _userList.status();
            lock.unlock();
            status.insert(0, std::to_string(user->money()) + "\n");
            if (!packed_send(ssl, status.c_str(), ip))
              throw std::runtime_error("Error while send message");
          } else {
            if (!packed_send(ssl, "220 AUTH_FAIL\n", ip))
              throw std::runtime_error("Error while send message");
          }
        } else {
          if (!packed_send(
                  ssl, "WTF YOU ARE DOING WITH THIS OUT OF RANGE PORT ?????\n",
                  ip))
            throw std::runtime_error("Error while send message");
          continue;
        }
      } else if (recv_data.compare("Exit") == 0) {
        if (user) {
          std::string logout_username = user->name();
          std::unique_lock<std::mutex> lock(this->_userListMutex);
          bool success = _userList.logout(user->name());
          lock.unlock();
          user = NULL;

          if (success) {
            std::cout << "[" << currentDateTime() << "] "
                      << "User Logout " << logout_username << std::endl;
            if (!packed_send(ssl, "Bye\n", ip))
              throw std::runtime_error("Error while send message");
          } else {
            if (!packed_send(ssl, "220 AUTH_FAIL\n", ip))
              throw std::runtime_error("Error while send message");
          }

          SSL_shutdown(ssl);
          SSL_free(ssl);
          close(client);
          return;

        } else {
          if (!packed_send(ssl, "220 AUTH_FAIL\n", ip))
            throw std::runtime_error("Error while send message");
        }
      } else {
        if (!packed_send(ssl, "WTF YOU ARE DOING ?????\n", ip))
          throw std::runtime_error("Error while send message");
      }
    } catch (const std::exception &ex) {
      if (user) {
        std::unique_lock<std::mutex> lock(this->_userListMutex);
        std::string logout_username = user->name();
        _userList.logout(user->name());
        std::cout << "[" << currentDateTime() << "] "
                  << "Error Occured! Force User Logout " << logout_username
                  << std::endl;
        lock.unlock();
        user = NULL;
      }
      SSL_shutdown(ssl);
      SSL_free(ssl);
      close(client);
      std::cerr << "[" << currentDateTime() << "] " << ex.what() << std::endl;
      return;
    } catch (...) {
      SSL_shutdown(ssl);
      SSL_free(ssl);
      close(client);
      std::cerr << "[" << currentDateTime() << "] "
                << "Unknow Error" << std::endl;
      return;
    }
  }
}