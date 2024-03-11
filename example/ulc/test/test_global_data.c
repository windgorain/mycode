/*********************************************************
*   Copyright (C) LiXingang
*
********************************************************/
#include "utl/ulc_user.h"

static int g_count1 = 0;
static int g_count2 = 1000;
static char *g_string = "global-test";

SEC("tcmd/main")
int main()
{
    g_count1 ++;
    g_count2 ++;

    printf("%s: count1:%d count2:%d \n", g_string, g_count1, g_count2);

    return 0;
}


