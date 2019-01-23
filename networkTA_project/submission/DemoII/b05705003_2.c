#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

char* LastCharDel(char* name) {
    int i = 0;
    while (name[i] != '\0') i++;
    name[i-1] = '\0';
    return name;
}

int main(int argc, char *argv[]) {
    int sockfd, portno;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    char bufferP[256];
    char bufferB[256];

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

    //Socket Allocation
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    //Socket Connection
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) error("ERROR connecting");

    //Connection accepted or not
    bzero(buffer,256);
    read(sockfd,buffer,255);
    printf("%s\n",buffer);

    do {
        //Register or Login
        printf("Enter 1 for Register, 2 for Login, 8 for Exit: ");

        bzero(buffer,256);
        fgets(buffer,255,stdin);

        //Register
        char regist[256] = "REGISTER#";
        if (strcmp(buffer, "1\n") == 0) {
            printf("Enter the name you want to register: ");
            bzero(buffer,256);
            fgets(buffer,255,stdin);
            printf("Enter the initial balance: ");
            bzero(bufferB, 256);
            fgets(bufferB,255,stdin);
            LastCharDel(buffer);
            strcat(regist, buffer);
            strcat(regist, "#");
            strcat(regist, bufferB);
            write(sockfd, regist, strlen(regist));
            bzero(buffer,256);
            read(sockfd,buffer,255);
            printf("%s\n",buffer);
        }
        //Login and List or Exit
        else if (strcmp(buffer, "2\n") == 0) {
            printf("Enter your name: ");
            bzero(buffer,256);
            fgets(buffer,255,stdin);
            printf("Enter the port number: ");
            bzero(bufferP,256);
            fgets(bufferP,255,stdin);
            LastCharDel(buffer);
            strcat(buffer, "#");
            strcat(buffer, bufferP);
            write(sockfd, buffer, strlen(buffer));
            bzero(buffer,256);
            read(sockfd,buffer,255);
            printf("%s",buffer);
            printf("Enter the number of actions you wnat to take.\n1 to ask for the latest list, 8 to Exit: ");
            bzero(buffer,256);
            fgets(buffer,255,stdin);
            if (strcmp(buffer, "1\n") == 0) {
                write(sockfd, "List\n", 5);
                bzero(buffer,256);
                read(sockfd,buffer,255);
                printf("%s",buffer);
            }
            else if (strcmp(buffer, "8\n") == 0) break;
        }
        else if (strcmp(buffer, "8\n") == 0) break;
        else printf("Error input.\n");
    } while (strcmp(buffer, "8\n") != 0);

    //Exit
    write(sockfd, "Exit\n", 5);
    bzero(buffer,256);
    read(sockfd,buffer,255);
    printf("%s\n",buffer);

    close(sockfd);
    return 0;
}