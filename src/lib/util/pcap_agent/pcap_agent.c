
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/mbuf_utl.h"
#include "utl/eth_utl.h"
#include "utl/dbg_utl.h"
#include "utl/mutex_utl.h"
#include "utl/pktcap_utl.h"
#include "utl/pcap_agent.h"


typedef struct
{
    DLL_NODE_S stLinkNode;
    USHORT usProtoType;     
    PF_PCAP_AGENT_PKT_INPUT_FUNC pfFunc;
    USER_HANDLE_S stUserHandle;
}PCAP_AGENT_SERVICE_S;

typedef struct
{
    BOOL_T bIsModeExist;
    BOOL_T bStart;
    CHAR szDevName[PKTCAP_NDIS_NAME_MAX_LEN + 1];
    MUTEX_S stMutex;
    PKTCAP_NDIS_HANDLE hNdis;
    THREAD_ID uiTID;
    DLL_HEAD_S stServiceList;
}PCAP_AGENT_S;

typedef struct
{
    UINT uiMaxAgentNum;
    UINT uiDbgFlag;
    PCAP_AGENT_S astAgent[0];
}PCAP_AGENT_CTRL_S;

static UINT g_auiPcapAgentDbgFlag[1] = {0};

static DBG_UTL_DEF_S g_astPcapAgentDbgDef[] = 
{
    {"pcap", "packet", 0, PCAP_AGENT_DBG_FLAG_PACKET},
    {0}
};

static DBG_UTL_CTRL_S g_stPcapAgentDbgCtrl
    = DBG_INIT_VALUE("PcapAgent", g_auiPcapAgentDbgFlag, g_astPcapAgentDbgDef, 1);

static VOID pcap_agent_PktInput(IN UCHAR *pucPktData, IN PKTCAP_PKT_INFO_S *pstPktInfo, IN HANDLE hUserHandle)
{
    PCAP_AGENT_S *pstAgent = hUserHandle;
    PCAP_AGENT_SERVICE_S *pstService;
    ETH_PKT_INFO_S stEthInfo;

    if (pstPktInfo->uiPktCaptureLen != pstPktInfo->uiPktRawLen)
    {
        return;
    }

    if (BS_OK != ETH_GetEthHeadInfo(pucPktData, pstPktInfo->uiPktCaptureLen, &stEthInfo))
    {
        return;
    }

    DBG_UTL_OUTPUT(&g_stPcapAgentDbgCtrl, 0, PCAP_AGENT_DBG_FLAG_PACKET,
        "Recv %d bytes packet, eth type=%d\r\n", pstPktInfo->uiPktRawLen, stEthInfo.usType);

    MUTEX_P(&pstAgent->stMutex);
    DLL_SCAN(&pstAgent->stServiceList, pstService)
    {
        if (pstService->usProtoType == stEthInfo.usType)
        {
            pstService->pfFunc (pucPktData, pstPktInfo, &pstService->stUserHandle);
        }
    }
    MUTEX_V(&pstAgent->stMutex);
}

static void pcap_agent_Main(IN USER_HANDLE_S *pstUserHandle)
{
    PCAP_AGENT_S *pstAgent = pstUserHandle->ahUserHandle[0];
    
    PKTCAP_Loop(pstAgent->hNdis, pcap_agent_PktInput, pstAgent);

    MUTEX_P(&pstAgent->stMutex);
    PKTCAP_CloseNdis(pstAgent->hNdis);
    pstAgent->hNdis = NULL;
    pstAgent->uiTID = 0;
    pstAgent->bStart = FALSE;
    MUTEX_V(&pstAgent->stMutex);

    return;
}

PCAP_AGENT_HANDLE PCAP_AGENT_CreateInstance(IN UINT uiMaxAgentNum)
{
    PCAP_AGENT_CTRL_S *pstAgentCtrl;
    UINT uiSize;
    UINT i;

    uiSize = sizeof(PCAP_AGENT_CTRL_S) + sizeof(PCAP_AGENT_S) * uiMaxAgentNum;

    pstAgentCtrl = MEM_ZMalloc(uiSize);
    if (NULL == pstAgentCtrl)
    {
        return NULL;
    }

    pstAgentCtrl->uiMaxAgentNum = uiMaxAgentNum;
    for (i=0; i<uiMaxAgentNum; i++)
    {
        MUTEX_Init(&pstAgentCtrl->astAgent[i].stMutex);
        DLL_INIT(&pstAgentCtrl->astAgent[i].stServiceList);
    }

    return pstAgentCtrl;
}


