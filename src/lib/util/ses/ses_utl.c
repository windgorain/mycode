/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-12-10
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/nap_utl.h"
#include "utl/vclock_utl.h"
#include "utl/ka_utl.h"
#include "utl/ses_utl.h"

#define _SES_VER 0

#define _SES_PKT_RESERVED_MBUF_LEN 200

#define _SES_DFT_PKT_RESEND_TICK 5  /* 多少个Tick之后重发报文 */
#define _SES_DFT_PKT_RESEND_COUNT_MAX 3 /* 重发多少次之后,认为失败了 */
#define _SES_DFT_KEEPALIVE_IDLE   300 /* 5 min */
#define _SES_DFT_KEEPALIVE_INTVAL 3   /* 5 s */
#define _SES_DFT_KEEPALIVE_MAX_TRYS 3 /* 多少个KeepAlive探测失败后老化 */

typedef struct
{
    UINT uiDbgFlag;
    UINT uiMaxSesNum;
    UINT uiUserContextSize;
    UINT uiMaxProperty;
    UINT uiCookie;
    HANDLE hNap;
    VCLOCK_INSTANCE_HANDLE hVClockInstance;
    PF_SES_RECV_PKT pfRecvPktFunc;
    PF_SES_SEND_PKT pfSendPktFunc;
    PF_SES_DFT_EVENT_NOTIFY pfEventNotify;  /* 缺省事件通知函数 */
    DLL_HEAD_S stCloseNotifyList;
    SES_OPT_KEEP_ALIVE_TIME_S stDftKeepAlive;
}_SES_CTRL_S;

#define _SES_NODE_FLAG_IS_CONNECTER 0x1 /* 是否主动发起者 */

typedef struct
{
    UINT uiStatus;
    UINT uiFlag;
    UINT uiLocalSesID;
    UINT uiPeerSesID;
    UINT uiCookie;        /* 用于避免误认SES而弄得一个标记 */
    MBUF_S *pstSendMbuf;  /* 发送缓冲区 */
    USHORT usResendCount;   /* 重发了多少次了 */
    USHORT usKeepAliveTrysCount; /* 已经连续多少次进行KeepAlive探测了 */
    VCLOCK_HANDLE hResendTimer;    /* 报文重发定时器 */
    HANDLE *phPropertys; /* 指向Propertys */
    VOID *pUserContext; /* 指向UserContext */
    PF_SES_EVENT_NOTIFY pfEventNotify; /* SES连接的事件通知函数，优先级高于缺省的事件通知函数 */
    USER_HANDLE_S stEventNotifyUserHandle;
    KA_S stKeepAliveOpt;
}_SES_NODE_S;

typedef struct
{
    DLL_NODE_S stNode;
    PF_SES_CLOSE_NOTIFY_FUNC pfNotifyFunc;
    USER_HANDLE_S stUserHandle;
}_SES_CLOSE_NOTIFY_S;

#define _SES_PKT_FLAG_DATA      0x1  /* 数据报文,非协议报文 */
#define _SES_PKT_FLAG_SYN       0x2
#define _SES_PKT_FLAG_ACK       0x4
#define _SES_PKT_FLAG_RST       0x8
#define _SES_PKT_FLAG_KEEPALIVE 0x10

#define _SES_PKT_FLAG_STRING_LEN 31

typedef struct
{
    USHORT usVer;
    USHORT usPktFlag;
    UINT uiDstSesId;
    UINT uiSrcSesId;
    UINT uiCookie;
    UINT uiPayloadLen;
}_SES_PKT_HEAD_S;

typedef struct
{
    _SES_PKT_HEAD_S stPktHeader;
    VOID *pUserContext;
}_SES_PKT_INFO_S;

static CHAR * ses_GetPktFlagString(IN USHORT usFlag, OUT CHAR szFlagString[_SES_PKT_FLAG_STRING_LEN + 1])
{
    UINT uiIndex = 0;
    
    if (usFlag & _SES_PKT_FLAG_DATA)
    {
        szFlagString[uiIndex] = 'D';
        uiIndex ++;
    }

    if (usFlag & _SES_PKT_FLAG_SYN)
    {
        szFlagString[uiIndex] = 'S';
        uiIndex ++;
    }

    if (usFlag & _SES_PKT_FLAG_ACK)
    {
        szFlagString[uiIndex] = 'A';
        uiIndex ++;
    }

    if (usFlag & _SES_PKT_FLAG_RST)
    {
        szFlagString[uiIndex] = 'R';
        uiIndex ++;
    }

    if (usFlag & _SES_PKT_FLAG_KEEPALIVE)
    {
        szFlagString[uiIndex] = 'K';
        uiIndex ++;
    }

    szFlagString[uiIndex] = '\0';

    return szFlagString;
}

static VOID ses_FillPktHeader
(
    IN _SES_NODE_S *pstSesNode,
    IN USHORT usPktFlag,
    IN UINT uiPayLoadLen,
    INOUT _SES_PKT_HEAD_S *pstPktHeader
)
{
    pstPktHeader->usVer = htons(_SES_VER);
    pstPktHeader->usPktFlag = htons(usPktFlag);
    pstPktHeader->uiDstSesId = htonl(pstSesNode->uiPeerSesID);
    pstPktHeader->uiSrcSesId = htonl(pstSesNode->uiLocalSesID);
    pstPktHeader->uiCookie = pstSesNode->uiCookie;
    pstPktHeader->uiPayloadLen = htonl(uiPayLoadLen);
}

