#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>


//client information
const int CLI_LEN = 20;
int count_online = 0;
int count_member = 0;
char* start = NULL;         //register:REGISTER login:name
char* second = NULL;        //register:name login:port number

char* front = NULL;         //afterlogin:instruction
char* back = NULL;          //afterlogin:payee name


char inputBuffer[256] = {};

//client strcture
struct clientlist{
    bool isonline;
    char client_name[25];
    char* online_IP;
    char online_portnum[10];
    char client_account[30];
};
struct clientlist clientlist[CLI_LEN];

//for new thread
struct arg_thread{
    int newsd;
    char* client_IP;
};





//new thread
void* socketThread(void* arg)
{
    struct arg_thread* args = arg;
    int forclient_sd = args->newsd;
    char* clientIP = args->client_IP;


    //accept client connection
    char connectionmessage[] = {"Connection Accepted\n"};
    send(forclient_sd, connectionmessage, sizeof(connectionmessage), 0);
    printf("Connection with: %s\n",clientIP);

    char name[32] = {};
    char tmp_inputBuffer[256] = {};
    bool login = false;     //at register and login mode


    while(true){
        memset(inputBuffer, 0, sizeof(inputBuffer));
        memset(tmp_inputBuffer, 0, sizeof(tmp_inputBuffer));
        recv(forclient_sd, inputBuffer, sizeof(inputBuffer),0);
        strcpy(tmp_inputBuffer, inputBuffer);
        printf("Get new message: %s\n",tmp_inputBuffer);

        //********------------------Register&Login------------------********//
        if(login == false){

            start = strtok(tmp_inputBuffer, "#");
            second = strtok(NULL, "#");
            second[strlen(second)-1] = '\0';

            //-----------------------register-----------------------//
            if(strcmp(start, "REGISTER") == 0){
                printf("REGISTER_MODE\n");

                char* reg_name = strtok(second, "$");
                char* money = strtok(NULL, "$");

                for(int i = 0; i < count_member; i++){
                    if(strcmp(reg_name, clientlist[i].client_name) == 0){
                        printf("REGISTER_FAILED:Already have this client name.\n");
                        //register fail
                        char errormessage[] = {"210 FAIL\n"};
                        send(forclient_sd, errormessage, sizeof(errormessage), 0);
                        continue;
                    }
                }

                //too many member 
                if(count_member > CLI_LEN){
                    printf("REGISTER_FAILED: Too many member.\n");
                    //register fail
                    char errormessage[] = {"210 FAIL\n"};
                    send(forclient_sd, errormessage, sizeof(errormessage), 0);
                }


                
                else{
                    //add to member list
                    strcpy(name, reg_name);

                    //find empty space to put the clientinfo
                    for(int i = 0; i < CLI_LEN; i++){
                        if(clientlist[i].isonline == false){
                            strcpy(clientlist[i].client_name, name);
                            strcpy(clientlist[i].client_account, money);
                            break;
                        }
                    }
                    count_member++;



                    //register sucecced
                    char successmessage[] = {"100 OK\n"};
                    send(forclient_sd, successmessage, sizeof(successmessage), 0);
                    printf("REGISTER_SUCCEED: %s with $%s\n",name, money);
                }
            }

            //-----------------------login-----------------------//
            else{
                printf("LOGIN_MODE\n");
                int who;
                //find whether he has the same name
                for(int i = 0; i < count_member; i++){
                    if(strcmp(start, clientlist[i].client_name) == 0){
                        strcpy(name, clientlist[i].client_name);
                        who = i;
                        break;
                    }
                }
                if(strcmp(start, name) == 0){
                    

                    //change clientinfo
                    clientlist[who].online_IP = clientIP;
                    strcpy(clientlist[who].online_portnum, second);
                    clientlist[who].isonline = true;
                    count_online++;


                    //check online people
                    printf("<OnlineList>\n");
                    for(int i = 0; i < count_member; i++){
                        if(clientlist[i].isonline == true){
                            printf("Name: %s|",clientlist[i].client_name);
                            printf(" Account: %s|",clientlist[i].client_account);
                            printf(" IP:%s|",clientlist[i].online_IP);
                            printf(" PN:%s\n",clientlist[i].online_portnum);
                        }

                    }

                    //send account message
                    char accountmessage[50] = {};
                    snprintf(accountmessage, sizeof(accountmessage), "%s%s%d%s",clientlist[who].client_account ,"\nnumber of accounts online: ", count_online, "\n");
                    send(forclient_sd, accountmessage, sizeof(accountmessage), 0);
                    

                    //send online message
                    char ONM[100] = {};
                    //online list
                    for(int j = 0; j < count_member; j++){
                        if(clientlist[j].isonline == true){
                            strcat(ONM, clientlist[j].client_name);
                            strcat(ONM, "#");
                            strcat(ONM, clientlist[j].online_IP);
                            strcat(ONM, "#");
                            strcat(ONM, clientlist[j].online_portnum);
                            strcat(ONM, "\n");
                        }
                    }
                    send(forclient_sd, ONM, sizeof(ONM), 0);

                    //login complete
                    printf("LOGIN_SUCCEED: %s| %s\n",name, clientlist[who].online_portnum);


                    login = true; 
                }
                else{
                    printf("LOGIN_FAIL: %s\n", start);
                    char errormessage[] = {"220 AUTH_FAIL\n"};
                    send(forclient_sd, errormessage, sizeof(errormessage), 0);
                }
            }
        }

        //********------------------List&End------------------********//
        else{
            printf("FUNCTION_MODE\n");

            front = strtok(tmp_inputBuffer, "#");
            back = strtok(NULL, "#");
            int who;
            for(int i = 0; i < count_member; i++){
                if(strcmp(name, clientlist[i].client_name) == 0){
                    who = i;
                }
            }

            //-----------------------onlinelist-----------------------//
            //Client want online list
            if(strcmp(front, "List") == 0){
                printf("SENDINGLIST_MODE\n");


                //send account message
                char accountmessage[50] = {};
                snprintf(accountmessage, sizeof(accountmessage), "%s%s%d%s",clientlist[who].client_account ,"\nnumber of accounts online: ", count_online, "\n");
                send(forclient_sd, accountmessage, sizeof(accountmessage), 0);
                printf("complete sending account number.\n");
                    

                //send online message
                char ONM[100] = {};
                //online list
                for(int j = 0; j < count_member; j++){
                    if(clientlist[j].isonline == true){
                        strcat(ONM, clientlist[j].client_name);
                        strcat(ONM, "#");
                        strcat(ONM, clientlist[j].online_IP);
                        strcat(ONM, "#");
                        strcat(ONM, clientlist[j].online_portnum);
                        strcat(ONM, "\n");
                    }
                }
                send(forclient_sd, ONM, sizeof(ONM), 0);

                printf("complete sending online list.\n");
            }


            //-----------------------END-----------------------//
            //Client want end
            else if(strcmp(front, "Exit") == 0){
                //send bye message
                char byemessage[15] = {"Bye\n"};
                send(forclient_sd, byemessage, sizeof(byemessage), 0);

                for(int k = 0; k < count_member; k++){
                    if(strcmp(clientlist[k].client_name, name) == 0){
                        memset(clientlist[k].online_portnum, 0, sizeof(clientlist[k].online_portnum));
                        clientlist[k].online_IP = NULL;                
                        clientlist[k].isonline = false;
                        break;
                    }
                }

                printf("Disconnect with %s\n", name);
                login = false;
                count_online --;

                printf("End SocketThread\n");
                close(forclient_sd);
                pthread_exit(NULL);
            }


            //-----------------------pay-----------------------//
            //Client want payee ip port num
            else if(strcmp(front, "Pay") == 0){
                printf("Asking_PayeeInfo_MODE\n");
                back = strtok(back, "\n");
                int payee_index = -1;

                for(int i = 0; i < count_member; i++){
                    if(strcmp(back, clientlist[i].client_name) == 0){
                        payee_index = i;
                        break;
                    }
                }


                //send payee id &portnum
                char payeemessage[50] = {};
                if (payee_index != -1)
                    snprintf(payeemessage, sizeof(payeemessage), "%s%s%s%s",clientlist[payee_index].online_IP ,"#", clientlist[payee_index].online_portnum, "\n");
                else
                    snprintf(payeemessage, sizeof(payeemessage), "%s","This member does not exist.\n");

                send(forclient_sd, payeemessage, sizeof(payeemessage), 0);
                printf("complete sending payee ID & portnum.\n");
            }

            //-----------------------pay & receive-----------------------//
            //Client want change account num
            else if(strcmp(front, "Change") == 0){
                printf("Change_account_MODE\n");
                char* payman = NULL;
                char* paymoney = NULL;
                payman = strtok(back, "$");
                paymoney = strtok(NULL, "$");

                //change payman account
                for(int i = 0; i < count_member; i++){
                    if(strcmp(payman, clientlist[i].client_name) == 0){
                        int total = atoi(clientlist[i].client_account);
                        total = total - atoi(paymoney);
                        char newaccount[10] = {};
                        snprintf(newaccount, sizeof(newaccount), "%d",total);
                        strcpy(clientlist[i].client_account,newaccount);
                        printf("Change %s's Account to %s\n",clientlist[i].client_name,clientlist[i].client_account);
                        break;
                    }
                }

                //change payee user money
                int total = atoi(clientlist[who].client_account);
                total = total + atoi(paymoney);
                char newaccount[10] = {};
                snprintf(newaccount, sizeof(newaccount), "%d",total);
                strcpy(clientlist[who].client_account,newaccount);
                printf("Change %s's Account to %s\n",clientlist[who].client_name,clientlist[who].client_account);
            }

            else{
                //send wrong message
                char errormessage[100] = {"Wrong Instruction\n"};
                send(forclient_sd, errormessage, sizeof(errormessage), 0);
                printf("WRONG_Message\n");
            }
        }
    }

    printf("End SocketThread\n");
    close(forclient_sd);
    pthread_exit(NULL);        
}


