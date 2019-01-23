#ifndef COMMON_H
#define COMMON_H

#define MESSAGE_LEN 512
#define INT_STR_LEN 16

#include <iostream>
#include "aes.h"

enum ClientStatus {
    CLIENT_CONNECTED,
    CLIENT_LOGGEDIN,
    CLIENT_DISCONNECTED
};

int encrypted_send(int fd, const char* buf, size_t len, int flags) {
    char* encrypt_string = NULL;
    // std::cout << "before encryped: " << buf << std::endl;
    int encrypt_len = encrypt(buf, &encrypt_string);
    // std::cout << "after encryped: " << encrypt_string << std::endl;
    return send(fd, encrypt_string, encrypt_len, 0);
}

int decrypted_recv(int fd, char* buf, size_t len, int flags) {
    char *decryto_string = NULL;
    char beforeDecrypt[MESSAGE_LEN] = {0};
    // std::cout << "before decrypted: " << beforeDecrypt << std::endl;
    int ret = recv(fd, beforeDecrypt, len, flags);
    if (ret == 0) return 0;

    decrypt(beforeDecrypt, &decryto_string, ret);
    // std::cout << "after decrypted: " << decryto_string << std::endl;
    strcpy(buf, decryto_string);
    return ret;
}

#endif