UINT PCAP_AGENT_GetAFreeIndex(IN PCAP_AGENT_HANDLE hPcapAgent)
{
    PCAP_AGENT_CTRL_S *pstAgentCtrl = hPcapAgent;
    UINT i;
    
    if (pstAgentCtrl == NULL )
    {
        return PCAP_AGENT_INDEX_INVALID;
    }

    for (i=0; i<pstAgentCtrl->uiMaxAgentNum; i++)
    {
        if (pstAgentCtrl->astAgent[i].szDevName[0] == '\0')
        {
            return i;
        }
    }

    return PCAP_AGENT_INDEX_INVALID;
}

BS_STATUS PCAP_AGENT_SetNdis
(
    IN PCAP_AGENT_HANDLE hPcapAgent,
    IN UINT uiIndex,
    IN CHAR *pcNdisName
)
{
    PCAP_AGENT_CTRL_S *pstAgentCtrl = hPcapAgent;
    
    if ((pstAgentCtrl == NULL ) || (uiIndex >= pstAgentCtrl->uiMaxAgentNum))
    {
        return BS_BAD_PARA;
    }

    TXT_Strlcpy(pstAgentCtrl->astAgent[uiIndex].szDevName, pcNdisName, sizeof(pstAgentCtrl->astAgent[uiIndex].szDevName));

    return BS_OK;
}

UINT PCAP_AGENT_FindIndexByNdisName(IN PCAP_AGENT_HANDLE hPcapAgent, IN CHAR *pcNdisName)
{
    PCAP_AGENT_CTRL_S *pstAgentCtrl = hPcapAgent;
    UINT i;
    
    if ((pstAgentCtrl == NULL) || (NULL == pcNdisName))
    {
        return PCAP_AGENT_INDEX_INVALID;
    }

    for (i=0; i<pstAgentCtrl->uiMaxAgentNum; i++)
    {
        if (strcmp(pstAgentCtrl->astAgent[i].szDevName, pcNdisName) == 0)
        {
            return i;
        }
    }

    return PCAP_AGENT_INDEX_INVALID;
}

CHAR * PCAP_AGENT_GetNdisName
(
    IN PCAP_AGENT_HANDLE hPcapAgent,
    IN UINT uiIndex
)
{
    PCAP_AGENT_CTRL_S *pstAgentCtrl = hPcapAgent;
    
    if ((pstAgentCtrl == NULL ) || (uiIndex >= pstAgentCtrl->uiMaxAgentNum))
    {
        return NULL;
    }

    if (pstAgentCtrl->astAgent[uiIndex].szDevName[0] == '\0')
    {
        return NULL;
    }

    return pstAgentCtrl->astAgent[uiIndex].szDevName;
}

BS_STATUS PCAP_AGENT_Start
(
    IN PCAP_AGENT_HANDLE hPcapAgent,
    IN UINT uiIndex
)
{
    PKTCAP_NDIS_HANDLE hNdis;
    THREAD_ID uiTID;
    USER_HANDLE_S stUserHandle;
    PCAP_AGENT_CTRL_S *pstAgentCtrl = hPcapAgent;
    
    if ((pstAgentCtrl == NULL ) || (uiIndex >= pstAgentCtrl->uiMaxAgentNum))
    {
        return BS_BAD_PARA;
    }

    if (pstAgentCtrl->astAgent[uiIndex].bStart == TRUE)
    {
        return BS_OK;
    }

    if (pstAgentCtrl->astAgent[uiIndex].szDevName[0] == '\0')
    {
        EXEC_OutString(" Please config ndis-name.\r\n");
        return BS_NOT_READY;
    }

    hNdis = PKTCAP_OpenNdis(pstAgentCtrl->astAgent[uiIndex].szDevName, 0, 1);
    if (hNdis == NULL)
    {
        EXEC_OutString(" Can't open ndis.\r\n");
        return BS_CAN_NOT_OPEN;
    }

    stUserHandle.ahUserHandle[0] = &pstAgentCtrl->astAgent[uiIndex];
    uiTID = THREAD_Create("PcapAgent", NULL, pcap_agent_Main, &stUserHandle);
    if (uiTID == THREAD_ID_INVALID) {
        EXEC_OutString(" Can't create thread.\r\n");
        PKTCAP_CloseNdis(hNdis);
        return BS_CAN_NOT_OPEN;
    }

    pstAgentCtrl->astAgent[uiIndex].hNdis = hNdis;
    pstAgentCtrl->astAgent[uiIndex].uiTID = uiTID;
    pstAgentCtrl->astAgent[uiIndex].bStart = TRUE;

    return BS_OK;
}

