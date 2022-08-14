/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-5-13
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sem_utl.h"
#include "utl/exec_utl.h"
#include "utl/socket_utl.h"
#include "utl/ssl_net.h"
#include "utl/bitmap1_utl.h"
#include "utl/mbuf_utl.h"

/* ---define--- */
#define _DEF_SSLTCP_MAX_NAME_LEN_2_RECORD 64
#define _DEF_SSLTCP_MAX_PROTO_NUM         64
#define _DEF_SSLTCP_INC_NUM_MASK          0xffff0000
#define _DEF_SSLTCP_INVALID_INDEX         0xffffffff


/* ---structs--- */
typedef struct
{
    UINT  ulSslTcpId;
    UINT  ulFamily;
    UINT  ulRemoteIp;
    UINT  ulHostIp;
    USHORT usIncNum;    /* 自增数 */
    USHORT usRemotePort;
    USHORT usHostPort;
    USHORT usType;  /*_SSLTCP_TCP_TYPE, _SSLTCP_SSL_TYPE*/
    BOOL_T bIsAsyn;
    BOOL_T bIsListen;
    HANDLE  hFileHandle;    /* Socket Id 或SSL HANDLE */
    MBUF_S *pstSendMbuf;    /* 用于存放没有发送完成的数据 */
    MBUF_S *pstRecvMbuf;    /* 用于存放没有接收完整的数据 */
    SSLTCP_PROTO_S *pstProto;

    PF_SSLTCP_ASYN_FUNC pfAsynCBFunc;
    USER_HANDLE_S     stUserHandle;
    HANDLE hAsynHandle;     /* 本SSLTCP属于的AsynHandle */
}_SSLTCP_CTRL_S;

/* --- vars ---*/
static BITMAP_S       stSslTcpBitMap;
static _SSLTCP_CTRL_S g_stSslTcpCtrl[SSLTCP_MAX_SSLTCP_NUM];
static SSLTCP_PROTO_S g_astSslTcpProtoTbl[_DEF_SSLTCP_MAX_PROTO_NUM];
static SEM_HANDLE g_hSsltcpSem = 0;

static BS_STATUS ssltcp_Init();

CONSTRUCTOR(init) {
    ssltcp_Init();
}

/* ---funcs--- */
static BS_STATUS ssltcp_Init()
{
    g_hSsltcpSem = SEM_CCreate("ssltcp", 1);
    if (g_hSsltcpSem == 0)
    {
        BS_WARNNING(("\r\n Can not create sem."));
        RETURN(BS_ERR);
    }
    Mem_Zero(g_astSslTcpProtoTbl, sizeof(SSLTCP_PROTO_S) * _DEF_SSLTCP_MAX_PROTO_NUM);
    BITMAP_Create(&stSslTcpBitMap, SSLTCP_MAX_SSLTCP_NUM);
    return BS_OK;
}

static UINT _SSLTCP_GetIndexFromSslTcpId(IN UINT ulSslTcpId)
{
    UINT ulMask = _DEF_SSLTCP_INC_NUM_MASK;

    return (ulSslTcpId & (~ulMask)) - 1;
}

static SSLTCP_PROTO_S * _SSLTCP_GetProto(IN CHAR *pszProtoName)
{
    UINT i;

    for (i=0; i<_DEF_SSLTCP_MAX_PROTO_NUM; i++)
    {
        if (g_astSslTcpProtoTbl[i].bIsUsed == FALSE)
        {
            continue;
        }

        if (strcmp(pszProtoName, g_astSslTcpProtoTbl[i].szProtoName) == 0)
        {
            return &g_astSslTcpProtoTbl[i];
        }
    }

    return NULL;
}

static VOID _SSLTCP_Clear(IN UINT ulIndexFrom0)
{
    USHORT usIncNum;
    
    usIncNum = g_stSslTcpCtrl[ulIndexFrom0].usIncNum;
    Mem_Zero(&g_stSslTcpCtrl[ulIndexFrom0], sizeof(_SSLTCP_CTRL_S));
    g_stSslTcpCtrl[ulIndexFrom0].usIncNum = usIncNum;
}

