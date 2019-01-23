#include <iostream>
#include <cstdio>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <queue>
#include <sstream>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BACKLOG 5
#define BLEN 1000   
#define NUM_THREADS 10
#define LISTSIZE 1024

using namespace std;

SSL_CTX *ctx;
SSL *ssl[BLEN];

char CLIENT_CERT[BLEN] = "./clientcert.pem";
char CLIENT_PRI[BLEN] = "./clientkey.pem";

string list[LISTSIZE];    //register list
string online[LISTSIZE]; 
int sockfd = 0;
int new_fd = 0;
int indivi = 0;
char buf[BLEN];
int regiCount = 0;    //num of register
int onlineCount = 0;    //num of online



class client
{
public:
    bool online;
    string name;
    string money;
    string portNum;
    string ipAddress;
    
};



struct sockaddr_in serv_addr, cli_addr;

vector<client> clientList;
queue<int> clientid;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;   // thread
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;   // user
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;   // online

void LoadCertificatesServer(SSL_CTX* ctx)
{

    if (SSL_CTX_use_certificate_file(ctx, CLIENT_CERT, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx, CLIENT_PRI, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    
    if (!SSL_CTX_check_private_key(ctx))
    {
        ERR_print_errors_fp(stderr);
        cout << "Private key does not match the public certification." << endl;
        abort();
    }
    
    SSL_CTX_load_verify_locations(ctx, "cacert.pem", NULL);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, 4);
}

SSL_CTX* initCTX(void)
{   
    const SSL_METHOD *ssl_method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ssl_method = TLSv1_server_method();
    ctx = SSL_CTX_new(ssl_method);
    if(ctx == NULL)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

string trans(int i)
{
    stringstream ss;
    ss << i;
    return ss.str();
}

void registerOption(string r_msg, SSL *ssl)
{
    //cout << r_msg << "register\n";   

    r_msg = r_msg.substr(8); 
    //cout << r_msg << endl;   
    int pos = r_msg.find("@");
    string name = r_msg.substr(0, pos);
    //cout << name << endl;
    string money = r_msg.substr(pos + 1);
    //cout << money << endl;
    //cout << clientList.size() << endl;
    client c1;
    pthread_mutex_lock(&mutex2);
   	c1.online = false;
    c1.name = name;
    c1.money = money;
    //cout << c1.name << " " << c1.money << endl;

	clientList.push_back(c1);
	pthread_mutex_unlock(&mutex2);
	//cout << clientList.front().name << endl;

    
    //cout << clientList.size() << endl;
    string s_msg;
    s_msg = " 100 OK\n";    // return successful signal
    cout << name << " new rigister\n";
    SSL_write(ssl, s_msg.c_str(), s_msg.length());
}

string loginOption(string r_msg, SSL *ssl)
{
    //cout << r_msg << endl;
    cout << "login\n";
    string temp;
    temp = r_msg.substr(5);
    //cout << temp << endl;
    int pos = temp.find("#");
    string curName = temp.substr(0, pos);
    //cout << curName << endl;
    string port = temp.substr(pos+1);
    //cout << port << endl;
    string money;
    bool login = 0;
    for(int i=0; i<LISTSIZE; i++)
    {
        if(clientList[i].name == curName)
        {
            //cout << "yes"<< endl;
            login = 1;
            pthread_mutex_lock(&mutex2);
            char temp[BLEN];
            inet_ntop(AF_INET, &cli_addr.sin_addr, temp, INET_ADDRSTRLEN);    // transfer IP address
            string IP(temp);

            clientList[i].portNum = port;
            clientList[i].ipAddress = IP;
            clientList[i].online = true;
            money = clientList[i].money;
            //cout << clientList[i].portNum << " " << clientList[i].ipAddress << " " << clientList[i].online << " " << clientList[i].money << endl;
            pthread_mutex_unlock(&mutex2);

            pthread_mutex_lock(&mutex3);
            online[i] = curName + "#" + IP + "#" + port + "\n";
            //cout << online[i] << endl;
            onlineCount++;
            pthread_mutex_unlock(&mutex3);

            break;      
        }
    }

    string s_msg;
    if(login == 1)    // login successful
    {
        s_msg = "AccountBalance: " + money + "\n";
        //cout << s_msg << endl;
        int n = SSL_write(ssl, s_msg.c_str(), s_msg.length());
        //cout << n << endl;
        cout << curName << " new login\n";
    }
    else    // havn't been register
    {
        s_msg = "220 AUTH_FAIL\n";
        SSL_write(ssl, s_msg.c_str(), s_msg.length());
        cout << "fail login\n";
    }
    
    return curName;
}

void findOption(string r_msg, SSL* ssl)
{
    bool find = false;

    string s_msg;
    r_msg = r_msg.substr(16);
    int pos = r_msg.find("&");
    string DesPortNum;
    DesPortNum = r_msg.substr(0, pos);
    for (int i = 0; i < clientList.size(); i ++)
    {
    	if (clientList[i].portNum == DesPortNum)
    	{
    		find = true;
    		break;
    	}
    }
    if (find == true)
    {
    	s_msg = "1";
    	SSL_write(ssl, s_msg.c_str(), s_msg.length());
    }
    else
    {
    	s_msg = "0";
    	SSL_write(ssl, s_msg.c_str(), s_msg.length());
    }
        
}

void transferOption(string r_msg, string curName, SSL *ssl)
{
	string destName = curName;
	//cout << "dest name" << destName << endl;
	r_msg = r_msg.substr(8);
	int pos = r_msg.find("&");
	string srcPort = r_msg.substr(0, pos);
	//cout << "src name" << srcPort << endl;
	string money = r_msg.substr(pos + 1);
	//cout << "money to send " << money << endl;
	string ttemp;

	for(int i = 0; i < clientList.size(); i++)
	{
		if (srcPort == clientList[i].name)
		{
			pthread_mutex_lock(&mutex2);
			int num = stoi(clientList[i].money);
			//cout << "src money" << num << endl;
			if (num < stoi(money))
			{
				cout << clientList[i].name <<  ": Insufficient Balance." << endl;
				string s_msg;
				s_msg = "Transfer failed, beacuse of insufficient balance. \n";
				SSL_write(ssl, s_msg.c_str(), s_msg.length());
				pthread_mutex_unlock(&mutex2);
				break;
			}
			else
			{
				int curSrcMoney = num - stoi(money);
				cout << "src " << curSrcMoney << endl;
				clientList[i].money = trans(curSrcMoney);
				pthread_mutex_unlock(&mutex2);
				for (int j = 0; j < clientList.size(); j++)
				{
					if (destName == clientList[j].name)
					{
						pthread_mutex_lock(&mutex2);
						int num2 = stoi(clientList[j].money);
						int destMoney = num2+ stoi(money);
						cout << "dest " << destMoney << endl;
						clientList[j].money = trans(destMoney);
						ttemp = clientList[j].money;
						pthread_mutex_unlock(&mutex2);
						break;
					}
				}
				string s_msg;
		    	s_msg = ttemp + "#" + "Transfer Successfully.\n";
		    	SSL_write(ssl, s_msg.c_str(), s_msg.length());
		    
			}
		}
	}

    	
}


void listOption(string curName, SSL *ssl)
{
    cout << "list\n";
    string s_msg;
    for(int i = 0; i < clientList.size(); i++)
    {
        if(curName == clientList[i].name)
    	{
            s_msg = "AccountBalance:" + clientList[i].money + "\n";
		    string c = trans(onlineCount);
		    s_msg = s_msg + "Number of users:" + c + "\n";
			for(int i=0; i<LISTSIZE; i++)
			    s_msg = s_msg + online[i];
			SSL_write(ssl, s_msg.c_str(), s_msg.length());
			break;
        }
    }
}

string exitOption(string curName, SSL *ssl)
{
    string s_msg;
    for(int i = 0; i < clientList.size(); i++)
    {
        if(curName == clientList[i].name)
    	{
            cout << curName << " Exit\n";
            pthread_mutex_lock(&mutex2);
            clientList[i].online = false;
            onlineCount--;
            pthread_mutex_unlock(&mutex2);

            s_msg = "Bye\n";
            SSL_write(ssl, s_msg.c_str(), s_msg.length());
            curName = "";
			break;
        }
    }
    return curName;
}

void communicate(int currentClientID, char * currentClientIP, SSL *ssl)
{
	
    string curName = "";
	while(true)
	{
		//cout << "com" << endl;
		memset((char *)buf, 0, BLEN);
		int n = SSL_read(ssl, buf, sizeof(buf));
    	string r_msg(buf);
    	//cout << r_msg << endl;
	    if (n == 0)
	    {
	    	break;
	    }
	    //cout << n << endl;
		if(r_msg.find("Register") != string::npos)
	    {
	        registerOption(r_msg, ssl);
	    }

	    //list
	    else if (r_msg.find("List") != string::npos)
	    {
	        listOption(curName, ssl);
	    }

	    //exit
	    else if (r_msg.find("Exit") != string::npos)
	    {
	        curName = exitOption(curName, ssl);
	    }

	    //login
	    else if(r_msg.find("login") != string::npos)
	    {
	        curName = loginOption(r_msg, ssl);
	    }
	    
	    else if (r_msg.find("FindAccountValid") != string::npos)
	    {
	        findOption(r_msg, ssl);
	    }

	    else if (r_msg.find("Transfer") != string :: npos)
	    {
	        transferOption(r_msg, curName, ssl);
	    }

	    // else
	    // {
	    //     cout << r_msg << "\n";
	    // }
	}
}

void* request(void* arg)
{
	int threadid = -1;

    while(true)
    {
        int cliID = -1;

        char clientIP[BLEN];
        pthread_mutex_lock(&mutex1);

        if(clientid.size() <= 0)
        {
            pthread_mutex_unlock(&mutex1);
            continue;
        }

        cliID = clientid.front();
        clientid.pop();
        pthread_mutex_unlock(&mutex1);

        if(cliID <= 0)
        {
            cout << " Fail to make connect. \n" << endl;
            continue;
        }

        char temp[BLEN];  
        inet_ntop(AF_INET, &cli_addr.sin_addr.s_addr, clientIP, sizeof(clientIP));
        memset((char *)temp, 0, BLEN);
        communicate(cliID, clientIP, ssl[cliID]);

    }

}

void accept(int sockdf)
{
	while(true)
    {
        socklen_t len = sizeof(cli_addr);
        new_fd = accept(sockfd, (struct sockaddr*)&cli_addr, &len);

        ssl[new_fd] = SSL_new(ctx);
        SSL_set_fd(ssl[new_fd], new_fd);
        SSL_accept(ssl[new_fd]);
        
        string str = "Successfully connected. \n";
        cout << str << endl;
        SSL_write(ssl[new_fd], str.c_str(), str.size());

        clientid.push(new_fd);
    }
}



int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout<<"Error input\n";
        exit(0);
    }


    pthread_t threadID[NUM_THREADS];
    pthread_attr_t attr;
    

    // initialize SSL
    SSL_library_init();
    ctx = initCTX();
    LoadCertificatesServer(ctx);

    // build socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        cout << " ERROR create socket.";
        return -1;
    }

    // addr
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));

    
    // bind
    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cout<<" ERROR binding\n";
        return -1;
    }


    // listen
    listen(sockfd, BACKLOG);

    for(int i = 0; i < NUM_THREADS; i++)
    {
        pthread_attr_init(&attr);
        pthread_create(&threadID[i], &attr, request, (void*)(threadID));
    }
    
    // accept
    accept(sockfd);
    

    close(sockfd);
    SSL_CTX_free(ctx);

    return 0;
}
























































