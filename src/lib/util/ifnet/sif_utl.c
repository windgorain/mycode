/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-14
* Description: single if. 单实例if
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/net.h"
#include "utl/txt_utl.h"
#include "utl/sif_utl.h"
#include "utl/mutex_utl.h"

static IF_CONTAINER g_hSifContainer = NULL;
static UINT g_uiSifFlag = 0;
static MUTEX_S g_stSifMutex;

static inline VOID _sif_Lock()
{
    if (g_uiSifFlag & SIF_FLAG_LOCK)
    {
        MUTEX_P(&g_stSifMutex);
    }
}

static inline VOID _sif_UnLock()
{
    if (g_uiSifFlag & SIF_FLAG_LOCK)
    {
        MUTEX_V(&g_stSifMutex);
    }
}

BS_STATUS SIF_Init(IN UINT uiFlag)
{
    g_hSifContainer = IF_CreateContainer();
    if (NULL == g_hSifContainer)
    {
        return BS_ERR;
    }

    if (uiFlag & SIF_FLAG_LOCK)
    {
        MUTEX_Init(&g_stSifMutex);
    }

    g_uiSifFlag = uiFlag;

    return BS_OK;
}

UINT SIF_AllocUserDataIndex()
{
    UINT uiIndex;

    _sif_Lock();
    uiIndex = IF_AllocUserDataIndex(g_hSifContainer);
    _sif_UnLock();

    return uiIndex;
}

BS_STATUS SIF_RegEvent(IN PF_IF_EVENT_FUNC pfEventFunc, IN USER_HANDLE_S *pstUserHandle)
{
    BS_STATUS eRet;
    
    _sif_Lock();
    eRet = IF_RegEvent(g_hSifContainer, pfEventFunc, pstUserHandle);
    _sif_UnLock();

    return eRet;
}

BS_STATUS SIF_SetProtoType(IN CHAR *pcProtoType, IN IF_PROTO_PARAM_S *pstParam)
{
    BS_STATUS eRet;
    
    _sif_Lock();
    eRet = IF_SetProtoType(g_hSifContainer, pcProtoType, pstParam);
    _sif_UnLock();

    return eRet;
}

IF_PROTO_PARAM_S * SIF_GetProtoType(IN CHAR *pcProtoType)
{
    IF_PROTO_PARAM_S *pstParam;
    
    _sif_Lock();
    pstParam = IF_GetProtoType(g_hSifContainer, pcProtoType);
    _sif_UnLock();

    return pstParam;
}

BS_STATUS SIF_SetLinkType(IN CHAR *pcLinkType, IN IF_LINK_PARAM_S *pstParam)
{
    BS_STATUS eRet;
    
    _sif_Lock();
    eRet = IF_SetLinkType(g_hSifContainer, pcLinkType, pstParam);
    _sif_UnLock();

    return eRet;
}

IF_LINK_PARAM_S * SIF_GetLinkType(IN CHAR *pcLinkType)
{
    IF_LINK_PARAM_S *pstParam;
    
    _sif_Lock();
    pstParam = IF_GetLinkType(g_hSifContainer, pcLinkType);
    _sif_UnLock();

    return pstParam;
}

BS_STATUS SIF_SetPhyType(IN CHAR *pcPhyType, IN IF_PHY_PARAM_S *pstParam)
{
    BS_STATUS eRet;
    
    _sif_Lock();
    eRet = IF_SetPhyType(g_hSifContainer, pcPhyType, pstParam);
    _sif_UnLock();

    return eRet;
}

IF_PHY_PARAM_S * SIF_GetPhyType(IN CHAR *pcPhyType)
{
    IF_PHY_PARAM_S *pstParam;
    
    _sif_Lock();
    pstParam = IF_GetPhyType(g_hSifContainer, pcPhyType);
    _sif_UnLock();

    return pstParam;
}