BS_STATUS SSLTCP_RegProto(IN SSLTCP_PROTO_S *pstProto)
{
    UINT i;

    SEM_P(g_hSsltcpSem, BS_WAIT, BS_WAIT_FOREVER);
    if (NULL != _SSLTCP_GetProto(pstProto->szProtoName))
    {
        SEM_V(g_hSsltcpSem);
        BS_WARNNING(("SSLTCP Protocol %s already exist", pstProto->szProtoName));
        RETURN(BS_ALREADY_EXIST);
    }

    for (i=0; i<_DEF_SSLTCP_MAX_PROTO_NUM; i++)
    {
        if (g_astSslTcpProtoTbl[i].bIsUsed == FALSE)
        {
            break;
        }
    }

    if (i >= _DEF_SSLTCP_MAX_PROTO_NUM)
    {
        SEM_V(g_hSsltcpSem);
        BS_WARNNING(("SSLTCP protocol table is full, can't regist protocol %s", pstProto->szProtoName));
        RETURN(BS_FULL);
    }

    g_astSslTcpProtoTbl[i] = *pstProto;
    g_astSslTcpProtoTbl[i].bIsUsed = TRUE;

    SEM_V(g_hSsltcpSem);

    return BS_OK;
}


UINT SSLTCP_Create(IN CHAR *pszProtoName, IN UINT ulFamily, IN VOID *pParam)
{
    UINT ulSslTcpId = 0;
    SSLTCP_PROTO_S *pstProto;
    HANDLE hFileHandle;
    UINT  ulIndexFrom1 = 0;
    UINT  ulIncNum;

    pstProto = _SSLTCP_GetProto(pszProtoName);
    if (NULL == pstProto)
    {
        return 0;
    }

    if (BS_OK != pstProto->pfCreate (ulFamily, pParam, &hFileHandle))
    {
        return 0;
    }
    
    SEM_P(g_hSsltcpSem, BS_WAIT, BS_WAIT_FOREVER);
    ulIndexFrom1 = BITMAP1_GetFreeCycle(&stSslTcpBitMap);
    if (ulIndexFrom1 != 0)
    {
        BITMAP_SET(&stSslTcpBitMap, ulIndexFrom1);
    }
    SEM_V(g_hSsltcpSem);

    if (ulIndexFrom1 == 0)
    {
        pstProto->pfClose(hFileHandle);
        BS_WARNNING(("No empty ssltcp"));
        RETURN(BS_NO_RESOURCE);
    }

    ulIncNum = g_stSslTcpCtrl[ulIndexFrom1-1].usIncNum;
    ulIncNum ++;
    ulSslTcpId = ((ulIncNum << 16) & _DEF_SSLTCP_INC_NUM_MASK) | ulIndexFrom1;

    _SSLTCP_Clear(ulIndexFrom1-1);

    g_stSslTcpCtrl[ulIndexFrom1-1].pstProto    = pstProto;
    g_stSslTcpCtrl[ulIndexFrom1-1].hFileHandle = hFileHandle;
    g_stSslTcpCtrl[ulIndexFrom1-1].ulSslTcpId  = ulSslTcpId;
    g_stSslTcpCtrl[ulIndexFrom1-1].ulFamily    = ulFamily;
    g_stSslTcpCtrl[ulIndexFrom1-1].usIncNum = ulIncNum;

    return ulSslTcpId;
}

BOOL_T SSLTCP_IsValid (IN UINT ulSslTcpId)
{
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(ulSslTcpId);

    if (ulIndex >= SSLTCP_MAX_SSLTCP_NUM)
    {
        return FALSE;
    }

    if (g_stSslTcpCtrl[ulIndex].ulSslTcpId != ulSslTcpId)
    {
        return FALSE;
    }

    return TRUE;
}

