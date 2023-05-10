#include "types.h"
#include "defs.h"
#include "param.h"

int
sys_mmap(void) { //Project3
  uint addr;
  int length, prot, flags, fd, offset;
  if (argint(0, (int*)&addr) < 0 ||
    argint(1, &length) < 0 ||
    argint(2, &prot) < 0 ||
    argint(3, &flags) < 0 ||
    argint(4, &fd) < 0 ||
    argint(5, &offset))
    exit();
  return mmap(addr, length, prot, flags ,fd, offset);
}

int
sys_munmap(void) { //Project3
  uint addr;
  if (argint(0, (int*)&addr) < 0) exit();
  return munmap(addr);
}

int
sys_freemem(void) { //Project3
  return freemem();
}