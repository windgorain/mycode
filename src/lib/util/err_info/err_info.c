/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-20
* Description: 根据BS_STATUS获取err info
* History:     
******************************************************************************/
#include "bs.h"

static CHAR * g_apcErrInfos[] =
{
    "Success",
    "Error",
    "Null parameter",
    "Bad parameter",
    "Bad pointer",
    "Can't open",
    "Wrong file",
    "Not support",
    "Out of range",
    "Time out",
    "Have no memory",
    "Have no such",
    "Have no resource",
    "Already exist",
    "Not permit",
    "Full",
    "Empty",
    "Pause",
    "Stop",
    "Continue",
    "Not found",
    "Not complete",
    "Can't connect",
    "Conflict",
    "Too long",
    "Too small",
    "Bad request",
    "Again",
    "Can't write",
    "Not ready",
    "Processed",
    "Peer closed",
    "Not init",
    "Refrence count is not zero"
};

CHAR * ErrInfo_Get(IN BS_STATUS eRet)
{
    if (eRet >= sizeof(g_apcErrInfos)/sizeof(CHAR *))
    {
        return "error";
    }

    return g_apcErrInfos[eRet];
}

