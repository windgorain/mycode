/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-6-15
* Description:  使用select的异步TCP
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_PLUG_BASE
    
#include "bs.h"

#include "utl/sem_utl.h"
#include "utl/txt_utl.h"
#include "utl/local_info.h"
#include "utl/msocket.h"

#define _SSLTCP_TCP_ASYN_HASH_SIZE 3072

typedef struct
{
    DLL_NODE_S stDllNode;
    DLL_NODE_S stHashDllNode;
    MSOCKET_ID uiSocketID;
    PF_SSLTCP_INNER_CB_FUNC pfFunc;
    USER_HANDLE_S stUserHandle;
}_SSLTCP_SELECT_ASYN_NODE_S;

typedef struct
{
    BOOL_T bIsSelectting;
    MSOCKET_ID uiTriggerSrcSocket;
    MSOCKET_ID uiTriggerDstSocket;
    ULONG ulCareEvent;   /* 关心的事件，可以设置为SSLTCP_EVENT_READ/SSLTCP_EVENT_WRITE */
    fd_set socketReadFds;
    fd_set socketWriteFds;
    fd_set socketExpFds;
    fd_set socketAsynNotSelectted;   /* 是异步Socket, 但是并不在本次select内. 每次select之前清空 */
    UINT ulMaxSocketFd;            /*  最大的异步socket 的ID */
    DLL_HEAD_S stSocketList;         /* _SSLTCP_SELECT_ASYN_NODE_S , 本实例内所有的异步Socket串在一起*/
}_SSLTCP_SELECT_ASYN_INFO_S;

typedef struct
{
    _SSLTCP_SELECT_ASYN_INFO_S stSslTcpTcpAsynInfo;
    BOOL_T bTrigger;
    PF_SSLTCP_TRIGGER_FUNC pfTriggerFunc;
    USER_HANDLE_S stTriggerUserHandle;
    SSLTCP_ASYN_MODE_E eAsynMode;
    SEM_HANDLE hSsltcpTcpAsynSem;
    DLL_HEAD_S g_pstSslTcpTcpHashTable[_SSLTCP_TCP_ASYN_HASH_SIZE];
}_SSLTCP_SELECT_ASYN_INSTANCE_S;

