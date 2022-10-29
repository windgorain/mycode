/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "../h/ulc_fd.h"
#include "../h/ulc_map.h"
#include "../h/ulc_hookpoint.h"
#include "../h/ulc_loader.h"

int ULC_Init()
{
    int ret = 0;
    static int inited = 0;

    if (inited == 0) {
        inited = 1;
        ret |= ULC_Loader_Init();
        ret |= ULC_MapArray_Init();
        ret |= ULC_MapHash_Init();
    }

    return ret;
}

