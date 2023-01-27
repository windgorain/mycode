/*********************************************************
*   Copyright (C) LiXingang
*
********************************************************/
#include "utl/ulc_user.h"

SEC("xdp/hello_xdp")
int hello_xdp(struct xdp_md *ctx)
{
    BPF_Print("Hello xdp! \n");
    return XDP_PASS;
}


