#include "stdio.h"


struct S1 {
    int a, b;
};

struct S2 {
    int x;
    struct S1 s;
};

void func1(struct S1 s) {
    printf("s.a: %d, s.b: %d\n", s.a, s.b);
}

void func2(struct S2 s2) {
    printf("x: %d - s.a: %d, s.b: %d\n", s2.x, s2.s.a, s2.s.b);
}


