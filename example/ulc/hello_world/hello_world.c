/*********************************************************
*   Copyright (C) LiXingang
*
********************************************************/
#include "utl/ulc_user.h"

SEC("tcmd/hello_world")
int main()
{
    printf("Hello world! \n");
    return XDP_PASS;
}


