#include "types.h"
#include "defs.h"
#include "param.h"
#include "file.h"
#include "stat.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "mmap.h"
#define NULL ((uint)0)
#define BITCHECK(mask, i) (mask & (1 << i))

struct {
  struct spinlock lock;
  struct mmap_area m_area[MAX_MMAPAREA];
} mtable;

uint
mmap(uint addr, int length, int prot, int flags, int fd, int offset) {
  struct proc* p = myproc();
  struct file* fp = NULL;
  struct stat st = { 0 };
  int m_id;
  if (addr & 0xFFF || length & 0xFFF) return 0;  // return 0: Must be page aligned
  if (!(BITCHECK(prot, 1))) return 0;            // return 0: PROT_READ must be included
  acquire(&mtable.lock);
  for (m_id = 0; m_id < MAX_MMAPAREA; m_id++) {
    if (mtable.m_area[m_id].addr == 0) break;    // addr == 0 : regards available.
  } 
  release(&mtable.lock);
  if (m_id == MAX_MMAPAREA) return 0;            // return 0: There is no m_area
  if (BITCHECK(flags, 2)) {
    if (fd == -1) return 0;                      // not anonymous, but when the fd is -1
    fp = p->ofile[fd];
    if (filestat(fd, &st) < 0) return 0;         // return 0: fail to filestat()
    if (BITCHECK(prot, 1) && !fp->readable) return 0; //protection and prot parameter are different
    if (BITCHECK(prot, 2) && !fp->writable) return 0; //protection and prot parameter are different

  }

  return 0;
}

/* Return current number of free memory pages */
int
freemem(void) {
  return kmem_len();
}