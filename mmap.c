#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "file.h"
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
  struct file* f = NULL;
  struct stat st = { 0 };
  uint vaddr = addr + MMAP_BASE;
  int m_id;
  if (addr & 0xFFF || length & 0xFFF || length <= 0) return 0;  // return 0: Must be page aligned
  if (!BITCHECK(flags, 0)) {                     // Not anonymous : try to file mapping
    if (fd < 0 || fd >= NOFILE) return 0;        // return 0: not anonymous, but when the fd is not in bound
    f = p->ofile[fd];
    if (filestat(f, &st) < 0) return 0;          // return 0: fail to filestat()
    filedup(f);
    if (BITCHECK(prot, 0) && !f->readable) return 0; //protection and prot parameter are different
    if (BITCHECK(prot, 1) && !f->writable) return 0; //protection and prot parameter are different
  }
  if (BITCHECK(flags, 1)) { // MAP_POPULATE is given.
    void* physical_page;
    for (int j = (length + PGSIZE - 1) / PGSIZE, i = 0; i < j; i++) {
      if ((physical_page = kalloc()) < 0) return 0;                                   //return 0: kalloc function fail
      else if (BITCHECK(flags, 0)) memset(physical_page, 0, PGSIZE);                  // MAP_ANONYMOUS is given
      else { // MAP_ANONYMOUS is NOT given
        f->off = offset;
        if ((fileread(f, physical_page, PGSIZE)) < 0) return 0;
      } 
      if ((mmap_helper(p, (void*)(vaddr + i * PGSIZE), PGSIZE, physical_page, prot)) < 0) return 0;
    }
    acquire(&mtable.lock);
    for (m_id = 0; m_id < MAX_MMAPAREA; m_id++) {  // occupied == 0 : regards available.
      if (!mtable.m_area[m_id].occupied) {
        mtable.m_area[m_id].occupied = 0x1;
        mtable.m_area[m_id].f = f;
        mtable.m_area[m_id].addr = vaddr;
        mtable.m_area[m_id].length = length;
        mtable.m_area[m_id].offset = offset;
        mtable.m_area[m_id].prot = prot;
        mtable.m_area[m_id].flags = flags;
        mtable.m_area[m_id].p = p;
        break;
      }
    }
    release(&mtable.lock);
    if (m_id == MAX_MMAPAREA) return 0;            // return 0: There is no m_area
  }
  else { // MAP_POPULATE is NOT given.
    acquire(&mtable.lock);
    for (m_id = 0; m_id < MAX_MMAPAREA; m_id++) {  // occupied == 0 : regards available.
      if (!mtable.m_area[m_id].occupied) {
        mtable.m_area[m_id].occupied = 0x2;
        mtable.m_area[m_id].f = f;
        mtable.m_area[m_id].addr = vaddr;
        mtable.m_area[m_id].length = length;
        mtable.m_area[m_id].offset = offset;
        mtable.m_area[m_id].prot = prot;
        mtable.m_area[m_id].flags = flags;
        mtable.m_area[m_id].p = p;
        break;
      }
    }
    release(&mtable.lock);
    if (m_id == MAX_MMAPAREA) return 0;            // return 0: There is no m_area
  }
  return vaddr;
}

int
munmap(uint addr) {
  // struct proc* p = myproc();
  // struct file* f = NULL;
  struct mmap_area mcopy;
  int m_id;
  acquire(&mtable.lock);
  for (m_id = 0; m_id < MAX_MMAPAREA; m_id++) {  
    if (mtable.m_area[m_id].addr == addr && mtable.m_area[m_id].occupied == 1) {
      break;
    }
  }
  mcopy = mtable.m_area[m_id];
  release(&mtable.lock);
  if (m_id == MAX_MMAPAREA) return -1;
  if (!BITCHECK(mcopy.flags, 0)) {
    mcopy.f->off = mcopy.offset;
    filewrite(mcopy.f, (void*)(mcopy.addr), mcopy.length);
    fileclose(mcopy.f);
  }
  
  acquire(&mtable.lock);
  mtable.m_area[m_id].occupied = 0;
  release(&mtable.lock);
  return 0;
}

// Return current number of free memory pages; Alias of kmem_len(void)
int
freemem(void) {
  return kmem_len();
}