static BS_STATUS __SSLTCP_SELECT_Dispatch(IN _SSLTCP_SELECT_ASYN_INSTANCE_S *pstInstance)
{
    fd_set stReadFds, stWriteFds, stExecptFds;
    UCHAR aucData[100];
    DLL_NODE_S *pstNode, *pstNodeNext;
    _SSLTCP_SELECT_ASYN_NODE_S *pstASocketNode;
    ULONG ulEvent;
    _SSLTCP_SELECT_ASYN_INFO_S *pstSslTcpTcpAsynInfo = &pstInstance->stSslTcpTcpAsynInfo;
    SEM_HANDLE hSsltcpTcpAsynSem = pstInstance->hSsltcpTcpAsynSem;
    SSLTCP_ASYN_MODE_E eAsynMode = pstInstance->eAsynMode;
    BS_STATUS eRet;
    UINT uiReadLen;

    SEM_P (pstInstance->hSsltcpTcpAsynSem, BS_WAIT, BS_WAIT_FOREVER);
    for (;;)
    {
        pstSslTcpTcpAsynInfo->bIsSelectting = TRUE;
        stReadFds = pstSslTcpTcpAsynInfo->socketReadFds;
        stWriteFds = pstSslTcpTcpAsynInfo->socketWriteFds;
        stExecptFds = pstSslTcpTcpAsynInfo->socketExpFds;

        FD_ZERO(&pstSslTcpTcpAsynInfo->socketAsynNotSelectted);
        
        SEM_V (hSsltcpTcpAsynSem);
        
        select ((INT) pstSslTcpTcpAsynInfo->ulMaxSocketFd+1, &stReadFds, &stWriteFds, &stExecptFds, NULL);

        SEM_P (hSsltcpTcpAsynSem, BS_WAIT, BS_WAIT_FOREVER);
        pstSslTcpTcpAsynInfo->bIsSelectting = FALSE;

        do{
            eRet = MSocket_Read(pstSslTcpTcpAsynInfo->uiTriggerDstSocket, aucData, 100, &uiReadLen, 0);
        }while ((eRet == BS_OK) && (uiReadLen != 0));

        if (pstInstance->bTrigger)
        {
            pstInstance->bTrigger = FALSE;
            pstInstance->pfTriggerFunc(&pstInstance->stTriggerUserHandle);
        }

        /*  遍历异步Socket 链表  */
        DLL_SAFE_SCAN (&pstSslTcpTcpAsynInfo->stSocketList, pstNode, pstNodeNext)
        {
            pstASocketNode = container_of(pstNode, _SSLTCP_SELECT_ASYN_NODE_S, stDllNode);

            if (MSocket_FdIsSet(pstASocketNode->uiSocketID, &pstSslTcpTcpAsynInfo->socketAsynNotSelectted))
            {
                /* 表示不是这个Socket的事件, 而是以前的相同ID 的socket 的事件, 对其事件不处理 */
                continue;
            }

            ulEvent = 0;
            
            if (MSocket_FdIsSet (pstASocketNode->uiSocketID, &stReadFds))
            {
                if (eAsynMode == SSLTCP_ASYN_MODE_EDGE)
                {
                    MSocket_FdClr (pstASocketNode->uiSocketID, &pstSslTcpTcpAsynInfo->socketReadFds);
                }
                ulEvent |= SSLTCP_EVENT_READ;
            }

            if (MSocket_FdIsSet (pstASocketNode->uiSocketID, &stWriteFds))
            {
                if (eAsynMode == SSLTCP_ASYN_MODE_EDGE)
                {
                    MSocket_FdClr (pstASocketNode->uiSocketID, &pstSslTcpTcpAsynInfo->socketWriteFds);
                }
                ulEvent |= SSLTCP_EVENT_WRITE;
            }

            if (MSocket_FdIsSet (pstASocketNode->uiSocketID, &stExecptFds))
            {
                if (eAsynMode == SSLTCP_ASYN_MODE_EDGE)
                {
                    MSocket_FdClr (pstASocketNode->uiSocketID, &pstSslTcpTcpAsynInfo->socketExpFds);
                }
                ulEvent |= SSLTCP_EVENT_EXECPT;
            }
            if (ulEvent != 0)
            {
                pstASocketNode->pfFunc (UINT_HANDLE(pstASocketNode->uiSocketID), ulEvent, &pstASocketNode->stUserHandle);
            }
        }
    }

    SEM_V (hSsltcpTcpAsynSem);
}

static inline UINT _SSLTCP_SELECT_AsynGetHashIndex(IN UINT ulSocketID)
{
    return (ulSocketID % _SSLTCP_TCP_ASYN_HASH_SIZE);
}

static BS_STATUS __SSLTCP_SELECT_UnSetAsyn(IN MSOCKET_ID uiSocketID)
{
    UINT ulHashIndex;
    _SSLTCP_SELECT_ASYN_NODE_S *pstNode;
    DLL_NODE_S *pstDllNode;
    UINT ulIoMode = 0;
    _SSLTCP_SELECT_ASYN_INSTANCE_S *pstAsynInstance;
    
    if (BS_OK != MSocket_GetUserHandle(uiSocketID, &pstAsynInstance))
    {
        RETURN(BS_ERR);
    }

    if (NULL == pstAsynInstance)
    {
        return BS_OK;
    }

    ulHashIndex = _SSLTCP_SELECT_AsynGetHashIndex (uiSocketID);

    SEM_P (pstAsynInstance->hSsltcpTcpAsynSem, BS_WAIT, BS_WAIT_FOREVER);
    DLL_SCAN (&pstAsynInstance->g_pstSslTcpTcpHashTable[ulHashIndex],pstDllNode)
    {
        pstNode = container_of(pstNode, _SSLTCP_SELECT_ASYN_NODE_S, stHashDllNode);
        if (pstNode->uiSocketID == uiSocketID)
        {
            DLL_DEL (&pstAsynInstance->g_pstSslTcpTcpHashTable[ulHashIndex], &pstNode->stHashDllNode);
            DLL_DEL (&pstAsynInstance->stSslTcpTcpAsynInfo.stSocketList, &pstNode->stDllNode);
            MEM_Free (pstNode);
            break;
        }
    }
    MSocket_FdClr (uiSocketID, &pstAsynInstance->stSslTcpTcpAsynInfo.socketReadFds);
    MSocket_FdClr (uiSocketID, &pstAsynInstance->stSslTcpTcpAsynInfo.socketWriteFds);
    MSocket_FdClr (uiSocketID, &pstAsynInstance->stSslTcpTcpAsynInfo.socketExpFds);
    pstAsynInstance->stSslTcpTcpAsynInfo.ulCareEvent = 0;
    MSocket_SetUserHandle(uiSocketID, NULL);
    SEM_V (pstAsynInstance->hSsltcpTcpAsynSem);

    MSocket_Ioctl (uiSocketID, (INT) FIONBIO, &ulIoMode);

    return BS_OK;
}

