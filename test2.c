#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
// #include "file.h"
#include "mmap.h"
#define NULL ((uint)0)
int
main() {
  static char buf[10000] = { 0 };
  int fd, len;
  // char* cp = NULL, * bp;
  // struct stat st;
  len = 40;
  if ((fd = open("o.txt", O_RDWR | O_CREATE)) < 0) exit();
  // if (filestat(f, &st) < 0) exit();
  // if ((cp = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) exit();
  for (int i = 0; i < len; i++) {
    printf(fd, "z");
  }
  strcpy(buf, "qwer");
  int q[5] = { 1,2,3,4,5 };
  while (1) {
    uint arg;
    int a = -1;
    printf(1, "Q ADDR %p\n", q);
    printf(1, "ARG INPUT\n");
    gets(buf, sizeof(buf));
    arg = atoi(buf);
    printf(1, "ARG IS %p\n", arg);
    a = *(int*)arg;
    printf(1, "REF: %d\n", a);
  }
  // munmap(cp, 5);
  close(fd);


}