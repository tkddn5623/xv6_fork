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
  int fd, len;
  char* cp = NULL;
  // struct stat st;
  len = 4096;
  if ((fd = open("o.txt", O_RDWR | O_CREATE)) < 0) exit();
  printf(1, "maybe, open is done[%d]\n", fd);
  for (int i = 0; i < len; i++) {
    printf(fd, "Q");
  }
  // if (filestat(f, &st) < 0) exit();
  if ((cp = (char*)mmap(0, len, PROT_READ | PROT_WRITE, 0, fd, 0)) == MAP_FAILED) exit();
  printf(1, "maybe, mmap is done[%p]\n", cp);
  strcpy(buf, "qwer");
  printf(1, "Start first of cp...\n");
  cp[5] = '\0';
  cp[0] = 'P';
  printf(1, "[%s]\n", cp);
  /*int q[5] = { 1,2,3,4,5 };
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
  }*/
  gets(buf, 100);
  printf(1, "\n unmap go!\n");
  // printf(1, "\n [ret %d], close go!\n", munmap((uint)cp));
  // close(fd);
  // printf(1, "%d\n", freemem());

}