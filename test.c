#include "types.h"
#include "stat.h"
#include "user.h"
int
main(int argc, char* argv[]) {
    if (argc < 2) {
        printf(1, "Usage: Need two (or three) cmd args.\n");
        exit();
    }
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    int c = 20;
    if (a == 1 && argc < 3) {
        printf(1, "Usage: setnice need three args.\n");
        exit();
    }
    if (a == 1) {
        c = atoi(argv[3]);
    }
    printf(1, "argument: [%d, %d]\n", a, b);
    switch (a) {
    case 0: ps(b); break;
    case 1: printf(1, "setnice ret: %d\n", setnice(b, c)); break;
    case 2: printf(1, "getnice ret: %d\n", getnice(b)); break;
    }
    exit();
}