/* IP/Port:主机序 */
BS_STATUS SSLTCP_Listen(IN UINT uiSslTcpId, IN UINT uiIp, IN USHORT usPort, IN UINT uiBackLog)
{
    BS_STATUS eRet;
    UINT uiIndex = _SSLTCP_GetIndexFromSslTcpId(uiSslTcpId);

    g_stSslTcpCtrl[uiIndex].usHostPort = usPort;
    g_stSslTcpCtrl[uiIndex].bIsListen  = TRUE;
    eRet = g_stSslTcpCtrl[uiIndex].pstProto->pfListen (g_stSslTcpCtrl[uiIndex].hFileHandle, uiIp, usPort, uiBackLog);

    return eRet;
}

/***************************************************
 Description  : 填充连接的一些常用信息
 Input        : uiSsltcpId: SSLTCP ID
 Output       : None
 Return       : 成功: BS_OK
                失败: 错误码
 Caution      : 
****************************************************/
static BS_STATUS ssltcp_FillConnInfo(IN UINT ulSslTcpId)
{
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(ulSslTcpId);

    g_stSslTcpCtrl[ulIndex].pstProto->pfGetHostIpPort(g_stSslTcpCtrl[ulIndex].hFileHandle,
            &g_stSslTcpCtrl[ulIndex].ulHostIp, &g_stSslTcpCtrl[ulIndex].usHostPort);

    g_stSslTcpCtrl[ulIndex].pstProto->pfGetPeerIpPort(g_stSslTcpCtrl[ulIndex].hFileHandle,
        &g_stSslTcpCtrl[ulIndex].ulRemoteIp, &g_stSslTcpCtrl[ulIndex].usRemotePort);

    return BS_OK;
}

/* BS_OK:成功, BS_AGAIN:本次Accept失败但listen的fd是好的; 其他:listen的fd出错了 */
/* 不继承Listen Socket的异步属性 */
BS_STATUS SSLTCP_Accept(IN UINT hListenSslTcpId, OUT UINT *puiAcceptSslTcpId)
{
    UINT          ulRet;
    UINT          ulIndexFrom1 = 0;
    HANDLE          hFileHandle = 0;
    UINT          ulIndex = _SSLTCP_GetIndexFromSslTcpId(hListenSslTcpId);
    UINT          ulIncNum;
    UINT          ulSslTcpId;

    ulRet = g_stSslTcpCtrl[ulIndex].pstProto->pfAccept (g_stSslTcpCtrl[ulIndex].hFileHandle, &hFileHandle);
    if (BS_OK != ulRet)
    {
        return ulRet;
    }

    SEM_P(g_hSsltcpSem, BS_WAIT, BS_WAIT_FOREVER);
    ulIndexFrom1 = BITMAP1_GetFreeCycle(&stSslTcpBitMap);
    if (ulIndexFrom1 != 0)
    {
        BITMAP_SET (&stSslTcpBitMap, ulIndexFrom1);
    }
    SEM_V(g_hSsltcpSem);

    if (ulIndexFrom1 == 0)
    {
        g_stSslTcpCtrl[ulIndex].pstProto->pfClose(hFileHandle);
        BS_WARNNING (("No empty ssltcp"));
        RETURN(BS_NO_RESOURCE);
    }

    ulIncNum = g_stSslTcpCtrl[ulIndexFrom1-1].usIncNum;
    ulIncNum ++;
    ulSslTcpId = ((ulIncNum << 16) & _DEF_SSLTCP_INC_NUM_MASK) | ulIndexFrom1;
    _SSLTCP_Clear(ulIndexFrom1-1);
    g_stSslTcpCtrl[ulIndexFrom1-1] = g_stSslTcpCtrl[ulIndex];
    g_stSslTcpCtrl[ulIndexFrom1-1].ulSslTcpId = ulSslTcpId;
    g_stSslTcpCtrl[ulIndexFrom1-1].bIsAsyn = FALSE;
    g_stSslTcpCtrl[ulIndexFrom1-1].bIsListen = FALSE;
    g_stSslTcpCtrl[ulIndexFrom1-1].hFileHandle = hFileHandle;
    g_stSslTcpCtrl[ulIndexFrom1-1].usIncNum = ulIncNum;

    ssltcp_FillConnInfo(ulSslTcpId);
    
    *puiAcceptSslTcpId = ulSslTcpId;

    return BS_OK;
}

