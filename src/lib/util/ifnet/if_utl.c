/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-13
* Description: ifnet. 多实例的
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/net.h"
#include "utl/txt_utl.h"
#include "utl/large_bitmap.h"
#include "utl/if_utl.h"
#include "utl/object_utl.h"

typedef struct
{
    DLL_NODE_S stLinkNode;
    UINT uiPri; /* 优先级,数字越小优先级越高 */
    PF_IF_PKT_PORCESSER_FUNC pfPktFunc;
}_IF_PKT_PROCESSER_NODE_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
    PF_IF_EVENT_FUNC pfEventFcun;
    USER_HANDLE_S stUserHandle;
}_IF_EVENT_CALL_NODE_S;

typedef struct
{
    DLL_HEAD_S stListnerList;   /* _IF_EVENT_CALL_NODE_S 事件监听链表 */
    NO_HANDLE hNetTypeNo;       /* 网络层类型命名对象集合 */
    NO_HANDLE hLinkTypeNo;      /* 链路层类型命名对象集合 */
    NO_HANDLE hPhyTypeNo;       /* 物理层类型命名对象集合 */
    NO_HANDLE hIfTypeNo;        /* IF Type命名对象集合 */
    NO_HANDLE hIfNo;            /* IF命名对象集合 */
    UINT      uiNextUserDataIndex; /* 下一个要分配的UserIndexID */
}IF_CONTAINER_S;

typedef struct
{
    UINT uiFlag;
    IF_PROTO_PARAM_S *pstProtoType;
    IF_LINK_PARAM_S *pstLinkType;
    IF_PHY_PARAM_S *pstPhyType;
    PF_IF_TYPE_CTL pfTypeCtl;
    LBITMAP_HANDLE hIndexBitmap;
}IF_TYPE_S;

typedef struct
{
    IF_PROTO_INPUT_FUNC pfProtoInput;
    IF_LINK_INPUT_FUNC pfLinkInput;
    IF_LINK_OUTPUT_FUNC pfLinkOutput;
    IF_PHY_OUTPUT_FUNC pfPhyOutput;
    IF_TYPE_S *pstType;
    MAC_ADDR_S stMacAddr;
    UINT uiIndex;   /* 在此类型的接口中的Index */
    UINT uiVrf;     /* 此接口所属于的vrf */
    UINT bitPhyStatus:1;
    UINT bitLinkStatus:1;
    UINT bitProtoStatus:1;
    UINT bitShutdown:1;
    UINT bitL3; /* 是L2还是L3接口 */
    DLL_HEAD_S stLinkInputPktProcesser;  /* LinkInput报文业务点 */
    DLL_HEAD_S stLinkOutputPktProcesser;  /* LinkOutput报文业务点 */
    DLL_HEAD_S stProtoInputPktProcesser;  /* ProtoInput报文业务点 */
    DLL_HEAD_S stPhyOutputPktProcesser;  /* PhyOutput报文业务点 */   
}IF_NODE_S;

static VOID _if_EventNotify(IN IF_CONTAINER_S *pstContainer, IN UINT ifIndex, IN UINT uiEvent)
{
    _IF_EVENT_CALL_NODE_S *pstNode;

    DLL_SCAN(&pstContainer->stListnerList, pstNode)
    {
        pstNode->pfEventFcun(ifIndex, uiEvent, &pstNode->stUserHandle);
    }
}

static INT _if_ProcesserCmp(IN DLL_NODE_S *pstNode1, IN DLL_NODE_S *pstNode2, IN HANDLE hHandle)
{
    _IF_PKT_PROCESSER_NODE_S *pstNode = (VOID*)pstNode1;
    _IF_PKT_PROCESSER_NODE_S *pstNodeTmp = (VOID*)pstNode2;

    return (int)(pstNode->uiPri - pstNodeTmp->uiPri);
}

