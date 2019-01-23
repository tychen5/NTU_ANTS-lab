/*
    C socket server example, handles multiple clients using threads
    Compile
    gcc server.c -lpthread -o server
*/
 
#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
 
//the thread function
char* user_list[10] = {0};
int usercnt = 0;
int money_list[10] = {};
char* IP_List[10] = {};
char* port_list[10] = {};
int online_list[10] = {};
int online_user_cnt = 0;
void *connection_handler(void *);

int main(int argc , char *argv[])
{
    int socket_desc , client_sock ;
    struct sockaddr_in server , client;
    int c;
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 3500 );
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 5);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
     
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    pthread_t thread_id[10];

    //Accept and incoming connection
    
	
    int i = 0;
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {   
        puts("Connection success");
        
        while( pthread_create( &thread_id[i] , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            printf("Failed to create thread\n");
            i++;
        }
        //Now join the thread , so that we dont terminate before the thread
        // pthread_join( thread_id , NULL);
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
char * split_str(char * str) //split string before token '#'
{ 
  const char *tok = "#";
  char *token;
  token = strtok(str, tok); //username
  return(token);
}
int find(char *element, char ** list, int listlen)
{

  for (int i = 0; i < listlen; ++i)
  {
    if(strcmp(list[i], element) == 0)
    {
      return(i); //return user ID
    }
  }
  return(-1); //can't find
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    char *message , client_message[2000], *name = malloc(100 * sizeof(char));
    int userID = 0;
    int login = 0;
    while(recv(sock , client_message ,1024 , 0) > 0){
        //register
        if(strstr(client_message, "REGISTER#") != NULL){
            printf("%s\n",client_message);
            int c = 0;
            while (c < sizeof(client_message)) {
              name[c] = client_message[9+c];
              c++;
            }
            
            if (find(name, user_list, usercnt) != -1){
                memset(&message, '\0', sizeof(message));
                message = "210 FAIL\n";
                send(sock,message,sizeof(message),0);
                memset(client_message, '\0', sizeof(client_message));
                continue;
            }
            userID = usercnt;
            user_list[userID] = &name[0]; 
            usercnt++;
            memset(&message, '\0', sizeof(message));
            message = "100 OK\n";
            send(sock,message,sizeof(message),0); 
            memset(client_message, '\0', sizeof(client_message));
            continue;
        }
        //list
        else if((strcmp(client_message, "List") == 0) && login == 1){
            printf("ask list\n");
            char message_1[200] = {};
            strcat(message_1,"Account balance:");
            sprintf(message_1, "%d", money_list[userID]); 
            send(sock,message_1,sizeof(message_1),0);
            memset(message_1, '\0', sizeof(message_1));

            char list[1024] = {};
            int online_cnt = 0;
            for (int i = 0; i < 10; i++)
            {

              if(online_list[i] == 1)
              {
                online_cnt++;
                char message_list[1024];
                memset(message_list, '\0', sizeof(message_list));
                // sprintf(port, "%u", ntohs(clientInfo.sin_port));
                snprintf(message_list, 1024, "%s#%s\r\n", user_list[i], port_list[i]); //online list
                strcat(list, message_list);   
              }        
            }

            sprintf(message_1, "number of accounts online: %d\n", online_cnt); 
            send(sock,message_1,sizeof(message_1),0); 
            // write(sock , message_1 , strlen(message_1));
            memset(message_1, 0, sizeof(message_1));

            //online list
            strcpy(message_1, list);
            send(sock,message_1,sizeof(message_1),0);
            // write(sock , message_1 , strlen(message_1));
            memset(message_1, 0, sizeof(message_1));
            memset(client_message, 0, sizeof(client_message));



            continue;
        }
        
        //login

        else if(find(client_message,user_list,usercnt) && login == 0){
            char * login_name = malloc(20 * sizeof(char));
            login = 1;
            login_name = split_str(client_message);
            userID = find(login_name, user_list, usercnt);
            printf("login : %s\n",login_name);
            int a = 0;
            char *port = malloc(100 * sizeof(char));
            while (a < sizeof(client_message)) {
              port[a] = client_message[sizeof(login_name)-1+a];
              a++;
            }

            port_list[userID] = port;
            online_list[userID] = 1;
            online_user_cnt++;   
            memset(&message, '\0', sizeof(message));

            message = "Enter the amount you want to deposit:";
            write(sock , message , strlen(message));

            memset(client_message, '\0', sizeof(client_message));
            recv(sock , client_message ,sizeof(client_message) , 0);

            int deposit_amount = atoi(client_message);
            printf("deposit amount: %d\n",  deposit_amount);
            

            char message_1[200] = {};
            money_list[userID] = deposit_amount;
            strcat(message_1,"Account balance:");
            strcat(message_1,client_message);
            write(sock , message_1 , strlen(message_1));
            memset(message_1, '\0', sizeof(message_1));

            char list[1024] = {};
            int online_cnt = 0;
            for (int i = 0; i < 10; i++)
            {

              if(online_list[i] == 1)
              {
                online_cnt++;
                char message_list[1024];
                memset(message_list, '\0', sizeof(message_list));
                // sprintf(port, "%u", ntohs(clientInfo.sin_port));
                snprintf(message_list, 1024, "%s#%s\r\n", user_list[i], port_list[i]); //online list
                strcat(list, message_list);   
              }        
            }

            sprintf(message_1, "number of accounts online: %d\n", online_cnt); 
            write(sock , message_1 , strlen(message_1));
            memset(message_1, 0, sizeof(message_1));

            //online list
            strcat(message_1, list);
            write(sock , message_1 , strlen(message_1));
            memset(message_1, 0, sizeof(message_1));
            memset(client_message, 0, sizeof(client_message));
            continue;

        }
        else if((strcmp(client_message, "Exit") == 0) && login == 1){
            char message[20];
            printf("goodbye%s\n",user_list[userID]);
            strcat(message, "Bye!\n");
            send(sock,message,sizeof(message),0);
            memset(&message, '\0', sizeof(message));
            online_list[userID] = 0;
            online_user_cnt--;
            memset(client_message, 0, sizeof(client_message));

        }

        else
        {
            char message[20];
            strcpy(message, "220 AUTH_FAIL\n");
            send(sock,message,sizeof(message),0);
            memset(&message, 0, sizeof(message));
            memset(client_message, 0, sizeof(client_message));
        }
            

        
        


    
    }
    // usercnt++;
    // message = "wellcome";
    // write(sock , message , strlen(message));
    // //Send some messages to the client
    // recv(sock , client_message , 2000 , 0);
    // printf("%s",client_message);
    
    // // name[c] = '\0';
    // // user_list[usercnt]
    // memset(client_message, '\0', sizeof(client_message));
    // printf("%s\n",name);

    // write(sock , message , strlen(message));
    //Receive a message from client
  //   while( (read_size = ) > 0 )
  //   {
  //       //end of string marker
		// client_message[read_size] = '\0';
		
		// //Send the message back to client
  //       write(sock , client_message , strlen(client_message));
		
		// //clear the message buffer
		// memset(client_message, 0, 2000);
  //   }
     
    // if(read_size == 0)
    // {
    //     puts("Client disconnected");
    //     fflush(stdout);
    // }
    // else if(read_size == -1)
    // {
    //     perror("recv failed");
    // }
         
    return 0;
} 