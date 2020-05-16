/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-9-1
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ip_list.h"
#include "utl/lstr_utl.h"
#include "utl/num_list.h"

#define SVPNC_TR_FILE_NAME "c:\\sslvpnlsp.data"

typedef struct
{
    DLL_NODE_S stLinkNode;
    IPLIST_S stIpList;
    NUM_LIST_S stPortList;
}_SVPNC_TR_RES_NODE_S;


static DLL_HEAD_S g_stSvpncTrResList = DLL_HEAD_INIT_VALUE(&g_stSvpncTrResList);

static VOID _svpnc_trres_ExitNotify(IN INT lExitNum, IN USER_HANDLE_S *pstUserHandle)
{
    FILE_DelFile(SVPNC_TR_FILE_NAME);
}

BS_STATUS SVPNC_TrRes_Init()
{
    SYSRUN_RegExitNotifyFunc(_svpnc_trres_ExitNotify, NULL);
}

BS_STATUS SVPNC_TrRes_Add(IN CHAR *pcName, IN CHAR *pcServerAddress)
{
    _SVPNC_TR_RES_NODE_S *pstResNode;
    LSTR_S stStr;
    LSTR_S stAddress;
    LSTR_S stPorts;    

    pstResNode = MEM_ZMalloc(sizeof(_SVPNC_TR_RES_NODE_S));
    if (NULL == pstResNode)
    {
        return BS_NO_MEMORY;
    }

    stStr.pcData = pcServerAddress;
    stStr.uiLen = strlen(pcServerAddress);

    LSTR_Split(&stStr, ':', &stAddress, &stPorts);

    IPList_Init(&pstResNode->stIpList);
    NumList_Init(&pstResNode->stPortList);

    IPList_ParseString(&pstResNode->stIpList, &stAddress);
    IPList_Continue(&pstResNode->stIpList);

    NumList_ParseLstr(&pstResNode->stPortList, &stPorts);
    NumList_Continue(&pstResNode->stPortList);

    DLL_ADD(&g_stSvpncTrResList, pstResNode);

    return BS_OK;
}

VOID SVPNC_TR_Write2File()
{
    _SVPNC_TR_RES_NODE_S *pstResNode;
    FILE *fp;
    UINT uiStartIp;
    UINT uiStopIp;
    USHORT usStartPort = 1;
    USHORT usStopPort = 65535;

    FILE_DelFile(SVPNC_TR_FILE_NAME);
    
    fp = FILE_Open(SVPNC_TR_FILE_NAME, TRUE, "wb+");
    if (NULL == fp)
    {
        return;
    }

    DLL_SCAN(&g_stSvpncTrResList, pstResNode)
    {
        IPLIST_SCAN_BEGIN(&pstResNode->stIpList, uiStartIp, uiStopIp)
        {
            fwrite(&uiStartIp, 1, 4, fp);
            fwrite(&uiStopIp, 1, 4, fp);
            fwrite(&usStartPort, 1, 2, fp);  /* 暂时不支持port写入,故仅仅写入1-65535 */
            fwrite(&usStopPort, 1, 2, fp);
        }IPLIST_SCAN_END();
    }

    FILE_Close(fp);

    return;
}

BOOL_T SVPNC_TR_IsPermit(IN UINT uiIp/* 主机序 */, IN USHORT usPort/* 主机序 */)
{
    _SVPNC_TR_RES_NODE_S *pstNode;

    DLL_SCAN(&g_stSvpncTrResList, pstNode)
    {
        if ((TRUE == IPList_IsIPInTheList(&pstNode->stIpList, uiIp))
            && (TRUE == NumList_IsNumInTheList(&pstNode->stPortList, usPort)))
        {
            return TRUE;
        }
    }

    return FALSE;
}

