#include "types.h"
#include "stat.h"
#include "user.h"

void
z(int a) {
    char* p = (void*)a;
    *p = 5;
}
int
main(int argc, char* argv[]) {
    if (argc >= 3 && !strcmp(argv[1], "z")) {
        z(atoi(argv[2]));
        exit();
    }
    printf(1, "START\n");
    int a[500] = { 1 };
    int* b = malloc(500 * sizeof(int));
    printf(1, "alloc done! a [%p]\n", a);
    printf(1, "alloc done! b [%p]\n", b);
    exit();
}