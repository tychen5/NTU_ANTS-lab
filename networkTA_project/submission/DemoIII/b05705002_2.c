#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>


bool isLogIn = false;
int serverfd = 0;

int openConnection(const char* ip, int port)
{
    struct sockaddr_in dest; // an Internet endpoint address
    int sd; //socket descripter

    memset(&dest, 0, sizeof(dest)); 
    dest.sin_port = htons(port);
    dest.sin_addr.s_addr = inet_addr(ip);
    dest.sin_family = AF_INET;

    sd = socket(AF_INET, SOCK_STREAM, 0); //allocate socket
    if(connect(sd, (struct sockaddr*)&dest, sizeof(dest)) < 0)
    {
        printf("Connection ERROR\n");
        abort();
    }
    return sd;
}

void* function(void* p) //thread to receive the transaction messages
{
    //SSL_CTX *ctx3;
    //ctx3 = InitServerCTX();

    char* port = (char*)p;
    //char CertFile[] = "mycert.pem";
    //char KeyFile[] = "mykey.pem";
    //LoadCertificates(ctx3, CertFile, KeyFile); // load certs and key

    struct sockaddr_in dest;
    dest.sin_port = htons(atoi(port));
    dest.sin_addr.s_addr = INADDR_ANY;
    dest.sin_family = AF_INET;

    int sd3 = socket(AF_INET, SOCK_STREAM, 0);

    int b = -1;
    b = bind(sd3, (struct sockaddr*)&dest, sizeof(dest));

    if(b < 0)
        printf("\nunable to bind the port\n");

    listen(sd3, 10); //listen to the port

    while(isLogIn)
    {
        //SSL *ssl3;
        int clientfd; //client descriptor
        struct sockaddr_in client_addr;
        int addrlen = sizeof(client_addr);
        clientfd = accept(sd3, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen);

        //ssl3 = SSL_new(ctx3);              // get new SSL state with context 
        //SSL_set_fd(ssl3, clientfd);      // set connection socket to SSL state

        //if ( SSL_accept(ssl3) == FAIL ) //do SSL-protocol accept
        //  ERR_print_errors_fp(stderr);
        //else
        //{
        char buf[1024];
        bzero(buf, 1024);
        recv(clientfd, buf, sizeof(buf), 0);
        printf("\nTransaction with client: " );
        printf("%s\n",buf);
        //int n = SSL_read(ssl3, buf, sizeof(buf)); // get reply & decrypt 
        //printf("\nmsgs from peer : %s\n", buf);
        send(serverfd, buf, sizeof(buf), 0);  // serverfd = sockfd
        //SSL_write(ssl, buf, strlen(buf)); //send to server
        //}
    }
//  SSL_CTX_free(ctx3);        // release context
}

