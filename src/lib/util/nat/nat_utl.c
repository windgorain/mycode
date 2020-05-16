
#include "bs.h"

#include "utl/bit_opt.h"
#include "utl/ip_utl.h"
#include "utl/tcp_utl.h"
#include "utl/udp_utl.h"
#include "utl/hash_utl.h"
#include "utl/mutex_utl.h"
#include "utl/nat_utl.h"
#include "utl/vclock_utl.h"
#include "utl/in_checksum.h"

#include "nat_inner.h"



typedef struct
{
    UINT uiDbgFlag;
    _NAT_TCP_CTRL_S stTcpCtrl;
    _NAT_UDP_CTRL_S stUdpCtrl;
}NAT_CTRL_S;

NAT_HANDLE NAT_Create
(
    IN USHORT usMinPort,   /* 主机序 ,对外可转换的端口号最小值 */
    IN USHORT usMaxPort,    /* 主机序 ,对外可转换的端口号最大值 */
    IN UINT   uiMsInTick,  /* 多少ms为一个Tick */
    IN BOOL_T bCreateMutex
)
{
    NAT_CTRL_S *pstCtrl;

    if (usMinPort > usMaxPort)
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    pstCtrl = MEM_ZMalloc(sizeof(NAT_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    if (BS_OK != _NAT_TCP_Init(&pstCtrl->stTcpCtrl, usMinPort, usMaxPort, uiMsInTick, bCreateMutex))
    {
        MEM_Free(pstCtrl);
        return NULL;
    }

    if (BS_OK != _NAT_UDP_Init(&pstCtrl->stUdpCtrl, usMinPort, usMaxPort, uiMsInTick, bCreateMutex))
    {
        _NAT_TCP_Fini(&pstCtrl->stTcpCtrl);
        MEM_Free(pstCtrl);
        return NULL;
    }

    return pstCtrl;
}

VOID NAT_Destory(IN NAT_HANDLE hNatHandle)
{
    NAT_CTRL_S *pstCtrl = hNatHandle;

    if (NULL != pstCtrl)
    {
        _NAT_TCP_Fini(&pstCtrl->stTcpCtrl);
        _NAT_UDP_Fini(&pstCtrl->stUdpCtrl);
        MEM_Free(pstCtrl);
    }
}

VOID NAT_SetPubIp
(
    IN NAT_HANDLE hNatHandle,
    IN UINT auiPubIp[NAT_MAX_PUB_IP_NUM] /* 网络序，提供的对外公网IP */
)
{
    NAT_CTRL_S *pstCtrl = hNatHandle;

    _NAT_TCP_SetPubIp(&pstCtrl->stTcpCtrl, auiPubIp);
    _NAT_UDP_SetPubIp(&pstCtrl->stUdpCtrl, auiPubIp);
}

BS_STATUS NAT_SetTcpTimeOutTick
(
    IN NAT_HANDLE hNatHandle,
    IN UINT uiTick
)
{
    NAT_CTRL_S *pstCtrl = hNatHandle;

    if (NULL == hNatHandle)
    {
        BS_DBGASSERT(0);
        return BS_BAD_PARA;
    }

    pstCtrl->stTcpCtrl.uiTcpTimeOutTick = uiTick;

    return BS_OK;
}

BS_STATUS NAT_SetUdpTimeOutTick
(
    IN NAT_HANDLE hNatHandle,
    IN UINT uiTick
)
{
    NAT_CTRL_S *pstCtrl = hNatHandle;

    if (NULL == hNatHandle)
    {
        BS_DBGASSERT(0);
        return BS_BAD_PARA;
    }

    pstCtrl->stUdpCtrl.uiUdpTimeOutTick = uiTick;

    return BS_OK;
}

BS_STATUS NAT_PacketTranslate
(
    IN NAT_HANDLE hNatHandle,
    INOUT UCHAR *pucData, /* IP报文 */
    IN UINT uiDataLen,
    IN BOOL_T bFromPub,
    INOUT UINT *puiDomainId   /* 私有报文为IN, Pub报文为OUT */
)
{
	IP_HEAD_S  *pstIpHead  = NULL;
    BS_STATUS   eRet = BS_OK;
    NAT_CTRL_S *pstCtrl = hNatHandle;

    if (NULL == hNatHandle)
    {
        BS_DBGASSERT(0);
        return BS_BAD_PARA;
    }

    if (NULL == pucData)
    {
        BS_DBGASSERT(0);
        return BS_BAD_PARA;
    }

    pstIpHead = IP_GetIPHeader(pucData, uiDataLen, NET_PKT_TYPE_IP);
    if (NULL == pstIpHead)
    {
        return BS_ERR;
    }

    BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, NAT_DBG_PACKET,
        ("NAT: Recv packet from %pI4 to %pI4.\r\n",
        &pstIpHead->unSrcIp.uiIp, &pstIpHead->unDstIp.uiIp));

    switch (pstIpHead->ucProto)
    {
        case IP_PROTO_TCP:
        {
            eRet = _NAT_TCP_PktIn(&pstCtrl->stTcpCtrl, pstIpHead, pucData, uiDataLen, bFromPub, puiDomainId);
            break;
        }

        case IP_PROTO_UDP:
        {
            eRet = _NAT_UDP_PktIn(&pstCtrl->stUdpCtrl, pstIpHead, pucData, uiDataLen, bFromPub, puiDomainId);
            break;
        }

        default:
        {
            eRet = BS_NOT_SUPPORT;
            break;
        }
    }

    if (BS_OK == eRet)
    {
        pstIpHead->usCrc = 0;
        pstIpHead->usCrc = IP_CheckSum((UCHAR*)pstIpHead, IP_HEAD_LEN(pstIpHead));

        BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, NAT_DBG_PACKET,
            ("NAT: Tranlate packet: From %pI4 to %pI4.\r\n",
            &pstIpHead->unSrcIp.uiIp, &pstIpHead->unDstIp.uiIp));
    }

	return eRet;
}

