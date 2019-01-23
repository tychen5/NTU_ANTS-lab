#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>
#include <stdbool.h>
#include <string>
#include <iostream>
#include <vector>
#include <queue>
#include "openssl/ssl.h"
#include "openssl/err.h"
using namespace std;

pthread_mutex_t mutex_users = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_waiting = PTHREAD_MUTEX_INITIALIZER;
queue<int> waiting; //store int socket_fd
struct sockaddr_in serverInfo, clientInfo;
char mycert[] = "./serv_cert.pem"; 
char mykey[] = "./serv_key.pem";

//Init server instance and context
SSL_CTX* InitServerCTX(void)
{   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc. 
    SSL_load_error_strings();   // load all error messages */
    method = TLSv1_server_method();  // create new server-method instance 
    ctx = SSL_CTX_new(method);   // create new context from method
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

//Load the certificate 
void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile){
    //New lines
    if (SSL_CTX_load_verify_locations(ctx, CertFile, KeyFile) != 1)
        ERR_print_errors_fp(stderr);
    
    if (SSL_CTX_set_default_verify_paths(ctx) != 1)
        ERR_print_errors_fp(stderr);
    //End new lines
    
    /* set the local certificate from CertFile */
    if (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if (SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if (!SSL_CTX_check_private_key(ctx))
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
    
    //New lines - Force the client-side have a certificate
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, 4);
    //End new lines
}


#define MAX_BUF 1000
SSL* ssl[MAX_BUF];
SSL_CTX *ctx;

class User
{
  private:
    string name;
    string IP;
    string port;
    int balance;
    bool online;

  public:
    User(string name){ this->name = name; this->balance = 0; this->IP = "0"; this->port = "0"; this->online = false; }
    string getName() { return this->name; }
    int getBalance() { return this->balance; }
    string getPort() { return this->port; }
    string getIP() { return this->IP; }
    bool isOnline() { return this->online; }
    void setOnline() { this->online = true;}
    void setOffline() { this->online = false; }
    void setIP(string IP) { this->IP = IP; }
    void setPort(string port) { this->port = port; }
    void deposit(int money) 
    { 
      pthread_mutex_lock(&mutex_users);
      this->balance += money; 
      cout << this->getName() << " deposit amount: " << money << "\n";
      pthread_mutex_unlock(&mutex_users);
    }
    void withdraw(int money) 
    {
      pthread_mutex_lock(&mutex_users); 
      this->balance -= money; 
      cout << this->getName() << " withdraw amount: " << money << "\n";
      pthread_mutex_unlock(&mutex_users);
    }

};

vector<User> users;

char * split_str(char * str) //split string before token '#'
{ 
  const char *tok = "#";
  char *token;
  token = strtok(str, tok); //username
  return(token);
}

int getlen(string str)
{
  for (int i = 0; i < sizeof(str); ++i)
  {
    if(str[i] == '\0')
    {
      return(i);
    } 
  }
  return(200);
}

int find(string element, string* list, int listlen)
{
  for (int i = 0; i < listlen; ++i)
  {
    if(list[i] == element)
    {
      return i; //return user ID
    }
  }
  return -1; //can't find
}


void getOnlineList(int newSocket, User user)
{
  pthread_mutex_lock(&mutex_users);
  string message;
  message = "Account balance: " + to_string(user.getBalance()) + "\n";
  // send(newSocket,message.c_str(),message.size(),0);
  
  //show list
  string list;
  int online_cnt = 0;
  for (int i = 0; i < users.size(); ++i)
  {
    // user = users[i];
    if(users[i].isOnline())
    {
      online_cnt++;
      string temp_message;
      temp_message = users[i].getName() + "#" + users[i].getIP() +  "#" + users[i].getPort() + "\n";  
      list = list +  temp_message;
    }        
  }

  //online accounts
  message = message + "number of accounts online: " + to_string(online_cnt) + "\n";

  //online list
  message = message + list;
  SSL_write(ssl[newSocket],message.c_str(),message.size());
  pthread_mutex_unlock(&mutex_users);

}

bool isRegistered(string client_name)
{
  //check if the username has been used
  pthread_mutex_lock(&mutex_users);
  for (int i = 0; i < users.size(); ++i)
  {
    if(users[i].getName() == client_name)
    {
      pthread_mutex_unlock(&mutex_users);
      return true;      
    }
  }
  pthread_mutex_unlock(&mutex_users);
  return false;
}

int findUser(string client_name)
{
  pthread_mutex_lock(&mutex_users);
  int userID = -1;
  for (int i = 0; i < users.size(); ++i)
  {
    User user = users[i];
    if(user.getName() == client_name)
    {
      userID = i;
      pthread_mutex_unlock(&mutex_users);
      return userID;     
    }
  }
  pthread_mutex_unlock(&mutex_users);
  return userID;  
}


void * socketThread(void *arg)
{
  int newSocket = -1;
  while (true) 
  {
    pthread_mutex_lock(&mutex_waiting);
    if (waiting.size() > 0) 
    {
      newSocket = waiting.front(); //get the client_fd in waiting queue
      waiting.pop();
    }
    pthread_mutex_unlock(&mutex_waiting);

    if(newSocket > 0)
    {
      int msgcnt = 0;
      char message[1024] = {0};
      int userID = -1; 
      char client_message[2000];
      string client_mes;
      string client_name;

      while(SSL_read(ssl[newSocket] , client_message ,sizeof(client_message)) > 0)
      {
        printf("No. %d msg: ", msgcnt++);
        printf("%s\n", client_message);

        //Register
        if (strstr(client_message, "REGISTER#") != NULL) 
        {

            client_mes = client_message;
            client_name = client_mes.substr(9, sizeof(client_mes));
            memset(message, '\0', sizeof(message)); 

            //check if the username has been used
            if(isRegistered(client_name))
            {
              strcpy(message, "210 FAIL\n");
              SSL_write(ssl[newSocket],message,sizeof(message));
              continue;  
            }

            //register successfully
            
            pthread_mutex_lock(&mutex_users);
            users.push_back(User(client_name));
            pthread_mutex_unlock(&mutex_users);
            // for (int i = 0; i < users.size(); ++i)
            // {
            //   cout << "users: " << users[i].getName() << " " << users[i].getBalance() << "\n";
            // }
                  
            strcpy(message, "100 OK\n");
            SSL_write(ssl[newSocket],message,sizeof(message)); 
            memset(message, '\0', sizeof(message));   
            continue;
        }

        //List
        else if((strcmp(client_message, "List\n") == 0) && users[userID].isOnline())
        {

          getOnlineList(newSocket, users[userID]);
          // memset(client_message, '\0', sizeof(client_message));
          continue;

        }
        //Exit
        else if((strcmp(client_message, "Exit\n") == 0) && users[userID].isOnline())
        {
          strcpy(message, "Bye!\n");
          SSL_write(ssl[newSocket],message,sizeof(message));
          memset(message, '\0', sizeof(message));
          users[userID].setOffline();
          cout << "User " << users[userID].getName() << " exit.\n";

        }

        // PayRequest
        else if(strstr(client_message, "PayRequest#") != NULL) //PayRequest
        {

            //parse PayRequest#payer#payAmount#payee
            string temp, payer, payAmount, payee, payeePort, reply;
            temp = strtok (client_message,"#");
            payer = strtok (NULL, "#");
            payer = payer;
            payAmount = strtok (NULL, "#");
            payee = strtok (NULL, "#");
            int transMoney = stoi(payAmount);
            bool payee_exist = false;
            bool balance_enough = false;
            int payerID = -1, payeeID = -1;
            payerID = findUser(payer);
            payeeID = findUser(payee);
            memset(client_message, '\0', sizeof(client_message));
              //check payee is online
              if( payeeID != -1 and users[payeeID].isOnline() and payee_exist == false) 
              {
                payee_exist = true;
                payeePort.assign(users[payeeID].getPort());
              }
              //check payer has enough balance
              if( payerID != -1 and users[payerID].getBalance() >= transMoney and balance_enough == false)
              {
                balance_enough = true;
              }

            if(payee_exist and balance_enough)
            {
              SSL_write(ssl[newSocket],payeePort.c_str(),payeePort.size());
              memset(message, '\0', sizeof(message));

              //Transfer
              users[payeeID].deposit(transMoney);
              users[payerID].withdraw(transMoney);

            }
            else if(not payee_exist)
            {
              reply = "FAIL: Payee is not online.\n";
              SSL_write(ssl[newSocket],reply.c_str(),reply.size());
              reply.clear();            
            }
            else
            {
              reply = "FAIL: Your account balance is not enough.\n";
              SSL_write(ssl[newSocket],reply.c_str(),reply.size());
              reply.clear();
            }
            continue;
        }

        //Login

        else
        {
          client_mes = client_message;
          client_name = split_str(client_message);
          userID = findUser(client_name);
          

          if (userID != -1) //already login
          {
            string portNum = client_mes.substr(client_name.length()+1, client_mes.length() - client_name.length() - 1);
            users[userID].setPort(portNum);
            users[userID].setIP(inet_ntoa(clientInfo.sin_addr));
            users[userID].setOnline();


            strcpy(message, "Please enter the amount you want to deposit: ");
            SSL_write(ssl[newSocket],message,sizeof(message));  
            memset(message, '\0', sizeof(message));

            //deposit
            memset(client_message, '\0', sizeof(client_message));
            SSL_read(ssl[newSocket] , client_message ,sizeof(client_message));
            int deposit_amount = atoi(client_message);
            users[userID].deposit(deposit_amount);

            getOnlineList(newSocket, users[userID]);

            continue;

          }
          else
          {
            strcpy(message, "220 AUTH_FAIL\n");
            SSL_write(ssl[newSocket],message,sizeof(message));
            memset(message, 0, sizeof(message));
          }
        }  
      }
      users[userID].setOffline();
      newSocket = -1;
    }
  }  
  printf("Exit socketThread \n");
  close(newSocket);
  pthread_exit(NULL);
}
int main(int argc , char *argv[]){


  int serverSocket, newSocket;
  struct sockaddr_in serverAddr;

  struct sockaddr_storage serverStorage;

  socklen_t addr_size;
  serverSocket = socket(PF_INET, SOCK_STREAM, 0);
  serverAddr.sin_family = AF_INET;
  int servPort = atoi(argv[1]);
  serverAddr.sin_port = htons(servPort);
  printf("Server port number: %d\n", servPort);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
  bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
  
  SSL_library_init(); //init SSL library
  printf("Initialize SSL library.\n");
  
  ctx = InitServerCTX();  //initialize SSL 
  LoadCertificates(ctx, mycert, mykey); // load certs and key

  if(listen(serverSocket,10)==0)
    printf("Listening\n");
  else
    printf("Listening error\n");

  // create thread pool
  pthread_t tid[10];
  for (int i = 0; i < 10; ++i)
  {
    if( pthread_create(&tid[i], NULL, socketThread, NULL) != 0 )
    {
      printf("Failed to create thread\n");
    }
  }

    while(1)
    {

      //Accept call creates a new socket for the incoming connection
      addr_size = sizeof serverStorage;      
      int client_fd = -1;
      while((client_fd = accept(serverSocket, (struct sockaddr *) &clientInfo, &addr_size))) 
      {

        /// SSL connect
        ssl[client_fd] = SSL_new(ctx);
        SSL_set_fd(ssl[client_fd], client_fd); // set connection socket to SSL state
        if (SSL_accept(ssl[client_fd])  < 0)
        {
          cout << "SSL connect error!\n";
        }
        else 
        {
          string success = "Connected accepted. Welcome to Yi Chin's server.\n";

          SSL_write (ssl[client_fd], success.c_str(), success.length());
        }

        pthread_mutex_lock(&mutex_waiting);
        waiting.push(client_fd);
        pthread_mutex_unlock(&mutex_waiting);


      }     

    }

  close(serverSocket); //close the socket of server

  return 0;
}