void
mmap_fork(struct proc* new, struct proc* curproc) {
  int m_id;
  acquire(&mtable.lock);
  for (m_id = 0; m_id < MAX_MMAPAREA; m_id++) {
    if (mtable.m_area[m_id].p == curproc && mtable.m_area[m_id].occupied == 1) {
      for (int i = m_id + 1; i < MAX_MMAPAREA; i++) {
        if (mtable.m_area[i].occupied) continue;
        mtable.m_area[i].occupied = mtable.m_area[m_id].occupied;
        mtable.m_area[i].f = mtable.m_area[m_id].f;
        mtable.m_area[i].addr = mtable.m_area[m_id].addr;
        mtable.m_area[i].length = mtable.m_area[m_id].length;
        mtable.m_area[i].offset = mtable.m_area[m_id].offset;
        mtable.m_area[i].prot = mtable.m_area[m_id].prot;
        mtable.m_area[i].flags = mtable.m_area[m_id].flags;
        mtable.m_area[i].p = new;
      }
    }
  }
  release(&mtable.lock);
}

int
mmap_anony(struct proc* p) {
  int m_id, length, flags, prot, offset;
  struct file* f = NULL;
  uint vaddr;
  acquire(&mtable.lock);
  for (m_id = 0; m_id < MAX_MMAPAREA; m_id++) {
    if (mtable.m_area[m_id].p == p && mtable.m_area[m_id].occupied == 0x2) {
      mtable.m_area[m_id].occupied = 0;
      vaddr = mtable.m_area[m_id].addr;
      length = mtable.m_area[m_id].length;
      flags = mtable.m_area[m_id].flags;
      prot = mtable.m_area[m_id].prot;
      offset = mtable.m_area[m_id].offset;
      break;
    }
  }
  release(&mtable.lock);
  if (m_id == MAX_MMAPAREA) return 0;

  if (BITCHECK(flags, 1)) { // MAP_POPULATE is given.
    void* physical_page;
    for (int j = (length + PGSIZE - 1) / PGSIZE, i = 0; i < j; i++) {
      if ((physical_page = kalloc()) < 0) return 0;                                   //return 0: kalloc function fail
      else if (BITCHECK(flags, 0)) memset(physical_page, 0, PGSIZE);                  // MAP_ANONYMOUS is given
      else { // MAP_ANONYMOUS is NOT given
        f->off = offset;
        if ((fileread(f, physical_page, PGSIZE)) < 0) return 0;
      }
      if ((mmap_helper(p, (void*)(vaddr + i * PGSIZE), PGSIZE, physical_page, prot)) < 0) return 0;
    }
    acquire(&mtable.lock);
    for (m_id = 0; m_id < MAX_MMAPAREA; m_id++) {  // occupied == 0 : regards available.
      if (!mtable.m_area[m_id].occupied) {
        mtable.m_area[m_id].occupied = 0x1;
        mtable.m_area[m_id].f = f;
        mtable.m_area[m_id].addr = vaddr;
        mtable.m_area[m_id].length = length;
        mtable.m_area[m_id].offset = offset;
        mtable.m_area[m_id].prot = prot;
        mtable.m_area[m_id].flags = flags;
        mtable.m_area[m_id].p = p;
        break;
      }
    }
    release(&mtable.lock);
    if (m_id == MAX_MMAPAREA) return 0;            // return 0: There is no m_area
  }
  else { // MAP_POPULATE is NOT given.
    acquire(&mtable.lock);
    for (m_id = 0; m_id < MAX_MMAPAREA; m_id++) {  // occupied == 0 : regards available.
      if (!mtable.m_area[m_id].occupied) {
        mtable.m_area[m_id].occupied = 0x2;
        mtable.m_area[m_id].f = f;
        mtable.m_area[m_id].addr = vaddr;
        mtable.m_area[m_id].length = length;
        mtable.m_area[m_id].offset = offset;
        mtable.m_area[m_id].prot = prot;
        mtable.m_area[m_id].flags = flags;
        mtable.m_area[m_id].p = p;
        break;
      }
    }
    release(&mtable.lock);
    if (m_id == MAX_MMAPAREA) return 0;            // return 0: There is no m_area
  }

  
  return 1;
}