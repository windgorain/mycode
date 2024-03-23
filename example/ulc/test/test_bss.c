/*********************************************************
*   Copyright (C) LiXingang
*
********************************************************/
#include "utl/ulc_user.h"

static int g_count1 = 0;

SEC("tcmd/main")
int main()
{
    g_count1 ++;

    printf("count1:%d \n", g_count1);

    return 0;
}


