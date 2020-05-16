/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-5-4
* Description: vnic instance
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_vnic.h"

#define _VNIC_INS_MAX_RECEIVER_NUM 8 /* 最多支持32个receiver */

typedef struct
{
    PF_VNICAPP_PKT_RECEIVER pfRecver;
}_VNIC_INS_RECEIVER_S;

static VNIC_HANDLE g_hVnicIns = NULL;
static VNIC_AGENT_HANDLE g_hVnicAgent = NULL;
static MAC_ADDR_S g_stVnicHostMac;
static _VNIC_INS_RECEIVER_S g_astVnicPktRecver[_VNIC_INS_MAX_RECEIVER_NUM];

static BS_STATUS _vnic_PktInput(IN HANDLE hVnicAgentId, IN MBUF_S *pstMbuf, IN HANDLE hUserHandle)
{
    PF_VNICAPP_PKT_RECEIVER pfRecver;
    MBUF_S *pstMbufTmp;
    UINT i;
    UINT uiNum = 0;
    _VNIC_INS_RECEIVER_S astRecver[_VNIC_INS_MAX_RECEIVER_NUM];

    for (i=0; i<_VNIC_INS_MAX_RECEIVER_NUM; i++)
    {
        pfRecver = g_astVnicPktRecver[i].pfRecver;
        if (NULL == pfRecver)
        {
            continue;
        }
        astRecver[uiNum].pfRecver = pfRecver;
        uiNum++;
    }

    if (uiNum == 0)
    {
        MBUF_Free(pstMbuf);
        return BS_OK;
    }

    for (i=1; i<uiNum; i++)
    {
        pstMbufTmp = MBUF_RawCopy(pstMbuf, 0, MBUF_TOTAL_DATA_LEN(pstMbuf), 0);
        if (NULL != pstMbufTmp)
        {
            astRecver[i].pfRecver(pstMbufTmp);
        }
    }

    astRecver[0].pfRecver(pstMbuf);

    return BS_OK;
}

BS_STATUS VnicIns_Init()
{
    UINT uiLen;
    UINT uiStatus = 1;

    if (g_hVnicIns != NULL)
    {
        return BS_OK;
    }

    g_hVnicIns = VNIC_Dev_Open();

    if (NULL == g_hVnicIns)
    {
        return BS_ERR;
    }

    g_hVnicAgent = VNIC_Agent_Create();
    if (NULL == g_hVnicAgent)
    {
        VNIC_Delete(g_hVnicIns);
        g_hVnicIns = NULL;
        return BS_ERR;
    }

    VNIC_Agent_SetVnic(g_hVnicAgent, g_hVnicIns);

    VNIC_Ioctl(g_hVnicIns, TAP_WIN_IOCTL_GET_MAC, NULL, 0, (UCHAR*)&g_stVnicHostMac, sizeof(g_stVnicHostMac), &uiLen);

    VNIC_Agent_Start(g_hVnicAgent, _vnic_PktInput, NULL);

    return BS_OK;
}

BS_STATUS VnicIns_Start()
{
    UINT uiStatus = 1;
    UINT uiLen;

    VNIC_Ioctl(g_hVnicIns, TAP_WIN_IOCTL_SET_MEDIA_STATUS, (UCHAR*)&uiStatus, 4, (UCHAR*)&uiStatus, 4, &uiLen);

    return BS_OK;
}

BS_STATUS VnicIns_Stop()
{
    UINT uiStatus = 0;
    UINT uiLen;

    return VNIC_Ioctl(g_hVnicIns, TAP_WIN_IOCTL_SET_MEDIA_STATUS, (UCHAR*)&uiStatus, 4, (UCHAR*)&uiStatus, 4, &uiLen);
}

BS_STATUS VnicIns_RegRecver(IN PF_VNICAPP_PKT_RECEIVER pfRecver)
{
    UINT i;

    for (i=0; i<_VNIC_INS_MAX_RECEIVER_NUM; i++)
    {
        if (g_astVnicPktRecver[i].pfRecver == NULL)
        {
            g_astVnicPktRecver[i].pfRecver = pfRecver;
            break;
        }
    }

    if (i < _VNIC_INS_MAX_RECEIVER_NUM)
    {
        return BS_OK;
    }

    return BS_NO_RESOURCE;
}

VNIC_HANDLE VnicIns_GetVnicHandle()
{
    return g_hVnicIns;
}

BS_STATUS VnicIns_Output(IN MBUF_S *pstMbuf)
{
    return VNIC_Agent_Write(g_hVnicAgent, pstMbuf);
}

MAC_ADDR_S * VnicIns_GetVnicMac()
{
    return &g_stVnicHostMac;
}

