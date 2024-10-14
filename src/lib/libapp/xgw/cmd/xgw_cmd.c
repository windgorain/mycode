/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "utl/socket_utl.h"
#include "utl/exec_utl.h"
#include "utl/endian_utl.h"
#include "../h/xgw_vht.h"
#include "../h/xgw_vrt.h"
#include "../h/xgw_cfg_lock.h"
#include "../h/xgw_ovsdb.h"

static int _xgw_cmd_add_vht(int argc, char **argv)
{
    U32 vid, oip, hip;
    int ret;

    if (argc < 5) {
        RETURN(BS_ERR);
    }

    vid = TXT_Strtol(argv[2], 10);
    vid = htonl(vid);
    oip = Socket_Ipsz2IpNet(argv[3]);
    hip = Socket_Ipsz2IpNet(argv[4]);

    if ((! vid) || (! oip) || (! hip)) {
        EXEC_OutString("bad cmd\r\n");
        RETURN(BS_ERR);
    }

    if ((ret = XGW_VHT_Add(vid, oip, hip)) < 0) {
        EXEC_OutString("add vht error\r\n");
        return ret;
    }

    return 0;
}

static int _xgw_cmd_del_vht(int argc, char **argv)
{
    U32 vid, oip;

    if (argc < 4) {
        RETURN(BS_ERR);
    }

    vid = TXT_Strtol(argv[2], 10);
    vid = htonl(vid);
    oip = Socket_Ipsz2IpNet(argv[3]);

    if ((! vid) || (! oip)) {
        EXEC_OutString("bad cmd\r\n");
        RETURN(BS_ERR);
    }

    return XGW_VHT_Del(vid, oip);
}

static int _xgw_cmd_show_vht(U32 vid, U32 oip, U32 hip, void *ud)
{
    EXEC_OutInfo("vni:%u, oip:%pI4, hip:%pI4 \r\n", ntohl(vid), &oip, &hip);
    return 0;
}

static int _xgw_cmd_save_vht_rule(U32 vid, U32 oip, U32 hip, void *ud)
{
    CMD_EXP_OutputCmd(ud, "vht add %u %pI4 %pI4", ntohl(vid), &oip, &hip);
    return 0;
}

static int _xgw_cmd_save_vht(void *file)
{
    return XGW_VHT_Walk(_xgw_cmd_save_vht_rule, file);
}

static int _xgw_cmd_add_vrt(int argc, char **argv)
{
    U32 vid, oip, depth, nvid, hip;
    U64 nexthop;
    int ret;

    if (argc < 7) {
        RETURN(BS_ERR);
    }

    vid = TXT_Strtol(argv[2], 10);
    oip = Socket_Ipsz2IpHost(argv[3]);
    depth = TXT_Strtol(argv[4], 10);
    nvid = TXT_Strtol(argv[5], 10);
    nvid = hton3B(nvid);
    hip = Socket_Ipsz2IpNet(argv[6]);

    if ((! vid) || (! oip) || (! depth) || ((! nvid) && (! hip))) {
        EXEC_OutString("bad cmd\r\n");
        RETURN(BS_ERR);
    }

    nexthop = (U64)nvid << 32 | hip;

    if ((ret = XGW_VRT_AddRoute(vid, oip, depth, nexthop)) < 0) {
        EXEC_OutString("add vrt error\r\n");
        return ret;
    }

    return 0;
}

static int _xgw_cmd_del_vrt(int argc, char **argv)
{
    U32 vid, oip, depth;

    if (argc < 5) {
        RETURN(BS_ERR);
    }

    vid = TXT_Strtol(argv[2], 10);
    oip = Socket_Ipsz2IpHost(argv[3]);
    depth = TXT_Strtol(argv[4], 10);

    if ((! vid) || (! oip) || (! depth)) {
        EXEC_OutString("bad cmd\r\n");
        RETURN(BS_ERR);
    }

    return XGW_VRT_DelRoute(vid, oip, depth);
}

static int _xgw_cmd_show_vrt(U32 vni, U32 ip, int depth, UINT64 nexthop, void *ud)
{
    int nvid = nexthop >> 32;
    U32 hip = nexthop;

    ip = htonl(ip);
    nvid = ntoh3B(nvid);

    EXEC_OutInfo("vni:%u, oip:%pI4, depth:%d, nvni:%d, hip:%pI4 \r\n", vni, &ip, depth, nvid, &hip);

    return 0;
}

static int _xgw_cmd_save_vrt_rule(U32 vni, U32 ip, int depth, UINT64 nexthop, void *ud)
{
    void *file = ud;
    int nvid = nexthop >> 32;
    U32 hip = nexthop;

    ip = htonl(ip);
    nvid = ntoh3B(nvid);

    CMD_EXP_OutputCmd(file, "vrt add %u %pI4 %u %d %pI4", vni, &ip, depth, nvid, &hip);

    return 0;
}

static int _xgw_cmd_save_vrt(void *file)
{
    XGW_VRT_Walk(_xgw_cmd_save_vrt_rule, file);
    return 0;
}


PLUG_API int XGW_CMD_AddVht(int argc, char **argv)
{
    XGW_CFGLOCK_Lock();
    int ret = _xgw_cmd_add_vht(argc, argv);
    XGW_CFGLOCK_Unlock();

    return ret;
}


PLUG_API int XGW_CMD_DelVht(int argc, char **argv)
{
    XGW_CFGLOCK_Lock();
    int ret = _xgw_cmd_del_vht(argc, argv);
    XGW_CFGLOCK_Unlock();

    return ret;
}


PLUG_API int XGW_CMD_ShowVht(int argc, char **argv)
{
    XGW_CFGLOCK_Lock();
    XGW_VHT_Walk(_xgw_cmd_show_vht, NULL);
    XGW_CFGLOCK_Unlock();

    return 0;
}


PLUG_API int XGW_CMD_AddVrt(int argc, char **argv)
{
    XGW_CFGLOCK_Lock();
    int ret = _xgw_cmd_add_vrt(argc, argv);
    XGW_CFGLOCK_Unlock();

    return ret;
}


PLUG_API int XGW_CMD_DelVrt(int argc, char **argv)
{
    XGW_CFGLOCK_Lock();
    int ret = _xgw_cmd_del_vrt(argc, argv);
    XGW_CFGLOCK_Unlock();

    return ret;
}


PLUG_API int XGW_CMD_ShowVrt(int argc, char **argv)
{
    XGW_CFGLOCK_Lock();
    XGW_VRT_Walk(_xgw_cmd_show_vrt, NULL);
    XGW_CFGLOCK_Unlock();

    return 0;
}

PLUG_API int XGW_CMD_Save(void *file)
{
    XGW_CFGLOCK_Lock();

    _xgw_cmd_save_vht(file);
    _xgw_cmd_save_vrt(file);
    XGW_CMD_OvsdbSave(file);

    XGW_CFGLOCK_Unlock();

    return 0;
}


