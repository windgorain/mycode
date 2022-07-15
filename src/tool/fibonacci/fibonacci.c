/*================================================================
*   Created：2018.09.14
*   Description：
*
================================================================*/
#include <stdio.h>

int fibonacci(int x)
{
    if (x < 2) {
        return x;
    }

    return fibonacci(x-1) + fibonacci(x-2);
}

int main()
{
    printf("%d\n", fibonacci(40));

    return 0;
}
