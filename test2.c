#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "param.h"
#include "mmap.h"
#define NULL ((uint)0)
void
printaddr() {
  char buf[500] = { 0 };
  int y[3] = { 1,1,1 };
  char* p;
  printf(1, "y: [%p][%p][%p]\n", y, y + 1, y + 2);
  while (strcmp(gets(buf, 500), "exit")) {
    printf(1, "The freemem(void) : %d\n", freemem());
    *strchr(buf, '\n') = '\0';
    printf(1, "[%s]\n", buf);
    if (!(strcmp(buf, "malloc"))) {
      *strchr(buf, '\n') = '\0';
      int N = atoi(gets(buf, 500));
      p = malloc(N);
      printf(1, "The pointer %p\n", p);
    }
  }


}
int
main(int argc, char* argv[]) {
  printaddr();
  /*int fd, len;
  // char* cp = NULL;
  struct stat st;
  len = 40;
  if ((fd = open("o.txt", O_RDWR | O_CREATE)) < 0) exit();
  if (filestat(fd, &st) < 0) exit();
  // if ((cp = mmap(0, len, PROT_READ | PROT_WRITE, MAP_POPULATE, fd, 0)) == MAP_FAILED) exit();
  printf(1, "fd is %d\n", fd);
  for (int i = 0; i < len; i++) {
      write(fd, "z", 1);
  }
  strcpy(cp, "ABCD");
  cp[0] = 'Q';
  cp[1] = 'W';
  cp[2] = '\0';

  printf("[%p][%s]\n", cp, cp);
  cp[10] = 'X';
  // munmap(cp, 5);
  printf("[%p]\n", cp);
  // cp[10] = 'X';
  close(fd);*/


}