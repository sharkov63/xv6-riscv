#include "param.h"
#include "types.h"

#include "riscv.h"

#include "spinlock.h"

#include "defs.h"
#include "proc.h"

static void print_pagetable(pagetable_t pagetable, int level) {
  for (int pte_idx = 0; pte_idx < 512; ++pte_idx) {
    pte_t pte = pagetable[pte_idx];
    if (!(pte & PTE_V))
      continue; // invalid pte
    for (int i = 0; i < level; ++i)
      printf(" ..");
    uint64 pa = PTE2PA(pte);
    printf("%d: pte %p pa %p\n", pte_idx, pte, pa);
    if ((pte & (PTE_R | PTE_W | PTE_X)))
      continue; // leaf
    print_pagetable((pagetable_t)pa, level + 1);
  }
}

uint64 sys_vmprint() {
  pagetable_t pagetable = myproc()->pagetable;
  printf("page table %p\n", pagetable);
  print_pagetable(pagetable, 1);
  return 0;
}