static BS_STATUS _if_RegProcesser
(
    IN DLL_HEAD_S *pstList,
    IN UINT uiPri,  /* 优先级,数字越小优先级越高 */
    IN PF_IF_PKT_PORCESSER_FUNC pfFunc
)
{
    _IF_PKT_PROCESSER_NODE_S *pstNode;

    pstNode = MEM_RcuZMalloc(sizeof(_IF_PKT_PROCESSER_NODE_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    DLL_SortAdd(pstList, (DLL_NODE_S*)pstNode, _if_ProcesserCmp, 0);

    return BS_OK;
}

static VOID _if_UnRegAllProcesser(IN DLL_HEAD_S *pstList)
{
    _IF_PKT_PROCESSER_NODE_S *pstNode;

    while ((pstNode = DLL_Get(pstList)) != NULL)
    {
        MEM_RcuFree(pstNode);
    }
}

static BS_STATUS _if_Shutdown(IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex, IN BOOL_T bShutdown)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_NODE_S *pstNode;

    pstNode = NO_GetObjectByID(pstContainer->hIfNo, ifIndex);

    if (NULL == pstNode)
    {
        return BS_NO_SUCH;
    }

    if (bShutdown)
    {
        bShutdown = 1;
    }

    if (bShutdown == pstNode->bitShutdown)
    {
        return BS_OK;
    }

    pstNode->bitShutdown = bShutdown;

    if ((pstNode->pstType->pstPhyType != NULL) && (pstNode->pstType->pstPhyType->pfPhyIoctl != NULL))
    {
        pstNode->pstType->pstPhyType->pfPhyIoctl(ifIndex, IF_PHY_IOCTL_CMD_SET_SHUTDOWN, (VOID*)(ULONG)bShutdown);
    }

    return BS_OK;
}

IF_CONTAINER IF_CreateContainer(void *memcap)
{
    IF_CONTAINER_S *pstContainer;

    pstContainer = MEM_ZMalloc(sizeof(IF_CONTAINER_S));
    if (NULL == pstContainer)
    {
        return NULL;
    }

    OBJECT_PARAM_S no_param = {0};
    no_param.memcap = memcap;

    no_param.uiObjSize = sizeof(IF_PROTO_PARAM_S);
    pstContainer->hNetTypeNo = NO_CreateAggregate(&no_param);

    no_param.uiObjSize = sizeof(IF_LINK_PARAM_S);
    pstContainer->hLinkTypeNo = NO_CreateAggregate(&no_param);

    no_param.uiObjSize = sizeof(IF_PHY_PARAM_S);
    pstContainer->hPhyTypeNo = NO_CreateAggregate(&no_param);

    no_param.uiObjSize = sizeof(IF_TYPE_S);
    pstContainer->hIfTypeNo = NO_CreateAggregate(&no_param);

    no_param.uiObjSize = sizeof(IF_NODE_S);
    pstContainer->hIfNo = NO_CreateAggregate(&no_param);

    if ((NULL == pstContainer->hNetTypeNo)
        || (NULL == pstContainer->hLinkTypeNo)
        || (NULL == pstContainer->hPhyTypeNo)
        || (NULL == pstContainer->hIfTypeNo)
        || (NULL == pstContainer->hIfNo)) {
        IF_DestroyContainer(pstContainer);
        return NULL;
    }

    DLL_INIT(&pstContainer->stListnerList);

    return pstContainer;
}

VOID IF_DestroyContainer(IN IF_CONTAINER hContainer)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    _IF_EVENT_CALL_NODE_S *pstNode, *pstNodeNext;

    DLL_SAFE_SCAN(&pstContainer->stListnerList, pstNode, pstNodeNext)
    {
        DLL_DEL(&pstContainer->stListnerList, pstNode);
        MEM_Free(pstNode);
    }

    if (NULL != pstContainer->hNetTypeNo)
    {
        NO_DestroyAggregate(pstContainer->hNetTypeNo);
    }

    if (NULL != pstContainer->hLinkTypeNo)
    {
        NO_DestroyAggregate(pstContainer->hLinkTypeNo);
    }

    if (NULL != pstContainer->hPhyTypeNo)
    {
        NO_DestroyAggregate(pstContainer->hPhyTypeNo);
    }

    if (NULL != pstContainer->hIfTypeNo)
    {
        NO_DestroyAggregate(pstContainer->hIfTypeNo);
    }

    if (NULL != pstContainer->hIfNo)
    {
        NO_DestroyAggregate(pstContainer->hIfNo);
    }

    MEM_Free(pstContainer);

    return;
}

