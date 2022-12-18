/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
*************************************************/
#include "bs.h"
#include "../h/bpffunc_func.h"

int BPFFUNC_Init(void)
{
    int ret = 0;

    ret |= BPFFUNC_RuntimeInit();

    return ret;
}
