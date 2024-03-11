/*********************************************************
*   Copyright (C) LiXingang
*
********************************************************/
#include "utl/ulc_user.h"

noinline int test2(int a)
{
    a ++;
    return a;
}

noinline int test(int a)
{
    a ++;
    return test2(a);
}

SEC("tcmd/hello_test")
int main()
{
    if (test(0) != 2) {
        printf("Test Failed\n");
        return -1;
    }

    printf("Test OK \n");
    return 0;
}


