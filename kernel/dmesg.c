#include "types.h"

#include "param.h"
#include "riscv.h"

#include "defs.h"

#include "spinlock.h"

#define DMESG_BUFFER_PAGES 3
#define DMESG_BUFFER_SIZE (DMESG_BUFFER_PAGES * PGSIZE)

extern int consolewrite(int user_src, uint64 src, int n);

///
/// dmesg storage entities
///

static char *tail; ///< Points to '\0' symbol that
                   ///< ends the buffer.
static char
    buffer[DMESG_BUFFER_SIZE]; ///< Diagntostics messages buffer.
                               ///< Guaranteed to always be 0-terminated.
static struct spinlock mutex;  ///< lock for dmesg entities

///
/// dmesg methods.
///
/// Must be called only when \p mutex is locked
///

static void append_char(char ch) {
  if (tail < buffer + DMESG_BUFFER_SIZE - 1) {
    *tail++ = ch;
    *tail = '\0';
  }
}

static void append(const char *str) {
  for (; tail < buffer + DMESG_BUFFER_SIZE - 1 && *str != 0; ++tail, ++str)
    *tail = *str;
  *tail = '\0';
}

static void append_uint(unsigned number) {
  if (number == 0)
    return append_char('0');

  static char as_str[12];
  char *head = as_str + 11;
  *head = '\0';
  for (; number > 0; number /= 10)
    *(--head) = '0' + (number % 10);
  append(head);
}

///
/// Implementation of dmesg.h interface.
///

void dmesg_init() {
  initlock(&mutex, "dmesg lock");
  tail = buffer;
  *tail = '\0';
}

void pr_msg(const char *message) {
  acquire(&mutex);
  append("[Time: ");
  acquire(&tickslock);
  unsigned cur_ticks = ticks;
  release(&tickslock);
  append_uint(cur_ticks);
  append(" ticks]: ");
  append(message);
  append_char('\n');
  release(&mutex);
}

///
/// Implementation of dmesg syscall
///
uint64 sys_dmesg() {
  consolewrite(0, (uint64)buffer, tail - buffer + 1);
  return 0;
}
