#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h> //for open
#include <unistd.h> //for close
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h> 
#include <malloc.h>
#include <resolv.h>
#include "openssl/ssl.h"
#include "openssl/err.h"

char client_m[256];
char buffer[256];
SSL_CTX *ctx;
//Acount data
struct account {
    char name[25];
    char port[10];
    char balance[10];
};

char* LastCharDel(char* name) {
    int i = 0;
    while (name[i] != '\0') i++;
    name[i-1] = '\0';
    return name;
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
void LoadCertificates(SSL_CTX* fctx)
{
    char *CertFile = "cert.pem";
    char *KeyFile = "key.pem";
    SSL_CTX_use_certificate_file(fctx, CertFile, SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(fctx, KeyFile, SSL_FILETYPE_PEM);
}

struct account user[100];
int user_num = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *socketThread(void *arg){
    int newSocket = *((int *)arg);
    SSL* c_ssl = SSL_new(ctx);
    SSL_set_fd(c_ssl, newSocket);
    SSL_accept(c_ssl);
    //Send message to the client socket 
    strcpy(buffer, "Connection Accepted.\n");
    SSL_write(c_ssl,buffer, strlen(buffer));

    struct account this_client[20];
    int this_client_num = 0;
    char latest_balance[10] = "";
    char last_login[10] = "";

    //Communication with client
    do {
        bzero(client_m, 256);
        SSL_read(c_ssl, client_m, 256);

        int hashtag = 0;
        for (int i = 0; i < strlen(client_m); i++) {
            if (client_m[i] == '#' && client_m[strlen(client_m)-1] == '\n') {
                hashtag = 1;
                break;
            }
        }

        if (hashtag) {
            //REGISTER
            if (client_m[0] == 'R' && client_m[1] == 'E' && client_m[2] == 'G' && client_m[3] == 'I' && client_m[4] == 'S' && client_m[5] == 'T' && client_m[6] == 'E' && client_m[7] == 'R') {
                char new_user[20];
                int count = 0;
                for (int i = 9; i < strlen(client_m)-1; i++) {
                    if (client_m[i] == '#') break;
                    new_user[count] = client_m[i];
                    count++;
                }

                //Input balance
                char temp_balance[10] = "";
                count = 0;
                for (int i = 10 + strlen(new_user); i < strlen(client_m); i++) {
                    temp_balance[count] = client_m[i];
                    count++;
                }

                int new = 1;
                for (int i = 0; i < user_num; i++) {
                    if (strcmp(user[i].name, new_user) == 0) {
                        SSL_write(c_ssl, "210 FAIL\n", 9);
                        new = 0;
                    }
                }
                if (new) {
                    strcpy(user[user_num].name, new_user);
                    strcpy(user[user_num].balance, temp_balance);
                    strcpy(latest_balance, temp_balance);
                    strcpy(this_client[this_client_num].name, new_user);
                    SSL_write(c_ssl, "100 OK\n", 7);
                    user_num++;
                    this_client_num++;
                }
            }

            //Transfer
            else if (client_m[0] == 'T' && client_m[1] == 'R' && client_m[2] == 'A' && client_m[3] == 'N' && client_m[4] == 'S') {
                char amount[25] = "";
                int count = 0;
                for (int i = 6; i < strlen(client_m)-1; i++) {
                    if (client_m[i] == '#') break;
                    amount[count] = client_m[i];
                    count++;
                }
                int payamount = atoi(amount);
                char payee[25] = "";
                count = 0;
                for (int i = 7 + strlen(amount); i < strlen(client_m)-1; i++) {
                    payee[count] = client_m[i];
                    count++;
                }

                int none = 0;

                //Transfer successfully
                for (int i = 0; i < user_num; i++) {
                    if (strcmp(user[i].name, payee) == 0) {
                        for (int j = 0; j < user_num; j++) {
                            if (strcmp(user[j].name, last_login) == 0) {
                                int money = atoi(user[j].balance);
                                if (money < payamount) {
                                    SSL_write(c_ssl, "Not enough balance.\n", 20);
                                    break;
                                }
                                money -= payamount;
                                bzero(user[j].balance, strlen(user[j].balance));
                                sprintf(user[j].balance, "%d", money);
                            }
                        }
                        int initial = atoi(user[i].balance);
                        initial += payamount;
                        bzero(user[i].balance, strlen(user[i].balance));
                        sprintf(user[i].balance, "%d", initial);
                        SSL_write(c_ssl, "Transfer Succeeded. To see your balance please login again.\n", 60);
                    }
                    else none++;
                }

                //Transfer failed
                if (none == user_num) SSL_write(c_ssl, "TRANS_FAIL\n", 11);
            }

            //Login
            else {
                //Input name
                char temp_name[25] = "";
                int count = 0;
                for (int i = 0; i < strlen(client_m); i++) {
                    if (client_m[i] == '#') break;
                    temp_name[count] = client_m[i];
                    count++;
                }
                //Input port
                char temp_port[10] = "";
                count = 0;
                for (int i = strlen(temp_name)+1; i < strlen(client_m); i++) {
                    temp_port[count] = client_m[i];
                    count++;
                }

                int none = 0;
                char usernum[5] = "";
                sprintf(usernum, "%d", user_num);

                //Login successfully
                for (int i = 0; i < user_num; i++) {
                    char sentence[1024] = "";
                    if (strcmp(user[i].name, temp_name) == 0) {
                        strcpy(user[i].port, temp_port);
                        strcpy(latest_balance, user[i].balance);
                        strcpy(last_login, user[i].name);
                        strcpy(sentence, latest_balance);
                        strcat(sentence, "Number of accounts online: ");
                        strcat(sentence, usernum);
                        strcat(sentence, "\n");
                        for (int j = 0; j < user_num; j++) {
                            strcat(sentence, user[j].name);
                            strcat(sentence, "#127.0.0.1#");
                            strcat(sentence, user[j].port);
                        }
                        strcat(sentence, "\n");
                        SSL_write(c_ssl, sentence, strlen(sentence));
                        break;
                    }
                    else none++;
                }

                //Login failed
                if (none == user_num) SSL_write(c_ssl, "220 AUTH_FAIL\n", 14);
            }
        }

        char sentence[1024] = "";
        char usernum[5] = "";
        sprintf(usernum, "%d", user_num);
        if (strcmp(client_m, "List\n") == 0) {
            strcpy(sentence, latest_balance);
            strcat(sentence, "Number of accounts online: ");
            strcat(sentence, usernum);
            strcat(sentence, "\n");
            for (int j = 0; j < user_num; j++) {
                strcat(sentence, user[j].name);
                strcat(sentence, "#127.0.0.1#");
                strcat(sentence, user[j].port);
                strcat(sentence, "\n");
            }
            SSL_write(c_ssl, sentence, strlen(sentence));
        }
        if (strcmp(client_m, "Exit\n") == 0) {
            int temp = user_num;
            //If the account is this client's, logout.
            for (int i = 0; i < user_num; i++) {
                for (int j = 0; j < this_client_num; j++) {
                    if (strcmp(user[i].name, this_client[j].name) == 0) {
                        bzero(user[i].name, 25);
                        bzero(user[i].balance, 10);
                        bzero(user[i].port, 10);
                        temp--;
                        if (i < user_num-1) {
                            strcpy(user[i].name, user[i+1].name);
                            strcpy(user[i].port, user[i+1].port);
                            strcpy(user[i].balance, user[i+1].balance);
                        }
                    }
                }
            }
            user_num = temp;
            break;
        }
    } while (strcmp(client_m, "Exit\n") != 0);

    SSL_write(c_ssl, "Bye\n", 4);
    close(newSocket);
    pthread_exit(NULL);
}

int main(){
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;
    SSL_library_init(); //init SSL library
    ctx = InitServerCTX();  //initialize SSL 
    LoadCertificates(ctx);

    //Create the socket. 
    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(33220);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    //Bind the address struct to the socket 
    bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    //Listen on the socket, with 40 max connection requests queued 
    if(listen(serverSocket,50)==0) printf("Listening\n");
    else printf("Error\n");

    pthread_t tid[60];
    int i = 0;
    while(1){

        //Accept call creates a new socket for the incoming connection
        addr_size = sizeof serverStorage;
        newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);

        //for each client request creates a thread and assign the client request to it to process
        //so the main thread can entertain next request
        if( pthread_create(&tid[i], NULL, socketThread, &newSocket) != 0)
            printf("Failed to create thread\n");
        if( i >= 50){
            i = 0;
            while(i < 50){
                pthread_join(tid[i++], NULL);
            }
            i = 0;
        }
    }
    return 0;
}