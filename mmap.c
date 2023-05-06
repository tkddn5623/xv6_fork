#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
// #include "proc.h"
#include "spinlock.h"
#include "mmap.h"

struct {
    struct spinlock lock;
    struct mmap_area m_area[MAX_MMAPAREA];
} mtable;