BS_STATUS PCAP_AGENT_Stop
(
    IN PCAP_AGENT_HANDLE hPcapAgent,
    IN UINT uiIndex
)
{
    PCAP_AGENT_CTRL_S *pstAgentCtrl = hPcapAgent;
    
    if ((pstAgentCtrl == NULL ) || (uiIndex >= pstAgentCtrl->uiMaxAgentNum))
    {
        return BS_BAD_PARA;
    }

    if (pstAgentCtrl->astAgent[uiIndex].bStart == FALSE)
    {
        return BS_OK;
    }

    PKTCAP_BreakLoop(pstAgentCtrl->astAgent[uiIndex].hNdis);

    return BS_OK;
}

BOOL_T PCAP_AGENT_IsStart
(
    IN PCAP_AGENT_HANDLE hPcapAgent,
    IN UINT uiIndex
)
{
    PCAP_AGENT_CTRL_S *pstAgentCtrl = hPcapAgent;
    
    if ((pstAgentCtrl == NULL ) || (uiIndex >= pstAgentCtrl->uiMaxAgentNum))
    {
        return FALSE;
    }

    return pstAgentCtrl->astAgent[uiIndex].bStart;
}

static BS_STATUS pcap_agent_RegService
(
    IN PCAP_AGENT_CTRL_S *pstAgentCtrl,
    IN UINT uiIndex,
    IN USHORT usProtoType,
    IN PF_PCAP_AGENT_PKT_INPUT_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    PCAP_AGENT_SERVICE_S *pstService;

    pstService = MEM_ZMalloc(sizeof(PCAP_AGENT_SERVICE_S));
    if (NULL == pstService)
    {
        return BS_NO_MEMORY;
    }

    pstService->pfFunc = pfFunc;
    if (NULL != pstUserHandle)
    {
        pstService->stUserHandle = *pstUserHandle;
    }

    pstService->usProtoType = usProtoType;

    DLL_ADD(&pstAgentCtrl->astAgent[uiIndex].stServiceList, pstService);

    return BS_OK;
}

static BS_STATUS pcap_agent_UnregService
(
    IN PCAP_AGENT_CTRL_S *pstAgentCtrl,
    IN UINT uiIndex,
    IN USHORT usProtoType,
    IN PF_PCAP_AGENT_PKT_INPUT_FUNC pfFunc
)
{
    PCAP_AGENT_SERVICE_S *pstService;

    DLL_SCAN(&pstAgentCtrl->astAgent[uiIndex].stServiceList, pstService)
    {
        if ((pstService->pfFunc == pfFunc) && (pstService->usProtoType == usProtoType))
        {
            DLL_DEL(&pstAgentCtrl->astAgent[uiIndex].stServiceList, pstService);
            MEM_Free(pstService);
            break;
        }
    }

    return BS_OK;
}

BS_STATUS PCAP_AGENT_RegService
(
    IN PCAP_AGENT_HANDLE hPcapAgent,
    IN UINT uiIndex,
    IN USHORT usProtoType,
    IN PF_PCAP_AGENT_PKT_INPUT_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    BS_STATUS eRet;
    PCAP_AGENT_CTRL_S *pstAgentCtrl = hPcapAgent;
    
    if ((pstAgentCtrl == NULL ) || (uiIndex >= pstAgentCtrl->uiMaxAgentNum))
    {
        return BS_BAD_PARA;
    }

    MUTEX_P(&pstAgentCtrl->astAgent[uiIndex].stMutex);
    eRet = pcap_agent_RegService(pstAgentCtrl, uiIndex, usProtoType, pfFunc, pstUserHandle);
    MUTEX_V(&pstAgentCtrl->astAgent[uiIndex].stMutex);

    return eRet;
}

