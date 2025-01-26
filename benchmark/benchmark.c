#include <stdio.h>

long fib(long n)
{
    long fibn_minus_2 = 0;
    long fibn_minus_1 = 1;
    long fibn = fibn_minus_1 + fibn_minus_2;
    if (n == 0) {
        return fibn_minus_2;
    } else if (n == 1) {
        return fibn_minus_1;
    }
    for (long i = 2; i <= n; i++) {
        fibn = fibn_minus_1 + fibn_minus_2;
        fibn_minus_2 = fibn_minus_1;
        fibn_minus_1 = fibn;
    }
    return fibn;
}

int main(void)
{
    long num_iters = 1000000;
    long x = 2;
    for (long i = 0; i < num_iters; i++) {
        x = fib(90) - x / 2;
    }
    printf("done:%ld\n", x);
}
