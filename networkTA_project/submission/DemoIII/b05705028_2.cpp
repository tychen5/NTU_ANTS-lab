#include "./include/pki.h"

#include <iomanip>
#include <iostream>
#include <string>

#include <openssl/err.h>

using namespace std;

int main() {
  JPay::PKI clientA = JPay::PKI();
  JPay::PKI clientB = JPay::PKI();
  string msg = "Helllo world";

  char* apKey = clientA.sharePUBKEY();
  char* bpKey = clientB.sharePUBKEY();

  RSA* arsa = JPay::PKI::loadPUBKEY(apKey);
  RSA* brsa = JPay::PKI::loadPUBKEY(bpKey);

  char* cipher = clientA.encrypt(msg.c_str(), brsa);

  char* newPlain = clientB.decrypt(cipher, arsa);
  cout << newPlain << endl;
}