/* 申请一个User Data Index */
UINT IF_AllocUserDataIndex(IN IF_CONTAINER hContainer)
{
    IF_CONTAINER_S *pstContainer = hContainer;

    return pstContainer->uiNextUserDataIndex ++;
}

BS_STATUS IF_RegEvent(IN IF_CONTAINER hContainer, IN PF_IF_EVENT_FUNC pfEventFunc, IN USER_HANDLE_S *pstUserHandle)
{
    _IF_EVENT_CALL_NODE_S *pstNode;
    IF_CONTAINER_S *pstContainer = hContainer;

    pstNode = MEM_ZMalloc(sizeof(_IF_EVENT_CALL_NODE_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    pstNode->pfEventFcun = pfEventFunc;
    if (NULL != pstUserHandle)
    {
        pstNode->stUserHandle = *pstUserHandle;
    }

    DLL_ADD(&pstContainer->stListnerList, pstNode);

    return BS_OK;
}

/* 设置网络层参数 */
BS_STATUS IF_SetProtoType(IN IF_CONTAINER hContainer, IN CHAR *pcProtoType, IN IF_PROTO_PARAM_S *pstParam)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_PROTO_PARAM_S *pstNode;

    pstNode = NO_GetObjectByName(pstContainer->hNetTypeNo, pcProtoType);
    if (NULL == pstNode)
    {
        pstNode = NO_NewObject(pstContainer->hNetTypeNo, pcProtoType);
        if (NULL == pstNode)
        {
            return BS_ERR;
        }
    }

    *pstNode = *pstParam;

    return BS_OK;
}

IF_PROTO_PARAM_S * IF_GetProtoType(IN IF_CONTAINER hContainer, IN CHAR *pcProtoType)
{
    IF_CONTAINER_S *pstContainer = hContainer;

    return NO_GetObjectByName(pstContainer->hNetTypeNo, pcProtoType);
}

/* 设置链路层参数 */
BS_STATUS IF_SetLinkType(IN IF_CONTAINER hContainer, IN CHAR *pcLinkType, IN IF_LINK_PARAM_S *pstParam)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_LINK_PARAM_S *pstNode;

    pstNode = NO_GetObjectByName(pstContainer->hLinkTypeNo, pcLinkType);
    if (NULL == pstNode)
    {
        pstNode = NO_NewObject(pstContainer->hLinkTypeNo, pcLinkType);
        if (NULL == pstNode)
        {
            return BS_ERR;
        }
    }

    *pstNode = *pstParam;

    return BS_OK;
}

IF_LINK_PARAM_S * IF_GetLinkType(IN IF_CONTAINER hContainer, IN CHAR *pcLinkType)
{
    IF_CONTAINER_S *pstContainer = hContainer;

    return NO_GetObjectByName(pstContainer->hLinkTypeNo, pcLinkType);
}

/* 设置物理层参数 */
BS_STATUS IF_SetPhyType(IN IF_CONTAINER hContainer, IN CHAR *pcPhyType, IN IF_PHY_PARAM_S *pstParam)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_PHY_PARAM_S *pstNode;

    pstNode = NO_GetObjectByName(pstContainer->hPhyTypeNo, pcPhyType);
    if (NULL == pstNode)
    {
        pstNode = NO_NewObject(pstContainer->hPhyTypeNo, pcPhyType);
        if (NULL == pstNode)
        {
            return BS_ERR;
        }
    }

    *pstNode = *pstParam;

    return BS_OK;
}

IF_PHY_PARAM_S * IF_GetPhyType(IN IF_CONTAINER hContainer, IN CHAR *pcPhyType)
{
    IF_CONTAINER_S *pstContainer = hContainer;

    return NO_GetObjectByName(pstContainer->hPhyTypeNo, pcPhyType);
}