static BS_STATUS __SSLTCP_SELECT_SetAsyn
(
    IN _SSLTCP_SELECT_ASYN_INSTANCE_S *pstAsynInstance,
    IN MSOCKET_ID uiSocketID,
    IN ULONG ulEvent,
    IN PF_SSLTCP_INNER_CB_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _SSLTCP_SELECT_ASYN_NODE_S *pstNode;
    UINT ulIoMode = 1;
    UINT ulHashIndex;
    UINT uiInnerSocketId;
    
    __SSLTCP_SELECT_UnSetAsyn(uiSocketID);
    
    /* 更改Socket 为非阻塞 */
    if (BS_OK != MSocket_Ioctl (uiSocketID, (INT) FIONBIO, &ulIoMode))
    {
        RETURN(BS_ERR);
    }
    
    /* 将ASocket 节点加入异步链表 */
    pstNode = MEM_Malloc (sizeof(_SSLTCP_SELECT_ASYN_NODE_S));
    if (NULL == pstNode)
    {
        ulIoMode = 0;
        MSocket_Ioctl (uiSocketID, (INT) FIONBIO, &ulIoMode);
        RETURN(BS_NO_MEMORY);
    }
    Mem_Zero (pstNode, sizeof(_SSLTCP_SELECT_ASYN_NODE_S));

    pstNode->pfFunc = pfFunc;
    pstNode->uiSocketID = uiSocketID;
    pstNode->stUserHandle = *pstUserHandle;

    ulHashIndex = _SSLTCP_SELECT_AsynGetHashIndex(uiSocketID);

    SEM_P (pstAsynInstance->hSsltcpTcpAsynSem, BS_WAIT, BS_WAIT_FOREVER);
    DLL_ADD (&pstAsynInstance->stSslTcpTcpAsynInfo.stSocketList, &pstNode->stDllNode);
    DLL_ADD_TO_HEAD (&pstAsynInstance->g_pstSslTcpTcpHashTable[ulHashIndex], &pstNode->stHashDllNode);
    pstAsynInstance->stSslTcpTcpAsynInfo.ulCareEvent = ulEvent;
    if (ulEvent & SSLTCP_EVENT_READ)
    {
        MSocket_FdSet (uiSocketID, &pstAsynInstance->stSslTcpTcpAsynInfo.socketReadFds);
    }
    if (ulEvent & SSLTCP_EVENT_WRITE)
    {
        MSocket_FdSet (uiSocketID, &pstAsynInstance->stSslTcpTcpAsynInfo.socketWriteFds);
    }
    MSocket_FdSet (uiSocketID, &pstAsynInstance->stSslTcpTcpAsynInfo.socketExpFds);
    MSocket_FdSet (uiSocketID, &pstAsynInstance->stSslTcpTcpAsynInfo.socketAsynNotSelectted);
    MSocket_SetUserHandle(uiSocketID, pstAsynInstance);
    SEM_V (pstAsynInstance->hSsltcpTcpAsynSem);

    uiInnerSocketId = MSocket_GetSocketId(uiSocketID);
    if (pstAsynInstance->stSslTcpTcpAsynInfo.ulMaxSocketFd < uiInnerSocketId)
    {
        pstAsynInstance->stSslTcpTcpAsynInfo.ulMaxSocketFd = uiInnerSocketId;
    }

    if (pstAsynInstance->stSslTcpTcpAsynInfo.bIsSelectting == TRUE)
    {
        MSocket_Write (pstAsynInstance->stSslTcpTcpAsynInfo.uiTriggerSrcSocket, (UCHAR*)"1", 1, 0);
    }
    
    return BS_OK;
}

