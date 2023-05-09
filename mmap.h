#ifndef MMAP_PARAM    // param.h also has definitions
#define MMAP_PARAM
#define PROT_READ     0x1
#define PROT_WRITE    0x2
#define PROT_EXEC	  0x4  // JUST DECORATION :D
#define MAP_ANONYMOUS 0x1
#define MAP_POPULATE  0x2
#define MAX_MMAPAREA  64
#define MAP_FAILED	  ((uint)-1)
#define MMAP_BASE     0x40000000
#endif

struct mmap_area {
    struct file* f;
    uint addr;
    int length;
    int offset;
    int prot;
    int flags;
    struct proc* p; // the process with this mmap_area
    int occupied;
};


/*
Written 2023.5.6 Sat
project3
*/