UINT IF_AddIfType
(
    IN IF_CONTAINER hContainer,
    IN CHAR *pcIfType,
    IN IF_TYPE_PARAM_S *pstParam
)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_TYPE_S *pstNode;
    IF_PROTO_PARAM_S *pstProtoType = NULL;
    IF_LINK_PARAM_S *pstLinkType = NULL;
    IF_PHY_PARAM_S *pstPhyType = NULL;

    pstNode = NO_GetObjectByName(pstContainer->hIfTypeNo, pcIfType);
    if (NULL != pstNode)
    {
        return 0;
    }

    if (NULL != pstParam->pcProtoType)
    {
        pstProtoType = IF_GetProtoType(hContainer, pstParam->pcProtoType);
        if (pstProtoType == NULL)
        {
            return 0;
        }
    }

    if (NULL != pstParam->pcLinkType)
    {
        pstLinkType = IF_GetLinkType(hContainer, pstParam->pcLinkType);
        if (NULL == pstLinkType)
        {
            return 0;
        }
    }

    if (NULL != pstParam->pcPhyType)
    {
        pstPhyType = IF_GetPhyType(hContainer, pstParam->pcPhyType);
        if (NULL == pstPhyType)
        {
            return 0;
        }
    }

    pstNode = NO_NewObject(pstContainer->hIfTypeNo, pcIfType);
    if (NULL == pstNode)
    {
        return 0;
    }

    pstNode->hIndexBitmap = LBitMap_Create(NULL);
    if (NULL == pstNode->hIndexBitmap)
    {
        NO_FreeObject(pstContainer->hIfTypeNo, pstNode);
        return 0;
    }

    pstNode->pstProtoType = pstProtoType;
    pstNode->pstLinkType = pstLinkType;
    pstNode->pstPhyType = pstPhyType;
    pstNode->uiFlag = pstParam->uiFlag;
    pstNode->pfTypeCtl = pstParam->pfTypeCtl;

    return (UINT)NO_GetObjectID(pstContainer->hIfTypeNo, pstNode);
}

CHAR * IF_GetTypeNameByType(IN IF_CONTAINER hContainer, IN UINT uiType)
{
    IF_CONTAINER_S *pstContainer = hContainer;

    return NO_GetNameByID(pstContainer->hIfTypeNo, uiType);
}

UINT IF_GetIfTypeFlag(IN IF_CONTAINER hContainer, IN CHAR *pcIfType)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_TYPE_S *pstType;

    pstType = NO_GetObjectByName(pstContainer->hIfTypeNo, pcIfType);
    if (NULL == pstType)
    {
        return 0;
    }

    return pstType->uiFlag;
}

static inline int _if_TypeCtl(IF_TYPE_S *pstType, IF_INDEX ifindex,
        int cmd, void *data)
{
    if (pstType->pfTypeCtl) {
        return pstType->pfTypeCtl(ifindex, cmd, data);
    }

    return 0;
}

static int _if_CreateIf(IF_CONTAINER_S *pstContainer, IF_TYPE_S *pstType,
        IF_NODE_S *pstNode)
{
    IF_INDEX ifIndex;

    if (pstType->pstLinkType != NULL) {
        pstNode->pfLinkInput = pstType->pstLinkType->pfLinkInput;
        pstNode->pfLinkOutput = pstType->pstLinkType->pfLinkOutput;
    }

    if (pstType->pstProtoType != NULL) {    
        pstNode->pfProtoInput = pstType->pstProtoType->pfProtoInput;
    }

    if (pstType->pstPhyType != NULL) {
        pstNode->pfPhyOutput = pstType->pstPhyType->pfPhyOutput;
    }

    ifIndex = (IF_INDEX)NO_GetObjectID(pstContainer->hIfNo, pstNode);

    if (0 != _if_TypeCtl(pstType, ifIndex, IF_TYPE_CTL_CMD_CREATE, NULL)) {
        LBitMap_ClrBit(pstNode->pstType->hIndexBitmap, pstNode->uiIndex);
        NO_FreeObjectByID(pstContainer->hIfNo, ifIndex);
        return 0;
    }

    _if_EventNotify(pstContainer, ifIndex, IF_EVENT_CREATE);

    return ifIndex;
}

IF_INDEX IF_CreateIf(IN IF_CONTAINER hContainer, IN CHAR *pcIfType)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_TYPE_S *pstType;
    IF_NODE_S *pstNode;
    UINT uiIndex;
    CHAR szName[IF_MAX_NAME_LEN + 1];

    pstType = NO_GetObjectByName(pstContainer->hIfTypeNo, pcIfType);
    if (NULL == pstType) {
        return 0;
    }

    if (BS_OK != LBitMap_AllocByRange(pstType->hIndexBitmap,
                0, 0xffffffff, &uiIndex)) {
        return 0;
    }

    snprintf(szName, sizeof(szName), "%s%c%d",
            pcIfType, IF_TYPE_INDEX_LINKER, uiIndex);

    pstNode = NO_NewObject(pstContainer->hIfNo, szName);
    if (NULL == pstNode) {
        LBitMap_ClrBit(pstType->hIndexBitmap, uiIndex);
        return 0;
    }

    pstNode->uiIndex = uiIndex;
    pstNode->pstType = pstType;

    return _if_CreateIf(pstContainer, pstType, pstNode);
}

