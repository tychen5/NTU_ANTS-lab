#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>		//Unix系統服務函數原型
#include <sys/types.h>	//Unix系統數據類型定義
#include <sys/socket.h>	//提供socket函數與數據結構
#include <netinet/in.h>	//定義數據結構sockaddr_in
#include <arpa/inet.h>

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
    info.sin_addr.s_addr = inet_addr(argv[1]);	     //inet_addr()負責將字串型式的IP轉換為整數型式的IP
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
        int instruct;
        scanf("%d", &instruct);
        /*Register*/
        if(instruct == 1){        
            char name[50] = {};
            printf("Enter the name you want to register: ");
            scanf("%s", &name[0]);

            char register_message[1000] = {};
            snprintf(register_message, sizeof(register_message), "%s%s%s", "REGISTER#", name, "\n");
            send(sd, register_message, sizeof(register_message),0);

            char register_receiveMessage[100] = {};
            recv(sd, register_receiveMessage, sizeof(register_receiveMessage), 0);
            printf("%s\n", register_receiveMessage);
        }

        /*Login*/
        else if(instruct == 2){
            char name[50] = {};
            char port_n[10] = {};
            printf("Enter your name: ");
            scanf("%s", &name[0]);
            printf("Enter the port number: ");
            scanf("%s", &port_n[0]);

            char login_message[256] = {};
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
        else{
             printf("Please type the right number!\n\n");
        }
    }

    bool b = true;
	/*send message to server*/
    while(b){
        printf("Enter the number of actions you want to take.\n1 to ask for the latest list, 8 to Exit: ");
        int instruct;
        scanf("%d", &instruct);

        /*online-list*/
        if(instruct == 1){
            char message[] = {"List\n"};
            char receiveMessage[200] = {};
            send(sd, message, sizeof(message),0);
            for(int i = 0 ; i < 2; i++){
                recv(sd, receiveMessage, sizeof(receiveMessage), 0);
                printf("%s",receiveMessage);
                memset(receiveMessage, 0, sizeof(receiveMessage));
            }
            printf("\n");

        }

        /*end program*/
    	else if(instruct == 8){
            char message[] = {"Exit\n"};
            char receiveMessage[200] = {};
            send(sd, message, sizeof(message),0);
            recv(sd, receiveMessage, sizeof(receiveMessage), 0);
            printf("%s\n",receiveMessage);
            memset(receiveMessage, 0, sizeof(receiveMessage));
            b = false;
        }

        else{
             printf("Please type the right number!\n\n");
        }
	}

    close(sd);
    return 0;
}