static inline MBUF_S * ses_BuildPkt
(
    IN _SES_PKT_HEAD_S *pstPktHead   /* 里面各个字段都是网络序 */
)
{
    return MBUF_CreateByCopyBuf(_SES_PKT_RESERVED_MBUF_LEN, (UCHAR*)pstPktHead, sizeof(_SES_PKT_HEAD_S), 0);
}

static MBUF_S * ses_BuildProtocolPkt
(
    IN _SES_NODE_S *pstSesNode,
    IN UINT uiPktFlag
)
{
    _SES_PKT_HEAD_S stPkt = {0};

    ses_FillPktHeader(pstSesNode, uiPktFlag, 0, &stPkt);

    return ses_BuildPkt(&stPkt);
}

static VOID ses_EventNotify
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode,
    IN UINT uiEvent
)
{
    if (pstSesNode->pfEventNotify != NULL)
    {
        pstSesNode->pfEventNotify(pstSesNode->uiLocalSesID, uiEvent, &pstSesNode->stEventNotifyUserHandle);
    }
    else
    {
        pstCtrl->pfEventNotify(pstSesNode->uiLocalSesID, uiEvent);
    }
}

static inline BS_STATUS ses_SendPkt
(
    IN _SES_CTRL_S *pstCtrl,
    IN VOID *pContext,
    IN MBUF_S *pstMbuf
)
{
    _SES_PKT_HEAD_S *pstHead;
    CHAR szPktFlagString[_SES_PKT_FLAG_STRING_LEN + 1];

    pstHead = MBUF_MTOD(pstMbuf);

    if (pstHead->usPktFlag & htons(_SES_PKT_FLAG_DATA))
    {
        BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, SES_DBG_FLAG_DATA_PKT,
            ("SES Send: Dst:%x,Src:%x,Flag:%s,Payload:%d\r\n",
             ntohl(pstHead->uiDstSesId),
             ntohl(pstHead->uiSrcSesId),
             ses_GetPktFlagString(ntohs(pstHead->usPktFlag), szPktFlagString),
             ntohl(pstHead->uiPayloadLen)
             ));
    }
    else
    {
        BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, SES_DBG_FLAG_PROTOCOL_PKT,
            ("SES Send: Dst:%x,Src:%x,Flag:%s,Payload:%d\r\n",
             ntohl(pstHead->uiDstSesId),
             ntohl(pstHead->uiSrcSesId),
             ses_GetPktFlagString(ntohs(pstHead->usPktFlag), szPktFlagString),
             ntohl(pstHead->uiPayloadLen)
             ));
    }

    (VOID) pstCtrl->pfSendPktFunc(pstMbuf, pContext);

    return BS_OK;
}

static inline VOID ses_StopResenderTimer
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode
)
{
    VCLOCK_DestroyTimer(pstCtrl->hVClockInstance, pstSesNode->hResendTimer);
    pstSesNode->hResendTimer = NULL;
}

static inline VOID ses_StopTimer(IN _SES_CTRL_S *pstCtrl, IN _SES_NODE_S *pstSesNode)
{
    /* 停止重发/老化/keepalive定时器 */
    if (pstSesNode->hResendTimer != NULL)
    {
        VCLOCK_DestroyTimer(pstCtrl->hVClockInstance, pstSesNode->hResendTimer);
        pstSesNode->hResendTimer = NULL;
        pstSesNode->usResendCount = 0;
    }

    KA_Stop(&pstSesNode->stKeepAliveOpt);
}

static inline VOID ses_Abort
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode,
    IN UINT uiEvent
)
{
    ses_StopTimer(pstCtrl, pstSesNode);

    pstSesNode->uiStatus = SES_STATUS_CLOSED;

    ses_EventNotify(pstCtrl, pstSesNode, uiEvent);
}

static VOID ses_ResendTimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    _SES_CTRL_S *pstCtrl = pstUserHandle->ahUserHandle[0];
    _SES_NODE_S *pstSesNode = pstUserHandle->ahUserHandle[1];
    MBUF_S *pstNewMbuf;

    if (pstSesNode->pstSendMbuf == NULL)
    {
        ses_StopResenderTimer(pstCtrl, pstSesNode);
        return;
    }

    if (pstSesNode->usResendCount >= _SES_DFT_PKT_RESEND_COUNT_MAX)
    {
        ses_Abort(pstCtrl, pstSesNode, SES_EVENT_CONNECT_FAILED);
        return;
    }

    pstSesNode->usResendCount ++;

    pstNewMbuf = MBUF_RawCopy(pstSesNode->pstSendMbuf, 0,
        MBUF_TOTAL_DATA_LEN(pstSesNode->pstSendMbuf),
        _SES_PKT_RESERVED_MBUF_LEN);
    if (NULL != pstNewMbuf)
    {
        ses_SendPkt(pstCtrl, pstSesNode->pUserContext, pstNewMbuf);
    }
}

static VOID ses_KeepAliveFailed(IN KA_S *pstKA)
{
    _SES_CTRL_S *pstCtrl = pstKA->stUserHandle.ahUserHandle[0];
    _SES_NODE_S *pstSesNode = pstKA->stUserHandle.ahUserHandle[1];

    ses_Abort(pstCtrl, pstSesNode, SES_EVENT_PEER_CLOSED);
}