/* ip/port:主机序 */
BS_STATUS SSLTCP_Connect(IN UINT ulSslTcpId, IN UINT ulIp, IN USHORT usPort)
{
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(ulSslTcpId);

    g_stSslTcpCtrl[ulIndex].ulRemoteIp = ulIp;
    g_stSslTcpCtrl[ulIndex].usRemotePort = usPort;

    return g_stSslTcpCtrl[ulIndex].pstProto->pfConnect (g_stSslTcpCtrl[ulIndex].hFileHandle, ulIp, usPort);
}

BS_STATUS SSLTCP_Write(IN UINT ulSslTcpId, IN UCHAR * pucBuf, IN UINT ulSize, OUT UINT *puiWriteSize)
{
    INT lSize;
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(ulSslTcpId);

    lSize = g_stSslTcpCtrl[ulIndex].pstProto->pfWrite(g_stSslTcpCtrl[ulIndex].hFileHandle, pucBuf, ulSize, 0);
    if (lSize < 0)
    {
        RETURN(BS_ERR);
    }

    if (puiWriteSize)
    {
        *puiWriteSize = (UINT)lSize;
    }

    return BS_OK;
}

/* 只能是阻塞式SSLTCP 才能调这个函数 */
BS_STATUS SSLTCP_WriteUntilFinish(IN UINT ulSslTcpId, IN UCHAR * pucBuf, IN UINT ulSize)
{
    UINT ulWriteLenTotle;
    UINT ulWriteLenTmp;

    ulWriteLenTotle = 0;
    
    do {
        if (BS_OK != SSLTCP_Write(ulSslTcpId, pucBuf + ulWriteLenTotle, ulSize, &ulWriteLenTmp))
        {
            RETURN(BS_ERR);
        }
        ulWriteLenTotle += ulWriteLenTmp;
        ulSize -= ulWriteLenTmp;
    }while(ulSize > 0);    

    return BS_OK;
}

BS_STATUS SSLTCP_Read
(
    IN UINT ulSslTcpId,
    OUT void* pucBuf,
    IN UINT ulBufSize,
    OUT UINT *pulReadSize
)
{
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(ulSslTcpId);

    *pulReadSize = 0;

    return g_stSslTcpCtrl[ulIndex].pstProto->pfRead
               (g_stSslTcpCtrl[ulIndex].hFileHandle, pucBuf, (INT)ulBufSize, pulReadSize, 0);
}

BS_STATUS SSLTCP_Close(IN UINT ulSslTcpId)
{
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(ulSslTcpId);

    if (! SSLTCP_IsValid(ulSslTcpId))
    {
        return BS_OK;
    }

    if (g_stSslTcpCtrl[ulIndex].bIsAsyn == TRUE)
    {
        SSLTCP_UnSetAsyn(ulSslTcpId);
    }

    g_stSslTcpCtrl[ulIndex].pstProto->pfClose(g_stSslTcpCtrl[ulIndex].hFileHandle);

    if (g_stSslTcpCtrl[ulIndex].pstSendMbuf != NULL)
    {
        MBUF_Free(g_stSslTcpCtrl[ulIndex].pstSendMbuf);
        g_stSslTcpCtrl[ulIndex].pstSendMbuf = NULL;
    }

    if (g_stSslTcpCtrl[ulIndex].pstRecvMbuf != NULL)
    {
        MBUF_Free(g_stSslTcpCtrl[ulIndex].pstRecvMbuf);
        g_stSslTcpCtrl[ulIndex].pstRecvMbuf = NULL;
    }

    _SSLTCP_Clear(ulIndex);

    SEM_P(g_hSsltcpSem, BS_WAIT, BS_WAIT_FOREVER);
    BITMAP1_CLR(&stSslTcpBitMap, ulIndex + 1);
    SEM_V(g_hSsltcpSem);

    return BS_OK;
}

