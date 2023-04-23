#include "kernel/types.h"
#include "user/user.h"

#define BUF_SIZE (1 << 15)

int main() {
  static const char buffer[BUF_SIZE];
  dmesg(buffer, BUF_SIZE);
  printf("%s", buffer);
  exit(0);
}
