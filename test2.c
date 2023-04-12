#include "types.h"
#include "stat.h"
#include "user.h"
int main() {
    static int ar[1000];
    int range = 1 << 30;
    for (int i = 0; i < range; i++) {
        int ii = i % 10000;
        ar[ii]++;
        if (i < range - 10) i = 0;
    }
    printf(1, "%d!\n", ar[0]);
}