BS_STATUS _SSLTCP_SELECT_AsynSocketInit(IN _SSLTCP_SELECT_ASYN_INSTANCE_S *pstInstance, IN SSLTCP_ASYN_MODE_E eMode)
{
    UINT i;
    UINT ulIoMode = 1;
    MSOCKET_ID auiFd[2];

    pstInstance->eAsynMode = eMode;

    /* 初始化异步Socket控制结构 */
    DLL_INIT(&pstInstance->stSslTcpTcpAsynInfo.stSocketList);

    /* 初始化Hash表 */
    for (i=0; i<_SSLTCP_TCP_ASYN_HASH_SIZE; i++)
    {
        DLL_INIT(&pstInstance->g_pstSslTcpTcpHashTable[i]);
    }

    /* 申请信号量 */
    if (0 == (pstInstance->hSsltcpTcpAsynSem = SEM_MCreate("AsynSocket")))
    {
        BS_WARNNING(("Can't create asyn socket sem!"));
        RETURN(BS_NO_RESOURCE);
    }

    /* 创建唤醒Socket */
    if (BS_OK != MSocket_Pair(SOCK_STREAM, auiFd))
    {
        BS_WARNNING(("Can't create socket pair"));
        RETURN(BS_NO_RESOURCE);
    }
    pstInstance->stSslTcpTcpAsynInfo.uiTriggerSrcSocket = auiFd[0];
    pstInstance->stSslTcpTcpAsynInfo.uiTriggerDstSocket = auiFd[1];

    MSocket_Ioctl(pstInstance->stSslTcpTcpAsynInfo.uiTriggerSrcSocket, (INT)FIONBIO, &ulIoMode);
    MSocket_Ioctl(pstInstance->stSslTcpTcpAsynInfo.uiTriggerDstSocket, (INT)FIONBIO, &ulIoMode);
    
    MSocket_FdSet(pstInstance->stSslTcpTcpAsynInfo.uiTriggerDstSocket, &pstInstance->stSslTcpTcpAsynInfo.socketReadFds);
    pstInstance->stSslTcpTcpAsynInfo.ulMaxSocketFd = MSocket_GetSocketId(pstInstance->stSslTcpTcpAsynInfo.uiTriggerDstSocket);
    
    return BS_OK;
}

static BS_STATUS _SSLTCP_SELECT_AsynAccept
(
    IN MSOCKET_ID listenSocketId,
    OUT MSOCKET_ID *pAcceptSocketId
)
{
    UINT ulIoMode = 0;
    UINT ulAcceptSocketId;
    BS_STATUS eRet;
    _SSLTCP_SELECT_ASYN_INSTANCE_S *pstAsynInstance;

    eRet = MSocket_GetUserHandle(listenSocketId, &pstAsynInstance);
    if (eRet != BS_OK)
    {
        return eRet;
    }

    eRet = MSocket_Accept(listenSocketId, &ulAcceptSocketId);

    /* 数据读完了, 如果是异步Socket, 则将其加入Select Read */
    if (pstAsynInstance != NULL)
    {
        SEM_P (pstAsynInstance->hSsltcpTcpAsynSem, BS_WAIT, BS_WAIT_FOREVER);
        MSocket_FdSet (listenSocketId, &pstAsynInstance->stSslTcpTcpAsynInfo.socketReadFds);
        SEM_V (pstAsynInstance->hSsltcpTcpAsynSem);

        if (pstAsynInstance->stSslTcpTcpAsynInfo.bIsSelectting == TRUE)
        {
            MSocket_Write (pstAsynInstance->stSslTcpTcpAsynInfo.uiTriggerSrcSocket, (UCHAR*)"2", 1, 0);
        }
        if (BS_OK == eRet)
        {
            MSocket_Ioctl (ulAcceptSocketId, (INT)FIONBIO, &ulIoMode);
        }
    }

    *pAcceptSocketId = ulAcceptSocketId;

    return eRet;
}

