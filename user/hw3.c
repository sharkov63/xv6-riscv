#include "kernel/types.h"
#include "user.h"

int out;                                                                   

#define PGACCESS(address)                                                      \
  do {                                                                         \
    pgaccess(address, 1, (char *)&out);                                        \
    printf("pgaccess at %p: %d\n", address, out);                              \
  } while (0)

int main() {
  int *x = malloc(4);
  PGACCESS(x);
  PGACCESS(x);
  *x = 3;
  PGACCESS(x);
  int y = *x;
  printf("read %d\n", y);
  PGACCESS(x);
  PGACCESS(x);
  vmprint();
  exit(0);
}
