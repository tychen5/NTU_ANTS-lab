#include <iostream>
#include <cstdio>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <cstdlib>
#include <vector>

#define BLUE    "\x1b[34m"
#define RESET   "\x1b[0m"
#define MAX 256

using namespace std;
int main(int argc, char *argv[])
{
    char command[MAX];
    char money[MAX];
    char portNum[MAX];
    char regisCommand[MAX] = "REGISTER#";
    char logCommand[MAX] = "#";
    int option = 0;
    int status = 0;
    int cnt = 0;

    if(argc < 2)
    {
        cout << "Not enough information.";
        return -1;
    }

    // build socket
    int socketback = 0;
    socketback = socket(AF_INET, SOCK_STREAM, 0);
    if(socketback < 0)
    {
        cout << "Fail to create socket.";
        return -1;
    }

    // socket connect
    struct sockaddr_in connection;
    bzero(&connection, sizeof(connection));
    connection.sin_family = AF_INET;
    // connection.sin_port = htons(8080);
    // inet_aton("127.0.0.1", &connection.sin_addr);
    connection.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &connection.sin_addr);

    int connectback = connect(socketback, (struct sockaddr *)&connection, sizeof(connection));
    if(connectback < 0)
    {
        cout << "Fail to connect.";
        return -1;
    }

    // receive register
    char buff[MAX];
    int reciback = recv(socketback, buff, sizeof(buff), 0);
    if(reciback < 0)
    {
        cout << "Fail to receive.";
        return -1;
    }
    cout << buff << endl;
    
    // input variable command
    while(status == 0)
    {
        cout << BLUE << "1 for register, 2 for login, 3 for exit: " << RESET;
        cin >> option;
        
        if(option == 1)
        {
            cnt += 1;
            if(cnt > 1)
            {
                cout << BLUE << "You have registered before." << RESET << endl;
                continue;
            }
            cout << BLUE << "Please enter your username for register: " << RESET;

            // register
            cin >> command;
            strcat(regisCommand, command);

            cout << BLUE << "Please enter amount: " << RESET;
            cin >> money;
            strcat(regisCommand, "#");
            strcat(regisCommand, money);
            send(socketback, regisCommand, strlen(regisCommand), 0);

            // receive register message
            bzero(buff, MAX);
            recv(socketback, buff, sizeof(buff), 0);
            cout << buff << endl;
        }
        else if(option == 2)
        {
            // log in
            cout << BLUE << "Please enter your username for login: " << RESET;
            bzero(command, MAX);
            cin >> command;
            cout << BLUE << "Please enter your port number: " << RESET;
            cin >> portNum;
            while(stoi(portNum) < 1024 || stoi(portNum) > 65535)
            {
                cout << BLUE << "The port number is out of range. Please reenter the port number: " << RESET;
                cin >> portNum;
            }
            strcat(command, logCommand);
            strcat(command, portNum);

            // send log message
            send(socketback, command, strlen(command), 0);

            // receive log message
            bzero(buff, MAX);
            recv(socketback, buff, sizeof(buff), 0);
            cout << buff << endl;

            string authen(buff);
            if(authen.find("220 AUTH_FAIL") != string::npos)
            {
                cout << BLUE << "Username or password error." << RESET << endl;;
                status = 0;
            }
            else
            {
                status = 2;
            }
        }
        else if(option == 3)
        {
            return -1;
        }
    }

    while(status == 2)
    {
        cout << BLUE << "7 for latest list, 8 for exit: " << RESET;
        cin >> option;
        if(option == 7)
        {
            bzero(command, MAX);
            strcpy(command, "List");

            // send
            send(socketback, command, strlen(command), 0);

            // receive
            bzero(buff, MAX);
            recv(socketback, buff, sizeof(buff), 0);
            cout << buff << endl;
        }
        else if(option == 8)
        {
            bzero(command, MAX);
            strcpy(command, "Exit");

            // send
            send(socketback, command, strlen(command), 0);

            // receive
            bzero(buff, MAX);
            recv(socketback, buff, sizeof(buff), 0);
            cout << buff << endl;

            return -1;
        }

        // close
        // close(socketback);
    }

    return 0;
}
