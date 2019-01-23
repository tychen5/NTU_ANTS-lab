#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h> //for open
#include <unistd.h> //for close
#include <pthread.h>

char client_m[256];
char buffer[256];
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


struct account user[100];
int user_num = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *socketThread(void *arg){
    int newSocket = *((int *)arg);

    //Send message to the client socket 
    pthread_mutex_lock(&lock);
    char *message = malloc(256);
    strcpy(message, "Connection Accepted.\n");
    strcpy(buffer, message);
    free(message);
    pthread_mutex_unlock(&lock);
    sleep(1);
    send(newSocket,buffer, strlen(buffer), 0);

    struct account this_client[20];
    int this_client_num = 0;
    char latest_balance[10];


    //Communication with client
    do {
        bzero(client_m, 256);
        recv(newSocket, client_m, 256, 0);

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
                        write(newSocket, "210 FAIL\n", 9);
                        new = 0;
                    }
                }
                if (new) {
                    strcpy(user[user_num].name, new_user);
                    strcpy(user[user_num].balance, temp_balance);
                    strcpy(latest_balance, temp_balance);
                    strcpy(this_client[this_client_num].name, new_user);
                    write(newSocket, "100 OK\n", 7);
                    user_num++;
                    this_client_num++;
                }
            }

            //Login
            else {
                //Input name
                char temp_name[25];
                int count = 0;
                for (int i = 0; i < strlen(client_m); i++) {
                    if (client_m[i] == '#') break;
                    temp_name[count] = client_m[i];
                    count++;
                }

                //Input port
                char temp_port[10];
                count = 0;
                for (int i = strlen(temp_name)+1; i < strlen(client_m); i++) {
                    temp_port[count] = client_m[i];
                    count++;
                }

                int none = 0;
                char usernum[5];
                sprintf(usernum, "%d", user_num);

                //Login successfully
                for (int i = 0; i < user_num; i++) {
                    if (strcmp(user[i].name, temp_name) == 0) {
                        strcpy(user[i].port, temp_port);
                        write(newSocket, latest_balance, strlen(latest_balance));
                        write(newSocket, "Number of accounts online: ", 27);
                        write(newSocket, usernum, strlen(usernum));
                        write(newSocket, "\n", 1);
                        for (int j = 0; j < user_num; j++) {
                            write(newSocket, user[j].name, strlen(user[j].name));
                            write(newSocket, "#127.0.0.1#", 11);
                            write(newSocket, user[j].port, strlen(user[j].port));
                        }
                        write(newSocket, "\n", 1);
                        break;
                    }
                    else none++;
                }

                //Login failed
                if (none == user_num) write(newSocket, "220 AUTH_FAIL\n", 14);
            }
        }

        if (strcmp(client_m, "List\n") == 0) {
            write(newSocket, latest_balance, strlen(latest_balance));
            for (int j = 0; j < user_num; j++) {
                write(newSocket, user[j].name, strlen(user[j].name));
                write(newSocket, "#127.0.0.1#", 11);
                write(newSocket, user[j].port, strlen(user[j].port));
            }
            write(newSocket, "\n", 1);
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

    write(newSocket, "Bye\n", 4);
    close(newSocket);
    pthread_exit(NULL);
}

int main(){
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;

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