static BS_STATUS _SSLTCP_SELECT_AsynRead
(
    IN MSOCKET_ID uiSocketId,
    OUT UCHAR *pucBuf,
    IN UINT ulLen,
    OUT UINT *puiReadLen,
    IN UINT ulFlag
)
{
    _SSLTCP_SELECT_ASYN_INSTANCE_S *pstAsynInstance;
    BS_STATUS eRet;
    UINT uiReadLen;

    *puiReadLen = 0;
    
    if (BS_OK != MSocket_GetUserHandle(uiSocketId, &pstAsynInstance))
    {
        return BS_ERR;
    }

    eRet = MSocket_Read(uiSocketId, pucBuf, ulLen, &uiReadLen, ulFlag);

    if (eRet != BS_OK)
    {
        return eRet;
    }

    if (uiReadLen < ulLen)
    {
        /* 数据读完了, 如果是异步Socket, 则将其加入Select Read */
        if ((pstAsynInstance != NULL) && 
            (pstAsynInstance->eAsynMode == SSLTCP_ASYN_MODE_EDGE) &&
            (pstAsynInstance->stSslTcpTcpAsynInfo.ulCareEvent & SSLTCP_EVENT_READ))
        {
            SEM_P (pstAsynInstance->hSsltcpTcpAsynSem, BS_WAIT, BS_WAIT_FOREVER);
            MSocket_FdSet (uiSocketId, &pstAsynInstance->stSslTcpTcpAsynInfo.socketReadFds);
            SEM_V (pstAsynInstance->hSsltcpTcpAsynSem);

            if (pstAsynInstance->stSslTcpTcpAsynInfo.bIsSelectting == TRUE)
            {
                MSocket_Write (pstAsynInstance->stSslTcpTcpAsynInfo.uiTriggerSrcSocket, (UCHAR*)"3", 1, 0);
            }
        }
    }

    *puiReadLen = uiReadLen;

    return BS_OK;
}

static INT _SSLTCP_SELECT_AsynWrite(IN MSOCKET_ID uiSocketId, IN UCHAR *pucBuf, IN UINT ulLen, IN UINT ulFlag)
{
    INT lLen;
    _SSLTCP_SELECT_ASYN_INSTANCE_S *pstAsynInstance;
    
    if (BS_OK != MSocket_GetUserHandle(uiSocketId, &pstAsynInstance))
    {
        return -1;
    }

    lLen = MSocket_Write(uiSocketId, pucBuf, ulLen, ulFlag);

    if (lLen < 0)
    {
        return lLen;
    }

    if (lLen < (INT)ulLen)
    {
        /* 数据发送不完, 如果是异步Socket, 则将其加入Select Write */
        if ((pstAsynInstance != NULL) && 
            (pstAsynInstance->eAsynMode == SSLTCP_ASYN_MODE_EDGE) &&
            (pstAsynInstance->stSslTcpTcpAsynInfo.ulCareEvent & SSLTCP_EVENT_WRITE))
        {
            SEM_P (pstAsynInstance->hSsltcpTcpAsynSem, BS_WAIT, BS_WAIT_FOREVER);
            MSocket_FdSet (uiSocketId, &pstAsynInstance->stSslTcpTcpAsynInfo.socketWriteFds);
            SEM_V (pstAsynInstance->hSsltcpTcpAsynSem);

            if (pstAsynInstance->stSslTcpTcpAsynInfo.bIsSelectting == TRUE)
            {
                MSocket_Write (pstAsynInstance->stSslTcpTcpAsynInfo.uiTriggerSrcSocket, (UCHAR*)"4", 1, 0);
            }
        }
    }

    return lLen;
}

static INT _SSLTCP_SELECT_Write(IN HANDLE hFileHandle, IN UCHAR *pucBuf, IN UINT ulLen, IN UINT ulFlag)
{
    return _SSLTCP_SELECT_AsynWrite((MSOCKET_ID)HANDLE_UINT(hFileHandle), pucBuf, ulLen, ulFlag);
}

static BS_STATUS _SSLTCP_SELECT_Read
(
    IN HANDLE hFileHandle,
    OUT UCHAR *pucBuf,
    IN UINT ulLen,
    OUT UINT *puiReadLen,
    IN UINT ulFlag
)
{
    return _SSLTCP_SELECT_AsynRead((MSOCKET_ID)HANDLE_UINT(hFileHandle), pucBuf, ulLen, puiReadLen, ulFlag);
}

static BS_STATUS _SSLTCP_SELECT_Accept(IN HANDLE hListenSocket, OUT HANDLE *phAcceptSocket)
{
    MSOCKET_ID uiAcceptId;
    BS_STATUS eRet;
    
    eRet = _SSLTCP_SELECT_AsynAccept((MSOCKET_ID)HANDLE_UINT(hListenSocket), &uiAcceptId);

    *phAcceptSocket = UINT_HANDLE(uiAcceptId);

    return eRet;    
}

