/*********************************************************
*   Copyright (C) LiXingang
*       仅仅支持jitted, 且call back内不使用ctx的情况
*
********************************************************/
#include "utl/ulc_user.h"
#include "utl/ulc_user_sys.h"

static noinline int _test_call_back()
{
    printf("call back \n");
    return 0;
}

SEC("tcmd/main")
int main()
{
    ulc_call_back(_test_call_back, 0, 0, 0, 0);

    return 0;
}


