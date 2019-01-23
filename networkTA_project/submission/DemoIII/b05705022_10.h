#ifndef UTIL_H
#define UTIL_H

#include <openssl/ssl.h>
#include <iostream>
#include <cassert>
#include <cstring>
using namespace std;

int mySSL_read(SSL* ssl, char* buf, int len) {
    int ret = SSL_read(ssl, buf, len);
    if (ret < 0) {
        cerr << "Receving error!\n";
        ERR_print_errors_fp(stderr);
        return ret;
    }
    #ifdef VERBOSE
    cerr << "[Verbose] Response: " << buf << endl;
    #endif // VERBOSE
    return ret;
}

int mySSL_write(SSL* ssl, const char* buf, int len) {
    #ifdef VERBOSE
    cerr << "[Verbose] Send: " << buf << endl;
    #endif // VERBOSE
    int ret = SSL_write(ssl, buf, len);
    if (ret < 0) {
        cerr << "Sending error!\n";
        ERR_print_errors_fp(stderr);
    }
    return ret;
}

SSL_CTX* loadAndVerify(const char* CertFile, const char* KeyFile) {
   // create ctx
    const SSL_METHOD *method = SSLv23_method();
    SSL_CTX* ctx = SSL_CTX_new(method); 
    if (!ctx) {
        cerr << "Error: unable to create SSL context\n";
        ERR_print_errors_fp(stderr);
        return 0;
    }

    // Set the key and cert
    if (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0) {
        cerr << "Error: unable to load certification\n";
        ERR_print_errors_fp(stderr);
        return 0;
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 ) {
        cerr << "Error: unable to load key\n";
        ERR_print_errors_fp(stderr);
        return 0;
    }
    if (!SSL_CTX_check_private_key(ctx)){
        cerr << "Error: key and certificate don't match\n";
        return 0;
    }

    // verift CA
    SSL_CTX_load_verify_locations(ctx, "cacert.pem", NULL);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, 4);
    return ctx;
}

SSL_CTX* initClientCTX(int& file) {
    cout << "Which certificate and key? [1/2]: ";
    cout.flush();
    char c[3];
    cin.getline(c, 2);
    char mcert[20] = {0};
    char mkey[20] = {0};
    if (strcmp(c, "1") == 0) { file = 1; strcpy(mcert, "cliCert.pem"); strcpy(mkey, "cliKey.pem"); }
    else                     { file = 2; strcpy(mcert, "cli2Cert.pem"); strcpy(mkey, "cli2Key.pem"); }

    return loadAndVerify(mcert, mkey);
}

SSL_CTX* initServerCTX(const char* CertFile, const char* KeyFile) {
    return loadAndVerify(CertFile, KeyFile);
}

void showCerts(SSL* ssl){
    cout << "Encryption with: " << SSL_get_cipher(ssl) << endl;
    X509 *cert = SSL_get_peer_certificate(ssl);
    if (cert){
        cout << "-------------- Certificates ----------------\n";
        cout << "Subject: " << X509_NAME_oneline(X509_get_subject_name(cert), 0, 0) << "\n";
        cout << "Issuer: " << X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0) << "\n";
        cout << "--------------------------------------------" << endl;
        X509_free(cert);
    }
    else
        cerr << "Error: no certificates.\n";
}

int myStrNCmp(const string& cmd, const string& input, unsigned n)
{
   assert(n > 0);
   unsigned n2 = input.size();
   if (n2 == 0) return -1;
   unsigned n1 = cmd.size();
   assert(n1 >= n);
   for (unsigned i = 0; i < n1; ++i) {
      if (i == n2)
         return (i < n)? 1 : 0;
      char ch1 = (isupper(cmd[i]))? tolower(cmd[i]) : cmd[i];
      char ch2 = (isupper(input[i]))? tolower(input[i]) : input[i];
      if (ch1 != ch2)
         return (ch1 - ch2);
   }
   return (n1 - n2);
}

#endif // UTIL_H