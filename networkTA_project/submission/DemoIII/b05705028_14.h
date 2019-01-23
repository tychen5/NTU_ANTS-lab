#ifndef PKI_HPP
#define PKI_HPP

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

namespace JPay {

class PKI {
 public:
  PKI(const int keyBits = 2046) {
    ERR_load_crypto_strings();
    rsa = RSA_new();
    BIGNUM* bne = BN_new();
    if (!BN_set_word(bne, RSA_F4)) {
      throw;
    }
    if (!RSA_generate_key_ex(rsa, keyBits, bne, NULL)) {
      throw;
    }
    BN_clear_free(bne);
  }

  char* sharePUBKEY() {
    BIO* bp = BIO_new(BIO_s_mem());

    if (!PEM_write_bio_RSA_PUBKEY(bp, rsa)) {
      BIO_free(bp);
      return NULL;
    }

    char* pem = (char*)malloc(451 + 1);

    memset(pem, 0, 451 + 1);
    BIO_read(bp, pem, 451);

    BIO_free(bp);

    return pem;
  }

  static RSA* loadPUBKEY(char* pem) {
    BIO* bp = BIO_new_mem_buf(pem, strlen(pem));
    RSA* rsa;
    rsa = PEM_read_bio_RSA_PUBKEY(bp, NULL, NULL, NULL);
    BIO_free(bp);
    return rsa;
  }

  char* encrypt(const char* plain, RSA* rsaPub) {
    unsigned char u_plain[strlen(plain) + 1];
    strncpy((char*)u_plain, plain, strlen(plain) + 1);

    unsigned char* cipher = new unsigned char[RSA_size(rsa)];
    int len = RSA_private_encrypt(strlen(plain) + 1, u_plain, cipher, rsa,
                                  RSA_PKCS1_PADDING);
    if (len == -1) {
      ERR_print_errors_fp(stderr);
    }

    len = RSA_public_encrypt(len, cipher, cipher, rsaPub, RSA_NO_PADDING);
    if (len == -1) {
      ERR_print_errors_fp(stderr);
    }

    return (char*)cipher;
  }

  char* decrypt(const char* cipher, RSA* rsaPub) {
    unsigned char u_cipher[RSA_size(rsa)];
    memcpy((char*)u_cipher, cipher, RSA_size(rsa));

    unsigned char* plane = new unsigned char[1024];
    if (RSA_private_decrypt(RSA_size(rsa), u_cipher, plane, rsa,
                            RSA_NO_PADDING) == -1) {
      ERR_print_errors_fp(stderr);
    }

    if (RSA_public_decrypt(RSA_size(rsa), plane, plane, rsaPub,
                           RSA_PKCS1_PADDING) == -1) {
      ERR_print_errors_fp(stderr);
    }

    return (char*)plane;
  }

 private:
  RSA* rsa;
};

}  // namespace JPay

#endif