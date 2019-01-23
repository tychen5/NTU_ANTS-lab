#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/sendfile.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <signal.h>
#include "process.h"
#include "threadpool.h"
#include "../util/def.h"
#include "../util/rdwrn.h"
#include "../packet/packet.h"
#include "def.h"
#include "user.h"

pthread_mutex_t lock;
OnlineUsers users;

void client_handler(void *sockfdP);
void send_msg(SSL* sslp, char *str);
SSL_CTX *newServerCtx();
int newListenfd(int port);
threadpool_t* newThreadPool();

int main(int argc, char *argv[])
{
  int sockfd = 0;
  int listenfd = 0;
	struct sockaddr_in client_addr;
	socklen_t socksize = sizeof(struct sockaddr_in);

	//parse arg
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s [port]\n\n", argv[0]);
    exit(1);
  }

	SSL_CTX *ctx = newServerCtx(); //the object about cert, key, ...etc

  threadpool_t *pool = newThreadPool();

	for (size_t i = 0; i != THREAD; i++){
		users.users[i].threadId = getThreadId(pool, i);
	}

	listenfd = newListenfd(atoi(argv[1]));

  while (1)
  {
    SSL *sslP; // the object ablout security protocol

    printf("Waiting for a client to connect...\n");
    sockfd = accept(listenfd, (struct sockaddr *)&client_addr, &socksize);

    printf("%s is connected on socket:  %d\n", inet_ntoa(client_addr.sin_addr), sockfd);

    sslP = SSL_new(ctx);
    SSL_set_fd(sslP, sockfd);

    if (SSL_accept(sslP) <= 0)
    {
			printf("cannot build up the secure connection!\n");
      SSL_free(sslP);
      close(sockfd);
    }
    else{

      printf("free thread count: %d\n", free_thread_count(pool));
      if (free_thread_count(pool) == 0)
      {
        send_msg(sslP, "server is bu5y, please wait for a second!\n\n");
      }

      while (threadpool_add(pool, &client_handler, (void *)sslP, 0) != 0)
      {
        printf("fail to add more connection! \n");
			}
    }
    printf("\n");
  }

	close(listenfd);
	SSL_CTX_free(ctx);
	exit(EXIT_SUCCESS);
}

void client_handler(void *sslP)
{
  send_msg(sslP, "welcome the b05202044 server!\n\n");

  size_t stage = STAGE_CONNECT;
	User *userP = NULL;
	pthread_t threadId = pthread_self();

	pthread_mutex_lock(&lock); 
	for (size_t i = 0; i != THREAD; i++)
	{
		if(threadId == users.users[i].threadId){
			userP = &(users.users[i]);
			break;
		}
	}
	pthread_mutex_unlock(&lock);

	if(!userP){
		fprintf(stderr, "threadId problems!\n");
	}

  while (stage == STAGE_CONNECT)
  {
    PacketRequest *reqP = recvReq(sslP);
		stage = process(reqP, sslP, userP);

		if (reqP != NULL && reqP != &req_discon)
    {
      freeReq(reqP);
    }
  }

	pthread_mutex_lock(&lock);
	if(userP->cookieP){
		freeCookie(userP->cookieP);
		userP->cookieP = NULL;
		users.users_n -= 1;
		userP->balance = 0;
		userP->port = 0;
	}
	pthread_mutex_unlock(&lock);

  SSL_free(sslP);
  close(SSL_get_fd(sslP));

  printf("Thread %lu exiting\n\n", (unsigned long)pthread_self());

}

void send_msg(SSL* sslp, char *str)
{
  size_t n = LEN(str);
  SSL_write(sslp, (unsigned char *)&n, sizeof(size_t));
  SSL_write(sslp, (unsigned char *)str, n);
}

SSL_CTX *newServerCtx()
{
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();

	SSL_CTX *ctx = SSL_CTX_new(SSLv23_server_method());

	if (!ctx)
	{
		perror("Unable to create SSL context");
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	SSL_CTX_set_ecdh_auto(ctx, 1);

	/* Set the key and cert */
	if (SSL_CTX_use_certificate_file(ctx, "db/server/server.crt", SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, "db/server/server.key", SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	return ctx;
}

int newListenfd(int port)
{

	int listenfd;

	signal(SIGPIPE, SIG_IGN);
	struct sockaddr_in serv_addr;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	printf("ip: 127.0.0.1\n");
	printf("port: %d\n\n", port);

	bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	if (listen(listenfd, 10) == -1)
	{
		perror("Failed to listen\n\n");
		exit(EXIT_FAILURE);
	}

	return listenfd;
}

threadpool_t *newThreadPool()
{
	threadpool_t *pool;
	pthread_mutex_init(&lock, NULL);
	assert((pool = threadpool_create(THREAD, QUEUE, 0)) != NULL);
	printf("Pool started with %d threads and queue size of %d\n", THREAD, QUEUE);
	return pool;
}