IF_INDEX IF_CreateIfByName(IN IF_CONTAINER hContainer, IN CHAR *pcIfType, IN CHAR *pcIfName)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_NODE_S *pstNode;
    IF_TYPE_S *pstType;
    UINT uiIndex;

    pstType = NO_GetObjectByName(pstContainer->hIfTypeNo, pcIfType);
    if (NULL == pstType) {
        return 0;
    }

    if (0 != IF_GetIfIndex(hContainer, pcIfName)) {
        return 0;
    }

    if (BS_OK != LBitMap_AllocByRange(pstType->hIndexBitmap,
                0x7fffffff, 0xffffffff, &uiIndex)) {
        return 0;
    }

    pstNode = NO_NewObject(pstContainer->hIfNo, pcIfName);
    if (NULL == pstNode) {
        LBitMap_ClrBit(pstType->hIndexBitmap, uiIndex);
        return 0;
    }

    pstNode->uiIndex = uiIndex;
    pstNode->pstType = pstType;

    return _if_CreateIf(pstContainer, pstType, pstNode);
}

VOID IF_DeleteIf(IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_NODE_S *pstNode;

    pstNode = NO_GetObjectByID(pstContainer->hIfNo, ifIndex);
    if (NULL == pstNode) {
        return;
    }

    _if_EventNotify(pstContainer, ifIndex, IF_EVENT_DELETE);

    _if_TypeCtl(pstNode->pstType, ifIndex, IF_TYPE_CTL_CMD_DELETE, NULL);

    _if_UnRegAllProcesser(&pstNode->stLinkInputPktProcesser);
    _if_UnRegAllProcesser(&pstNode->stLinkOutputPktProcesser);
    _if_UnRegAllProcesser(&pstNode->stProtoInputPktProcesser);
    _if_UnRegAllProcesser(&pstNode->stPhyOutputPktProcesser);

    LBitMap_ClrBit(pstNode->pstType->hIndexBitmap, pstNode->uiIndex);
    NO_FreeObjectByID(pstContainer->hIfNo, ifIndex);
}

BS_STATUS IF_Ioctl(IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex, IN IF_IOCTL_CMD_E enCmd, IN HANDLE hData)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_NODE_S *pstNode;
    BS_STATUS eRet = BS_OK;

    pstNode = NO_GetObjectByID(pstContainer->hIfNo, ifIndex);

    if (NULL == pstNode)
    {
        return BS_NO_SUCH;
    }

    switch (enCmd)
    {
        case IFNET_CMD_PHY_SET_STATUS:
        {
            pstNode->bitPhyStatus = HANDLE_UINT(hData);
            break;
        }
        case IFNET_CMD_LINK_SET_STATUS:
        {
            pstNode->bitLinkStatus = HANDLE_UINT(hData);
            break;
        }
        case IFNET_CMD_PROTO_SET_STATUS:
        {
            pstNode->bitProtoStatus = HANDLE_UINT(hData);
            break;
        }
        case IFNET_CMD_GET_TYPE:
        {
            *(UINT*)hData = (UINT)NO_GetObjectID(pstContainer->hIfTypeNo, pstNode->pstType);
            break;
        }
        case IFNET_CMD_SET_VRF:
        {
            pstNode->uiVrf = HANDLE_UINT(hData);
            break;
        }
        case IFNET_CMD_GET_VRF:
        {
            *(UINT*)hData = pstNode->uiVrf;
            break;
        }
        case IFNET_CMD_SET_MAC:
        {
            MEM_Copy(&pstNode->stMacAddr, hData, 6);
            break;
        }
        case IFNET_CMD_GET_MAC:
        {
            MEM_Copy(hData, &pstNode->stMacAddr, 6);
            break;
        }
        case IFNET_CMD_GET_IFNAME:
        {
            TXT_Strlcpy((CHAR*)hData, NO_GetNameByID(pstContainer->hIfNo, ifIndex), IF_MAX_NAME_LEN + 1);
            break;
        }
        case IFNET_CMD_SET_SHUTDOWN:
        {
            eRet = _if_Shutdown(hContainer, ifIndex, (BOOL_T)HANDLE_UINT(hData));
            break;
        }
        case IFNET_CMD_GET_SHUTDOWN:
        {
            *(BOOL_T*)hData = pstNode->bitShutdown;
            break;
        }
        case IFNET_CMD_IS_L3: 
        {
            *(BOOL_T*)hData = pstNode->bitL3;
            break;
        }

        default:
        {
            BS_DBGASSERT(0);
            eRet = BS_NOT_SUPPORT;
            break;
        }
    }

    return eRet;
}