static HANDLE _SSLTCP_SELECT_CreateAsynInstance(IN SSLTCP_ASYN_MODE_E eMode)
{
    _SSLTCP_SELECT_ASYN_INSTANCE_S *pstInstance;

    pstInstance = MEM_ZMalloc(sizeof(_SSLTCP_SELECT_ASYN_INSTANCE_S));
    if (NULL == pstInstance)
    {
        return NULL;
    }

    _SSLTCP_SELECT_AsynSocketInit(pstInstance, eMode);

    return pstInstance;
}

static VOID _SSLTCP_SELECT_DeleteAsynInstance(IN HANDLE hAsynHandle)
{
    _SSLTCP_SELECT_ASYN_INSTANCE_S *pstInstance = (_SSLTCP_SELECT_ASYN_INSTANCE_S *)hAsynHandle;

    if (pstInstance->hSsltcpTcpAsynSem != 0)
    {
        SEM_Destory(pstInstance->hSsltcpTcpAsynSem);
    }

    if (pstInstance->stSslTcpTcpAsynInfo.uiTriggerSrcSocket != 0)
    {
        MSocket_Close(pstInstance->stSslTcpTcpAsynInfo.uiTriggerSrcSocket);
    }

    if (pstInstance->stSslTcpTcpAsynInfo.uiTriggerDstSocket != 0)
    {
        MSocket_Close(pstInstance->stSslTcpTcpAsynInfo.uiTriggerDstSocket);
    }

    MEM_Free(pstInstance);
}


static BS_STATUS _SSLTCP_SELECT_SetAsyn
(
    IN HANDLE hAsynInstance,
    IN HANDLE hFileHandle,
    IN ULONG ulEvent, /* 关心的事件 */
    IN PF_SSLTCP_INNER_CB_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    return __SSLTCP_SELECT_SetAsyn(hAsynInstance,
        (MSOCKET_ID)HANDLE_UINT(hFileHandle), ulEvent, pfFunc, pstUserHandle);
}

static BS_STATUS _SSLTCP_SELECT_UnSetAsyn(IN HANDLE hAsynInstance, IN HANDLE hFileHandle)
{
    return __SSLTCP_SELECT_UnSetAsyn((MSOCKET_ID)HANDLE_UINT(hFileHandle));
}

static BS_STATUS _SSLTCP_SELECT_SetAsynTrigger
(
    IN HANDLE hAsynInstance,
    PF_SSLTCP_TRIGGER_FUNC pfTriggerFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _SSLTCP_SELECT_ASYN_INSTANCE_S *pstAsynInstance = hAsynInstance;

    SEM_P (pstAsynInstance->hSsltcpTcpAsynSem, BS_WAIT, BS_WAIT_FOREVER);
    pstAsynInstance->pfTriggerFunc = pfTriggerFunc;
    pstAsynInstance->stTriggerUserHandle = *pstUserHandle;
    SEM_V (pstAsynInstance->hSsltcpTcpAsynSem);

    return BS_OK;
}

static BS_STATUS _SSLTCP_SELECT_AsynTrigger
(
    IN HANDLE hAsynInstance
)
{
    _SSLTCP_SELECT_ASYN_INSTANCE_S *pstAsynInstance = hAsynInstance;

    pstAsynInstance->bTrigger = TRUE;
    MSocket_Write (pstAsynInstance->stSslTcpTcpAsynInfo.uiTriggerSrcSocket, (UCHAR*)"1", 1, 0);

    return BS_OK;
}

static BS_STATUS _SSLTCP_SELECT_Ioctl
(
    IN HANDLE hFileHandle,
    IN INT lCmd,
    IN VOID *pParam
)
{
    return MSocket_Ioctl ((MSOCKET_ID)HANDLE_UINT(hFileHandle), lCmd, pParam);
}

static UINT _SSLTCP_SELECT_GetConnectionNum(IN HANDLE hAsynInstance)
{
    _SSLTCP_SELECT_ASYN_INSTANCE_S *pstAsynInstance = hAsynInstance;

    return DLL_COUNT(&pstAsynInstance->stSslTcpTcpAsynInfo.stSocketList);
}

static BS_STATUS _SSLTCP_SELECT_Dispatch(IN HANDLE hAsynInstance)
{
    return __SSLTCP_SELECT_Dispatch(hAsynInstance);
}

static BS_STATUS _SSLTCP_SELECT_Close(IN HANDLE hFileHandle)
{
    __SSLTCP_SELECT_UnSetAsyn((MSOCKET_ID)HANDLE_UINT(hFileHandle));
    return MSocket_Close((MSOCKET_ID)HANDLE_UINT(hFileHandle));
}

