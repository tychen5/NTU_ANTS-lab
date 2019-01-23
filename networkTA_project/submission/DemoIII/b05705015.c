#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
 
#define MAXBUF 1024
 
int main(int argc, char **argv)
{
  int sockfd, new_fd, fd;
  socklen_t len;
  struct sockaddr_in my_addr, their_addr;
  unsigned int myport, lisnum;
  char buf[MAXBUF + 1];
  char new_fileName[50]="/newfile/";
  SSL_CTX *ctx;
  mode_t mode;
  char pwd[100];
  int* amount;
  char* temp;
 
  /* found a flie of newfile */
  mkdir("/newfile",mode);
 
  if (argv[1])
    myport = atoi(argv[1]);
  else
  {
    myport = 7838;
    argv[2]=argv[3]=NULL;
  }
 
  if (argv[2])
    lisnum = atoi(argv[2]);
  else
  {
    lisnum = 2;
    argv[3]=NULL;
  }
 
  /* reset ssl */
  SSL_library_init();
  /* load ssl algorithm */
  OpenSSL_add_all_algorithms();
  /* load ssl error situation */
  SSL_load_error_strings();
  /* set a new ssl_ctx by standard SSL V2 and V3 ，which means SSL Content Text */
  ctx = SSL_CTX_new(SSLv23_server_method());
  if (ctx == NULL)
  {
    ERR_print_errors_fp(stdout);
    exit(1);
  }
  /*  send the pki to client */
  getcwd(pwd,100);
  if(strlen(pwd)==1)
    pwd[0]='\0';
  if (SSL_CTX_use_certificate_file(ctx, temp=strcat(pwd,"/cacert.pem"), SSL_FILETYPE_PEM) <= 0)
  {
    ERR_print_errors_fp(stdout);
    exit(1);
  }
  /* load the pki */
  getcwd(pwd,100);
  if(strlen(pwd)==1)
    pwd[0]='\0';
  if (SSL_CTX_use_PrivateKey_file(ctx, temp=strcat(pwd,"/privkey.pem"), SSL_FILETYPE_PEM) <= 0)
  {
    ERR_print_errors_fp(stdout);
    exit(1);
  }
  /* check if pki is correct */
  if (!SSL_CTX_check_private_key(ctx))
  {
    ERR_print_errors_fp(stdout);
    exit(1);
  }
 
  /* open a new socket to listen */
  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }
  else
    printf("socket created\n");
 
  bzero(&my_addr, sizeof(my_addr));
  my_addr.sin_family = PF_INET;
  my_addr.sin_port = htons(myport);
  if (argv[3])
    my_addr.sin_addr.s_addr = inet_addr(argv[3]);
  else
    my_addr.sin_addr.s_addr = INADDR_ANY;
 
  if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1)
  {
    perror("bind");
    exit(1);
  }
  else
    printf("binded\n");
 
  if (listen(sockfd, lisnum) == -1)
  {
    perror("listen");
    exit(1);
  }
  else
    printf("begin listen\n");
 
  while (1)
  {
    SSL *ssl;
    len = sizeof(struct sockaddr);
    /* wait for connection */
    if ((new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &len)) == -1)
    {
      perror("accept");
      exit(errno);
    }
    else
      printf("server: got connection from %s, port %d, socket %d\n", inet_ntoa(their_addr.sin_addr), ntohs(their_addr.sin_port), new_fd);
 
    /* found a new ssl by ctx */
    ssl = SSL_new(ctx);
    /* join the client's SSL to the new SSL */
    SSL_set_fd(ssl, new_fd);
    /* built the ssl connection */
    if (SSL_accept(ssl) == -1)
    {
      perror("accept");
      close(new_fd);
      break;
    }
 
    /* accept the account information from the client a make a new file to store the information */
    bzero(buf, MAXBUF + 1);
    bzero(new_fileName+9, 42);
    len = SSL_read(ssl, buf, MAXBUF);
    if(len == 0)
      printf("Receive Complete !\n");
    else if(len < 0)
      printf("Failure to receive message ! Error code is %d，Error messages are '%s'\n", errno, strerror(errno));
    if((fd = open(strcat(new_fileName,buf),O_CREAT | O_TRUNC | O_RDWR,0666))<0)
    {
      perror("open:");
      exit(1);
    }
 
    /* accept the data from the client and store it into client's account file */
    while(1)
    {
      bzero(buf, MAXBUF + 1);
      len = SSL_read(ssl, buf, MAXBUF);
      if(len == 0)
      {
        printf("Receive Complete !\n");
        break;
      }
      else if(len < 0)
      {
        printf("Failure to receive message ! Error code is %d，Error messages are '%s'\n", errno, strerror(errno));
        exit(1);
      }
      if(write(fd,buf,len)<0)
      {
        perror("write:");
	scanf("%d", &amount);
        exit(1);
      }
    }
 
    /* close the file */
    close(fd);
    /* close the connection of ssl */
    SSL_shutdown(ssl);
    /* free SSL */
    SSL_free(ssl);
    /* clock socket */
    close(new_fd);
  }
 
  /* clock listening socket */
  close(sockfd);
  /* free CTX */
  SSL_CTX_free(ctx);
  return 0;
}