static VOID ses_SendKeepAlive(IN KA_S *pstKA)
{
    _SES_CTRL_S *pstCtrl = pstKA->stUserHandle.ahUserHandle[0];
    _SES_NODE_S *pstSesNode = pstKA->stUserHandle.ahUserHandle[1];
    MBUF_S *pstMbuf;

    pstMbuf = ses_BuildProtocolPkt(pstSesNode, _SES_PKT_FLAG_KEEPALIVE);
    if (NULL == pstMbuf)
    {
        return;
    }

    ses_SendPkt(pstCtrl, pstSesNode->pUserContext, pstMbuf);
}

static inline BS_STATUS ses_StartResenderTimer
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode
)
{
    VCLOCK_HANDLE hVClock;
    USER_HANDLE_S stUserHandle;

    if (pstSesNode->hResendTimer != NULL)
    {
        return BS_OK;
    }

    stUserHandle.ahUserHandle[0] = pstCtrl;
    stUserHandle.ahUserHandle[1] = pstSesNode;

    hVClock = VCLOCK_CreateTimer(pstCtrl->hVClockInstance, _SES_DFT_PKT_RESEND_TICK, _SES_DFT_PKT_RESEND_TICK,
                TIMER_FLAG_CYCLE, ses_ResendTimeOut, &stUserHandle);
    if (NULL == hVClock)
    {
        return BS_ERR;
    }

    pstSesNode->hResendTimer = hVClock;

    return BS_OK;
}

static inline BS_STATUS ses_StartKeepAliveTimer
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode
)
{
    return KA_Start(pstCtrl->hVClockInstance, &pstSesNode->stKeepAliveOpt);
}

static inline BS_STATUS ses_RecordAndSendPkt
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode,
    IN MBUF_S *pstMbuf
)
{
    MBUF_S *pstNewMbuf;

    BS_DBGASSERT(pstSesNode->pstSendMbuf == NULL);

    if (BS_OK != ses_StartResenderTimer(pstCtrl, pstSesNode))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    pstSesNode->pstSendMbuf = pstMbuf;

    pstNewMbuf = MBUF_RawCopy(pstMbuf, 0, MBUF_TOTAL_DATA_LEN(pstMbuf), _SES_PKT_RESERVED_MBUF_LEN);
    if (NULL != pstNewMbuf)
    {
        ses_SendPkt(pstCtrl, pstSesNode->pUserContext, pstNewMbuf);
    }

    return BS_OK;
}

static BS_STATUS ses_SendSyn
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstNode
)
{
    MBUF_S *pstMbuf;

    pstMbuf = ses_BuildProtocolPkt(pstNode, _SES_PKT_FLAG_SYN);
    if (NULL == pstMbuf)
    {
        return BS_NO_MEMORY;
    }

    pstNode->uiStatus = SES_STATUS_SYN_SEND;

    return ses_RecordAndSendPkt(pstCtrl, pstNode, pstMbuf);
}

_SES_NODE_S * ses_FindSesNode
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_PKT_INFO_S *pstPktInfo
)
{
    _SES_NODE_S *pstSesNode;

    pstSesNode = NAP_GetNodeByID(pstCtrl->hNap, pstPktInfo->stPktHeader.uiDstSesId);
    if (NULL == pstSesNode)
    {
        return NULL;
    }

    if ((pstSesNode->uiPeerSesID != 0)
        && (pstSesNode->uiPeerSesID != pstPktInfo->stPktHeader.uiSrcSesId))
    {
        return NULL;
    }

    if ((pstSesNode->uiStatus != SES_STATUS_SYN_SEND)
        && (pstSesNode->uiCookie != pstPktInfo->stPktHeader.uiCookie))
    {
        return NULL;
    }

    if (pstSesNode->uiStatus == SES_STATUS_CLOSED)
    {
        return NULL;
    }

    return pstSesNode;
}

static VOID ses_CopyUserContext(IN _SES_CTRL_S *pstCtrl, IN VOID *pSrc, OUT VOID *pDst)
{
    MEM_Copy(pDst, pSrc, pstCtrl->uiUserContextSize);
}

static VOID ses_FreeSes(IN _SES_CTRL_S *pstCtrl, IN _SES_NODE_S *pstSesNode)
{
    if (NULL != pstSesNode->pstSendMbuf)
    {
        MBUF_Free(pstSesNode->pstSendMbuf);
        pstSesNode->pstSendMbuf = NULL;
    }

    ses_StopTimer(pstCtrl, pstSesNode);

    if (pstSesNode->pUserContext != NULL)
    {
        MEM_Free(pstSesNode->pUserContext);
    }

    if (pstSesNode->phPropertys != NULL)
    {
        MEM_Free(pstSesNode->phPropertys);
    }

    NAP_Free(pstCtrl->hNap, pstSesNode);
}

static VOID ses_InitKA(IN _SES_CTRL_S *pstCtrl, IN _SES_NODE_S *pstSesNode)
{
    pstSesNode->stKeepAliveOpt.stSet.usIdle = pstCtrl->stDftKeepAlive.usIdle;
    pstSesNode->stKeepAliveOpt.stSet.usIntval = pstCtrl->stDftKeepAlive.usIntval;
    pstSesNode->stKeepAliveOpt.stSet.usMaxProbeCount = pstCtrl->stDftKeepAlive.usMaxProbeCount;
    pstSesNode->stKeepAliveOpt.stSet.pfSendKeepAlive = ses_SendKeepAlive;
    pstSesNode->stKeepAliveOpt.stSet.pfKeepAliveFailed = ses_KeepAliveFailed;

    pstSesNode->stKeepAliveOpt.stUserHandle.ahUserHandle[0] = pstCtrl;
    pstSesNode->stKeepAliveOpt.stUserHandle.ahUserHandle[1] = pstSesNode;
}