static BS_STATUS _SSLTCP_SELECT_Listen(IN HANDLE hFileHandle, UINT ulLocalIp, IN USHORT usPort, IN UINT uiBacklog)
{
    return MSocket_Listen((MSOCKET_ID)HANDLE_UINT(hFileHandle), htonl(ulLocalIp), htons(usPort), uiBacklog);
}

static BS_STATUS _SSLTCP_SELECT_Connect(IN HANDLE hFileHandle, IN UINT ulIp, IN USHORT usPort)
{
    int ret;
    ret = MSocket_Connect((MSOCKET_ID)HANDLE_UINT(hFileHandle), ulIp, usPort);
    if (ret < 0) {
        if (ret == SOCKET_E_AGAIN) {
            return BS_AGAIN;
        }
        return BS_ERR;
    }

    return ret;
}

static BS_STATUS _SSLTCP_SELECT_GetHostIpPort(IN HANDLE hFileHandle, OUT UINT *pulIp, OUT USHORT *pusPort)
{
    return MSocket_GetHostIpPort((MSOCKET_ID)HANDLE_UINT(hFileHandle), pulIp, pusPort);
}

static BS_STATUS _SSLTCP_SELECT_GetPeerIpPort(IN HANDLE hFileHandle, OUT UINT *pulIp, OUT USHORT *pusPort)
{
    return MSocket_GetPeerIpPort((MSOCKET_ID)HANDLE_UINT(hFileHandle), pulIp, pusPort);
}

static BS_STATUS _SSLTCP_SELECT_CreateTcp(IN UINT ulFamily, IN VOID *pParam, OUT HANDLE *hSslTcpId)
{
    UINT  ulIndexFrom1 = 0;
    MSOCKET_ID  uiSocketID;

    if (BS_OK != MSocket_Create (AF_INET, SOCK_STREAM, &uiSocketID))
    {
        RETURN(BS_ERR);
    }

    *hSslTcpId = UINT_HANDLE(uiSocketID);

    return BS_OK;
}

static BS_STATUS _SSLTCP_SELECT_Init()
{
    SSLTCP_PROTO_S stProto;

    MSocket_Init();

    Mem_Zero(&stProto, sizeof(SSLTCP_PROTO_S));

    TXT_Strlcpy(stProto.szProtoName, "tcp", sizeof(stProto.szProtoName));
    stProto.pfCreate =  _SSLTCP_SELECT_CreateTcp;
    stProto.pfListen =  _SSLTCP_SELECT_Listen;
    stProto.pfConnect =  _SSLTCP_SELECT_Connect;
    stProto.pfAccept =  _SSLTCP_SELECT_Accept;
    stProto.pfWrite =  _SSLTCP_SELECT_Write;
    stProto.pfRead =  _SSLTCP_SELECT_Read;
    stProto.pfClose =  _SSLTCP_SELECT_Close;
    stProto.pfCreateAsynInstance =  _SSLTCP_SELECT_CreateAsynInstance;
    stProto.pfDeleteAsynInstance = _SSLTCP_SELECT_DeleteAsynInstance;
    stProto.pfSetAsyn =  _SSLTCP_SELECT_SetAsyn;
    stProto.pfUnSetAsyn =  _SSLTCP_SELECT_UnSetAsyn;
    stProto.pfSetAsynTrigger = _SSLTCP_SELECT_SetAsynTrigger;
    stProto.pfAsynTrigger = _SSLTCP_SELECT_AsynTrigger;
    stProto.pfIoctl = _SSLTCP_SELECT_Ioctl;
    stProto.pfGetConnectionNum = _SSLTCP_SELECT_GetConnectionNum;
    stProto.pfDispatch = _SSLTCP_SELECT_Dispatch;
    stProto.pfGetHostIpPort =  _SSLTCP_SELECT_GetHostIpPort;
    stProto.pfGetPeerIpPort =  _SSLTCP_SELECT_GetPeerIpPort;

    return SSLTCP_RegProto(&stProto);
}

static int _plug_init()
{
    BS_STATUS eRet;
    
    if (BS_OK != (eRet = _SSLTCP_SELECT_Init())) {
        EXEC_OutString(" Can't init ssltcp tcp plug.\r\n");
    }

    return eRet;
}

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            return _plug_init();
        default:
            break;
    }

    return 0;
}

PLUG_MAIN