static BS_STATUS _SSLTCP_AsynCallBack
(
    IN HANDLE hFileHandle,
    IN ULONG ulEventType,
    IN USER_HANDLE_S *pstUserHandle
)
{
    PF_SSLTCP_ASYN_FUNC pfFunc = NULL;
    USER_HANDLE_S stUserHandle;
    UINT ulSslTcpId = HANDLE_UINT(pstUserHandle->ahUserHandle[0]);
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(ulSslTcpId);

    SEM_P(g_hSsltcpSem, BS_WAIT, BS_WAIT_FOREVER);
    if (BITMAP1_ISSET(&stSslTcpBitMap, ulIndex + 1))
    {
        pfFunc = g_stSslTcpCtrl[ulIndex].pfAsynCBFunc;
        stUserHandle = g_stSslTcpCtrl[ulIndex].stUserHandle;
    }
    SEM_V(g_hSsltcpSem);

    if (pfFunc)
    {
        pfFunc (ulSslTcpId, ulEventType, &stUserHandle);
    }

    return BS_OK;
}

HANDLE SSLTCP_CreateAsynInstance(IN SSLTCP_ASYN_MODE_E eAsynMode)
{
    SSLTCP_PROTO_S *pstProto;

    pstProto = _SSLTCP_GetProto("tcp");
    if (NULL == pstProto)
    {
        return NULL;
    }

    return pstProto->pfCreateAsynInstance(eAsynMode);
}

VOID SSLTCP_DeleteAsynInstance(IN HANDLE hAsynHandle)
{
    SSLTCP_PROTO_S *pstProto;

    pstProto = _SSLTCP_GetProto("tcp");
    if (NULL == pstProto)
    {
        return;
    }
    pstProto->pfDeleteAsynInstance(hAsynHandle);
}


static void _SSLTCP_DftAsynThread(IN USER_HANDLE_S *pstUserHandle)
{
    SSLTCP_PROTO_S *pstProto = pstUserHandle->ahUserHandle[0];

    while (1)
    {
        pstProto->pfDispatch(pstProto->hDftAsynInstance);
    }
}

static HANDLE _SSLTCP_CreateDftAsynInstance(IN SSLTCP_PROTO_S *pstProto)
{
    BOOL_T bCreateDftThread = FALSE;
    USER_HANDLE_S stUsrHandle;
    
    SEM_P(g_hSsltcpSem, BS_WAIT, BS_WAIT_FOREVER);
    if (pstProto->hDftAsynInstance == NULL)
    {
        pstProto->hDftAsynInstance = pstProto->pfCreateAsynInstance(SSLTCP_ASYN_MODE_EDGE);
        if (NULL != pstProto->hDftAsynInstance)
        {
            bCreateDftThread = TRUE;
        }
    }
    SEM_V(g_hSsltcpSem);

    if (bCreateDftThread == TRUE)
    {
        stUsrHandle.ahUserHandle[0] = pstProto;
        pstProto->uiDftAsynThreadId= THREAD_Create("SSLTCP_DFTASYN", NULL,
            _SSLTCP_DftAsynThread, &stUsrHandle);
    }

    return pstProto->hDftAsynInstance;
}

