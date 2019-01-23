#include "../include/ssl.h"

void JPay::init_openssl() {
  SSL_load_error_strings();
  ERR_load_crypto_strings();
  OpenSSL_add_ssl_algorithms();
}

void JPay::cleanup_openssl() { EVP_cleanup(); }

SSL_CTX *JPay::create_context() {
  const SSL_METHOD *method;
  SSL_CTX *ctx;

  method = SSLv23_server_method();

  ctx = SSL_CTX_new(method);
  if (!ctx) {
    perror("Unable to create SSL context");
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  return ctx;
}

void JPay::configure_context(SSL_CTX *ctx, const char *cert_path,
                             const char *key_path) {
  SSL_CTX_set_ecdh_auto(ctx, 1);

  /* Set the key and cert */
  if (SSL_CTX_use_certificate_file(ctx, cert_path, SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  if (SSL_CTX_use_PrivateKey_file(ctx, key_path, SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }
}