/* 调用者负责释放Mbuf，此函数不释放 */
BS_STATUS NAT_PacketTranslateByMbuf
(
    IN NAT_HANDLE hNatHandle,
    INOUT MBUF_S *pstMbuf, /* IP报文 */
    IN BOOL_T bFromPub,
    INOUT UINT *puiDomainId   /* 私有报文为IN, Pub报文为OUT */
)
{
    UCHAR *pucData;
    UINT uiDataLen;

    if (NULL == hNatHandle)
    {
        BS_DBGASSERT(0);
        return BS_BAD_PARA;
    }

    if (NULL == pstMbuf)
    {
        BS_DBGASSERT(0);
        return BS_BAD_PARA;
    }

    uiDataLen = MBUF_TOTAL_DATA_LEN(pstMbuf);
    if (uiDataLen <= 0)
    {
        return BS_TOO_SMALL;
    }

    if (BS_OK != MBUF_MakeContinue(pstMbuf, uiDataLen))
    {
        return BS_TOO_LONG;
    }

    pucData = MBUF_MTOD(pstMbuf);

    return NAT_PacketTranslate(hNatHandle, pucData, uiDataLen, bFromPub, puiDomainId);
}

VOID NAT_TimerStep(IN NAT_HANDLE hNatHandle)
{
    NAT_CTRL_S *pstCtrl = hNatHandle;

    if (NULL == hNatHandle)
    {
        return;
    }

    _NAT_TCP_TimerStep(&pstCtrl->stTcpCtrl);
    _NAT_UDP_TimerStep(&pstCtrl->stUdpCtrl);
}

VOID NAT_Walk
(
    IN NAT_HANDLE hNatHandle,
    IN PF_NAT_WALK_CALL_BACK_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    NAT_CTRL_S *pstCtrl = hNatHandle;

    if (NULL == hNatHandle)
    {
        return;
    }

    _NAT_TCP_Walk(&pstCtrl->stTcpCtrl, pfFunc, hUserHandle);
    _NAT_UDP_Walk(&pstCtrl->stUdpCtrl, pfFunc, hUserHandle);
}

VOID NAT_SetDbgFlag(IN NAT_HANDLE hNatHandle, IN UINT uiDbgFlag)
{
    NAT_CTRL_S *pstCtrl = hNatHandle;

    if (NULL == hNatHandle)
    {
        return;
    }

    pstCtrl->uiDbgFlag |= uiDbgFlag;
}

VOID NAT_ClrDbgFlag(IN NAT_HANDLE hNatHandle, IN UINT uiDbgFlag)
{
    NAT_CTRL_S *pstCtrl = hNatHandle;

    if (NULL == hNatHandle)
    {
        return;
    }

    BIT_CLR(pstCtrl->uiDbgFlag, uiDbgFlag);
}

CHAR * NAT_GetStatusString(IN UCHAR ucType, IN UINT uiStatus)
{
    static CHAR *apcTcpStatus[NAT_TCP_STATUS_MAX] =
    {
        "syn_send",
        "syn_receved",
        "established",
        
        "inside_fin_wait_1",
        "inside_fin_wait_2",
        "inside_time_wait",
        
        "outside_fin_wait_1",
        "outside_fin_wait_2",
        "outside_time_wait",

        "time_out",
        "closed"        
    };
    static CHAR *apcUdpStatus[NAT_UDP_STATUS_MAX] =
    {
        "syn_send",
        "syn_receved",
        "established",
    };

    if (ucType == IP_PROTO_TCP)
    {
        if (uiStatus >= NAT_TCP_STATUS_MAX)
        {
            BS_DBGASSERT(0);
            return "Error";
        }

        return apcTcpStatus[uiStatus];
    }

    if (ucType == IP_PROTO_UDP)
    {
        if (uiStatus >= NAT_UDP_STATUS_MAX)
        {
            BS_DBGASSERT(0);
            return "Error";
        }
        
        return apcUdpStatus[uiStatus];
    }

    BS_DBGASSERT(0);
    return "Error";
}


