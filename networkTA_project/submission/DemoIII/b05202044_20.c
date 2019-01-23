#include "util.h"

size_t my_read(char *buf)
{
  fgets(buf, BUF_SIZE, stdin);
  buf[strcspn(buf, "\n")] = 0;

  return LEN(buf);
}