BS_STATUS IF_PhyIoctl(IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex, IN IF_PHY_IOCTL_CMD_E enCmd, IN HANDLE hData)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_NODE_S *pstNode;

    pstNode = NO_GetObjectByID(pstContainer->hIfNo, ifIndex);

    if (NULL == pstNode)
    {
        return BS_NO_SUCH;
    }

    if ((pstNode->pstType->pstPhyType != NULL) && (pstNode->pstType->pstPhyType->pfPhyIoctl != NULL))
    {
        pstNode->pstType->pstPhyType->pfPhyIoctl(ifIndex, enCmd, hData);
    }
    else
    {
        return BS_NOT_SUPPORT;
    }

    return BS_OK;
}

IF_INDEX IF_GetIfIndex(IN IF_CONTAINER hContainer, IN CHAR *pcIfName)
{
    IF_CONTAINER_S *pstContainer = hContainer;

    return (UINT)NO_GetIDByName(pstContainer->hIfNo, pcIfName);
}

CHAR * IF_GetPhyIndexStringByIfName(IN CHAR *pcIfName)
{
    CHAR *pcTmp;
    
    pcTmp = TXT_ReverseStrchr(pcIfName, IF_TYPE_INDEX_LINKER);
    if (NULL == pcTmp)
    {
        return NULL;
    }

    return (pcTmp + 1);
}

UINT IF_GetPhyIndexByIfName(char *ifname)
{
    char * phy_index_string = IF_GetPhyIndexStringByIfName(ifname);
    if (phy_index_string == NULL) {
        return IF_INVALID_PHY_INDEX;
    }

    return TXT_Str2Ui(phy_index_string);
}

BS_STATUS IF_GetTypeNameByIfName(IN CHAR *pcIfName, OUT CHAR *pcTypeName)
{
    CHAR *pcIndex;
    UINT uiTypeNameLen;

    pcIndex = IF_GetPhyIndexStringByIfName(pcIfName);
    if (NULL == pcIndex)
    {
        return BS_ERR;
    }

    uiTypeNameLen = (pcIndex - pcIfName) - 1;

    memcpy(pcTypeName, pcIfName, uiTypeNameLen);
    pcTypeName[uiTypeNameLen] = '\0';

    return BS_OK;
}

IF_PHY_OUTPUT_FUNC IF_GetPhyOutPut (IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_NODE_S *pstNode;

    pstNode = NO_GetObjectByID(pstContainer->hIfNo, ifIndex);

    if (NULL == pstNode)
    {
        return NULL;
    }

    return pstNode->pfPhyOutput;
}

IF_LINK_INPUT_FUNC IF_GetLinkInput (IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_NODE_S *pstNode;

    pstNode = NO_GetObjectByID(pstContainer->hIfNo, ifIndex);

    if (NULL == pstNode)
    {
        return NULL;
    }

    return pstNode->pfLinkInput;
}

IF_LINK_OUTPUT_FUNC IF_GetLinkOutput (IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_NODE_S *pstNode;

    pstNode = NO_GetObjectByID(pstContainer->hIfNo, ifIndex);

    if (NULL == pstNode)
    {
        return NULL;
    }

    return pstNode->pfLinkOutput;
}

IF_PROTO_INPUT_FUNC IF_GetProtoInput(IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_NODE_S *pstNode;

    pstNode = NO_GetObjectByID(pstContainer->hIfNo, ifIndex);

    if (NULL == pstNode)
    {
        return NULL;
    }

    return pstNode->pfProtoInput;
}

