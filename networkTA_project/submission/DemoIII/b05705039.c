#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
#include <openssl/ssl.h>
#include <openssl/err.h>

struct Account
{
    char name[1024];
    int port;
    int accountBalance;
    int online;
};
struct Account accountData[1024]; //store the account infromation
int userNum = 0;
int all_thread = 20;
int socket_desc;
int onlineUserNum = 0;
int accountNum = 0;
SSL_CTX *context;
void *connection_handler(void *);

SSL_CTX* InitServerCTX(void)
{   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc. 
    SSL_load_error_strings();   // load all error messages */
    method = SSLv23_server_method();  // create new server-method instance 
    ctx = SSL_CTX_new(method);   // create new context from method
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
//Load the certificate 
void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
    //set the local certificate from CertFile
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //set the private key from KeyFile
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //verify private key 
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

char * split_str(char *str){ 
  char *tok = "#";
  char *token;
  // printf("inside");
  // printf("str%s\n",str);
  token = strtok(str, tok); //username
  // printf("split_str%s\n",token );
  return(token);
}
int find(char *name){
  for (int i = 0; i < userNum; i++){
    if(strcmp(name,accountData[i].name) == 0){
      return(i); //return user ID
    }
  }
  return(-1); //can't find
}
char* get_name(char * client_message){
    int a = 0;
    char *name = malloc(100 * sizeof(char));
    while (a < sizeof(client_message)) {
      name[a] = client_message[sizeof(split_str(client_message))+1+a];
      a++;
    }
    return(name);
}

char * regist(char * client_message,int userID){
    char *message;
    char *name = malloc(100 * sizeof(char));
    name = get_name(client_message);
    if (find(name) != -1){
        message = "210 FAIL\n";
        return(message);
    }
    else{
        strcpy(accountData[userID].name, name);
        accountData[userID].online = 0;
        accountData[userID].port = -1;
        accountData[userID].accountBalance = 1000;
        message = "100 OK\n";
        printf("register success\n" );
        return(message);
    }
}

char * get_list(char IPs[],int userID){
    int online_cnt = 0;
    char *message= malloc(100 * sizeof(char));
    char *message_1= malloc(100 * sizeof(char));
    sprintf(message_1, "Account Balance: %d\n", accountData[userID].accountBalance); 
    char list[1024] = {};
    for (int i = 0; i < userNum; i++){
      if(accountData[i].online == 1)
      {
        online_cnt++;
        char account_list[1024];
        memset(account_list, '\0', sizeof(account_list));
        snprintf(account_list, 1024, "%s#%s#%d\r\n", accountData[i].name,IPs,accountData[i].port); //online list
        // printf("here%s\n", account_list);
        strcat(list, account_list);   
      }        
    }
    strcat(message, message_1);
    memset(message_1, '\0', sizeof(&message_1));
    sprintf(message_1, "number of online accounts: %d\n", online_cnt); 
    strcat(message, message_1);
    //online list
    strcat(message, list);
    return(message);
}

char* get_port(char * client_message){
    int a = 0;
    char *port = malloc(100 * sizeof(char));
    while (a < sizeof(client_message)) {
      port[a] = client_message[sizeof(split_str(client_message))-1+a];
      a++;
    }
    return(port);
}
void login_user(char* client_message,int userID){
    char * login_name = malloc(100 * sizeof(char));
    login_name = split_str(client_message);
    accountData[userID].online = 1;
    accountData[userID].port = atoi(get_port(client_message));
    printf("login : %s\n",login_name);
}
void first_deposit(char * client_message,int userID){
    int deposit_amount = atoi(client_message);
    accountData[userID].accountBalance = deposit_amount;
    printf("deposit amount: %d\n",  deposit_amount);
}
char* acoountPrint(int userID){

    char *message= malloc(20 * sizeof(char));
    strcpy(message,"Account balance:");
    char *amount= malloc(20 * sizeof(char));
    sprintf(amount,"%d\n",accountData[userID].accountBalance);
    strcat(message,amount);
    return(message);
}

char transaction(char* payer, char* payee, int amount){
    char success='1';
    for (int i = 0; i < accountNum; i++){
        if (strcmp(payer, accountData[i].name)==0){   
            if(accountData[i].accountBalance < amount)
                success = '0';
        }
    }
    if (success == '1'){
        // printf("here3333");
        for(int i=0;i<accountNum;i++){
            if (strcmp(payer, accountData[i].name)==0){   
                accountData[i].accountBalance -= amount;
            }
            if(strcmp(payee, accountData[i].name)==0){
                accountData[i].accountBalance += amount;
            }
        }
    }
    return success;
}
int check(char*client_message){
    char* message = malloc(100 * sizeof(char));
    message[0] = client_message[0];
    // message = split_str(client_message);
    if(strcmp(message,"T") == 0){
        return 1;
    }
    else{
        return 0;
    }
}
void delchar(char *x,int a, int b){
  if ((a+b-1) <= strlen(x)){
    strcpy(&x[b-1],&x[a+b-1]);
    puts(x);
    }
}
char* myToken(char*client_message, int start,int end){
    char*message = malloc(100 * sizeof(char));
    int i = 0;
    int j = start+1;
    while(j < end){
        message[i] = client_message[j];
        i++;
    }
    return (message);
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void* thread_id)
{
    int threadID = *(int*)(thread_id);

    while(1)
    {
        int client_sock , c, read_size, length;
        struct sockaddr_in client;
        int temp = -1;
        c = sizeof(client);
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
        SSL *ssl;
        ssl = SSL_new(context);              // get new SSL state with context 
        SSL_set_fd(ssl, client_sock);      // set connection socket to SSL state

        char IPs[64];
        inet_ntop(AF_INET,&client.sin_addr.s_addr,IPs,sizeof(IPs)); 
        char client_message[2000];

        if ( SSL_accept(ssl) == -1 ) //do SSL-protocol accept
            ERR_print_errors_fp(stderr);
        else{

            while((read_size = SSL_read(ssl, client_message, sizeof(client_message))) > 0){

                char *name = malloc(100 * sizeof(char));
                char *message = malloc(100 * sizeof(char));
                int userID = 0;
                char *port = malloc(100 * sizeof(char));
                char temp_msg[50];
                //register
                if(strstr(client_message, "REGISTER#") != NULL){
                    printf("regist:%s\n",client_message);
                    userID = userNum;
                    memset(message, '\0', sizeof(&message));
                    message = regist(client_message,userID);
                    if(strcmp(message,"100 OK")){
                        userID = userNum;
                        userNum++;
                    }
                    SSL_write(ssl, message, strlen(message));
                    memset(client_message, '\0', sizeof(client_message));
                     //------first deposit
                    char message_1[100];
                    //recieve msg
                    length = 0;
                    memset(client_message,'\0',sizeof(client_message));
                    length = SSL_read(ssl, client_message, sizeof(client_message));
                    printf("%s\n",client_message);
                    first_deposit(client_message,userID);
                    continue;
                }
                
                else if(check(client_message) == 1){

                    char *first = malloc(3 * sizeof(char));
                    char *second = malloc(3 * sizeof(char));
                    char *third = malloc(3 * sizeof(char));
                    first = "angela";
                    second = "1000";
                    third = "hahaha";
                    // int a = 0;
                    // int index = 0;
                    // char *name = malloc(3 * sizeof(char));
                    // for(int i = 0;i<3;i++){
                    //     name[i] = malloc(100 * sizeof(char));
                    // }
                    // for(int i = 0;i<3;i++){
                    //      while (a < sizeof(client_message)){
                    //         if (client_message[a] == "#"){
                    //             index = a+1;
                    //             break;
                    //         }
                    //         else if(a >= index){
                    //             int t = a-index;
                    //             name[i][t] = client_message[a];
                    //         }
                    //     }
                    //     a++;
                    // }
                    char success = transaction(first,third, atoi(second));
                    if (success == '1'){
                        printf("transaction successfully\n");
                    }
                    else{
                        printf("transaction fail due to not sufficient funds\n");
                    }
                    continue;
                }     
                //login     
                else if(find(split_str(client_message)) != -1){
                    printf("client%s\n",client_message);
                    int temp = find(split_str(client_message));
                    userID = temp;
                    printf("temp%d\n",temp);
                    login_user(client_message,temp);
                    // ----get online_list
                    memset(message, '\0', sizeof(&message));
                    strcpy(message,acoountPrint(temp));
                    strcat(message,get_list(IPs,userID));
                    printf("%s\n",message);
                    SSL_write(ssl, message, strlen(message));
                    // write(client_sock , message , strlen(message));
                    // send(client_sock,message,sizeof(&message),0);
                    memset(message, 0, sizeof(&message));
                    memset(client_message, 0, sizeof(client_message));
                    read_size = 0;
                    continue;

                }
                //list
                else if((strcmp(client_message, "List") == 0)){
                    printf("Ask for list");
                    memset(message, 0, sizeof(&message));
                    strcpy(message,get_list(IPs,userID));
                    SSL_write(ssl, message, strlen(message));
                    memset(message, 0, sizeof(&message));
                    memset(client_message, 0, sizeof(client_message));
                    read_size = 0;
                    continue;
                }
                else if((strcmp(client_message, "Exit") == 0)){
                    char message[20];
                    printf("goodbye%s\n",accountData[userID].name);
                    strcat(message, "Bye!\n");
                    SSL_write(ssl, message, strlen(message));
                    memset(&message, '\0', sizeof(message));
                    accountData[userID].online = 0;
                    memset(client_message, 0, sizeof(client_message));
                    read_size = 0;
                    break;
                    continue;

                }
                else{
                    char message[20];
                    strcpy(message, "220 AUTH_FAIL\n");
                    SSL_write(ssl, message, strlen(message));
                    memset(&message, 0, sizeof(message));
                    memset(client_message, 0, sizeof(client_message));
                    continue;
                }

            }
        }
    }   
} 
int main(int argc , char *argv[]){
    SSL_library_init();
    SSL_load_error_strings();
    context = InitServerCTX();  //initialize SSL 
    LoadCertificates(context, "mycert.pem", "mykey.pem"); // load certs and key

    struct sockaddr_in server;
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1){
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 3500 );
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
     
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    
    pthread_t thread_id[all_thread];
    pthread_attr_t attr;

    printf("Open SSL server socket, port:%s\n","3500");
    for (int i = 0; i < all_thread; i++){
        pthread_attr_init(&attr);
        pthread_create(&thread_id[i], &attr, connection_handler, (void*)(thread_id));
    }
    
    pthread_join(thread_id[0], NULL);
    SSL_CTX_free(context);         // release context

    return 0;
}