#include "kernel/types.h"

#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

static int string_to_nonnegative_int(const char *string, int *out) {
  *out = 0;
  for (const char *current = string; *current; ++current) {
    if (*current < '0' || *current > '9')
      return -1;
    int digit = *current - '0';
    if (*out > ((1u << 31) - 1) / 10.0)
      return -1;
    *out *= 10;
    if (*out > (1u << 31) - 1 - digit)
      return -1;
    *out += *current - '0';
  }
  return 0;
}

#define E_INVALID_ARGS 1
#define E_SIZE_TOO_LARGE 2

static int parse_args(int argc, const char **argv, const char **file,
                      int *bytes) {
  if (argc < 3 || argc > 4)
    return E_INVALID_ARGS;
  *file = argv[1];
  if (string_to_nonnegative_int(argv[2], bytes))
    return E_INVALID_ARGS;
  if (argc == 3) {
    if (*bytes >= (1u << 31) / BSIZE)
      return E_SIZE_TOO_LARGE;
    *bytes *= BSIZE;
  }
  if (argc == 4) {
    if (strcmp(argv[3], "--bytes"))
      return -1;
  }
  if (*bytes >= 70000000)
    return E_SIZE_TOO_LARGE;
  return 0;
}

static void print_help() {
  printf("nullfile - create a file containing nulls with given size.\n"
         "\n"
         "Usage:\n"
         "\t$ nullfile FILE BLOCKS\n"
         "\t\t- write BLOCKS blocks of nulls into FILE\n"
         "\t$ nullfile FILE BYTES --bytes\n"
         "\t\t- write BYTES bytes of nulls into FILE\n");
}

static int create_nullfile(const char *file, int bytes) {
  int fd = open(file, O_WRONLY | O_CREATE | O_TRUNC);
  if (fd < 0)
    return -1;
  int ret = 0;
#define NULLBUFFER (BSIZE * 150)
  static char nullbuffer[NULLBUFFER];
  memset(nullbuffer, 0, NULLBUFFER);
  for (; bytes > 0; bytes -= NULLBUFFER) {
    int current_size = bytes < NULLBUFFER ? bytes : NULLBUFFER;
    if (write(fd, nullbuffer, current_size) != current_size) {
      ret = -1;
      goto close_file;
    }
  }
#undef NULLBUFFER
close_file:
  close(fd);
  return ret;
}

int main(int argc, const char **argv) {
  const char *file;
  int bytes;
  int error;
  if ((error = parse_args(argc, argv, &file, &bytes))) {
    if (error == E_INVALID_ARGS)
      printf("nullfile: invalid arguments\n\n");
    if (error == E_SIZE_TOO_LARGE)
      printf("nullfile: given size is too large\n\n");
    print_help();
    exit(error);
  }
  error = create_nullfile(file, bytes);
  if (error)
    printf("nullfile: failed\n");
  exit(error);
}
