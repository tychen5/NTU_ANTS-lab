#ifndef _ASE_H_
#define _ASE_H_
   
    int encrypt(const char *input_string, char **encrypt_string);
    void decrypt(const char *encrypt_string, char **decrypt_string, int len);

#endif