/***************************************************
 Description  : 设置异步属性
 Input        : hAsynInstance: 异步实例. 如果为NULL则表示使用对应协议的默认异步实例
                uiSsltcpId: SSLTCP ID
                ulEvent: 关心的事件,可以设置为SSLTCP_EVENT_READ/SSLTCP_EVENT_WRITE. 
                         SSLTCP_EVENT_EXECPT, 不管是否设置, 会一直等待该事件
                         SSLTCP_EVENT_TRIGGER, 不管是否设置, 会一直等待该事件
                pfFunc: 回调函数
                pstUserHandle: 用户回传参数
 Output       : None
 Return       : 成功: FCGI Channel句柄
                失败: NULL
 Caution      : 要求SsltcpId的模式和eMode必须匹配
****************************************************/
BS_STATUS SSLTCP_SetAsyn
(
    IN HANDLE hAsynInstance,
    IN UINT uiSslTcpId,
    IN ULONG ulEvent,
    IN PF_SSLTCP_ASYN_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    BS_STATUS eRet;
    USER_HANDLE_S stUserHandle;
    HANDLE hAsynInstanceTmp = hAsynInstance;
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(uiSslTcpId);

    if (hAsynInstanceTmp == NULL)
    {
        hAsynInstanceTmp = g_stSslTcpCtrl[ulIndex].pstProto->hDftAsynInstance;
        if (hAsynInstanceTmp == NULL)
        {
            hAsynInstanceTmp = _SSLTCP_CreateDftAsynInstance(g_stSslTcpCtrl[ulIndex].pstProto);
        }
        if (hAsynInstanceTmp == NULL)
        {
            RETURN(BS_ERR);
        }
    }

    g_stSslTcpCtrl[ulIndex].stUserHandle = *pstUserHandle;
    g_stSslTcpCtrl[ulIndex].pfAsynCBFunc  = pfFunc;

    stUserHandle.ahUserHandle[0] = UINT_HANDLE(uiSslTcpId);

    eRet = g_stSslTcpCtrl[ulIndex].pstProto->pfSetAsyn (hAsynInstanceTmp,
        g_stSslTcpCtrl[ulIndex].hFileHandle,
        ulEvent, _SSLTCP_AsynCallBack, &stUserHandle);
    if (BS_OK == eRet)
    {
        g_stSslTcpCtrl[ulIndex].hAsynHandle = hAsynInstance;
        g_stSslTcpCtrl[ulIndex].bIsAsyn = TRUE;
    }
    
    return eRet;
}

BS_STATUS SSLTCP_GetAsyn(IN UINT ulSslTcpId, OUT USER_HANDLE_S **ppstUserHandle)
{
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(ulSslTcpId);

    *ppstUserHandle = &g_stSslTcpCtrl[ulIndex].stUserHandle;

    return BS_OK;
}

BS_STATUS SSLTCP_SetAsynTrigger
(
    IN HANDLE hAsynInstance,
    IN PF_SSLTCP_TRIGGER_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    SSLTCP_PROTO_S *pstProto;

    BS_DBGASSERT(NULL != hAsynInstance);

    pstProto = _SSLTCP_GetProto("tcp");
    if (NULL == pstProto)
    {
        RETURN(BS_ERR);
    }

    return pstProto->pfSetAsynTrigger(hAsynInstance, pfFunc, pstUserHandle);
}

BS_STATUS SSLTCP_AsynTrigger(IN HANDLE hAsynInstance)
{
    SSLTCP_PROTO_S *pstProto;

    BS_DBGASSERT(NULL != hAsynInstance);

    pstProto = _SSLTCP_GetProto("tcp");
    if (NULL == pstProto)
    {
        RETURN(BS_ERR);
    }

    return pstProto->pfAsynTrigger(hAsynInstance);
}


HANDLE SSLTCP_GetAsynInstanceHandle(IN UINT uiSsltcpId)
{
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(uiSsltcpId);

    return g_stSslTcpCtrl[ulIndex].hAsynHandle;
}

BS_STATUS SSLTCP_UnSetAsyn(IN UINT ulSslTcpId)
{
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(ulSslTcpId);
    HANDLE hAsynInstance = SSLTCP_GetAsynInstanceHandle(ulSslTcpId);

    if (hAsynInstance == NULL)
    {
        hAsynInstance = g_stSslTcpCtrl[ulIndex].pstProto->hDftAsynInstance;
        if (hAsynInstance == NULL)
        {
            RETURN(BS_ERR);
        }
    }

    g_stSslTcpCtrl[ulIndex].pstProto->pfUnSetAsyn (hAsynInstance, g_stSslTcpCtrl[ulIndex].hFileHandle);
    g_stSslTcpCtrl[ulIndex].bIsAsyn = FALSE;
    g_stSslTcpCtrl[ulIndex].hAsynHandle = NULL;
    return BS_OK;
}

