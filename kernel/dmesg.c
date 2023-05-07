#include <stdarg.h>

#include "types.h"

#include "param.h"
#include "riscv.h"

#include "defs.h"

#include "spinlock.h"

#include "proc.h"

#include "dmesg.h"

#define DMESG_BUFFER_PAGES 10
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

/// dmesg logging storage entities

static int log_eventclass_limit_ticks[DMESG_LOG_EVENTCLASS_COUNT];

///
/// dmesg methods.
///
/// Must be called only when \p mutex is locked
///

#define ADVANCE(ptr)                                                           \
  do {                                                                         \
    ++ptr;                                                                     \
    if (ptr == buffer_end)                                                     \
      ptr = buffer;                                                            \
  } while (0);

static void append_char(char ch) {
  *tail = ch;
  ADVANCE(tail);
  *tail = '\0';
  if (head == tail)
    ADVANCE(head);
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

static void variadic_pr_msg(const char *format, va_list args) {
  append_string("[Time: ");
  unsigned cur_ticks = ticks;
  append_int(cur_ticks, 10);
  append_string(" ticks]: ");

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

  append_char('\n');
}

static int log_eventclass_is_enabled(enum dmesg_log_eventclass eventclass) {
  int cur_ticks = ticks;
  return cur_ticks <= log_eventclass_limit_ticks[eventclass];
}

#define INT_MAX (int)((1u << 31) - 1)
#define INT_MIN (int)(-(1u << 31))

static void log_eventclass_toggle_impl(enum dmesg_log_eventclass eventclass,
                                       int duration_ticks) {
  int limit_ticks = INT_MAX;
  if (duration_ticks != 0) {
    limit_ticks = ticks + duration_ticks;
  }
  log_eventclass_limit_ticks[eventclass] = limit_ticks;
}

///
/// Implementation of dmesg.h interface.
///

void dmesg_init() {
  initlock(&mutex, "dmesg lock");
  head = buffer;
  tail = buffer;
  *tail = '\0';
  for (int eventclass = 0; eventclass < DMESG_LOG_EVENTCLASS_COUNT;
       ++eventclass)
    log_eventclass_limit_ticks[eventclass] = 0;
}

void dmesg_log(enum dmesg_log_eventclass eventclass, const char *format, ...) {
  acquire(&mutex);
  if (!log_eventclass_is_enabled(eventclass))
    goto release_lock;
  va_list args;
  va_start(args, format);
  variadic_pr_msg(format, args);
  va_end(args);
release_lock:
  release(&mutex);
}

void dmesg_log_eventclass_toggle(enum dmesg_log_eventclass eventclass,
                                 int duration_ticks) {
  acquire(&mutex);
  log_eventclass_toggle_impl(eventclass, duration_ticks);
  release(&mutex);
}

void pr_msg(const char *format, ...) {
  acquire(&mutex);
  va_list args;
  va_start(args, format);
  variadic_pr_msg(format, args);
  va_end(args);
  release(&mutex);
}

///
/// Implementation of dmesg syscall
///
uint64 sys_dmesg() {
  uint64 user_out_addr;
  argaddr(0, &user_out_addr);
  int user_out_size;
  argint(1, &user_out_size);
  if (user_out_size <= 0)
    return 0; // nothing to copy
  pagetable_t pagetable = myproc()->pagetable;

  uint64 ret = 0;

#define append(src, _length)                                                   \
  do {                                                                         \
    uint64 length = _length;                                                   \
    if (length >= user_out_size)                                               \
      length = user_out_size - 1;                                              \
    ret |= copyout(pagetable, user_out_addr, src, length);                     \
    user_out_addr += length;                                                   \
    user_out_size -= length;                                                   \
  } while (0)

  acquire(&mutex);

  if (head < tail) {
    append(head, tail - head);
  } else {
    append(head, buffer_end - head);
    append(buffer, tail - buffer);
  }
  append("\0", 1);

  release(&mutex);

  return ret;
}

///
/// Implementation of dmesg_log_toggle syscall
///
uint64 sys_dmesg_log_toggle() {
  int eventclass, duration_ticks;
  argint(0, &eventclass);
  argint(1, &duration_ticks);
  dmesg_log_eventclass_toggle(eventclass, duration_ticks);
  return 0;
}