UINT SIF_AddIfType(IN CHAR *pcIfType, IN IF_TYPE_PARAM_S *pstParam)
{
    UINT uiType;
    
    _sif_Lock();
    uiType = IF_AddIfType(g_hSifContainer, pcIfType, pstParam);
    _sif_UnLock();

    return uiType;
}

CHAR * SIF_GetTypeNameByType(IN UINT uiType)
{
    CHAR *pcType;

    _sif_Lock();
    pcType = IF_GetTypeNameByType(g_hSifContainer, uiType);
    _sif_UnLock();

    return pcType;
}

UINT SIF_GetIfTypeFlag(IN CHAR *pcIfType)
{
    UINT uiFlag;

    _sif_Lock();
    uiFlag = IF_GetIfTypeFlag(g_hSifContainer, pcIfType);
    _sif_UnLock();

    return uiFlag;
}

IF_INDEX SIF_CreateIf(IN CHAR *pcIfType)
{
    IF_INDEX ifIndex;

    _sif_Lock();
    ifIndex = IF_CreateIf(g_hSifContainer, pcIfType);
    _sif_UnLock();

    return ifIndex;
}

IF_INDEX SIF_CreateIfByName(IN CHAR *pcIfType, IN CHAR *pcIfName)
{
    IF_INDEX ifIndex;

    _sif_Lock();
    ifIndex = IF_CreateIfByName(g_hSifContainer, pcIfType, pcIfName);
    _sif_UnLock();

    return ifIndex;
}


VOID SIF_DeleteIf(IN IF_INDEX ifIndex)
{
    _sif_Lock();
    IF_DeleteIf(g_hSifContainer, ifIndex);
    _sif_UnLock();
}

CHAR * SIF_GetIfName(IN IF_INDEX ifIndex, OUT CHAR szIfName[IF_MAX_NAME_LEN + 1])
{
    szIfName[0] = '\0';

    SIF_Ioctl(ifIndex, IFNET_CMD_GET_IFNAME, szIfName);

    return szIfName;
}

BS_STATUS SIF_Ioctl(IN IF_INDEX ifIndex, IN IF_IOCTL_CMD_E enCmd, IN HANDLE hData)
{
    BS_STATUS eRet;
    
    _sif_Lock();
    eRet = IF_Ioctl(g_hSifContainer, ifIndex, enCmd, hData);
    _sif_UnLock();

    return eRet;
}

BS_STATUS SIF_PhyIoctl(IN IF_INDEX ifIndex, IN IF_PHY_IOCTL_CMD_E enCmd, IN HANDLE hData)
{
    BS_STATUS eRet;
    
    _sif_Lock();
    eRet = IF_PhyIoctl(g_hSifContainer, ifIndex, enCmd, hData);
    _sif_UnLock();

    return eRet;
}

IF_INDEX SIF_GetIfIndex(IN CHAR *pcIfName)
{
    IF_INDEX ifIndex;
    
    _sif_Lock();
    ifIndex = IF_GetIfIndex(g_hSifContainer, pcIfName);
    _sif_UnLock();

    return ifIndex;
}

BS_STATUS SIF_PhyOutput(IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf)
{
    IF_PHY_OUTPUT_FUNC pfFunc;
    VOID *pProcesserList;
    BS_STATUS eRet;
    
    _sif_Lock();
    pfFunc = IF_GetPhyOutPut(g_hSifContainer, ifIndex);
    pProcesserList = IF_GetPktProcesserList(g_hSifContainer, IF_PKT_PROCESSER_TYPE_PHYOUTPUT, ifIndex);
    _sif_UnLock();

    eRet = IF_NotifyPktProcesser(pProcesserList, ifIndex, pstMbuf);
    if (eRet == BS_PROCESSED)
    {
        return BS_OK;
    }

    if (NULL == pfFunc)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return pfFunc(ifIndex, pstMbuf);
}

