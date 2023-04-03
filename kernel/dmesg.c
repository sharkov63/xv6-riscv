#include <stdarg.h>

#include "types.h"

#include "param.h"
#include "riscv.h"

#include "defs.h"

#include "spinlock.h"

#define DMESG_BUFFER_PAGES 1
#define DMESG_BUFFER_SIZE (DMESG_BUFFER_PAGES * PGSIZE)

extern int consolewrite(int user_src, uint64 src, int n);

///
/// dmesg storage entities
///

static char *head; ///< Points to the beginning of the buffer.
static char *tail; ///< Points to '\0' symbol that
                   ///< ends the buffer.
/// Diagntostics messages cyclic buffer.
static char buffer[DMESG_BUFFER_SIZE];
static char const *buffer_end = buffer + DMESG_BUFFER_SIZE;
static struct spinlock mutex; ///< lock for dmesg entities

///
/// dmesg methods.
///
/// Must be called only when \p mutex is locked
///

static void advance_head() {
  ++head;
  if (head == buffer_end)
    head = 0;
}

static void append_char(char ch) {
  if (tail < buffer_end - 1) {
    *tail++ = ch;
    *tail = '\0';
    advance_head();
  } else {
    *tail = ch;
    tail = buffer;
    *tail = '\0';
    head = tail + 1;
  }
}

static void append_string(const char *str) {
  for (; *str; ++str)
    append_char(*str);
}

static const char digits[] = "0123456789abcdef";

/// \pre
/// base must be from 5 to 16
/// number != 0
static void append_uint(unsigned number, int base) {
  char as_str[16];
  char *head = as_str + 15;
  *head = '\0';
  for (; number > 0; number /= base) {
    *(--head) = digits[number % base];
  }
  append_string(head);
}

/// \pre
/// base must be from 5 to 16
static void append_int(int number, int base) {
  if (number == 0)
    return append_char('0');
  if (number < 0) {
    append_char('-');
    number = -number;
  }
  append_uint((unsigned)number, base);
}

static void append_ptr(uint64 ptr) {
  append_char('0');
  append_char('x');
  for (int block = 1; block <= sizeof(uint64) * 2; ++block) {
    unsigned digit = (ptr >> (sizeof(uint64) * 8 - 4 * block)) & 15;
    append_char(digits[digit]);
  }
}

///
/// Implementation of dmesg.h interface.
///

void dmesg_init() {
  initlock(&mutex, "dmesg lock");
  head = buffer;
  tail = buffer;
  *tail = '\0';
}

void pr_msg(const char *format, ...) {
  acquire(&mutex);
  append_string("[Time: ");
  acquire(&tickslock);
  unsigned cur_ticks = ticks;
  release(&tickslock);
  append_int(cur_ticks, 10);
  append_string(" ticks]: ");

  va_list args;
  va_start(args, format);
  while (*format) {
    if (*format != '%') {
      append_char(*format++);
      continue;
    }
    char ch = *(++format);
    if (!ch)
      break;
    switch (ch) {
    case 'd':
      append_int(va_arg(args, int), 10);
      break;
    case 'x':
      append_int(va_arg(args, int), 16);
      break;
    case 'p':
      append_ptr(va_arg(args, uint64));
      break;
    case 's': {
      const char *s = va_arg(args, const char *);
      if (!s)
        s = "(null)";
      append_string(s);
      break;
    }
    case '%':
      append_char('%');
      break;
    default: {
      append_char('%');
      append_char(ch);
      break;
    }
    }
    ++format;
  }
  va_end(args);

  append_char('\n');
  release(&mutex);
}

///
/// Implementation of dmesg syscall
///
uint64 sys_dmesg() {
  if (head < tail) {
    consolewrite(0, (uint64)head, tail - head + 1);
  } else {
    consolewrite(0, (uint64)head, buffer_end - head);
    consolewrite(0, (uint64)buffer, tail - buffer + 1);
  }
  return 0;
}
