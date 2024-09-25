#include "kernel/types.h"
#include "user/user.h"

static void print_help() {
  printf("dmesg_log_toggle - toggles dmesg logging for a given eventclass.\n"
         "\n"
         "Usage:\n"
         "\t$ dmesg_log_toggle EVENTCLASS ACTION\n"
         "\n"
         "Positional arguments:\n"
         "\tEVENTCLASS - a number or a string representing the eventclass to "
         "toggle.\n"
         "\t\tPossible values:\n"
         "\t\t- '0' or 'interrupt' - hardware interrupts\n"
         "\t\t- '1' or 'procswitch' - process switches\n"
         "\t\t- '2' or 'syscall' - system calls\n"
         "\t\t- 'all' - all above\n"
         "\n"
         "\tACTION - what to do with given eventclass.\n"
         "\t\tPossible values:\n"
         "\t\t- 'disable' - disable logging\n"
         "\t\t- 'enable' - enable logging permanently\n"
         "\t\t- 'enable x', where x is a positive number - enable logging for "
         "x ticks.\n");
}

static int streq(const char *lhs, const char *rhs) {
  return strcmp(lhs, rhs) == 0;
}

typedef enum {
  EC_INTERRUPT = 0,
  EC_PROCSWITCH = 1,
  EC_SYSCALL = 2,
  EC_SIZE,
  EC_ALL,
  EC_INVALID,
} eventclass_t;

#define EVENTCLASS_INVALID (-1)
#define EVENTCLASS_ALL 4

static eventclass_t read_eventclass(const char *as_str) {
  if (streq(as_str, "0") || streq(as_str, "interrupt"))
    return EC_INTERRUPT;
  if (streq(as_str, "1") || streq(as_str, "procswitch"))
    return EC_PROCSWITCH;
  if (streq(as_str, "2") || streq(as_str, "syscall"))
    return EC_SYSCALL;
  if (streq(as_str, "all"))
    return EC_ALL;
  return EC_INVALID;
}

/// \return 0 if everything is ok
static int read_duration_ticks(int action_argc, const char **action_argv,
                               int *duration_ticks) {
  if (action_argc <= 0 || action_argc > 2)
    return 1;
  const char *action = action_argv[0];
  if (streq(action, "disable")) {
    if (action_argc > 1)
      return 1;
    *duration_ticks = -1;
    return 0;
  }
  if (streq(action, "enable")) {
    if (action_argc == 1) {
      *duration_ticks = 0;
      return 0;
    }
    *duration_ticks = atoi(action_argv[1]);
    return !(*duration_ticks > 0);
  }
  return 1;
}

int main(int argc, const char **argv) {
  if (argc < 3)
    goto print_help_and_exit;
  eventclass_t eventclass = read_eventclass(argv[1]);
  if (eventclass == EC_INVALID)
    goto print_help_and_exit;
  int duration_ticks;
  if (read_duration_ticks(argc - 2, argv + 2, &duration_ticks))
    goto print_help_and_exit;

  if (eventclass == EVENTCLASS_ALL) {
    for (int class = 0; class < EC_SIZE; ++class)
      dmesg_log_toggle(class, duration_ticks);
  } else {
    dmesg_log_toggle(eventclass, duration_ticks);
  }
  exit(0);

print_help_and_exit:
  print_help();
  exit(1);
}
