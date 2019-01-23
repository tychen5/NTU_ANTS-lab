#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
using namespace std;

#define BLEN 1024

int main(int argc, char *argv[]) {
  int sd = 0;
  sd = socket(AF_INET , SOCK_STREAM , 0);

  if (sd == -1) {
    cerr << "Fail to create a socket." << endl;
    return 0;
  }
  
  struct sockaddr_in server;
  bzero(&server,sizeof(server));
  server.sin_family = PF_INET;
  server.sin_addr.s_addr = inet_addr(argv[1]);
  server.sin_port = htons(atoi(argv[2]));
  
  int err = connect(sd, (struct sockaddr *)&server, sizeof(server)); 
  if (err == -1) {
    cerr << "Connection error" << endl;
    return 0;
  }


  char rrr[BLEN];
  recv(sd, rrr, sizeof(rrr), 0);
  
  while (true) {  
    char input[10];
    char *end;
    cout << "Hi, please enter 1 for Register, 2 for Login: ";
    cin >> input; 
    
    int mode = strtol(input, &end, 10);
    if (end == input) {
      cout << "(" << input << ") is not integer, please enter again!" << endl << endl;
    }
    else {
      if (mode == 1) {
        char name[20];
        char b[20];
        cout << "\n===== SIGN UP =====" << endl;
        cout << "Please enter your name: ";
        cin >> name;
        cout << "Please enter your balance: ";
        cin >> b;

        char buf_send[BLEN];
        strcpy(buf_send, "REGISTER#");
        strcat(buf_send, name);
        strcat(buf_send, "#");
        strcat(buf_send, b);
        strcat(buf_send, "\n");
        //cout << "send message: " << buf_send << endl;
        send(sd, buf_send, strlen(buf_send), 0);

        char ret[BLEN] = "";
        recv(sd, ret, sizeof(ret), 0);
        //cout << "return: " << ret << endl;
        if (strcmp(ret, "100 OK\n") == 0) {
          cout << "Successful!" << endl << endl;
        }
        else {
          cout << "Fail!" << endl << endl;
        }
      }
      else if (mode == 2) {
        char name[20] = "";
        char port[6] = "";
        cout << "\n===== SIGN IN =====" << endl;
        
        cout << "Please enter your user name: ";
        cin >> name;
        cout << "Hi " << name << ", please enter the port you want to login: ";
        cin >> port;
        int p = strtol(port, &end, 10);
        while (port == end) {
          cout << "(" << port << ") is a illegal port number, please enter again: ";
          cin >> port;
          p = strtol(port, &end, 10);
        }
        if (p < 1024 || p > 65535) {
          cout << "The port (" << p << ") is out of range!" << endl << endl;
          continue;
        }

        char buf_send[BLEN] = "";
        strcpy(buf_send, name);
        strcat(buf_send, "#");
        strcat(buf_send, port);
        strcat(buf_send, "\n");
        //cout << "send message: " << buf_send << endl;
        send(sd, buf_send, strlen(buf_send), 0);

        char ret1[BLEN] = "";
        recv(sd, ret1, sizeof(ret1), 0); 
        if (strcmp(ret1, "220 AUTH_FAIL\n") == 0) {
          cout << "Wrong login information, please enter again!" << endl << endl; 
        }
        else {
          string ret2(ret1);
          int newLine = ret2.find('\n');
          cout << "\n===== Welcome back!!! ===== \n";
          cout << "Here is your account balance: " << ret2.substr(0, newLine) << endl;
          cout << "=====Online List:=====\n" << ret2.substr(newLine + 1) << endl;
          break;
        }
      }
      else {
        cout << "Number (" << mode << ") is a illegal, please enter again!" << endl << endl;
      }
    }
  }

  char action[10] = "";
  while (true) {
    cout << "Please enter the action you want to take\n"
          << "1 for LIST and 8 for EXIT: ";
    cin >> action;
    char *end;
    int act = strtol(action, &end, 10);
    if (action == end) {
      cout << "(" << action << ") is a illegal input! Please enter again!" << endl << endl;
    }
    else if (act == 1) {
      char buf_send[] = "List\n";
      send(sd, buf_send, strlen(buf_send), 0);

      char ret1[BLEN] = "";
      recv(sd, ret1, sizeof(ret1), 0); 
      string ret2(ret1);
      int newLine = ret2.find('\n');
      cout << "\n===== Welcome back!!! ===== \n";
      cout << "Here is your account balance: " << ret2.substr(0, newLine) << endl;
      cout << "=====Online List:=====\n" << ret2.substr(newLine + 1) << endl;
    }
    else if (act == 8) {
      char  buf_send[] = "Exit\n";
      send(sd, buf_send, strlen(buf_send), 0);
      char ret[BLEN] = "";
      recv(sd, ret, sizeof(ret), 0);
      cout << ret << endl;
      break;
    }
    else {
      cout << "(" << action << ") is a illegal input! Please enter again!" << endl << endl;
    }
  }

  close(sd);
  return 0;
}