static _SES_NODE_S * ses_NewSes(IN _SES_CTRL_S *pstCtrl, IN VOID *pContext)
{
    _SES_NODE_S *pstNode;
    UINT uiSesID;

    pstNode = NAP_ZAlloc(pstCtrl->hNap);
    if (NULL == pstNode)
    {
        return NULL;
    }

    if (pstCtrl->uiUserContextSize > 0)
    {
        pstNode->pUserContext = MEM_Malloc(pstCtrl->uiUserContextSize);
        if (NULL == pstNode->pUserContext)
        {
            ses_FreeSes(pstCtrl, pstNode);
            return NULL;
        }
    }

    if (pstCtrl->uiMaxProperty > 0)
    {
        pstNode->phPropertys = MEM_ZMalloc(sizeof(HANDLE) * pstCtrl->uiMaxProperty);
        if (NULL == pstNode->phPropertys)
        {
            ses_FreeSes(pstCtrl, pstNode);
            return NULL;
        }
    }

    ses_CopyUserContext(pstCtrl, pContext, pstNode->pUserContext);

    uiSesID = (UINT)NAP_GetIDByNode(pstCtrl->hNap, pstNode);
    pstNode->uiLocalSesID = uiSesID;

    ses_InitKA(pstCtrl, pstNode);

    return pstNode;
}

static BS_STATUS ses_SendSynAck
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode
)
{
    MBUF_S *pstMbuf;

    pstMbuf = ses_BuildProtocolPkt(pstSesNode, _SES_PKT_FLAG_SYN | _SES_PKT_FLAG_ACK);
    if (NULL == pstMbuf)
    {
        return BS_NO_MEMORY;
    }

    return ses_RecordAndSendPkt(pstCtrl, pstSesNode, pstMbuf);
}

static MBUF_S * ses_BuildRstPkt
(
    IN _SES_PKT_HEAD_S *pstPktInfo
)
{
    _SES_PKT_HEAD_S stPkt = {0};

    stPkt.usVer = htons(_SES_VER);
    stPkt.usPktFlag = htons(_SES_PKT_FLAG_RST);
    stPkt.uiDstSesId = htonl(pstPktInfo->uiSrcSesId);
    stPkt.uiSrcSesId = htonl(pstPktInfo->uiDstSesId);
    stPkt.uiCookie = pstPktInfo->uiCookie;

    return ses_BuildPkt(&stPkt);
}

static BS_STATUS ses_SendRst
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_PKT_INFO_S *pstPktInfo
)
{
    MBUF_S *pstMbuf;
    CHAR szFlagString[_SES_PKT_FLAG_STRING_LEN + 1];

    pstMbuf = ses_BuildRstPkt(&pstPktInfo->stPktHeader);
    if (NULL == pstMbuf)
    {
        return BS_NO_MEMORY;
    }

    BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, SES_DBG_FLAG_PROTOCOL_PKT,
        ("SES Send: Dst:%x,Src:0,Flag:%s,Payload:0\r\n",
         pstPktInfo->stPktHeader.uiSrcSesId,
         ses_GetPktFlagString(_SES_PKT_FLAG_RST, szFlagString)
         ));

    return pstCtrl->pfSendPktFunc(pstMbuf, pstPktInfo->pUserContext);
}

static BS_STATUS ses_SynInput
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_PKT_INFO_S *pstPktInfo
)
{
    _SES_NODE_S *pstSesNode;

    if ((pstPktInfo->stPktHeader.usPktFlag & _SES_PKT_FLAG_SYN) == 0)
    {
        ses_SendRst(pstCtrl, pstPktInfo);
        return BS_NO_SUCH;
    }

    pstSesNode = ses_NewSes(pstCtrl, pstPktInfo->pUserContext);
    if (NULL == pstSesNode)
    {
        ses_SendRst(pstCtrl, pstPktInfo);
        return BS_NO_MEMORY;
    }

    pstSesNode->uiPeerSesID = pstPktInfo->stPktHeader.uiSrcSesId;
    pstSesNode->uiStatus = SES_STATUS_SYN_RCVD;
    pstSesNode->uiCookie = pstCtrl->uiCookie++;

    if (BS_OK != ses_SendSynAck(pstCtrl, pstSesNode))
    {
        ses_FreeSes(pstCtrl, pstSesNode);
        return BS_ERR;
    }

    return BS_OK;
}

static inline VOID ses_RefreshKeepAlive(IN _SES_CTRL_S *pstCtrl, IN _SES_NODE_S *pstSesNode)
{
    KA_Reset(&pstSesNode->stKeepAliveOpt);
}

static VOID ses_ClrSendBufBecauseAck
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode
)
{
    MBUF_Free(pstSesNode->pstSendMbuf);
    pstSesNode->pstSendMbuf = NULL;
    ses_StopResenderTimer(pstCtrl, pstSesNode);
}