VOID PCAP_AGENT_UnregService
(
    IN PCAP_AGENT_HANDLE hPcapAgent,
    IN UINT uiIndex,
    IN USHORT usProtoType,
    IN PF_PCAP_AGENT_PKT_INPUT_FUNC pfFunc
)
{
    PCAP_AGENT_CTRL_S *pstAgentCtrl = hPcapAgent;
    
    if ((pstAgentCtrl == NULL ) || (uiIndex >= pstAgentCtrl->uiMaxAgentNum))
    {
        return;
    }

    MUTEX_P(&pstAgentCtrl->astAgent[uiIndex].stMutex);
    pcap_agent_UnregService(pstAgentCtrl, uiIndex, usProtoType, pfFunc);
    MUTEX_V(&pstAgentCtrl->astAgent[uiIndex].stMutex);

    return;
}

static BS_STATUS _pcap_agent_SendPkt
(
    IN PCAP_AGENT_HANDLE hPcapAgent,
    IN UINT uiIndex,
    IN MBUF_S *pstMbuf
)
{
    UCHAR *pucData;
    PCAP_AGENT_CTRL_S *pstAgentCtrl = hPcapAgent;
    BS_STATUS eRet;
    
    if ((pstAgentCtrl == NULL ) || (uiIndex >= pstAgentCtrl->uiMaxAgentNum))
    {
        MBUF_Free(pstMbuf);
        return BS_BAD_PARA;
    }

    if (pstAgentCtrl->astAgent[uiIndex].hNdis == NULL)
    {
        MBUF_Free(pstMbuf);
        return BS_NOT_READY;
    }

    if (BS_OK != MBUF_MakeContinue(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf)))
    {
        MBUF_Free(pstMbuf);
        return (BS_NO_MEMORY);
    }

    pucData = MBUF_MTOD(pstMbuf);

    MUTEX_P(&pstAgentCtrl->astAgent[uiIndex].stMutex);

    eRet = BS_OK;
    if (pstAgentCtrl->astAgent[uiIndex].hNdis != NULL)
    {
        if (FALSE == PKTCAP_SendPacket(pstAgentCtrl->astAgent[uiIndex].hNdis, pucData, MBUF_TOTAL_DATA_LEN(pstMbuf)))
        {
            eRet = BS_CAN_NOT_WRITE;
        }
    }
    
    MUTEX_V(&pstAgentCtrl->astAgent[uiIndex].stMutex);

    MBUF_Free(pstMbuf);

    return eRet;
}

BS_STATUS PCAP_AGENT_SendPkt
(
    IN PCAP_AGENT_HANDLE hPcapAgent,
    IN UINT uiIndex,
    IN MBUF_S *pstMbuf
)
{
    BS_STATUS eRet;

    eRet = _pcap_agent_SendPkt(hPcapAgent, uiIndex, pstMbuf);

    DBG_UTL_OUTPUT(&g_stPcapAgentDbgCtrl, 0, PCAP_AGENT_DBG_FLAG_PACKET,
        "Send %d bytes packet result %d\r\n", MBUF_TOTAL_DATA_LEN(pstMbuf), eRet);

    return eRet;
}

VOID PCAP_AGENT_SetDbgFlag(IN UINT uiFlag)
{
    DBG_UTL_SetDebugFlag(&g_stPcapAgentDbgCtrl, 0, uiFlag);
}

VOID PCAP_AGENT_ClrDbgFlag(IN UINT uiFlag)
{
    DBG_UTL_ClrDebugFlag(&g_stPcapAgentDbgCtrl, 0, uiFlag);
}

VOID PCAP_AGENT_DbgCmd(IN CHAR *pcModuleName, IN CHAR *pcFlagName)
{
    DBG_UTL_DebugCmd(&g_stPcapAgentDbgCtrl, pcModuleName, pcFlagName);
}

VOID PCAP_AGENT_NoDbgCmd(IN CHAR *pcModuleName, IN CHAR *pcFlagName)
{
    DBG_UTL_NoDebugCmd(&g_stPcapAgentDbgCtrl, pcModuleName, pcFlagName);
}
