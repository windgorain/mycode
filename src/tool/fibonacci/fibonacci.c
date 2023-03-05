/*================================================================
*   Created：2018.09.14
*   Description：
*
================================================================*/
#include <stdio.h>
#include <stdlib.h>

#if 0
int fibonacci(int x)
{
    if (x < 2) {
        return x;
    }

    return fibonacci(x-1) + fibonacci(x-2);
}
#endif

static long long fibonacci(long x)
{
    long long ret;
    long long prev = 0;
    long long next = 1;

    if (x < 2) {
        return x;
    }

    while (x >= 2) {
        x--;
        ret = prev + next;
        prev = next;
        next = ret;
    }

    return ret;
}

int main(int argc, char **argv)
{
    long count;
    char *end;

    if (argc < 2) {
        printf("Usage: %s number \r\n", argv[0]);
        return -1;
    }

    count = strtol(argv[1], &end, 10);

    printf("%lld \n", fibonacci(count));

    return 0;
}

