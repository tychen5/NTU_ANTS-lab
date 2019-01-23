#ifndef JPAY_SSL_HPP
#define JPAY_SSL_HPP

#include <openssl/err.h>
#include <openssl/ssl.h>

namespace JPay {

void init_openssl();

void cleanup_openssl();

SSL_CTX *create_context();

void configure_context(SSL_CTX *ctx, const char *cert_path,
                       const char *key_path);

}  // namespace JPay

#endif