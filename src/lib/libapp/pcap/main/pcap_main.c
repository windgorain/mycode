/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "comp/comp_pcap.h"

#include "../h/pcap_main.h"
#include "../h/pcap_dbg.h"

static PCAP_AGENT_HANDLE g_hPcapAgent = NULL;
static PKTCAP_NIDS_INFO_HANDLE g_hPcapMainPcapListHead = NULL;

static PKTCAP_NIDS_INFO_HANDLE _pcapmain_GetNdisListHead()
{
    if (g_hPcapMainPcapListHead == NULL)
    {
        g_hPcapMainPcapListHead = PKTCAP_GetNdisInfoList();
    }

    return g_hPcapMainPcapListHead;
}

static BOOL_T _pcapmain_IsNdisExist(IN CHAR *pcName)
{
    PKTCAP_NIDS_INFO_HANDLE hNdis, hNdisCurrent;
    
    hNdis = _pcapmain_GetNdisListHead();
    hNdisCurrent = hNdis;

    while(hNdisCurrent)
    {
        if (strcmp(pcName, PKTCAP_GetNdisInfoName(hNdisCurrent)) == 0)
        {
            return TRUE;
        }
        hNdisCurrent = PKTCAP_GetNextNdisInfo(hNdisCurrent);
    }

    return FALSE;
}

BS_STATUS PCAP_InitPcapList()
{
    PKTCAP_NIDS_INFO_HANDLE hInfoList, hInfo;    
    UINT uiIndex;
    CHAR *pcName;

    hInfoList = _pcapmain_GetNdisListHead();
    if (NULL == hInfoList)
    {
        return BS_ERR;
    }

    hInfo = hInfoList;

    while(hInfo)
    {
        pcName = PKTCAP_GetNdisInfoName(hInfo);

        if ((PCAP_AGENT_INDEX_INVALID == PCAP_AGENT_FindIndexByNdisName(g_hPcapAgent, pcName))
            && (PKTCAP_IsEthDataLink(PKTCAP_GetDataLinkTypeByName(pcName))))
        {
            uiIndex = PCAP_AGENT_GetAFreeIndex(g_hPcapAgent);
            if (PCAP_AGENT_INDEX_INVALID == uiIndex)
            {
                break;
            }
            PCAP_AGENT_SetNdis(g_hPcapAgent, uiIndex, PKTCAP_GetNdisInfoName(hInfo));
        }

        hInfo = PKTCAP_GetNextNdisInfo(hInfo);
    }

	return BS_OK;
}

