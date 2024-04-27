/*********************************************************
*   Copyright (C) LiXingang
*
********************************************************/
#include "utl/ulc_user.h"

static char g_timer[128];

void timer_out(void)
{
    printf("time out \n");
    ulc_add_timer(g_timer, 1000);
}

SEC(".spf.cmd/main")
int main()
{
    ulc_init_timer(g_timer, timer_out);
    ulc_add_timer(g_timer, 100);
    ulc_sys_usleep(1000*1000*5);
    ulc_del_timer(g_timer);
    return 0;
}