BS_STATUS IF_SetUserData(IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex, IN UINT uiUserIndex, IN HANDLE hData)
{
    IF_CONTAINER_S *pstContainer = hContainer;

    return NO_SetPropertyByID(pstContainer->hIfNo, ifIndex,  uiUserIndex, hData);
}

BS_STATUS IF_GetUserData(IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex, IN UINT uiUserIndex, OUT HANDLE *phData)
{
    IF_CONTAINER_S *pstContainer = hContainer;

    return NO_GetPropertyByID(pstContainer->hIfNo, ifIndex,  uiUserIndex, phData);
}

IF_INDEX IF_GetNext(IN IF_CONTAINER hContainer, IN IF_INDEX ifIndexCurrent/* 0表示从头开始 */)
{
    IF_CONTAINER_S *pstContainer = hContainer;

    return (UINT)NO_GetNextID(pstContainer->hIfNo, ifIndexCurrent);
}

BS_STATUS IF_RegPktProcesser
(
    IN IF_CONTAINER hContainer,
    IN IF_INDEX ifIndex,
    IN IF_PKT_PROCESSER_TYPE_E enType,
    IN UINT uiPri,  /* 优先级,数字越小优先级越高 */
    IN PF_IF_PKT_PORCESSER_FUNC pfFunc
)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_NODE_S *pstNode;
    DLL_HEAD_S *pstListHead;

    pstNode = NO_GetObjectByID(pstContainer->hIfNo, ifIndex);
    if (NULL == pstNode)
    {
        return BS_NOT_FOUND;
    }

    switch (enType)
    {
        case IF_PKT_PROCESSER_TYPE_LINKINPUT:
        {
            pstListHead = &pstNode->stLinkInputPktProcesser;
            break;
        }
        case IF_PKT_PROCESSER_TYPE_LINKOUTPUT:
        {
            pstListHead = &pstNode->stLinkOutputPktProcesser;
            break;
        }
        case IF_PKT_PROCESSER_TYPE_PROTOINPUT:
        {
            pstListHead = &pstNode->stProtoInputPktProcesser;
            break;
        }
        case IF_PKT_PROCESSER_TYPE_PHYOUTPUT:
        {
            pstListHead = &pstNode->stPhyOutputPktProcesser;
            break;
        }
        default:
        {
            BS_DBGASSERT(0);
            return BS_ERR;
        }
    }

    return _if_RegProcesser(pstListHead, uiPri, pfFunc);
}

VOID * IF_GetPktProcesserList(IN IF_CONTAINER hContainer, IN IF_PKT_PROCESSER_TYPE_E enType, IN IF_INDEX ifIndex)
{
    IF_CONTAINER_S *pstContainer = hContainer;
    IF_NODE_S *pstNode;

    pstNode = NO_GetObjectByID(pstContainer->hIfNo, ifIndex);
    if (NULL == pstNode)
    {
        return NULL;
    }

    switch (enType)
    {
        case IF_PKT_PROCESSER_TYPE_LINKINPUT:
        {
            return &pstNode->stLinkInputPktProcesser;
        }
        case IF_PKT_PROCESSER_TYPE_LINKOUTPUT:
        {
            return &pstNode->stLinkOutputPktProcesser;
        }
        case IF_PKT_PROCESSER_TYPE_PROTOINPUT:
        {
            return &pstNode->stProtoInputPktProcesser;
        }
        case IF_PKT_PROCESSER_TYPE_PHYOUTPUT:
        {
            return &pstNode->stPhyOutputPktProcesser;
        }
        default:
        {
            BS_DBGASSERT(0);
            return NULL;
        }
    }

    return NULL;
}

BS_STATUS IF_NotifyPktProcesser(IN VOID *pProcesserList, IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf)
{
    _IF_PKT_PROCESSER_NODE_S *pstNode;
    BS_STATUS eRet = BS_OK;

    if (NULL == pProcesserList)
    {
        return BS_OK;
    }

    DLL_SCAN(pProcesserList, pstNode)
    {
        eRet = pstNode->pfPktFunc(ifIndex, pstMbuf);
        if (eRet == BS_PROCESSED)
        {
            break;
        }
    }

    return eRet;
}


