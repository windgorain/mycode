/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-20
* Description: 根据BS_STATUS获取err info
* History:     
******************************************************************************/
#include "bs.h"

static CHAR * g_apcErrInfos[] =
{
    "Success", //0
    "Error", //1
    "No such", //2
    "Already exist", //3
    "Bad pointer", //4
    "Can't open", //5
    "Wrong file", //6
    "Not support", //7
    "Out of range", //8
    "Time out", //9
    "No memory", //10
    "Null parameter", //11
    "No resource", //12
    "Bad parameter", //13
    "Not permit", //14
    "Full", //15
    "Empty", //16
    "Pause", //17
    "Stop", //18
    "Continue", //19
    "Not found", //20
    "Not complete", //21
    "Can't connect", //22
    "Conflict", //23
    "Too long", //24
    "Too small", //25
    "Bad request", //26
    "Again", //27
    "Can't write", //28
    "Not ready", //29
    "Processed", //30
    "Peer closed", //31
    "Not matched", //32
    "Verify failed", //33
    "Not init", //34
    "Refrenced", //35
    "Busy", //36
    "Parse failed", //37
    "Has been reatched max specifications.", //38
    "Strolen", //39
};

CHAR * ErrInfo_Get(IN BS_STATUS eRet)
{
    int index = -eRet;

    if (index >= sizeof(g_apcErrInfos)/sizeof(CHAR *)) {
        return "error";
    }

    return g_apcErrInfos[index];
}

