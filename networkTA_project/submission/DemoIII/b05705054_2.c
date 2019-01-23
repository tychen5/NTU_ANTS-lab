#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <pthread.h>

char mycert[] = "./cacert.pem"; 
char mykey[] = "./cakey.pem";

char pN[100];

void* anotherClient(void* data);

pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;

SSL_CTX* InitCTX(void){   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = TLSv1_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

SSL_CTX* initCTXServer(void){   
    const SSL_METHOD *ssl_method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ssl_method = TLSv1_server_method();
    ctx = SSL_CTX_new(ssl_method);
    if(ctx == NULL){
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}


//Load the certificate 
void ShowCerts(SSL* ssl){   
    X509 *cert;
    char *line;
 
    cert = SSL_get_peer_certificate(ssl); // get the server's certificate
    if ( cert != NULL ){
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       // free the malloc'ed string 
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);\
        free(line);       // free the malloc'ed string 
        X509_free(cert);     // free the malloc'ed certificate copy */
    }
    else{
        printf("No certificates.\n");
    }
}


//Load the certificate 
void certify_client(SSL_CTX* ctx){
    //set the local certificate from CertFile
    if ( SSL_CTX_use_certificate_file(ctx, mycert, SSL_FILETYPE_PEM) <= 0 ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    //set the private key from KeyFile
    if ( SSL_CTX_use_PrivateKey_file(ctx, mykey, SSL_FILETYPE_PEM) <= 0 ){
        ERR_print_errors_fp(stderr);
        abort();
    }
    //verify private key 
    if ( !SSL_CTX_check_private_key(ctx) ){
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}
//Load the certificate 
void certify_server(SSL_CTX* ctx){
    //New lines
    if (SSL_CTX_load_verify_locations(ctx, mycert, mykey) != 1){
        ERR_print_errors_fp(stderr);
    }   
    if (SSL_CTX_set_default_verify_paths(ctx) != 1){
        ERR_print_errors_fp(stderr);
    } 
    //End new lines
    
    /* set the local certificate from mycert */
    if (SSL_CTX_use_certificate_file(ctx, mycert, SSL_FILETYPE_PEM) <= 0){
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from mykey (may be the same as CertFile) */
    if (SSL_CTX_use_PrivateKey_file(ctx, mykey, SSL_FILETYPE_PEM) <= 0){
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if (!SSL_CTX_check_private_key(ctx)){
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
    
    //New lines - Force the client-side have a certificate
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, 4);
    //End new lines

}

SSL *ssl;

int main(int argc , char *argv[]){



    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //server
    struct sockaddr_in serverinfo;
    bzero(&serverinfo,sizeof(serverinfo));
    serverinfo.sin_family = PF_INET;
    serverinfo.sin_addr.s_addr = inet_addr(argv[1]);
    serverinfo.sin_port = htons(atoi(argv[2]));


    //socket的連線

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;

    //localhost test
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = INADDR_ANY;
    info.sin_port = htons(9999);
    bind(sockfd,(struct sockaddr *)&info,sizeof(info));

    int err = connect(sockfd,(struct sockaddr *)&serverinfo,sizeof(serverinfo));
    if(err==-1){
        printf("Connection error");
    }

    // char re[28];
    // recv(sockfd,&re,sizeof(re),0);
    // printf("%s",re);
   
    SSL_CTX *ctx;
    SSL_library_init();
    ctx = InitCTX();
    certify_client(ctx);
   

    ssl = SSL_new(ctx);      // create new SSL connection state
    SSL_set_fd(ssl, sockfd);    // attach the socket descriptor 
    
    
    if ( SSL_connect(ssl) <= 0 ){
        ERR_print_errors_fp(stderr);
    }   // perform the connection
    else{
        char inputBuffer[30]={0};
        SSL_read(ssl ,inputBuffer,sizeof(inputBuffer));
        printf("%s", inputBuffer);
        while(1){
            printf("please enter 1 for register, 2 for login, 3 for exit:");
            int option = 0;
            scanf("%d",&option);
            
            if(option == 1){
                //register
                printf("please enter name you want for registration:");
                char name[20]={0};
                scanf("%s",name);

                printf("please enter money you want for your account:");
                char money[20]={0};
                scanf("%s",money);

                char regisName[100]={0};
                strcpy(regisName,"REGISTER#");
                strcat(regisName,name);
                strcat(regisName,"#");
                strcat(regisName,money);
                
                SSL_write(ssl,regisName,sizeof(regisName));

                char msg[100]={0};
                SSL_read(ssl ,msg,sizeof(msg));
                printf("%s", msg);
                continue;
            }
            else if(option == 2){
                printf("please enter your name:");
                char name[20];
                scanf("%s",name);

                //port between 1024 ~ 65535
                printf("please enter your port number:");
                char portNum[20];
                scanf("%s",portNum);
                strcpy(pN,portNum);

                if(atoi(portNum)> 65535 || atoi(portNum)<1024){
                    printf("port number not exists\n");
                    continue;
                }

                char loginAcc[100]={0};
                strcpy(loginAcc,name);
                strcat(loginAcc,"#");
                strcat(loginAcc,portNum);

                SSL_write(ssl,loginAcc,strlen(loginAcc));
                //check if login is success
                char receiveMessage[1000]={0};
                SSL_read(ssl ,receiveMessage,sizeof(receiveMessage));
                printf("%s",receiveMessage);

                if(strcmp(receiveMessage,"220 AUTH_FAIL\n") == 0){
                    continue;
                }
                else{

                    while(1){

                        pthread_t tid;
                        pthread_create(&tid, NULL, &anotherClient, &pN);

                        // fprintf(stderr,"second\n"); 
                        printf("please enter 1 for latest list, 2 for logout, 3 for extratransaction:");
                        int option1 = 0;
                        scanf("%d",&option1);
                        if(option1 == 1){
                            char msg[5] = "List";
                            SSL_write(ssl,msg,sizeof(msg));

                            char receiveM[1000];
                            SSL_read(ssl ,receiveM,sizeof(receiveM));
                            printf("%s", receiveM);
                        }
                        else if(option1 == 2){
                            break;
                        }
                        else if(option1 == 3){
                            while(1){
                                printf("%s","1 for transfer, 2 for latest list, 3 for exit: ");
                                
                                int option2 = 0;
                                scanf("%d",&option2);
                                char command[20]={0};

                                const char delim[2] = "#";
                                

                                if(option2 == 2){
                                    char command[20] = "List";

                                    // send
                                    SSL_write(ssl, command, sizeof(command));

                                    // receive
                                    char recvbuff[1000] = {0};
                                    SSL_read(ssl, recvbuff, sizeof(recvbuff));
                                    printf("%s\n",recvbuff);
                                }
                                else if(option2 == 3){
                                    char msg[5] = "Exit";

                                    // send
                                    SSL_write(ssl, msg, sizeof(msg));

                                    // receive
                                    char recvbuff[1000] = {0};
                                    SSL_read(ssl, recvbuff, sizeof(recvbuff));
                                    printf("%s\n",recvbuff);

                                    break;
                                }
                                else if(option2 == 1){
                                    int socket_transfer = 0;
                                    int requestSuccess = 0;
                                    struct sockaddr_in connect_info;
                                    char connbuff[1000];
                                    char payeename[20];
                                    char payername[20];
                                    
                                    char payment[20];
                                    char tellname[20];
                                    char recvbuff[1000] = {0};
                                    
                                    printf("Requesting for payee via server\n");
                                    printf("Enter payee's name\n");
                                    
                                    scanf("%s", payeename);
                                    puts("jdwicks");

                                    while(requestSuccess == 0){
                                        puts("jdwicks");
                                        char requestMsg[90] = "SEARCH#";
                                        strcat(requestMsg, payeename);
                                        
                                        SSL_write(ssl, requestMsg, strlen(requestMsg));
                                        

                                        bzero(recvbuff,1000);
                                        SSL_read(ssl, recvbuff, sizeof(recvbuff));
                                        puts(recvbuff);
                                        if(strcmp(recvbuff, "404") == 0){
                                            puts("client not exist\n");
                                            puts("Re-Enter payee's name\n");
                                            bzero(payeename,20);
                                            scanf("%s", payeename);
                                        }
                                        else{
                                            requestSuccess = 1;
                                        }
                                    }
                                    char viaServer[400]={0};
                                    strcpy(viaServer, recvbuff);

                                    char anotherport[400] = {0};
                                    
                                    char *token;
                                    token = strtok(viaServer, delim);
                                    
                                    token = strtok(NULL, delim);

                                    strcpy(anotherport, token);

                                    socket_transfer = socket(PF_INET, SOCK_STREAM, 0);
                                    if(socket_transfer < 0){
                                        puts("Error in subjectively connectng socket\n");
                                        continue;
                                    }

                                    bzero((char *)&connect_info, sizeof(connect_info));
                                    connect_info.sin_family = AF_INET;
                                    connect_info.sin_addr.s_addr = inet_addr("127.0.0.1");
                                    connect_info.sin_port = htons(atoi(anotherport));

                                    // Connectng
                                    if(connect(socket_transfer, (struct sockaddr*)&connect_info, sizeof(connect_info)) < 0){
                                        puts("Error when connecting another client\n");
                                        continue;
                                    }

                                    SSL_CTX *ctx1;
                                    SSL *ssl1;
                                    ctx1 = InitCTX();
                                    certify_client(ctx1);
                                    ssl1 = SSL_new(ctx1);
                                    SSL_set_fd(ssl1, socket_transfer);
                                    if(SSL_connect(ssl1) <= 0){
                                        ERR_print_errors_fp(stderr);
                                    }
                                        
                                    else{
                                        printf("Now connecting to port:%s",anotherport);

                                        bzero(connbuff, 1000);
                                        SSL_read(ssl1, connbuff, sizeof(connbuff));
                                        printf("%s\n",connbuff);

                                        printf("Please enter your name:");
                                        scanf("%s", tellname);
                                        SSL_write(ssl1, tellname, strlen(tellname));

                                        printf("How much would you want to pay ?");
                                        scanf("%s", payment);

                                        SSL_write(ssl1, payment, strlen(payment));

                                        bzero(connbuff, 1000);
                                        SSL_read(ssl1, connbuff, sizeof(connbuff));
                                        printf("%s", connbuff);
                                    }

                                    SSL_free(ssl1);
                                }
                                else{
                                    printf("Option denied. Re-Enter another option");
                                }
                            }
                        }
                        else{
                            printf("option deny, please enter options again\n");
                            continue;
                        }
                    }
                }
            }
            else if(option == 3){
                char msg[5] = "Exit";
                SSL_write(ssl,msg, sizeof(msg));
                char receiveM[1000]={0};
                SSL_read(ssl ,&receiveM,sizeof(receiveM));
                printf("%s",receiveM);
                break;
            }

            

            else{
                printf("option deny, please enter options again\n");
                continue;
            }
        }
    }
    close(sockfd);
    SSL_free(ssl);

    SSL_CTX_free(ctx);
    return 0;
}

void* anotherClient(void* data){
  	int anotherSocket, clientSocket, clientlen, newSocket = 0;
    struct sockaddr_in server_addr, client_addr;
    char serverbuff[1000];
    char serversend[1000];
    char portnum[100];
    strcpy(portnum, pN);
    socklen_t client_len = sizeof(client_addr);
   	
    // Create a server socket
    anotherSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(anotherSocket < 0){
        printf("Error in another socket");
        exit(1);
    } 

    // Initialize socket structure
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = PF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(portnum));

    // Assign a port number to server
    int bindback = bind(anotherSocket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(bindback < 0){
        printf("Error when binding\n");
    	exit(1);
    }

    listen(anotherSocket, 3);
    // cout << "Now listening on port: " << portno << endl;

    SSL_CTX *ctx2;
    SSL_library_init();
    SSL *ssl2;

    while((newSocket = accept(anotherSocket, (struct sockaddr *)&client_addr, &client_len))){
        if(newSocket >= 0){
            pthread_mutex_lock(&mutex_lock);

            ctx2 = initCTXServer();
            certify_server(ctx2);
            ssl2 = SSL_new(ctx2);
            SSL_set_fd(ssl2, newSocket);
            SSL_accept(ssl2);
            
            bzero(serversend, 1000);
            strcpy(serversend, "Connection permitted.");
            SSL_write(ssl2, serversend, strlen(serversend));

            pthread_mutex_unlock(&mutex_lock);

            char anotherIP[1000];
            char anotherName[1000];
            inet_ntop(AF_INET, &client_addr.sin_addr, anotherIP, INET_ADDRSTRLEN);
            bzero(serverbuff, 1000);
            SSL_read(ssl2, serverbuff, sizeof(serverbuff));
            strcpy(anotherName, serverbuff);

            // cout << "\nNew connection." << endl;
            printf("\nNew Connection from %s : %hu, named %s \n", anotherIP, client_addr.sin_port, anotherName);

            char notifyServer[300];
            strcpy(notifyServer, anotherName);
            bzero(serverbuff, 1000);

            SSL_read(ssl2, serverbuff, sizeof(serverbuff));
            printf("%s pays you %s dollars\n", anotherName, serverbuff);

            char buf[300];
            strcpy(buf, serverbuff);

            char noticemsg[200];
            strcpy(noticemsg,"TRANSFER#");
            strcat(noticemsg, notifyServer);
            strcat(noticemsg, "#");
            strcat(noticemsg, buf);

            SSL_write(ssl, noticemsg, sizeof(noticemsg));

            bzero(serverbuff, 1000);
            SSL_read(ssl, serverbuff, 1000);

            printf("%s", serverbuff);

            SSL_write(ssl2, serverbuff, 1000);
            
            SSL_free(ssl2);
        } 
    }

    // close server socket 
    close(newSocket);
    SSL_CTX_free(ctx2);     
  
    return 0;
}
