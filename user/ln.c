#include "kernel/types.h"

#include "kernel/stat.h"

#include "user/user.h"

#define stderr 2

static void print_help() {
  fprintf(stderr, "Usage:\n"
                  "\t$ ln OLD NEW\n"
                  "\t\t- create a hard link\n"
                  "\t$ ln -s OLD NEW\n"
                  "\t\t- create a soft (symbolic) link\n");
}

static void hard_link(const char *old, const char *new) {
  if (link(old, new))
    fprintf(stderr, "link %s %s: failed\n", old, new);
}

static void symbolic_link(const char *old, const char *new) {
  if (symlink(old, new))
    fprintf(stderr, "symlink %s %s: failed\n", old, new);
}

int main(int argc, const char **argv) {
  if (argc == 3) {
    hard_link(argv[1], argv[2]);
    exit(0);
  }
  if (argc == 4 && !strcmp(argv[1], "-s")) {
    symbolic_link(argv[2], argv[3]);
    exit(0);
  }
  print_help();
  exit(1);
}