int main(int argc , char *argv[]){
    pthread_t tid;       /* the thread identifier */
    pthread_attr_t attr; /* set of attributes for the thread */
    
    /*create socket*/
    int sock = 0;
    sock = socket(AF_INET , SOCK_STREAM , 0);   //create socket, socket(domain, type, protocol)
    
    if (sock == -1){      //check socket performance
        printf("Fail to create a socket.");
    }
    
    /*client connect to server*/
    struct sockaddr_in info;
    bzero(&info,sizeof(info));      //initialize 
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr("10.0.2.15");  
    info.sin_port = htons(2016);        //htons:Host TO Network Short integer
    
    int err = connect(sock,(struct sockaddr *)&info,sizeof(info));
    if(err == -1){
        printf("Connection error");
    }
    

    struct sockaddr_in my_addr;

    char receiveMessage[2000] = {0};

    bool connected;
    connected = true;
    
    int tmp = 0;
    int flag=0;

    char message1[100] = {"REGISTER#"};     //message sent to server
    char tag[2] = {"#"};        //message sent to server
    char list[100] = {"List"};
    char exit[100] = {"Exit"};
    char account[100] = {"Account"};
    char peerIP[10] = {"peerIP"};
    char acc[100] = {"accountBalance#"};
    char name[20] = {};
    while (connected){
              
        printf("Enter 1 for Register, Enter 2 for Login : ");
        std::cin >> tmp;
        if(tmp == 1){
            //register
            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s\n",receiveMessage);
            
            
            printf("Enter the name you want to register : ");
            std::cin >> name;
            strcat(message1,name);      
            

            //send register message to server
            send(sock,message1,sizeof(message1),0);

            //
            memset(receiveMessage, '\0', sizeof(receiveMessage));   
            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s\n",receiveMessage);

            if(strcmp(receiveMessage, "100 OK\r\n") == 0){
                ///////////////////
                char money[20] = {};
                printf("Enter the money : ");
                std::cin >> money;
                strcat(acc, money);

                //send register message to server
                send(sock,acc,sizeof(acc),0);

                memset(receiveMessage, '\0', sizeof(receiveMessage));   
                recv(sock,receiveMessage,sizeof(receiveMessage),0);
                printf("%s\n",receiveMessage);
                ///////////////////
            }
            
        }
        
        else if(tmp == 2){
            /*-----login------*/
            char message2[20] = {};
            char portname[20] = {0};
            char nammmmmm[20] = {};
            
            printf("Enter your name: ");
            std::cin >> message2;
            strcpy(nammmmmm,message2);
            printf("%s\n",nammmmmm);
            strcat(message2,"#");
            
            printf("Enter the port name: ");
            std::cin >> portname;
            strcat(message2,portname);
            
            //send login message
            send(sock,message2,strlen(message2),0);
            
            memset(receiveMessage,'\0',sizeof(receiveMessage));
            recv(sock,receiveMessage,sizeof(receiveMessage),0);
            printf("%s\n",receiveMessage);

            pthread_attr_init(&attr);
            pthread_create(&tid, &attr, function, (void*)(portname));
            isLogIn = true;

                        
            flag=0;
            
            while(flag!=8){
                printf("Enter the number of actions you want to take.\n1 to ask for the latest List, 2 to do Transaction, 3 to see your account balance, 8 to Exit: ");
                std::cin >> flag;
                if(flag == 1){
                    send(sock,list,strlen(list),0);

                    memset(receiveMessage,'\0',sizeof(receiveMessage));
                    recv(sock,receiveMessage,sizeof(receiveMessage),0);
                    printf("%s\n",receiveMessage);
                }
                else if (flag == 8){
                    send(sock,exit,strlen(list),0);
            
                    memset(receiveMessage,'\0',sizeof(receiveMessage));
                    recv(sock,receiveMessage,sizeof(receiveMessage),0);
                    printf("%s\n",receiveMessage);
                }
                

                else if (flag == 3){
                    char reply[100]={};
                    memset(reply,'\0',sizeof(reply));
                    strcat(reply,account);
                    strcat(reply,tag);
                    strcat(reply,nammmmmm);
                    send(sock, reply,strlen(reply),0);

                    memset(receiveMessage,'\0',sizeof(receiveMessage));
                    recv(sock,receiveMessage,sizeof(receiveMessage),0);
                    printf("%s\n",receiveMessage);
                }

                else if (flag == 2){
                    char ip[64];
                    char peerName[64];
                    char portToConnect[64];
                    char amount[64];

                    strcpy(ip, "10.0.2.15");

                    printf("Please enter peer's name: ");
                    std::cin >> peerName;
                    printf("Please enter peer's port: ");
                    std::cin >> portToConnect;
                    printf("Please enter the amount: ");
                    std::cin >> amount;

                    
                    char tran[100] = {"Tran#"};
                    //return the changed money to the server
                    strcat(tran,nammmmmm);
                    strcat(tran,tag);
                    strcat(tran,peerName);
                    strcat(tran,tag);
                    strcat(tran,amount);

                    send(sock,tran,strlen(tran),0);

                    memset(receiveMessage,'\0',sizeof(receiveMessage));
                    recv(sock,receiveMessage,sizeof(receiveMessage),0);
                    printf("%s\n",receiveMessage);
                    

                    int sd_tran = openConnection(ip, atoi(portToConnect));
                    
                    //send transaction message to client B
                    char tran_msg[1024];
                    memset(tran_msg,'\0',sizeof(tran_msg));
                    strcat(tran_msg,nammmmmm);
                    strcat(tran_msg,tag);
                    strcat(tran_msg,amount);
                    strcat(tran_msg,tag);
                    strcat(tran_msg,peerName);
                    //snprintf(tran_msg, 1024, "%s#%s#%s", name, transferToB, Bname);
                    send(sd_tran, tran_msg, sizeof(tran_msg), 0);
                    
                }
                //////////////////
                else{
                    send(sock,exit,strlen(list),0);

                    memset(receiveMessage,'\0',sizeof(receiveMessage));
                    recv(sock,receiveMessage,sizeof(receiveMessage),0);
                    printf("%s\n",receiveMessage);
                    printf("Wrong\n");
                }
            }
            connected = false;
        }
        
        else{
            printf("Wrong Number\n");
            
        }
    }
    
    printf("close Socket\n");
    close(sock);
    return 0;
} 
