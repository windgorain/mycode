/*********************************************************
*   Copyright (C) LiXingang
*
********************************************************/
#include "utl/ulc_user.h"

char * g_global_str = "test";

SEC(".spf.cmd/main")
int main()
{
    printf("%s \n", g_global_str);
    return 0;
}