BS_STATUS SIF_LinkInput(IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf)
{
    IF_LINK_INPUT_FUNC pfFunc;
    VOID *pProcesserList;
    BS_STATUS eRet;
    
    _sif_Lock();
    pfFunc = IF_GetLinkInput(g_hSifContainer, ifIndex);
    pProcesserList = IF_GetPktProcesserList(g_hSifContainer, IF_PKT_PROCESSER_TYPE_LINKINPUT, ifIndex);
    _sif_UnLock();

    eRet = IF_NotifyPktProcesser(pProcesserList, ifIndex, pstMbuf);
    if (eRet == BS_PROCESSED)
    {
        return BS_OK;
    }

    if (NULL == pfFunc)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return pfFunc(ifIndex, pstMbuf);
}

BS_STATUS SIF_LinkOutput (IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType/* 网络序 */)
{
    IF_LINK_OUTPUT_FUNC pfFunc;
    VOID *pProcesserList;
    BS_STATUS eRet;
    
    _sif_Lock();
    pfFunc = IF_GetLinkOutput(g_hSifContainer, ifIndex);
    pProcesserList = IF_GetPktProcesserList(g_hSifContainer, IF_PKT_PROCESSER_TYPE_LINKOUTPUT, ifIndex);
    _sif_UnLock();

    eRet = IF_NotifyPktProcesser(pProcesserList, ifIndex, pstMbuf);
    if (eRet == BS_PROCESSED)
    {
        return BS_OK;
    }

    if (NULL == pfFunc)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return pfFunc(ifIndex, pstMbuf, usProtoType);
}

BS_STATUS SIF_ProtoInput(IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType/* 网络序 */)
{
    IF_PROTO_INPUT_FUNC pfFunc;
    VOID *pProcesserList;
    BS_STATUS eRet;
    
    _sif_Lock();
    pfFunc = IF_GetProtoInput(g_hSifContainer, ifIndex);
    pProcesserList = IF_GetPktProcesserList(g_hSifContainer, IF_PKT_PROCESSER_TYPE_PROTOINPUT, ifIndex);
    _sif_UnLock();

    eRet = IF_NotifyPktProcesser(pProcesserList, ifIndex, pstMbuf);
    if (eRet == BS_PROCESSED)
    {
        return BS_OK;
    }

    if (NULL == pfFunc)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return pfFunc(ifIndex, pstMbuf, usProtoType);
}

BS_STATUS SIF_SetUserData(IN IF_INDEX ifIndex, IN UINT uiIndex, IN HANDLE hData)
{
    BS_STATUS eRet;
    
    _sif_Lock();
    eRet = IF_SetUserData(g_hSifContainer, ifIndex, uiIndex, hData);
    _sif_UnLock();

    return eRet;
}

BS_STATUS SIF_GetUserData(IN IF_INDEX ifIndex, IN UINT uiIndex, IN HANDLE *phData)
{
    BS_STATUS eRet;
    
    _sif_Lock();
    eRet = IF_GetUserData(g_hSifContainer, ifIndex, uiIndex, phData);
    _sif_UnLock();

    return eRet;
}

IF_INDEX SIF_GetNext(IN IF_INDEX ifIndexCurrent/* 0表示从头开始 */)
{
    IF_INDEX ifIndex;
    
    _sif_Lock();
    ifIndex = IF_GetNext(g_hSifContainer, ifIndexCurrent);
    _sif_UnLock();
    
	return ifIndex;
}

BS_STATUS SIF_RegPktProcesser
(
    IN IF_INDEX ifIndex,
    IN IF_PKT_PROCESSER_TYPE_E enType,
    IN UINT uiPri,  /* 优先级,数字越小优先级越高 */
    IN PF_IF_PKT_PORCESSER_FUNC pfFunc
)
{
    BS_STATUS eRet;

    _sif_Lock();
    eRet = IF_RegPktProcesser(g_hSifContainer, ifIndex, enType, uiPri, pfFunc);
    _sif_UnLock();

    return eRet;
}

