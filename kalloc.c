// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "fs.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
} kmem;

struct page pages[PHYSTOP/PGSIZE];
struct page* page_lru_head;
// int num_free_pages;
int num_lru_pages = 0;
int swapbitmap[PGSIZE];
static int reclaim();

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;
  if(kmem.use_lock)
    acquire(&kmem.lock);
  do {
    r = kmem.freelist;
  } while (!r && reclaim());
  if (r) {
    kmem.freelist = r->next;
  } else {
    cprintf("Out of memory\n");
  } 
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}

/* Return current number of free memory pages [project3, 4]*/
int
kmem_len(void) {
  struct run* r;
  int i = 0;
  if (kmem.use_lock)
    acquire(&kmem.lock);
  for (r = kmem.freelist; r; r = r->next) {
    i++;
  }
  if (kmem.use_lock)
    release(&kmem.lock);
  return i;
}

static int
reclaim() {
  struct page* p, * head = page_lru_head, * tail;
  if (!head) return 0; //Fail
  uint blkno;
  tail = head->prev;
  for (p = head;; p = p->next) {
    if (!pte_logical_and(p->pgdir, p->vaddr, PTE_A)) {
      struct page* next = p->next;
      struct page* prev = p->prev;
      next->prev = prev;
      prev->next = next;
      p->next = (struct page*)0x0;
      p->prev = (struct page*)0x0;
      
      num_lru_pages--;
      blkno = balloc_for_swap();
      swapwrite(p->vaddr, blkno);   //1. Use swapwrite() function
      swapbitmap[blkno] = (uint)p->vaddr; //Bitmap entry
      pte_pte_swap_set(p->pgdir, p->vaddr);          //Marked unused bits as for SWAP
      pte_pte_p_clear(p->pgdir, p->vaddr);           //3. PTE_P will be cleared
      pte_set_swapoffset(p->pgdir, p->vaddr, blkno); //2. Victim page’s PTE will be set as swap space offset
      kmem.freelist = (struct run*)(p->vaddr);
      break;
    }
    else {
      pte_pte_a_clear(p->pgdir, p->vaddr);
      page_lru_head = page_lru_head->next;
    }
    if (p == tail) return 0;
  }
  return 1;
}

int
swapin(pde_t* pgdir, uint va) {
  char* np = kalloc();
  if (!np) return -1;
  uint blkno = va >> 12;
  swapbitmap[blkno] = 0; //Bit in bitmap is cleared when page swapped in
  swapread(np, blkno);
  swapin_helper(pgdir, (char*)va, np);
  return 0;
}

void
lru_insert(pde_t* pgdir, void* va, int pte_u_mask) {
  if (!pte_u_mask) return;
  struct page* p;
  num_lru_pages++;
  if (!page_lru_head) {
    page_lru_head = &pages[0];
    pages[0].next = &pages[0];
    pages[0].prev = &pages[0];
    pages[0].pgdir = pgdir;
    pages[0].vaddr = va;
  }
  else {
    int i;
    p = page_lru_head->prev;
    for (i = 0; i < PHYSTOP / PGSIZE; i++) {
      if (pages[i].next) break;
    }
    if (i >= PHYSTOP / PGSIZE)
      panic("full page array");
    pages[i].next = page_lru_head;
    pages[i].prev = p;
    p->next = &pages[i];
    page_lru_head->prev = &pages[i];
  }
}

//Present pages should be freed, set PTE bits to 0 and remove them from LRU list
void
lru_delete(pde_t* pgdir, char* va) { 
  struct page* p, * head = page_lru_head;
  if (!head) return;
  for (p = head;; p = p->next) {
    if (p->vaddr == va && p->next) {
      struct page* next = p->next;
      struct page* prev = p->prev;
      next->prev = prev;
      prev->next = next;
      p->next = (struct page*)0x0;
      p->prev = (struct page*)0x0;
      num_lru_pages--;
      break;
    }
  }
  for (int i = 0; i < PGSIZE; i++) {
    if (swapbitmap[i] == (uint)va) swapbitmap[i] = 0; //cleared in bitmap and set PTE bits to 0
  }
  return;
}