static BS_STATUS ses_Ack1Input
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode,
    IN _SES_PKT_INFO_S *pstPktInfo
)
{
    pstSesNode->uiStatus = SES_STATUS_ESTABLISH;
    ses_StartKeepAliveTimer(pstCtrl, pstSesNode);

    ses_ClrSendBufBecauseAck(pstCtrl, pstSesNode);

    ses_EventNotify(pstCtrl, pstSesNode, SES_EVENT_CONNECT);

    return BS_OK;
}

/* 握手过程中的第2个ack */
static BS_STATUS ses_Ack2Input
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode,
    IN _SES_PKT_INFO_S *pstPktInfo
)
{
    pstSesNode->uiStatus = SES_STATUS_ESTABLISH;
    ses_StartKeepAliveTimer(pstCtrl, pstSesNode);

    ses_ClrSendBufBecauseAck(pstCtrl, pstSesNode);

    ses_EventNotify(pstCtrl, pstSesNode, SES_EVENT_ACCEPT);

    return BS_OK;
}

/* 其他ACK */
static BS_STATUS sesAckInput
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode,
    IN _SES_PKT_INFO_S *pstPktInfo
)
{
    return BS_OK;
}

static BS_STATUS ses_SendAck
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode
)
{
    MBUF_S *pstMbuf;

    pstMbuf = ses_BuildProtocolPkt(pstSesNode, _SES_PKT_FLAG_ACK);
    if (NULL == pstMbuf)
    {
        return BS_NO_MEMORY;
    }

    return ses_SendPkt(pstCtrl, pstSesNode->pUserContext, pstMbuf);
}

static BS_STATUS ses_KeepAliveInput
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode,
    IN _SES_PKT_INFO_S *pstPktInfo
)
{
    return ses_SendAck(pstCtrl, pstSesNode);
}

static BS_STATUS ses_RstInput
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode,
    IN _SES_PKT_INFO_S *pstPktInfo
)
{
    if (pstSesNode->uiStatus != SES_STATUS_CLOSED)
    {
        ses_Abort(pstCtrl, pstSesNode, SES_EVENT_PEER_CLOSED);
    }

    return BS_OK;
}

static BS_STATUS ses_ProtocolPktIn
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_PKT_INFO_S *pstPktInfo
)
{
    _SES_NODE_S *pstSesNode;

    if (pstPktInfo->stPktHeader.uiDstSesId == 0)
    {
        return ses_SynInput(pstCtrl, pstPktInfo);
    }

    pstSesNode = ses_FindSesNode(pstCtrl, pstPktInfo);
    if (NULL == pstSesNode)
    {
        if ((pstPktInfo->stPktHeader.usPktFlag & _SES_PKT_FLAG_RST) == 0)
        {
            ses_SendRst(pstCtrl, pstPktInfo);
        }
        return BS_NO_SUCH;
    }

    ses_RefreshKeepAlive(pstCtrl, pstSesNode);

    if (pstPktInfo->stPktHeader.usPktFlag & _SES_PKT_FLAG_SYN)
    {
        if (pstSesNode->uiStatus != SES_STATUS_SYN_SEND)
        {
            ses_SendRst(pstCtrl, pstPktInfo);
            return BS_OK;
        }
        
        pstSesNode->uiPeerSesID = pstPktInfo->stPktHeader.uiSrcSesId;
        pstSesNode->uiCookie = pstPktInfo->stPktHeader.uiCookie;
        (VOID) ses_SendAck(pstCtrl, pstSesNode);
    }

    if (pstPktInfo->stPktHeader.usPktFlag & _SES_PKT_FLAG_ACK)
    {
        if (pstSesNode->uiStatus == SES_STATUS_SYN_SEND)
        {
            ses_Ack1Input(pstCtrl, pstSesNode, pstPktInfo);
        }
        else if (pstSesNode->uiStatus == SES_STATUS_SYN_RCVD)
        {
            ses_Ack2Input(pstCtrl, pstSesNode, pstPktInfo);
        }
        else
        {
            sesAckInput(pstCtrl, pstSesNode, pstPktInfo);
        }
    }

    if (pstPktInfo->stPktHeader.usPktFlag & _SES_PKT_FLAG_KEEPALIVE)
    {
        ses_KeepAliveInput(pstCtrl, pstSesNode, pstPktInfo);
    }

    if (pstPktInfo->stPktHeader.usPktFlag & _SES_PKT_FLAG_RST)
    {
        ses_RstInput(pstCtrl, pstSesNode, pstPktInfo);
    }

    return BS_OK;
}

static VOID ses_CloseNotify(IN _SES_CTRL_S *pstCtrl, IN _SES_NODE_S *pstSesNode)
{
    _SES_CLOSE_NOTIFY_S *pstNotifyNode;

    DLL_SCAN(&pstCtrl->stCloseNotifyList, pstNotifyNode)
    {
        pstNotifyNode->pfNotifyFunc(pstSesNode->uiLocalSesID, pstSesNode->phPropertys, &pstNotifyNode->stUserHandle);
    }
}

static VOID ses_FreeNotifyList(IN VOID *pNotifyNode)
{
    MEM_Free(pNotifyNode);
}

