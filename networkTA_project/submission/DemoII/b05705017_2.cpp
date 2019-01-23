#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <arpa/inet.h> 
#include <string.h>
using namespace std;

#define buffer_size 1024

int main(int argc, char const* argv[]){
    if(argc != 3){
        cout << "\n error argument\n";
        return -1; 
    }
    struct sockaddr_in server;
    int sock = 0;
    
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cout << "socket create failed\n";
    }
    // need modify

    struct hostent* serv;
    serv = gethostbyname(argv[1]);

    memset(&server, '0', sizeof(server));
    server.sin_family = AF_INET;
    memmove((char*)&server.sin_addr.s_addr, (char*)serv->h_addr, serv->h_length);
    server.sin_port = htons(atoi(argv[2]));

    // end need modify

    if(connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0){
        cout << "\n Connection failed\n";
        return -1;
    }
    char buffer[buffer_size];
	recv(sock, buffer, buffer_size, 0); 
	cout << buffer << endl;
    cout << "Enter 1 for register, 2 for login : ";
    string option;
    int portNum;
    cin >> option;
    string name, balance;
    //bool login = false;
    
    bool login = false;
    while(!login){
        if(option == "1"){
            cout << "\nEnter your name and initial balance (Please saparate them by a space.) : ";
            cin >> name >> balance;
            string r = "REGISTER#" + name + "#" + balance;
            send(sock, r.c_str(), r.length(), 0);
            recv(sock, buffer, buffer_size, 0);
            cout << buffer;
            // if(buffer[0] == '2')
            //     cout << "\nPlease register again.\n";
            //bzero(buffer, 128);
            cout << "Enter 1 for register, 2 for login : ";
            cin >> option;
        }
        else if(option == "2"){
            cout << "\nEnter your name : ";
            cin >> name;
            cout << "Enter port number : ";
            cin >> portNum;
            string r = name + "#" + to_string(portNum);
            send(sock, r.c_str(), r.length(), 0);
            memset(buffer, 0, buffer_size);
            recv(sock, buffer, buffer_size, 0);
            cout << buffer;
            if(buffer[0] == '2'){
                cout << "\nPlease try again.";
                cout << "\nEnter 1 for register, 2 for login : ";
                cin >> option;
            }
            else { login = true; }  
        }
        else{
            cout << "Invalid input. Please try again.\n";
            cout << "Enter 1 for register, 2 for login : ";
            cin >> option;
        }
    }
    cout << " login successfully.\n";
    while(true){
        cout << "Enter 1 to ask for online users list, 8 to exit : ";
        cin >> option;
        if(option == "1"){
            send(sock, "list", 4, 0);
            //bzero(buffer, 128); // memset()
            memset(buffer, 0, buffer_size);
            recv(sock, buffer, buffer_size, 0);
            cout << buffer << endl;
            //bzero(buffer, 128);
        }
        else if(option == "8"){
            send(sock, "exit", 4, 0);
            memset(buffer, 0, buffer_size);
            recv(sock, buffer, buffer_size, 0);
            cout << buffer << endl;
            break;
        }
        else { cout << "Invalid input. Please try again.\n"; }
    }
    close(sock);
    return 0;
}