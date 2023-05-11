#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "mmap.h"
#define NULL ((void*)0)
int
main() {
  static char buf[10000] = { 0 };
  int fd, len, pid;
  char* cp = NULL;
  len = 4096 * 1;
  if ((fd = open("o.txt", O_RDWR | O_CREATE)) < 0) exit();
  for (int i = 0; i < len; i++) {
    printf(fd, "Q");
  }
  printf(1, "free mem: %d\n", freemem());
  if ((cp = (char*)mmap(0, len, PROT_READ | PROT_WRITE, 0, fd, 0)) == MAP_FAILED) exit();
  strcpy(buf, "qwer");
  cp[10] = '\0';
  cp[0] = 'P';
  printf(1, "[%s]\n", cp);
  printf(1, "free mem: %d\n", freemem());

  if ((pid = fork()) == 0) {
    printf(1, "(%d)F\n", pid);
    cp[1] = 'W';
    printf(1, "[%s]\n", cp);
    exit();
  }
  else {
    wait();
    printf(1, "(%d)U\n", pid);
    cp[2] = 'E';
    printf(1, "[%s]\n", cp);
  }
  close(fd);
  printf(1, "before free mem: %d\n", freemem());
  munmap((uint)cp);
  printf(1, "after free mem: %d\n", freemem());
}