SES_HANDLE SES_CreateInstance
(
    IN UINT uiMaxSesNum,
    IN UINT uiUserContextSize,
    IN UINT uiMaxProperty,
    IN PF_SES_RECV_PKT pfRecvPktFunc,
    IN PF_SES_SEND_PKT pfSendPktFunc,
    IN PF_SES_DFT_EVENT_NOTIFY pfEventNotify
)
{
    _SES_CTRL_S *pstCtrl;
    HANDLE hNap;
    VCLOCK_INSTANCE_HANDLE hVClockInstance;

    pstCtrl = MEM_ZMalloc(sizeof(_SES_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    DLL_INIT(&pstCtrl->stCloseNotifyList);

    pstCtrl->stDftKeepAlive.usIdle = _SES_DFT_KEEPALIVE_IDLE;
    pstCtrl->stDftKeepAlive.usIntval = _SES_DFT_KEEPALIVE_INTVAL;
    pstCtrl->stDftKeepAlive.usMaxProbeCount = _SES_DFT_KEEPALIVE_MAX_TRYS;
    pstCtrl->uiMaxSesNum = uiMaxSesNum;
    pstCtrl->uiMaxProperty = uiMaxProperty;
    pstCtrl->uiUserContextSize = uiUserContextSize;
    pstCtrl->pfRecvPktFunc = pfRecvPktFunc;
    pstCtrl->pfSendPktFunc = pfSendPktFunc;
    pstCtrl->pfEventNotify = pfEventNotify;

    NAP_PARAM_S param = {0};
    param.enType = NAP_TYPE_HASH;
    param.uiMaxNum = uiMaxSesNum;
    param.uiNodeSize = sizeof(_SES_NODE_S);
    hNap = NAP_Create(&param);
    if (NULL == hNap)
    {
        SES_DestroyInstance(pstCtrl);
        return NULL;
    }

    NAP_EnableSeq(hNap, 0, uiMaxSesNum);

    hVClockInstance = VCLOCK_CreateInstance(FALSE);
    if (NULL == hVClockInstance)
    {
        SES_DestroyInstance(pstCtrl);
        return NULL;
    }
    
    pstCtrl->hNap = hNap;
    pstCtrl->hVClockInstance = hVClockInstance;

    return pstCtrl;
}

BS_STATUS SES_SetDftKeepAlive(IN SES_HANDLE hSesHandle, IN SES_OPT_KEEP_ALIVE_TIME_S *pstKeepAlive)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;

    if ((pstCtrl == NULL) || (pstKeepAlive == NULL))
    {
        return BS_NULL_PARA;
    }

    pstCtrl->stDftKeepAlive = *pstKeepAlive;

    return BS_OK;
}

VOID SES_DestroyInstance(IN SES_HANDLE hSesHandle)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;

    if (NULL != pstCtrl->hNap)
    {
        NAP_Destory(pstCtrl->hNap);
    }

    if (NULL != pstCtrl->hVClockInstance)
    {
        VCLOCK_DeleteInstance(pstCtrl->hVClockInstance);
    }

    DLL_FREE(&pstCtrl->stCloseNotifyList, ses_FreeNotifyList);

    MEM_Free(pstCtrl);
}

BS_STATUS SES_RegCloseNotifyEvent
(
    IN SES_HANDLE hSesHandle,
    IN PF_SES_CLOSE_NOTIFY_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;
    _SES_CLOSE_NOTIFY_S *pstNotify;

    pstNotify = MEM_ZMalloc(sizeof(_SES_CLOSE_NOTIFY_S));
    if (NULL == pstNotify)
    {
        return BS_ERR;
    }

    pstNotify->pfNotifyFunc = pfFunc;
    if (NULL != pstUserHandle)
    {
        pstNotify->stUserHandle = *pstUserHandle;
    }

    DLL_ADD(&pstCtrl->stCloseNotifyList, pstNotify);

    return BS_OK;
}

/* 创建Client Session, 返回SesID */
UINT SES_CreateClient(IN SES_HANDLE hSesHandle, IN VOID *pContext)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;
    _SES_NODE_S *pstSesNode;

    pstSesNode = ses_NewSes(pstCtrl, pContext);
    if (NULL == pstSesNode)
    {
        return SES_INVALID_ID;
    }

    pstSesNode->uiStatus = SES_STATUS_INIT;
    pstSesNode->uiFlag |= _SES_NODE_FLAG_IS_CONNECTER;

    return pstSesNode->uiLocalSesID;
}

SES_TYPE_E SES_GetType(IN SES_HANDLE hSesHandle, IN UINT uiSesID)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;
    _SES_NODE_S *pstNode;

    pstNode = NAP_GetNodeByID(pstCtrl->hNap, uiSesID);
    if (NULL == pstNode)
    {
        return SES_TYPE_MAX;
    }

    if (pstNode->uiFlag & _SES_NODE_FLAG_IS_CONNECTER)
    {
        return SES_TYPE_CONNECTER;
    }

    return SES_TYPE_APPECTER;
}

/* 设置这个SESID的Event事件通知函数. */
BS_STATUS SES_SetEventNotify
(
    IN SES_HANDLE hSesHandle,
    IN UINT uiSesID,
    IN PF_SES_EVENT_NOTIFY pfEventNotify,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;
    _SES_NODE_S *pstNode;

    pstNode = NAP_GetNodeByID(pstCtrl->hNap, uiSesID);
    if (NULL == pstNode)
    {
        return BS_NO_SUCH;
    }

    pstNode->pfEventNotify = pfEventNotify;
    if (NULL != pstUserHandle)
    {
        pstNode->stEventNotifyUserHandle = *pstUserHandle;
    }

    return BS_OK;
}

