#include "param.h"
#include "types.h"

#include "riscv.h"

#include "spinlock.h"

#include "defs.h"
#include "proc.h"

/// Maximum number of pages in pgaccess syscall
#define MAX_BYTES_FOR_PAGE 512
#define MAX_PAGES (MAX_BYTES_FOR_PAGE * 8)

uint64 sys_pgaccess() {
  uint64 virtual_addr;
  argaddr(0, &virtual_addr);
  int page_count;
  argint(1, &page_count);
  uint64 user_out_addr;
  argaddr(2, &user_out_addr);
  pagetable_t pagetable = myproc()->pagetable;
  if (page_count > MAX_PAGES)
    page_count = MAX_PAGES;

  unsigned char out[MAX_BYTES_FOR_PAGE];
  memset(out, 0, sizeof out);
  for (int page_index = 0; page_index < page_count;
       ++page_index, virtual_addr += PGSIZE) {
    pte_t *pte = walk(pagetable, virtual_addr, /* alloc = */ 0);
    if (!pte)
      continue;
    if (*pte & PTE_A) {
      out[page_index / 8] |= 1 << (page_index % 8);
      *pte ^= PTE_A;
    }
  }
  return copyout(pagetable, user_out_addr, (char *)out, ((page_count + 7) / 8));
}