BS_STATUS SSLTCP_Ioctl(IN UINT uiSslTcpId, IN INT lCmd, IN VOID *pParam)
{
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(uiSslTcpId);

    return g_stSslTcpCtrl[ulIndex].pstProto->pfIoctl (g_stSslTcpCtrl[ulIndex].hFileHandle, lCmd, pParam);
}

UINT SSLTCP_GetConnectionNum(IN HANDLE hAsynInstance)
{
    SSLTCP_PROTO_S *pstProto;

    pstProto = _SSLTCP_GetProto("tcp");
    if (NULL == pstProto)
    {
        return 0;
    }

    return pstProto->pfGetConnectionNum(hAsynInstance);
}

BS_STATUS SSLTCP_Dispatch(IN HANDLE hAsynInstance)
{
    SSLTCP_PROTO_S *pstProto;

    pstProto = _SSLTCP_GetProto("tcp");
    if (NULL == pstProto)
    {
        RETURN(BS_ERR);
    }

    return pstProto->pfDispatch(hAsynInstance);
}

UINT SSLTCP_GetFamily(IN UINT ulSslTcpId)
{
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(ulSslTcpId);

    return g_stSslTcpCtrl[ulIndex].ulFamily;
}

USHORT SSLTCP_GetHostPort(IN UINT ulSslTcpId)
{
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(ulSslTcpId);

    return g_stSslTcpCtrl[ulIndex].usHostPort;
}

UINT SSLTCP_GetHostIP(IN UINT ulSslTcpId)
{
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(ulSslTcpId);

    return g_stSslTcpCtrl[ulIndex].ulHostIp;
}

/* 返回主机序IP地址 */
UINT SSLTCP_GetPeerIP(IN UINT ulSslTcpId)
{
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(ulSslTcpId);

    return g_stSslTcpCtrl[ulIndex].ulRemoteIp;
}

/* 返回主机序Port */
USHORT SSLTCP_GetPeerPort(IN UINT ulSslTcpId)
{
    UINT ulIndex = _SSLTCP_GetIndexFromSslTcpId(ulSslTcpId);

    return g_stSslTcpCtrl[ulIndex].usRemotePort;
}

BS_STATUS SSLTCP_Display (IN UINT ulArgc, IN UCHAR **argv)
{
    UINT i;

    EXEC_OutString(" ID    Type  Family  IsAsyn   HPort  RIP              RPort  FileID  Listen\r\n"
        "------------------------------------------------------------------------------\r\n");

    for (i=1; i<= SSLTCP_MAX_SSLTCP_NUM; i++)
    {
        if (BITMAP1_ISSET(&stSslTcpBitMap, i))
        {
            if ((g_stSslTcpCtrl[i-1].bIsListen == FALSE) && (g_stSslTcpCtrl[i-1].usRemotePort == 0))
            {
                SSLTCP_GetPeerPort(i);
                SSLTCP_GetPeerIP(i);
            }

            EXEC_OutInfo(" %-4d  %-4s  %-6s  %-7s  %-5d  %-15s  %-5d  %-6d  %s\r\n",
                i,
                g_stSslTcpCtrl[i-1].pstProto->szProtoName,
                g_stSslTcpCtrl[i-1].ulFamily == AF_INET ? "IPv4" : "IPv6",
                g_stSslTcpCtrl[i-1].bIsAsyn == TRUE ? "True" : "Fasle",
                g_stSslTcpCtrl[i-1].usHostPort,
                Socket_IpToName(g_stSslTcpCtrl[i-1].ulRemoteIp),
                g_stSslTcpCtrl[i-1].usRemotePort,
                g_stSslTcpCtrl[i-1].hFileHandle,
                g_stSslTcpCtrl[i-1].bIsListen == TRUE ? "True" : "False");
        }
    }
    EXEC_OutString("\r\n");

    return BS_OK;
}