BS_STATUS PCAP_Main_Init()
{
    g_hPcapAgent = PCAP_AGENT_CreateInstance(PCAP_MAX_NUM);
    if (NULL == g_hPcapAgent)
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

BS_STATUS PCAP_Main_SetNdis(IN UINT uiIndex, IN CHAR *pcNdisName)
{
    if (! _pcapmain_IsNdisExist(pcNdisName))
    {
        return BS_ERR;
    }
    
    return PCAP_AGENT_SetNdis(g_hPcapAgent, uiIndex, pcNdisName);
}

CHAR * PCAP_Main_GetNdisName(IN UINT uiIndex)
{
    return PCAP_AGENT_GetNdisName(g_hPcapAgent, uiIndex);
}

BS_STATUS PCAP_Main_Start(IN UINT uiIndex)
{
    return PCAP_AGENT_Start(g_hPcapAgent, uiIndex);
}

BS_STATUS PCAP_Main_Stop(IN UINT uiIndex)
{
    return PCAP_AGENT_Stop(g_hPcapAgent, uiIndex);
}

BOOL_T PCAP_Main_IsStart(IN UINT uiIndex)
{
    return PCAP_AGENT_IsStart(g_hPcapAgent, uiIndex);
}

PLUG_API BS_STATUS PCAP_RegService
(
    IN UINT uiIndex,
    IN USHORT usProtoType,
    IN PF_PCAP_AGENT_PKT_INPUT_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    return PCAP_AGENT_RegService(g_hPcapAgent, uiIndex, usProtoType, pfFunc, pstUserHandle);
}

VOID PCAP_Main_UnregService
(
    IN UINT uiIndex,
    IN USHORT usProtoType,
    IN PF_PCAP_AGENT_PKT_INPUT_FUNC pfFunc
)
{
    PCAP_AGENT_UnregService(g_hPcapAgent, uiIndex, usProtoType, pfFunc);
}

PLUG_API BS_STATUS PCAP_SendPkt(IN UINT uiIndex, IN MBUF_S *pstMbuf)
{
    BS_STATUS eRet;
    
    eRet = PCAP_AGENT_SendPkt(g_hPcapAgent, uiIndex, pstMbuf);

    return eRet;
}

static BS_STATUS _pcapmain_GetNetInfo
(
    IN UINT uiIndex,
    OUT NETINFO_ADAPTER_S *pstInfo
)
{
    CHAR *pcNdisName;

    pcNdisName = PCAP_Main_GetNdisName(uiIndex);
    if (NULL == pcNdisName)
    {
        return BS_NO_SUCH;
    }

    return NETINFO_GetAdapterInfo(pcNdisName, pstInfo);
}

static BS_STATUS _pcapmain_GetName
(
    IN UINT uiIndex,
    OUT CHAR *pcName
)
{
    CHAR *pcNdisName;

    pcNdisName = PCAP_Main_GetNdisName(uiIndex);
    if (NULL == pcNdisName)
    {
        return BS_NO_SUCH;
    }

    TXT_StrCpy(pcName, pcNdisName);

    return BS_OK;
}

static BS_STATUS _pcapmain_SetStart(IN UINT uiIndex, OUT BOOL_T bStart)
{
    if (bStart)
    {
        return PCAP_Main_Start(uiIndex);
    }

    return PCAP_Main_Stop(uiIndex);
}

static BS_STATUS _pcapmain_GetStart(IN UINT uiIndex, OUT BOOL_T* pbStart)
{
    if (PCAP_Main_IsStart(uiIndex))
    {
        *pbStart = TRUE;
    }
    else
    {
        *pbStart = FALSE;
    }

    return BS_OK;
}

static BS_STATUS _pcapmain_GetPrivateInfo(IN UINT uiIndex, OUT LSTR_S *pstLstr)
{
    PKTCAP_NIDS_INFO_HANDLE hList;
    PKTCAP_NIDS_INFO_HANDLE hNdisInfo;
    CHAR *pcNdisName;
    UINT uiSize = pstLstr->uiLen;
    CHAR *pcData = pstLstr->pcData;

    pstLstr->uiLen = 0;

    pcNdisName = PCAP_Main_GetNdisName(uiIndex);
    if (TXT_IS_EMPTY(pcNdisName))
    {
        return BS_NOT_FOUND;
    }

    hList = _pcapmain_GetNdisListHead();
    hNdisInfo = PKTCAP_FindNdisByNameInNdisInfoList(hList, pcNdisName);
    if (hNdisInfo != NULL)
    {
        PKTCAP_GetNdisInfoString(hNdisInfo, uiSize, pcData);
    }

    return BS_OK;
}

PLUG_API BS_STATUS PCAP_Ioctl(IN UINT uiIndex, IN UINT uiCmd, INOUT VOID *pData)
{
    BS_STATUS eRet = BS_OK;

    switch (uiCmd)
    {
        case PCAP_IOCTL_CMD_GET_NET_INFO:
        {
            eRet = _pcapmain_GetNetInfo(uiIndex, pData);
            break;
        }

        case PCAP_IOCTL_CMD_GET_NAME:
        {
            eRet = _pcapmain_GetName(uiIndex, pData);
            break;
        }

        case PCAP_IOCTL_CMD_SET_START:
        {
            eRet = _pcapmain_SetStart(uiIndex, HANDLE_UINT(pData));
            break;
        }

        case PCAP_IOCTL_CMD_GET_START:
        {
            eRet = _pcapmain_GetStart(uiIndex, pData);
            break;
        }

        case PCAP_IOCTL_GET_PRIVATE_INFO:
        {
            eRet = _pcapmain_GetPrivateInfo(uiIndex, pData);
            break;
        }

        default:
        {
            eRet = BS_NOT_SUPPORT;
            break;
        }
    }

    return eRet;
}