int main(int argc , char *argv[]){

    /*socket descriptor*/
    int sd = 0;
    sd = socket(AF_INET , SOCK_STREAM , 0);     //socket(int domain, int type, int protocol);

    /*invalid socket*/
    if (sd == -1)
        printf("Fail to create a socket.");

    /*socket connection*/
    struct sockaddr_in serverInfo;
    bzero(&serverInfo,sizeof(serverInfo));      //initalization(將struct涵蓋的bits設為0)
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;    //INADDR_ANY -> 不在乎loacl IP是什麼
    serverInfo.sin_port = htons(8700);

 
    /*bind & listen*/
    bind(sd, (struct sockaddr *)&serverInfo, sizeof(serverInfo));

    if(listen(sd,CLI_LEN) == 0)
        printf("Listening\n");
    else
        printf("NotListening\n");


    /*socket for client descriptor*/
    int forclient_sd = 0;
    struct sockaddr_in serverStorage;
    socklen_t addrlen;  

    pthread_t tid[20];
    int i = 0;

    for (int i = 0; i < CLI_LEN; i++){
        clientlist[i].isonline = false;
    }

    while(true){
        /*create thread*/
        addrlen = sizeof(serverStorage);
        forclient_sd = accept(sd, (struct sockaddr*) &serverStorage, &addrlen);

        struct arg_thread temp_arg;
        temp_arg.newsd = forclient_sd;
        temp_arg.client_IP = inet_ntoa(serverStorage.sin_addr);

        /*invalid thread*/
        if( pthread_create(&tid[i++], NULL, socketThread, (void*)&temp_arg) != 0)
            printf("Failed to create the thread!\n");

        //too many people online
        if(i >= CLI_LEN)
        {
          i = 0;
          while(i < CLI_LEN)
          {
            pthread_join(tid[i++], NULL);
          }
          i = 0;
        }
    }

    close(sd);
    return 0;
}