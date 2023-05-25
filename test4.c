#include "types.h"
#include "stat.h"
#include "user.h"

int
main() {
    printf(1, "START\n");
    int a[500] = { 1 };
    int* b = malloc(500 * sizeof(int));
    printf(1, "alloc done! a [%p]\n", a);
    printf(1, "alloc done! b [%p]\n", b);
    exit();
}