VOID SES_Close(IN SES_HANDLE hSesHandle, IN UINT uiSesID)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;
    _SES_NODE_S *pstNode;

    pstNode = NAP_GetNodeByID(pstCtrl->hNap, uiSesID);
    if (NULL != pstNode)
    {
        ses_CloseNotify(pstCtrl, pstNode);
        ses_FreeSes(pstCtrl, pstNode);
    }
}

BS_STATUS SES_Connect(IN SES_HANDLE hSesHandle, IN UINT uiSesID)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;
    _SES_NODE_S *pstNode;
    BS_STATUS eRet;

    pstNode = NAP_GetNodeByID(pstCtrl->hNap, uiSesID);
    if (NULL == pstNode)
    {
        return BS_NOT_FOUND;
    }

    eRet = ses_SendSyn(pstCtrl, pstNode);

    return eRet;
}

BS_STATUS SES_PktInput(IN SES_HANDLE hSesHandle, IN MBUF_S *pstMbuf, IN VOID *pUserContext)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;
    _SES_PKT_HEAD_S *pstHead;
    UINT uiDstSesID;
    BS_STATUS eRet;
    _SES_NODE_S *pstSesNode;
    _SES_PKT_INFO_S stPktInfo;
    CHAR szPktFlagString[_SES_PKT_FLAG_STRING_LEN + 1];

    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(_SES_PKT_HEAD_S)))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    pstHead = MBUF_MTOD(pstMbuf);
    uiDstSesID = ntohl(pstHead->uiDstSesId);

    memset(&stPktInfo, 0, sizeof(stPktInfo));
    stPktInfo.stPktHeader.usVer = ntohs(pstHead->usVer);
    stPktInfo.stPktHeader.usPktFlag = ntohs(pstHead->usPktFlag);
    stPktInfo.stPktHeader.uiDstSesId = ntohl(pstHead->uiDstSesId);
    stPktInfo.stPktHeader.uiSrcSesId = ntohl(pstHead->uiSrcSesId);
    stPktInfo.stPktHeader.uiCookie = pstHead->uiCookie;
    stPktInfo.pUserContext = pUserContext;

    if (stPktInfo.stPktHeader.usPktFlag & _SES_PKT_FLAG_DATA)
    {
        BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, SES_DBG_FLAG_DATA_PKT,
            ("SES Recv: Dst:%x,Src:%x,Flag:%s,Payload:%d\r\n",
             stPktInfo.stPktHeader.uiDstSesId,
             stPktInfo.stPktHeader.uiSrcSesId,
             ses_GetPktFlagString(stPktInfo.stPktHeader.usPktFlag, szPktFlagString),
             ntohl(pstHead->uiPayloadLen)));

        pstSesNode = ses_FindSesNode(pstCtrl, &stPktInfo);
        if (NULL == pstSesNode)
        {
            ses_SendRst(pstCtrl, &stPktInfo);
            MBUF_Free(pstMbuf);
            return BS_NOT_FOUND;
        }

        ses_RefreshKeepAlive(pstCtrl, pstSesNode);

        MBUF_CutHead(pstMbuf, sizeof(_SES_PKT_HEAD_S));

        eRet = pstCtrl->pfRecvPktFunc(uiDstSesID, pstMbuf);
    }
    else
    {
        BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, SES_DBG_FLAG_PROTOCOL_PKT,
            ("SES Recv: Dst:%x,Src:%x,Flag:%s,Payload:%d\r\n",
             stPktInfo.stPktHeader.uiDstSesId,
             stPktInfo.stPktHeader.uiSrcSesId,
             ses_GetPktFlagString(stPktInfo.stPktHeader.usPktFlag, szPktFlagString),
             ntohl(pstHead->uiPayloadLen)));

        eRet = ses_ProtocolPktIn(pstCtrl, &stPktInfo);
        MBUF_Free(pstMbuf);
    }

    return eRet;
}

BS_STATUS SES_SendPkt(IN SES_HANDLE hSesHandle, IN UINT uiSesID, IN MBUF_S *pstMbuf)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;
    _SES_NODE_S *pstSesNode;
    UINT uiDataLen;
    _SES_PKT_HEAD_S *pstHeader;
    BS_STATUS eRet;

    pstSesNode = NAP_GetNodeByID(pstCtrl->hNap, uiSesID);
    if (NULL == pstSesNode)
    {
        MBUF_Free(pstMbuf);
        return BS_NOT_FOUND;
    }

    if (pstSesNode->uiStatus != SES_STATUS_ESTABLISH)
    {
        MBUF_Free(pstMbuf);
        return BS_NOT_READY;
    }

    uiDataLen = MBUF_TOTAL_DATA_LEN(pstMbuf);

    if (BS_OK != MBUF_Prepend(pstMbuf, sizeof(_SES_PKT_HEAD_S)))
    {
        MBUF_Free(pstMbuf);
        return BS_NO_MEMORY;
    }

    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(_SES_PKT_HEAD_S)))
    {
        MBUF_Free(pstMbuf);
        return BS_NO_MEMORY;
    }

    pstHeader = MBUF_MTOD(pstMbuf);

    ses_FillPktHeader(pstSesNode, _SES_PKT_FLAG_DATA, uiDataLen, pstHeader);

    eRet = ses_SendPkt(pstCtrl, pstSesNode->pUserContext, pstMbuf);

    return eRet;
}

VOID * SES_GetUsrContext(IN SES_HANDLE hSesHandle, IN UINT uiSesID)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;
    _SES_NODE_S *pstSesNode;

    pstSesNode = NAP_GetNodeByID(pstCtrl->hNap, uiSesID);
    if (NULL == pstSesNode)
    {
        return NULL;
    }

    return pstSesNode->pUserContext;
}

