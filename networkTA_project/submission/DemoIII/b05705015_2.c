#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
 
#define MAXBUF 1024
 
void ShowCerts(SSL * ssl)
{
  X509 *cert;
  char *line;
 
  cert = SSL_get_peer_certificate(ssl);
  if (cert != NULL) {
    printf("Digital certificate information:\n");
    line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
    printf("Certificate: %s\n", line);
    free(line);
    line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
    printf("Issuer: %s\n", line);
    free(line);
    X509_free(cert);
  }
  else
    printf("No certificate information！\n");
}
 
int main(int argc, char **argv)
{
  int i,j,sockfd, len, fd, size;
  char fileName[50],sendFN[20];
  struct sockaddr_in dest;
  char buffer[MAXBUF + 1];
  SSL_CTX *ctx;
  SSL *ssl;
 
  if (argc != 3)
  {
    printf("Parameter format error! Correct usage is as follows：\n\t\t%s IP Port\n\tSuch as:\t%s 127.0.0.1 80\n", argv[0], argv[0]); exit(0);
  }
 
  /* Reset SSL */
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();
  ctx = SSL_CTX_new(SSLv23_client_method());
  if (ctx == NULL)
  {
    ERR_print_errors_fp(stdout);
    exit(1);
  }
 
  /* set a socket for transfer */
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Socket");
    exit(errno);
  }
  printf("socket created\n");
 
  /* reset server address and portnum */
  bzero(&dest, sizeof(dest));
  dest.sin_family = AF_INET;
  dest.sin_port = htons(atoi(argv[2]));
  if (inet_aton(argv[1], (struct in_addr *) &dest.sin_addr.s_addr) == 0)
  {
    perror(argv[1]);
    exit(errno);
  }
  printf("address created\n");
 
  /* connect server */
  if (connect(sockfd, (struct sockaddr *) &dest, sizeof(dest)) != 0)
  {
    perror("Connect ");
    exit(errno);
  }
  printf("server connected\n\n");
 
  /* product a new ssl by ctx */
  ssl = SSL_new(ctx);
  SSL_set_fd(ssl, sockfd);
  /* set ssl link */
  if (SSL_connect(ssl) == -1)
    ERR_print_errors_fp(stderr);
  else
  {
    printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
    ShowCerts(ssl);
  }
 
  /* input account name */
  printf("\nPlease input the account  you want to load :\n>");
  scanf("%s",fileName);
  if((fd = open(fileName,O_RDONLY,0666))<0)
  {
    perror("open:");
    exit(1);
  }
    
  /* load money information by send transaction message to server */
  for(i=0;i<=strlen(fileName);i++)
  {
    if(fileName[i]=='/')
    {
      j=0;
      continue;
    }
    else {sendFN[j]=fileName[i];++j;}
  }
  len = SSL_write(ssl, sendFN, strlen(sendFN));
  if (len < 0)
    printf("'%s'message Send failure ！Error code is %d，Error messages are '%s'\n", buffer, errno, strerror(errno));
 
  /* loop connection to server */
  bzero(buffer, MAXBUF + 1); 
  while((size=read(fd,buffer,1024)))
  {
    if(size<0)
    {
      perror("read:");
      exit(1);
    }
    else
    {
      len = SSL_write(ssl, buffer, size);
      if (len < 0)
        printf("'%s'message Send failure ！Error code is %d，Error messages are '%s'\n", buffer, errno, strerror(errno));
    }
    bzero(buffer, MAXBUF + 1);
  }
  printf("Send complete !\n");
 
  /* close the connection */
  close(fd);
  SSL_shutdown(ssl);
  SSL_free(ssl);
  close(sockfd);
  SSL_CTX_free(ctx);
  return 0;
}

