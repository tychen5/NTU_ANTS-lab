#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define BLUE    "\x1b[34m"
#define RESET   "\x1b[0m"

int main(int argc, char *argv[])
{
    char command[128];
    char portNum[128];
    char regisCommand[128] = "REGISTER#";
    char logCommand[128] = "#";
    int option = 0;
    int status = 0;
    int cnt = 0;

    // build socket
    int socketback = 0;
    socketback = socket(PF_INET, SOCK_STREAM, 0);
    if(socketback < 0)
    {
        printf("%s\n", "Fail to create socket.");
        return -1;
    }

    // socket connect
    struct sockaddr_in connection;
    bzero(&connection, sizeof(connection));
    connection.sin_family = PF_INET;
    connection.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &connection.sin_addr);

    int connectback = connect(socketback, (struct sockaddr *)&connection, sizeof(connection));
    if(connectback < 0)
    {
        printf("%s\n", "Fail to connect.");
        return -1;
    }

    // receive register
    char buff[128];
    int reciback = recv(socketback, buff, sizeof(buff), 0);
    if(reciback < 0)
    {
        printf("%s\n", "Fail to receive.");
        return -1;
    }
    printf("%s\n", buff);
    
    // input variable command
    while(status == 0)
    {
        printf("%s%s%s", BLUE, "Please enter 1 for register or enter 2 for login: ", RESET);
        scanf("%d", &option);
        
        if(option == 1)
        {
            cnt += 1;
            if(cnt > 1)
            {
                printf("%s%s%s\n", BLUE, "You have registered before!", RESET);
                continue;
            }
            printf("%s%s%s", BLUE, "Please enter your username for register: ", RESET);

            // register
            scanf("%s", command);
            strcat(regisCommand, command);
            send(socketback, regisCommand, sizeof(command), 0);

            // receive register message
            bzero(buff, 128);
            recv(socketback, buff, sizeof(buff), 0);
            printf("%s\n", buff);
        }
        else if(option == 2)
        {
            status = 2;

            // log in
            printf("%s%s%s", BLUE, "Please enter your username for login: ", RESET);
            bzero(command, 128);
            scanf("%s", command);
            printf("%s%s%s", BLUE, "Please enter your port number: ", RESET);
            scanf("%s", portNum);
            while(atoi(portNum) < 1024 || atoi(portNum) > 65535)
            {
                printf("%s%s%s", BLUE, "The port number is out of range! Please reenter the port number: ", RESET);
                scanf("%s", portNum);
            }
            strcat(command, logCommand);

            // send log message
            send(socketback, command, sizeof(command), 0);
            send(socketback, portNum, sizeof(portNum), 0);

            // receive log message
            bzero(buff, 128);
            recv(socketback, buff, sizeof(buff), 0);
            printf("%s\n", buff);
            bzero(buff, 128);
            recv(socketback, buff, sizeof(buff), 0);
            printf("%s\n", buff);
        }
    }

    while(status == 2)
    {
        printf("%s%s%s", BLUE, "Please enter 7 for latest list or enter 8 for exit: ", RESET);
        scanf("%d", &option);
        if(option == 7)
        {
            bzero(command, 128);
            strcpy(command, "List");

            // send
            send(socketback, command, sizeof(command), 0);

            // receive
            bzero(buff, 128);
            recv(socketback, buff, sizeof(buff), 0);
            printf("%s\n", buff);
        }
        else if(option == 8)
        {
            bzero(command, 128);
            strcpy(command, "Exit");

            // send
            send(socketback, command, sizeof(command), 0);

            // receive
            bzero(buff, 128);
            recv(socketback, buff, sizeof(buff), 0);
            printf("%s\n", buff);

            return -1;
        }

        // close
        // close(socketback);
    }

    return 0;
}
