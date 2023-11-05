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
    "No such", 
    "Already exist", 
    "Bad pointer", 
    "Can't open", 
    "Wrong file", 
    "Not support", 
    "Out of range", 
    "Time out", 
    "No memory", 
    "Null parameter", 
    "No resource", 
    "Bad parameter", 
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
    "Not matched", 
    "Verify failed", 
    "Not init", 
    "Refrenced", 
    "Busy", 
    "Parse failed", 
    "Has been reatched max specifications.", 
    "Strolen", 
};

CHAR * ErrInfo_Get(IN BS_STATUS eRet)
{
    int index = -eRet;

    if (index >= sizeof(g_apcErrInfos)/sizeof(CHAR *)) {
        return "error";
    }

    return g_apcErrInfos[index];
}