VOID SES_TimerStep(IN SES_HANDLE hSesHandle)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;

    VCLOCK_Step(pstCtrl->hVClockInstance);
}

UINT SES_GetStatus(IN SES_HANDLE hSesHandle, IN UINT uiSesID)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;

    _SES_NODE_S *pstSesNode;

    pstSesNode = NAP_GetNodeByID(pstCtrl->hNap, uiSesID);
    if (NULL == pstSesNode)
    {
        return 0;
    }

    return pstSesNode->uiStatus;
}

UINT SES_GetPeerSESID(IN SES_HANDLE hSesHandle, IN UINT uiSesID)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;

    _SES_NODE_S *pstSesNode;

    pstSesNode = NAP_GetNodeByID(pstCtrl->hNap, uiSesID);
    if (NULL == pstSesNode)
    {
        return 0;
    }

    return pstSesNode->uiPeerSesID;
}

BS_STATUS SES_SetProperty(IN SES_HANDLE hSesHandle, IN UINT uiSesID, IN UINT uiPropertyIndex, IN HANDLE hValue)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;
    _SES_NODE_S *pstSesNode;

    if (uiPropertyIndex >= pstCtrl->uiMaxProperty)
    {
        return BS_OUT_OF_RANGE;
    }

    pstSesNode = NAP_GetNodeByID(pstCtrl->hNap, uiSesID);
    if (NULL == pstSesNode)
    {
        return BS_NO_SUCH;
    }

    pstSesNode->phPropertys[uiPropertyIndex] = hValue;

    return BS_OK;
}

HANDLE SES_GetProperty(IN SES_HANDLE hSesHandle, IN UINT uiSesID, IN UINT uiPropertyIndex)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;
    _SES_NODE_S *pstSesNode;

    if (uiPropertyIndex >= pstCtrl->uiMaxProperty)
    {
        return NULL;
    }

    pstSesNode = NAP_GetNodeByID(pstCtrl->hNap, uiSesID);
    if (NULL == pstSesNode)
    {
        return NULL;
    }

    return pstSesNode->phPropertys[uiPropertyIndex];
}

HANDLE * SES_GetAllProperty(IN SES_HANDLE hSesHandle, IN UINT uiSesID)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;
    _SES_NODE_S *pstSesNode;

    pstSesNode = NAP_GetNodeByID(pstCtrl->hNap, uiSesID);
    if (NULL == pstSesNode)
    {
        return NULL;
    }

    return pstSesNode->phPropertys;
}

static VOID ses_ResetKeepAliveTime
(
    IN _SES_CTRL_S *pstCtrl,
    IN _SES_NODE_S *pstSesNode,
    IN SES_OPT_KEEP_ALIVE_TIME_S *pstKeepAlive
)
{
    pstSesNode->stKeepAliveOpt.stSet.usIdle = pstKeepAlive->usIdle;
    pstSesNode->stKeepAliveOpt.stSet.usIntval = pstKeepAlive->usIntval;
    pstSesNode->stKeepAliveOpt.stSet.usMaxProbeCount = pstKeepAlive->usMaxProbeCount;

    KA_Reset(&pstSesNode->stKeepAliveOpt);
}

BS_STATUS SES_SetOpt(IN SES_HANDLE hSesHandle, IN UINT uiSesID, IN UINT uiOpt, IN VOID *pValue)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;
    _SES_NODE_S *pstSesNode;

    pstSesNode = NAP_GetNodeByID(pstCtrl->hNap, uiSesID);
    if (NULL == pstSesNode)
    {
        return BS_NO_SUCH;
    }

    switch (uiOpt)
    {
        case SES_OPT_KEEP_ALIVE_TIME:
        {
            ses_ResetKeepAliveTime(pstCtrl, pstSesNode, pValue);
            break;
        }

        default:
        {
            BS_DBGASSERT(0);
            break;
        }
    }

    return BS_OK;
}

VOID SES_Walk(IN SES_HANDLE hSesHandle, IN PF_SES_WALK_FUNC pfFunc, IN HANDLE hUserHandle)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;
    UINT uiIndex = 0;

    while ((uiIndex = NAP_GetNextIndex(pstCtrl->hNap, uiIndex)) != 0)
    {
        if (BS_WALK_STOP == pfFunc((UINT)NAP_GetIDByNode(pstCtrl->hNap, NAP_GetNodeByIndex(pstCtrl->hNap, uiIndex)), hUserHandle))
        {
            break;
        }
    }
}

CHAR * SES_GetStatusString(IN UINT uiStatus)
{
    static CHAR *apcStatusString[] = 
    {
        "init", "syn_send", "syn_rcvd", "established", "closed"
    };

    if (uiStatus >= SES_STATUS_MAX)
    {
        return "";
    }

    return apcStatusString[uiStatus];
}

VOID SES_AddDbgFlag(IN SES_HANDLE hSesHandle, IN UINT uiDbgFlag)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;

    pstCtrl->uiDbgFlag |= uiDbgFlag;
}

VOID SES_ClrDbgFlag(IN SES_HANDLE hSesHandle, IN UINT uiDbgFlag)
{
    _SES_CTRL_S *pstCtrl = hSesHandle;

    pstCtrl->uiDbgFlag &= (~uiDbgFlag);
}

