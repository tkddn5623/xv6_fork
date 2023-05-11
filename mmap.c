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
      if ((physical_page = kalloc()) < 0) return 0;                  //return 0: kalloc function fail      
      else if (BITCHECK(flags, 0)) memset(physical_page, 0, PGSIZE); // MAP_ANONYMOUS is given
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
  // struct file* f = NULL;
  struct proc* p = myproc();
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
  deallocuvm(p->pgdir, mcopy.addr + mcopy.length, mcopy.addr);
  
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
  struct file* f;
  struct stat st = { 0 };
  uint vaddr;
  int m_id, length, prot, flags, offset, occupied, fd = -1;
  for (m_id = 0; m_id < MAX_MMAPAREA; m_id++) {
    if (mtable.m_area[m_id].p == curproc && mtable.m_area[m_id].occupied) {
      occupied = mtable.m_area[m_id].occupied;
      vaddr = mtable.m_area[m_id].addr;
      length = mtable.m_area[m_id].length;
      prot = mtable.m_area[m_id].prot;
      flags = mtable.m_area[m_id].flags;
      offset = mtable.m_area[m_id].offset;
      f = mtable.m_area[m_id].f;
      if (!BITCHECK(flags, 0)) { //File mapping
        for (fd = 0; fd < NOFILE; fd++) 
          if (curproc->ofile[fd] == f) break;
      }
      else fd = -1;              //Anonymous mapping 

      // Below code is similar with mmap() 
      if (vaddr & 0xFFF || length & 0xFFF || length <= 0) goto CONTINUE;  // return 0: Must be page aligned
      if (!BITCHECK(flags, 0)) {                          // Not anonymous : try to file mapping
        if (fd < 0 || fd >= NOFILE) goto CONTINUE;        // return 0: not anonymous, but when the fd is not in bound
        f = new->ofile[fd];
        if (filestat(f, &st) < 0) goto CONTINUE;          // return 0: fail to filestat()
        filedup(f);
        if (BITCHECK(prot, 0) && !f->readable) goto CONTINUE; //protection and prot parameter are different
        if (BITCHECK(prot, 1) && !f->writable) goto CONTINUE; //protection and prot parameter are different
      }
      if (occupied == 0x1) { 
        void* physical_page;
        for (int j = (length + PGSIZE - 1) / PGSIZE, i = 0; i < j; i++) {
          if ((physical_page = kalloc()) < 0) goto CONTINUE;
          memmove(physical_page, (void*)vaddr, PGSIZE);        // COPY FROM PARENT
          if ((mmap_helper(new, (void*)(vaddr + i * PGSIZE), PGSIZE, physical_page, prot)) < 0) goto CONTINUE;
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
            mtable.m_area[m_id].p = new;
            break;
          }
        }
        release(&mtable.lock);
        if (m_id == MAX_MMAPAREA) goto CONTINUE;            // return 0: There is no m_area
      }
      else { // Page fault hanlder will be handle.
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
            mtable.m_area[m_id].p = new;
            break;
          }
        }
        release(&mtable.lock);
        if (m_id == MAX_MMAPAREA) goto CONTINUE;            // return 0: There is no m_area
      }
    }
  CONTINUE:;
  }
}

int
mmap_anony(struct proc* p) {
  int m_id, length, flags = 0, prot, offset;
  void* physical_page;
  struct file* f = NULL;
  uint vaddr;
  acquire(&mtable.lock);
  for (m_id = 0; m_id < MAX_MMAPAREA; m_id++) {
    if (mtable.m_area[m_id].p == p && mtable.m_area[m_id].occupied == 0x2) {
      mtable.m_area[m_id].occupied = 1;
      f = mtable.m_area[m_id].f;
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
  for (int j = (length + PGSIZE - 1) / PGSIZE, i = 0; i < j; i++) {
    if ((physical_page = kalloc()) < 0) return 0;                   // return 0: kalloc function fail
    else if (BITCHECK(flags, 0)) memset(physical_page, 0, PGSIZE);  // MAP_ANONYMOUS is given
    else {                                                          // MAP_ANONYMOUS is NOT given
      f->off = offset;
      if ((fileread(f, physical_page, PGSIZE)) < 0) return 0;
    }
    if ((mmap_helper(p, (void*)(vaddr + i * PGSIZE), PGSIZE, physical_page, prot)) < 0) return 0;
  }
  acquire(&mtable.lock);
  mtable.m_area[m_id].occupied = 0x1;
  release(&mtable.lock);
  return 1;
}

void
mmap_exit(struct proc* p) {
  int m_id;
  for (m_id = 0; m_id < MAX_MMAPAREA; m_id++) {
    if (mtable.m_area[m_id].p == p && mtable.m_area[m_id].occupied == 1) {
      munmap(mtable.m_area->addr);
    }
  }
}
