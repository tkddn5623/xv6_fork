// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
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
int num_free_pages;
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
try_again:
  r = kmem.freelist;
  if (!r && reclaim())
	  goto try_again;
  if(r)
    kmem.freelist = r->next;
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

void
mappages_helper(void* va, pde_t* pgdir, int pte_u_mask) {
  struct page* p;
  if (!pte_u_mask) return;
  // cprintf("It will be HELPED\n");
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
    if (i == PHYSTOP / PGSIZE)
      panic("full page array");
    pages[i].next = page_lru_head;
    pages[i].prev = p;
    p->next = &pages[i];
    page_lru_head->prev = &pages[i];
  }
}

static int
reclaim() {
  struct page* p, * head = page_lru_head;
  if (!head) return 0; //OOM
  for (p = head;; p = p->next) {
    if (is_pte_u(p->pgdir, p->vaddr)) {
      struct page* next = p->next;
      struct page* prev = p->prev;
      next->prev = prev;
      prev->next = next;
      kmem.freelist = p->vaddr;
      break;
    }
    else {
      page_lru_head = page_lru_head->next;
    }
  }
  return 1;
}