#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>		//Unix系統服務函數原型
#include <sys/types.h>	//Unix系統數據類型定義
#include <sys/socket.h>	//提供socket函數與數據結構
#include <netinet/in.h>	//定義數據結構sockaddr_in
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>


char port_n[10] = {};
char name[32] = {};


int main(int argc , char *argv[]){

    /*socket descriptor*/
    int sd = 0;
    sd = socket(AF_INET, SOCK_STREAM, 0);	//socket(int domain, int type, int protocol);

    /*invalid socket*/
    if (sd == -1)
        printf("Fail to create a socket.");


    /*socket connection*/
    struct sockaddr_in info;
    bzero(&info, sizeof(info));				             //initalization(將struct涵蓋的bits設為0)
    info.sin_family = PF_INET;				             //IPv4
    info.sin_addr.s_addr = inet_addr(argv[1]);	         //inet_addr()負責將字串型式的IP轉換為整數型式的IP
    info.sin_port = htons(atoi(argv[2]));							//port_number
    int err = connect(sd, (struct sockaddr *)&info, sizeof(info));

	/*fail connection*/
    if(err == -1){
        printf("Connection error\n");
        return 0;
    }

	/*receive connection message from server*/
	char connection_receiveMessage[100] = {};
	recv(sd, connection_receiveMessage, sizeof(connection_receiveMessage), 0);
	printf("%s\n", connection_receiveMessage);

    bool a = true;


    /*Register & Login*/
    while(a){

        printf("Enter 1 for Register, 2 for login: ");
        int instruct = 999;
        scanf("%d", &instruct);
        /*Register*/
        if(instruct == 1){        
            char name[32] = {};
            char account[10] = {};
            printf("Enter the name you want to register: ");
            scanf("%s", &name[0]);
            printf("How much money do you have: ");
            scanf("%s", &account[0]);

            char register_message[50] = {};
            snprintf(register_message, sizeof(register_message), "%s%s%s%s%s", "REGISTER#", name, "$", account, "\n");
            send(sd, register_message, sizeof(register_message),0);

            char register_receiveMessage[50] = {};
            recv(sd, register_receiveMessage, sizeof(register_receiveMessage), 0);
            printf("%s\n", register_receiveMessage);
        }

        /*Login*/
        else if(instruct == 2){

            
            printf("Enter your name: ");
            scanf("%s", &name[0]);
            printf("Enter the port number: ");
            scanf("%s", &port_n[0]);

            char login_message[50] = {};
            snprintf(login_message, sizeof(login_message), "%s%s%s%s", name, "#", port_n, "\n");
            send(sd, login_message, sizeof(login_message),0);
            
            char login_receiveMessage[100] = {};

            for(int i = 0 ; i < 2; i++){
                recv(sd, login_receiveMessage, sizeof(login_receiveMessage), 0);
                printf("%s",login_receiveMessage);
                if(strcmp(login_receiveMessage,"220 AUTH_FAIL\n") != 0)
                    a = false;
                else
                    break;
                memset(login_receiveMessage, 0, sizeof(login_receiveMessage));
            }
            printf("\n");
        }
        else if(instruct != 999 && instruct != 1 &&instruct != 2){
             printf("Please type the right number!\n\n");
        }
    }

    bool b = true;
	/*---------------------send message to server---------------------*/    
    while(b){
            printf("Enter the number of actions you want to take.\n1 to ask for the latest list, 2 to Pay, 3 to Receive money, 8 to Exit: ");
            int instruct = -1;
            scanf("%d", &instruct);

            /*online-list*/
            if(instruct == 1){
                char message[] = {"List#\n"};
                char receiveMessage[100] = {};
                send(sd, message, sizeof(message),0);
                for(int i = 0 ; i < 2; i++){
                    recv(sd, receiveMessage, sizeof(receiveMessage), 0);
                    printf("%s",receiveMessage);
                    memset(receiveMessage, 0, sizeof(receiveMessage));
                }
                printf("\n");

            }

            /*pay*/
            if(instruct == 2){
                printf("\n<!>Ask the payee user to turn on receive function first<!>\n");
                char message[50] = {};
                char money[32] = {};
                char payee[32] = {};
                char receiveMessage[100] = {};

                printf("Enter the payee name: ");
                scanf("%s", &payee[0]);
                printf("Enter the pay amount: ");
                scanf("%s", &money[0]);

                //ask payee id
                snprintf(message, sizeof(message), "%s%s%s", "Pay#",payee, "\n");
                send(sd, message, sizeof(message),0);



                char payee_info[50] = {};

                char* payee_ip = NULL;
                char* payee_pn = NULL;

                recv(sd, receiveMessage, sizeof(receiveMessage), 0);
                strcpy(payee_info, receiveMessage);
                memset(receiveMessage, 0, sizeof(receiveMessage)); 

                if(strcmp(payee_info, "This member does not exist.\n") == 0){
                    printf("%s\n",payee_info);
                }
                else{
                    payee_ip = strtok(payee_info, "#");
                    payee_pn = strtok(NULL, "#");
                    payee_pn = strtok(payee_pn, "\n");

                    //prepare to connect with payee
                    /*socket descriptor*/
                    int sd_pay = 0;
                    sd_pay = socket(AF_INET, SOCK_STREAM, 0);   //socket(int domain, int type, int protocol);

                    /*invalid socket*/
                    if (sd_pay == -1){
                        printf("Fail to Pay.\n");
                        continue;
                    }

                    /*socket connection*/
                    struct sockaddr_in info2;
                    bzero(&info2, sizeof(info2));                          //initalization(將struct涵蓋的bits設為0)
                    info2.sin_family = PF_INET;                           //IPv4
                    info2.sin_addr.s_addr = inet_addr(payee_ip);           //inet_addr()負責將字串型式的IP轉換為整數型式的IP
                    info2.sin_port = htons(atoi(payee_pn));                //port_number


                    int err2 = connect(sd_pay, (struct sockaddr *)&info2, sizeof(info2));

                    /*fail connection*/
                    if(err2 == -1){
                        printf("Fail to Pay.\n");
                        continue;
                    }

                    /*receive connection message from payee*/
                    char connection_receiveMessage2[100] = {};
                    recv(sd_pay, connection_receiveMessage2, sizeof(connection_receiveMessage2), 0);
                    printf("%s\n", connection_receiveMessage2);



                    char moneymessage[50] = {};

                    //send pay request to payee user
                    snprintf(moneymessage, sizeof(moneymessage), "%s%s%s%s", name, "#",money, "\n");
                    send(sd_pay, moneymessage, sizeof(moneymessage),0);

                    //receive ACK
                    char ackmessage[100] = {};
                    recv(sd_pay, ackmessage, sizeof(ackmessage), 0);
                    printf("%s\n", ackmessage);

                    //disconnect with payee
                    close(sd_pay);
                    
                }
            }


            else if(instruct == 3){
                //create socket to listen to other client
                /*socket descriptor*/
                int sd_lis = 0;
                sd_lis = socket(AF_INET , SOCK_STREAM , 0);     //socket(int domain, int type, int protocol);

                /*invalid socket*/
                if (sd_lis == -1)
                    printf("Can't use pay function.\n");

                /*socket connection*/
                struct sockaddr_in serverInfo;
                bzero(&serverInfo,sizeof(serverInfo));      //initalization(將struct涵蓋的bits設為0)
                serverInfo.sin_family = AF_INET;
                serverInfo.sin_addr.s_addr = INADDR_ANY;    //INADDR_ANY -> 不在乎loacl IP是什麼
                serverInfo.sin_port = htons(atoi(port_n));

                /*bind & listen*/
                bind(sd_lis, (struct sockaddr *)&serverInfo, sizeof(serverInfo));
                if(listen(sd_lis,1) == 0)
                    printf("\nWaiting for connection for receiving money...\n");
                else
                    printf("Fail Receive.\n");
                int forclient_sd = 0;
                struct sockaddr_in serverStorage;
                socklen_t addrlen;

                addrlen = sizeof(serverStorage);
                forclient_sd = accept(sd_lis, (struct sockaddr*) &serverStorage, &addrlen);
                char* clientIP = inet_ntoa(serverStorage.sin_addr);

                //accept client connection
                char connectionmessage[] = {"Connection Accepted\n"};
                send(forclient_sd, connectionmessage, sizeof(connectionmessage), 0);

                //receive message
                char inputBuffer[256] = {};
                char* pay_name = NULL;
                char* how_much = NULL;
                recv(forclient_sd, inputBuffer, sizeof(inputBuffer),0);
                
                pay_name = strtok(inputBuffer, "#");
                how_much = strtok(NULL, "#");
                how_much = strtok(how_much, "\n");
                printf("Receive $%s from %s\n",how_much,pay_name);

                //send message to server
                char change_message[50] = {};
                snprintf(change_message, sizeof(change_message), "%s%s%s%s%s", "Change#",pay_name,"$",how_much, "\n");
                send(sd, change_message, sizeof(change_message),0);

                //send message to client
                char certain[20] = {};
                snprintf(certain, sizeof(certain), "%s", "Transfer succeed!\n");
                send(forclient_sd, certain, sizeof(certain),0);

                //disconnect with client
                printf("Disconnect with %s\n\n",pay_name);
                close(forclient_sd);

            }

            /*end program*/
            else if(instruct == 8){
                char message[] = {"Exit#\n"};
                char receiveMessage[32] = {};
                send(sd, message, sizeof(message),0);
                recv(sd, receiveMessage, sizeof(receiveMessage), 0);
                printf("%s\n",receiveMessage);
                memset(receiveMessage, 0, sizeof(receiveMessage));
                b = false;
            }

            else if(instruct != -1 && instruct != 1 && instruct != 2 && instruct != 3 &&instruct != 8){
                 printf("Please type the right number!\n\n");
            }

	}

    close(sd);
    return 0;
}
