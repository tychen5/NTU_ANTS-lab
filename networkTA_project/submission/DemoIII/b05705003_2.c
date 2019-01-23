#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <malloc.h>
#include <resolv.h>
#include "openssl/ssl.h"
#include "openssl/err.h"

SSL_CTX *ctx;
SSL_CTX *sctx;
SSL* c_ssl;

SSL_CTX* InitCTX(void){   
    const SSL_METHOD *method;
    SSL_CTX *fctx;
 
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    method = TLSv1_client_method();
    fctx = SSL_CTX_new(method);
    return fctx;
}

SSL_CTX* InitServerCTX(void)
{   
    const SSL_METHOD *method;
    SSL_CTX *fctx;
 
    OpenSSL_add_all_algorithms(); 
    SSL_load_error_strings();
    method = TLSv1_server_method(); 
    fctx = SSL_CTX_new(method);
    return fctx;
}

//Load the certificate 
void ShowCerts(SSL* fssl, int isPrint){   
    X509 *cert;
    char *line;
 
    cert = SSL_get_peer_certificate(fssl);
    if ( cert != NULL && isPrint ){
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: ");
        printf("%s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: ");
        printf("%s\n", line);
        free(line);
        X509_free(cert);
    }
    else if (cert == NULL)
        printf("No certificates.\n");
}

char* LastCharDel(char* name) {
    int i = 0;
    while (name[i] != '\0') i++;
    name[i-1] = '\0';
    return name;
}

int main(int argc, char *argv[]) {
    int sockfd, portno;
    SSL_library_init();
    ctx = InitCTX();
    sctx = InitServerCTX();
    c_ssl = SSL_new(ctx);
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
    connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));

    SSL_set_fd(c_ssl, sockfd);
    SSL_connect(c_ssl);
    //ShowCerts(c_ssl, 1);

    //Connection accepted or not
    bzero(buffer,256);
    SSL_read(c_ssl,buffer,255);
    printf("%s\n",buffer);

    do {
        //Register or Login
        printf("Enter 1 for Register, 2 for Login, 8 for Exit: ");

        bzero(buffer,256);
        fgets(buffer,255,stdin);

        //Register
        char regist[256] = "REGISTER#";
        char trans[256] = "TRANS#";

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
            SSL_write(c_ssl, regist, strlen(regist));
            bzero(buffer,256);
            SSL_read(c_ssl,buffer,255);
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
            SSL_write(c_ssl, buffer, strlen(buffer));
            bzero(buffer,256);
            SSL_read(c_ssl,buffer,255);
            printf("%s",buffer);
            printf("Enter the number of actions you want to take.\n1 to ask for the latest list, 2 to transfer money, 8 to Exit: ");
            bzero(buffer,256);
            fgets(buffer,255,stdin);
            if (strcmp(buffer, "1\n") == 0) {
                SSL_write(c_ssl, "List\n", 5);
                bzero(buffer,256);
                SSL_read(c_ssl,buffer,255);
                printf("%s",buffer);
            }
            else if (strcmp(buffer, "2\n") == 0) {
                printf("Pay amount: ");
                bzero(buffer,256);
                fgets(buffer,255,stdin);
                printf("Payee username: ");
                bzero(bufferP,256);
                fgets(bufferP,255,stdin);
                LastCharDel(buffer);
                strcat(trans, buffer);
                strcat(trans, "#");
                strcat(trans, bufferP);
                SSL_write(c_ssl, trans, strlen(trans));
                bzero(buffer,256);
                SSL_read(c_ssl,buffer,255);
                printf("%s\n",buffer);
            }
            else if (strcmp(buffer, "8\n") == 0) break;
        }
        else if (strcmp(buffer, "8\n") == 0) break;
        else printf("Error input.\n");
    } while (strcmp(buffer, "8\n") != 0);

    //Exit
    SSL_write(c_ssl, "Exit\n", 5);
    bzero(buffer,256);
    SSL_read(c_ssl,buffer,255);
    printf("%s\n",buffer);
    SSL_free(c_ssl);
    SSL_CTX_free(ctx);
    SSL_CTX_free(sctx);
    close(sockfd);
    return 0;
}