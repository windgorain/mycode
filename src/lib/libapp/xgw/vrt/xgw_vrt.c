/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "utl/lpm_utl.h"
#include "utl/mem_cap.h"
#include "utl/array_bit.h"
#include "../h/xgw_rcu.h"
#include "../h/xgw_vrt.h"

static MEM_CAP_S g_xgw_vrt_memcap;
static UINT  g_xgw_vrt_bitmap[(XGW_VNI_MAX + 31) / 32];
static void * g_xgw_vrt_tables[XGW_VNI_MAX];

static void _xgw_vrt_free(void *buf)
{
    XGW_RCU_Sync();
    MEM_Free(buf);
}

static int _xgw_vrt_walk(U32 ip, int depth, UINT64 nexthop, void *ud)
{
    USER_HANDLE_S *uh = ud;
    PF_XGW_VRT_Walk walk_func = uh->ahUserHandle[0];
    UINT vni = HANDLE_UINT(uh->ahUserHandle[2]);

    return walk_func(vni, ip, depth, nexthop, uh->ahUserHandle[1]);
}

int XGW_VRT_Init(void)
{
    MemCap_Init(&g_xgw_vrt_memcap, NULL, _xgw_vrt_free, NULL);
    return 0;
}

int XGW_VRT_AddVni(U32 vni)
{
    LPM_S *lpm;
    int ret;

    if (vni >= XGW_VNI_MAX) {
        RETURN(BS_BAD_PARA);
    }

    if (g_xgw_vrt_tables[vni]) {
        return 0;
    }

    lpm = MEM_ZMalloc(sizeof(*lpm));
    if (! lpm) {
        RETURN(BS_NO_MEMORY);
    }

    DLPM64B_Init(lpm, &g_xgw_vrt_memcap);

    ret = LPM_SetLevel(lpm, 4, 8);
    ret |= LPM_EnableRecording(lpm);
    if (ret < 0) {
        LPM_Final(lpm);
        MEM_Free(lpm);
        return ret;
    }

    g_xgw_vrt_tables[vni] = lpm;

    ArrayBit_Set(g_xgw_vrt_bitmap, vni);

    return 0;
}

void XGW_VRT_DelVni(U32 vni)
{
    LPM_S *lpm;

    if (vni >= XGW_VNI_MAX) {
        return;
    }

    lpm = g_xgw_vrt_tables[vni];
    if (! lpm) {
        return;
    }

    g_xgw_vrt_tables[vni] = NULL;
    ArrayBit_Clr(g_xgw_vrt_bitmap, vni);

    XGW_RCU_Sync();

    LPM_Final(lpm);
    MEM_Free(lpm);
}

int XGW_VRT_AddRoute(U32 vni, U32 dip, U8 depth, U64 nexthop)
{
    LPM_S *lpm;

    if (vni >= XGW_VNI_MAX) {
        RETURN(BS_BAD_PARA);
    }

    lpm = g_xgw_vrt_tables[vni];
    if (! lpm) {
        int ret = XGW_VRT_AddVni(vni);
        if (ret < 0) {
            return ret;
        }
        lpm = g_xgw_vrt_tables[vni];
    }

    return LPM_Add(lpm, dip, depth, nexthop);
}

int XGW_VRT_DelRoute(U32 vni, U32 dip, U8 depth)
{
    LPM_S *lpm;

    if (vni >= XGW_VNI_MAX) {
        RETURN(BS_BAD_PARA);
    }

    lpm = g_xgw_vrt_tables[vni];
    if (! lpm) {
        RETURN(BS_NOT_READY);
    }

    return LPM_Del(lpm, dip, depth, 0, 0);
}


U64 XGW_VRT_Match(U32 vni, U32 dip)
{
    LPM_S *lpm;
    U64 nexthop;

    if (vni >= XGW_VNI_MAX) {
        return 0;
    }

    lpm = g_xgw_vrt_tables[vni];
    if (! lpm) {
        return 0;
    }

    if (LPM_Lookup(lpm, dip, &nexthop) < 0) {
        return 0;
    }

    return nexthop;
}

int XGW_VRT_Walk(PF_XGW_VRT_Walk walk_func, void *ud)
{
    UINT vni;
    LPM_S *lpm;
    USER_HANDLE_S uh;

    uh.ahUserHandle[0] = walk_func;
    uh.ahUserHandle[1] = ud;

    ARRAYBIT_SCAN_BUSY_BEGIN(g_xgw_vrt_bitmap, XGW_VNI_MAX, vni) {
        lpm = g_xgw_vrt_tables[vni];
        uh.ahUserHandle[2] = UINT_HANDLE(vni);
        if (lpm) {
            LPM_Walk(lpm, _xgw_vrt_walk, &uh);
        }
    }ARRAYBIT_SCAN_END();

    return 0;
}

int XGW_VRT_WalkVni(U32 vni, PF_LPM_WALK_CB walk_func, void *ud)
{
    LPM_S *lpm;

    if (vni >= XGW_VNI_MAX) {
        RETURN(BS_BAD_PARA);
    }

    lpm = g_xgw_vrt_tables[vni];
    if (! lpm) {
        return 0;
    }

    LPM_Walk(lpm, walk_func, ud);

    return 0;
}

