/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-9-7
* Description: 
* History:     
******************************************************************************/

#ifndef __MBUF_UTL_H_
#define __MBUF_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#include "utl/net.h"
#include "utl/event_utl.h"

#define MBUF_DFT_RESERVED_HEAD_SPACE 200

#define _MBUF_POOL_DFT_CLUSTER_SIZE 2048

#define MBUF_DATA_CONTROL                     0x01
#define MBUF_DATA_DATA                        0x02
#define MBUF_DATA_NAME                        0x03
#define MBUF_DATA_OOBDATA                     0x04


#define MBUF_POOL_NAME_MAX_LEN 32

#define MBUF_GET_DATABLOK_SIZE_BY_MBUF(pstMbuf)  (_MBUF_POOL_DFT_CLUSTER_SIZE)
#define MBUF_TOTAL_DATA_LEN(pstMbuf)             ((pstMbuf)->ulTotalDataLen)
#define MBUF_DATABLOCK_NUM(pstMbuf)              (DLL_COUNT(&(pstMbuf)->stMblkHead))
#define MBUF_GET_FIRST_DATABLOCK(pstMbuf)        ((NULL == (pstMbuf)) ? NULL : DLL_FIRST((pstMbuf)->stMblkHead))
#define MBUF_GET_LAST_DATABLOCK(pstMbuf)         ((NULL == (pstMbuf)) ? NULL : DLL_LAST((pstMbuf)->stMblkHead))
#define MBUF_GET_NEXT_DATABLOCK(pstMbuf,pstMblk) (DLL_NEXT(&(pstMbuf)->stMblkHead,pstMblk))
#define MBUF_CLUSTER_SIZE(pstCluster)            ((pstCluster)->ulSize)
#define MBUF_CLUSTER_REF(pstCluster)             ((pstCluster)->ulRefCnt)
#define MBUF_DATABLOCK_LEN(pstMblk)              ((pstMblk)->ulLen)
#define MBUF_DATABLOCK_HEAD_FREE_LEN(pstMblk)    ((UINT)((pstMblk)->pucData - (pstMblk)->pstCluster->pucData))
#define MBUF_DATABLOCK_TAIL_FREE_LEN(pstMblk)    ((pstMblk)->pstCluster->ulSize - MBUF_DATABLOCK_HEAD_FREE_LEN (pstMblk) - (pstMblk)->ulLen)
#define MBUF_GET_NEXT_MBUF(pstMbuf)              ((MBUF_S*)((pstMbuf)->pstNextMbuf))
#define MBUF_SET_NEXT_MBUF(pstMbuf,_pstNextMbuf)  ((pstMbuf)->pstNextMbuf = (VOID*)(_pstNextMbuf))
#define MBUF_GET_MBUF_TYPE(pstMbuf)              ((pstMbuf)->ucType)

#define MBUF_SCAN_DATABLOCK_BEGIN(_pstMbuf,_pucData,_ulDataLen)  \
    do {    \
        MBUF_MBLK_S *_pstMblk;  \
        DLL_SCAN (&_pstMbuf->stMblkHead, _pstMblk)   \
        {   \
            _pucData = _pstMblk->pucData;    \
            _ulDataLen = _pstMblk->ulLen;   \
            {

#define MBUF_SCAN_END()  \
    }}}while (0)


#define MBUF_CAT_EXT(pstDstMbuf,pstSrcMbuf)  \
    do{ \
        if ((pstDstMbuf) == NULL) \
        {   \
            (pstDstMbuf) = (pstSrcMbuf);    \
        }   \
        else    \
        {   \
            MBUF_NeatCat(pstDstMbuf, pstSrcMbuf);	\
        }   \
    }while(0);

typedef struct structMBUF_CLUSTER_S
{
    UINT           ulSize; 
    UINT           ulRefCnt;
    UCHAR           *pucData;
}MBUF_CLUSTER_S;

typedef struct structMBUF_MBLK_S
{
    DLL_NODE(structMBUF_MBLK_S) stMbufBlkLinkNode;  /* 必须定义在最前面 */
    UCHAR            *pucData;           /* location of data */
    UINT            ulLen;      
    MBUF_CLUSTER_S   *pstCluster;
}MBUF_MBLK_S;

#if 1

typedef VOID (*PF_MBUF_USER_CONTEXT_FREE)(IN HANDLE hUserContext);
typedef BS_STATUS (*PF_MBUF_USER_CONTEXT_DUP)(IN HANDLE hUserContext, OUT HANDLE *phUserContext);

typedef struct
{
    VOID *pUserContextData;
    PF_MBUF_USER_CONTEXT_FREE pfFreeFunc;
    PF_MBUF_USER_CONTEXT_DUP pfDupFunc;     /* 当Mbuf Copy的时候,会调用这个函数,进行pUserContextData的复制 */
}MBUF_USER_CONTEXT_S;

#define MBUF_GET_USER_CONTEXT_DATA(pstMbuf) ((pstMbuf)->stUserContext.pUserContextData)
#define MBUF_GET_USER_CONTEXT(pstMbuf) (&((pstMbuf)->stUserContext))


#endif

#if 1

#define MBUF_GET_RECV_IF_INDEX(pstMbuf) ((pstMbuf)->stTag.stIfInfo.uiRecvIfIndex)
#define MBUF_SET_RECV_IF_INDEX(pstMbuf, uiIfIndex) ((pstMbuf)->stTag.stIfInfo.uiRecvIfIndex = (uiIfIndex))
#define MBUF_GET_SEND_IF_INDEX(pstMbuf) ((pstMbuf)->stTag.stIfInfo.uiSendIfIndex)
#define MBUF_SET_SEND_IF_INDEX(pstMbuf, uiIfIndex) ((pstMbuf)->stTag.stIfInfo.uiSendIfIndex = (uiIfIndex))
#define MBUF_GET_NEXT_HOP(pstMbuf) ((pstMbuf)->stTag.stIfInfo.uiNextHop)
#define MBUF_SET_NEXT_HOP(pstMbuf, _uiNextHop) ((pstMbuf)->stTag.stIfInfo.uiNextHop = (_uiNextHop))

typedef struct
{
    UINT uiRecvIfIndex;
    UINT uiSendIfIndex;
    UINT uiNextHop;         /* 网络序 */
}MBUF_IF_INFO_S;

#endif

#if 1 /* ETH链路层信息 */

/* ETH MarkFlag设置和获取 */

/* 报文的MARK标记选项,链路层信息中的FLAG */
#define MBUF_L2_FLAG_SRC_MAC                  0x01      /* 指定报文的源MAC, 用于发送报文 */
#define MBUF_L2_FLAG_DST_MAC                  0x02      /* 指定报文的目的MAC, 用于发送报文 */
#define MBUF_L2_FLAG_ENCAPED                  0x04      /* 表示报文已经经过二层报文头封装，直接发送即可. 用于发送报文 */
#define MBUF_L2_FLAG_ALL                      0xffff

/* Eth链路信息定义 */
typedef struct tagMBufEthernetHdr
{
    USHORT usL2Type;                 /* 报文的二层协议类型 */
    USHORT usMarkFlag;               /* 报文的MARK标记,见MBUF_FLAG_MARK_XXXX的定义 */
    UCHAR  aucSrcAddr[6];            /* 报文源MAC地址 */
    UCHAR  aucDstAddr[6];            /* 报文目的MAC地址 */
    UCHAR  ucPktFmt;                 /* 以太网帧格式,参见PKTFMT_XXXXX的定义  */
} MBUF_ETHERNET_HDR_S;

/* 链路类型:在stTag.ucLinkType中设置 */
#define MBUF_LINK_TYPE_X25                    1
#define MBUF_LINK_TYPE_ETHERNET               2
#define MBUF_LINK_TYPE_FRAME_RELAY            3
#define MBUF_LINK_TYPE_ATM                    4
#define MBUF_LINK_TYPE_PPP                    5
#define MBUF_LINK_TYPE_DOT11                  6
#define MBUF_LINK_TYPE_CHDLC                  7
#define MBUF_LINK_TYPE_LAPB                   8

/* 与链路层相关的信息 */
typedef struct
{
    UCHAR  ucLinkType;             /* 参见MBUF_LINK_TYPE_XXXXX */
    UCHAR  ucLinkHeadSize;

    union
    {
        MBUF_ETHERNET_HDR_S stEthHdr;
    }unL2Hdr;
}MBUF_LINK_INFO_S;
#endif

#if 1 /* IP层信息 */

/* 在stTag.unL3Hdr.stIpHdr.uiIpPktType中设置 */
#define IP_PKT_HOST                 0x00000001   /* 表示该IP报文的目地地址是本机某个接口的IP地址 */
#define IP_PKT_HOSTSENDPKT          0x00000002   /* 本机发送的IP报文 */
#define IP_PKT_FORWARDPKT           0x00000004   /* 转发的IP报文 */
#define IP_PKT_IIFBRAODCAST         0x00000008   /* 报文输入接口的广播报文 */
#define IP_PKT_OIFBRAODCAST         0x00000010   /* 报文输出接口的广播报文 */
#define IP_PKT_MULTICAST            0x00000020   /* 报文是IP组播报文 */
#define IP_PKT_ETHBCAST             0x00000040   /* 表示该报文是以太网的广播报文 */
#define IP_PKT_ETHMCAST             0x00000080   /* 表示该报文是以太网的组播报文 */
#define IP_PKT_SRCROUTE             0x00000100   /* 表示该报文是源路由处理的报文 */
#define IP_PKT_FILLTAG              0x00000200   /* 填写 MBUF IPv4 控制域信息 */


typedef union tagPacketInfo
{
    struct tagIpTcpUdpPort
    {
        USHORT usSourcePort;
        USHORT usDestinationPort;
    }stTcpUdpPort;

    struct tagIcmpInfo
    {
        UCHAR ucIcmpType;
        UCHAR ucIcmpCode;
        USHORT usIcmpID;
    }stIcmpInfo;
}MBUF_PACKET_INFO_U;

/*  网络层信息定义*/
typedef struct tagMBufIpHdr        
{
    UINT uiSrcIp;              /* 源IP地址 */
    UINT uiDstIp;              /* 目的IP地址 */
    UINT uiNextHop;            /* 下一跳的IP地址,从路由表中获得 */
    UINT uiIpPktType;          /* 报文类型,如该报文是否是广播报文 */
    UINT uiCacheFlag;          /* 快转flag */
    /* IP承载协议的信息,如果是IP包是片段,则下面两个域无意义,如果不是片段,则它们的含义如注释中所说明 */
    MBUF_PACKET_INFO_U unPacketInfo;
    /* IP的信息,从IP包的各个域中获得 */
    UCHAR ucTos;                 /* the type of service domain in ip head, 其中包含precedence */
    UCHAR ucProtocol;            /* the protocol in ip head, TCP UDP ICMP */
    UCHAR ucIsFragment:1;        /* is fragment or not */
    UCHAR ucIsFirstFrag:1;       /* is the first fragment or not */
    UCHAR ucTCPFlags:6;          /* TCP连接建立报文标志 */
} MBUF_IP_HDR_S;
#endif

#if 1   /* VRF信息 */
typedef struct tag_MbufVrf
{
    VRF_INDEX vrfIndexRawIn;
    VRF_INDEX vrfIndexIn;
    VRF_INDEX vrfIndexOut;
}MBUF_VRF_S;

#endif

typedef struct
{
    /* 接口信息 */
    MBUF_IF_INFO_S stIfInfo;

    /* VRF信息 */
    MBUF_VRF_S stVrf;

    /* 链路层信息 */
    union {
        MBUF_LINK_INFO_S stLinkInfo;
    }unL2Hdr;

    /* 与网络层相关的信息 */
    union {
        MBUF_IP_HDR_S stIpHdr;
    }unL3Hdr;
    
}MBUF_TAG_S;

typedef struct structMBUF_S
{
    struct structMbuf_s *pstNextMbuf;
    DLL_HEAD_S          stMblkHead;
    UINT                ulTotalDataLen;
    UCHAR               ucType;

    /* 一些信息 */
    MBUF_TAG_S stTag;

    /*User Info*/
    MBUF_USER_CONTEXT_S stUserContext;    /* Copy时，需要拷贝过去 */
}MBUF_S;

typedef struct tagMBufQueue
{
    MBUF_S * pstHeadMBuf;
    MBUF_S * pstTailMBuf;
    UINT uiCurrentLength;
    UINT uiMaxLength;
}MBUF_QUE_S;

#define MBUF_QUE_INIT(_pstMBufQue, _uiMaxLength)\
{\
    (_pstMBufQue)->pstHeadMBuf = NULL;\
    (_pstMBufQue)->pstTailMBuf = NULL;\
    (_pstMBufQue)->uiCurrentLength = 0;\
    (_pstMBufQue)->uiMaxLength = (_uiMaxLength);\
}

#define MBUF_QUE_IS_EMPTY(_pstMBufQue) ((_pstMBufQue)->uiCurrentLength == 0)
#define MBUF_QUE_IS_FULL(_pstMBufQue) ((_pstMBufQue)->uiCurrentLength >= (_pstMBufQue)->uiMaxLength)
#define MBUF_QUE_GET_COUNT(_pstMBufQue) ((_pstMBufQue)->uiCurrentLength)


#define MBUF_QUE_PUSH(_pstMbufQue, _pstMbuf)    \
    do {    \
        MBUF_SET_NEXT_MBUF(_pstMbuf, NULL);     \
        if ((_pstMbufQue)->pstTailMBuf == NULL)     \
        {   \
            (_pstMbufQue)->pstHeadMBuf = (_pstMbuf);    \
        }   \
        else    \
        {   \
            MBUF_SET_NEXT_MBUF((_pstMbufQue)->pstTailMBuf, _pstMbuf);   \
        }   \
        (_pstMbufQue)->pstTailMBuf = (_pstMbuf);    \
        (_pstMbufQue)->uiCurrentLength++;  \
    }while(0)

#define MBUF_QUE_POP(_pstMBufQue, _pstMBuf)\
    do{ \
        (_pstMBuf) = (_pstMBufQue)->pstHeadMBuf;    \
        if((_pstMBufQue)->pstHeadMBuf != NULL)  \
        {   \
            (_pstMBufQue)->pstHeadMBuf = MBUF_GET_NEXT_MBUF((_pstMBufQue)->pstHeadMBuf);    \
            if((_pstMBufQue)->pstHeadMBuf == NULL)  \
            {   \
                (_pstMBufQue)->pstTailMBuf = NULL;  \
            }   \
            (_pstMBufQue)->uiCurrentLength--;  \
        }   \
    }while(0)

#define MBUF_QUE_PEEK(_pstMBufQue) ((_pstMBufQue)->pstHeadMBuf)

#define MBUF_QUE_FREE_ALL(_pstMBufQue)  \
    for(;;)   \
    {   \
        MBUF_S * _pstMbuf;  \
        MBUF_QUE_POP(_pstMBufQue, _pstMbuf);    \
        if (_pstMbuf == NULL)   \
        {   \
            break;  \
        }   \
        MBUF_Free(_pstMbuf);    \
    }

typedef struct structMBUF_ITOR_S
{
    MBUF_MBLK_S *pstMblk;
    UINT        ulOffset;
}MBUF_ITOR_S;

extern MBUF_CLUSTER_S * MBUF_CreateCluster();
extern VOID MBUF_FreeCluster(IN MBUF_CLUSTER_S *pstCluster);
extern MBUF_S * MBUF_Create (IN UCHAR ucType, IN UINT ulHeadSpaceLen);
extern VOID MBUF_Free (IN MBUF_S *pstMbuf);
/* 将MBUF 压缩到尽量少的Block中 */
extern BS_STATUS _MBUF_Compress (IN MBUF_S *pstMbuf);
extern BS_STATUS MBUF_ApppendBlock (IN MBUF_S *pstMbuf, IN UINT ulLen);
extern BS_STATUS MBUF_PrependBlock (IN MBUF_S *pstMbuf, IN UINT ulLen);
extern BS_STATUS MBUF_CutHead (IN MBUF_S *pstMbuf, IN UINT ulCutLen);
extern BS_STATUS MBUF_CutTail (IN MBUF_S *pstMbuf, IN UINT ulCutLen);
extern BS_STATUS MBUF_CutPart (IN MBUF_S *pstMbuf, IN UINT ulOffset, IN UINT ulCutLen);
extern VOID MBUF_Neat (IN MBUF_S *pstMbuf);
extern BS_STATUS _MBUF_MakeContinue (IN MBUF_S * pstMbuf, IN UINT ulLen);
extern BS_STATUS MBUF_CopyFromMbufToBuf (IN MBUF_S *pstMbuf, IN UINT ulOffset, IN UINT ulLen, OUT void *buf);
extern BS_STATUS MBUF_InsertBuf(IN MBUF_S *pstMbuf, IN UINT ulOffset, IN void *buf, IN UINT ulLen);
extern MBUF_S * MBUF_Fragment (IN MBUF_S *pstMbuf, IN UINT ulLen);
extern MBUF_S * MBUF_CreateByCopyBuf
(
    IN UINT ulReserveHeadSpace,
    IN void *pBuf,
    IN UINT ulLen,
    IN UCHAR ucType
);
extern MBUF_MBLK_S * MBUF_MblkFragment (IN MBUF_MBLK_S *pstMblk, IN UINT ulLen);
/* 尽最大可能根据buf创建一个mblk. 如果空间不够, 则只拷贝mblk支持的最多的数据 */
extern MBUF_MBLK_S * MBUF_CreateMblkByCopyBuf (IN void *buf, IN UINT ulLen, IN UINT ulReservrdHeadSpace);
extern BS_STATUS MBUF_CopyFromBufToMbuf (IN MBUF_S *pstMbuf, IN UINT ulOffset, IN void *buf, IN UINT ulLen);
extern MBUF_MBLK_S * MBUF_FindMblk (IN MBUF_S *pstMbuf, IN UINT ulOffset, OUT UINT *pulOffsetInThisMblk);
extern MBUF_S * MBUF_ReferenceCopy (IN MBUF_S *pstMbuf, IN UINT ulOffset, IN UINT ulLen);
extern MBUF_S * MBUF_RawCopy (IN MBUF_S *pstMbuf, IN UINT ulOffset, IN UINT ulLen, IN UINT ulReservedHeadSpace);
extern MBUF_S * MBUF_CreateByCluster
(
    IN MBUF_CLUSTER_S *pstCluster,
    IN UINT ulReserveHeadSpace,
    IN UINT ulLen,
    IN UCHAR ucType
);
/*如果找到,返回偏移(注意不是从ulStartOffset开始算的偏移),否则,返回负值*/
extern INT MBUF_Find (IN MBUF_S *pstMbuf, IN UINT ulStartOffset, IN UCHAR *pMemToFind, IN UINT ulMemLen);
extern INT MBUF_FindByte(IN MBUF_S *pstMbuf, IN UINT ulStartOffset, IN UCHAR ucByteToFind);
/* 在[ulStartOffset, ulStartOffset + N)之间查找.  如果找到,返回偏移,否则,返回负值.  */
extern INT MBUF_NFind (IN MBUF_S *pstMbuf, IN UINT ulStartOffset, IN UINT ulLen, IN UCHAR *pMemToFind, IN UINT ulMemLen);
extern BS_STATUS MBUF_AddCluster (IN MBUF_S *pstMbufDst, IN MBUF_CLUSTER_S *pstCluster, IN UINT ulOffset, IN UINT ulDataLen);
extern BS_STATUS MBUF_Set(IN MBUF_S *pstMbuf, IN UCHAR ucToSet, IN UINT ulOffset, IN UINT ulSetLen);
extern UCHAR * MBUF_GetContinueMem(IN MBUF_S *pstMbuf, IN UINT ulOffset, IN UINT ulLen, OUT HANDLE *phMemHandle);
extern void* MBUF_GetContinueMemRaw(IN MBUF_S *pstMbuf, IN UINT ulOffset, IN UINT ulLen, OUT HANDLE *phMemHandle);
extern VOID MBUF_FreeContinueMem(IN HANDLE hMemHandle);
extern UCHAR * MBUF_GetChar (IN MBUF_S *pstMbuf, IN UINT ulOffset, OUT MBUF_ITOR_S *pstItor);
extern UCHAR * MBUF_GetNextChar (IN MBUF_S *pstMbuf, INOUT MBUF_ITOR_S *pstItor);
/* 删除index两边的空白字符 */
extern UINT MBUF_DelBlankSideByIndex(IN MBUF_S *pstMbuf, IN UINT ulStartOffset, INOUT UINT *pulOffset);
extern VOID MBUF_PrintAsString(IN MBUF_S *pstMbuf);
extern VOID MBUF_PrintAsHex(IN MBUF_S *pstMbuf);
/* 向MBUF尾部添加空间 */
extern BS_STATUS MBUF_Append(IN MBUF_S *pstMbuf, IN UINT ulLen);
/* 向MBUF 尾部追加数据 */
extern BS_STATUS MBUF_AppendData(IN MBUF_S *pstMbuf, IN UCHAR *pucData, IN UINT uiDataLen);
/* 向MBUF头部添加空间 */
extern BS_STATUS MBUF_Prepend (IN MBUF_S *pstMbuf, IN UINT ulLen);
/* 向MBUF头部添加数据 */
extern BS_STATUS MBUF_PrependData(IN MBUF_S *pstMbuf, IN UCHAR *pucData, IN UINT uiDataLen);

static inline BS_STATUS MBUF_MakeContinue (IN MBUF_S * pstMbuf, IN UINT ulLen)
{
    MBUF_MBLK_S *pstMblk;

    pstMblk = (MBUF_MBLK_S *)DLL_FIRST (&pstMbuf->stMblkHead);

    if ((pstMblk != NULL) && (MBUF_DATABLOCK_LEN (pstMblk) >= ulLen))
    {
        return BS_OK;
    }

    return _MBUF_MakeContinue (pstMbuf, ulLen);
}

/* 将MBUF 压缩到尽量少的Block中 */
static inline BS_STATUS MBUF_Compress (IN MBUF_S * pstMbuf)
{
    BS_DBGASSERT(NULL != pstMbuf);
    
    if (MBUF_DATABLOCK_NUM(pstMbuf) <= 1)
    {
        return BS_OK;
    }

    return _MBUF_Compress(pstMbuf);
}

static inline BS_STATUS MBUF_Cat (IN MBUF_S *pstMbufDst, IN MBUF_S *pstMbufSrc)
{
    BS_DBGASSERT (NULL != pstMbufDst);
    BS_DBGASSERT (NULL != pstMbufSrc);

    DLL_CAT (&pstMbufDst->stMblkHead, &pstMbufSrc->stMblkHead);
    MBUF_TOTAL_DATA_LEN (pstMbufDst) += MBUF_TOTAL_DATA_LEN (pstMbufSrc);
    MBUF_Free (pstMbufSrc);

    return BS_OK;
}

static inline BS_STATUS MBUF_NeatCat (IN MBUF_S *pstMbufDst, IN MBUF_S *pstMbufSrc)
{
    MBUF_Cat (pstMbufDst, pstMbufSrc);
    MBUF_Neat (pstMbufDst);

    return BS_OK;
}

static inline VOID * MBUF_MTOD (IN MBUF_S *pstMbuf)
{
    MBUF_MBLK_S *pstMblk;
    
    BS_DBGASSERT (NULL != pstMbuf);

    pstMblk = (MBUF_MBLK_S *)DLL_FIRST (&pstMbuf->stMblkHead);

    return ((pstMblk == NULL) ? NULL : pstMblk->pucData);
}

static inline BS_STATUS MBUF_AddTailData(INOUT MBUF_S *pstMbuf, IN UCHAR *pucData, IN UINT uiDataLen)
{
    return MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf), pucData, uiDataLen);
}

static inline BS_STATUS MBUF_AddTailString(INOUT MBUF_S *pstMbuf, IN CHAR *pcString)
{
    return MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf), (UCHAR*)pcString, strlen(pcString));
}

#if 1  /* ETH链路层信息 */
static inline USHORT MBUF_GET_ETH_MARKFLAG (IN MBUF_S *pstMBuf)
{
    return pstMBuf->stTag.unL2Hdr.stLinkInfo.unL2Hdr.stEthHdr.usMarkFlag;
}
static inline VOID MBUF_SET_ETH_MARKFLAG (IN MBUF_S *pstMBuf, IN USHORT usFlag)
{
    pstMBuf->stTag.unL2Hdr.stLinkInfo.unL2Hdr.stEthHdr.usMarkFlag |= usFlag;
    pstMBuf->stTag.unL2Hdr.stLinkInfo.ucLinkType = MBUF_LINK_TYPE_ETHERNET;
    return;
}
static inline VOID MBUF_CLEAR_ETH_MARKFLAG (IN MBUF_S *pstMBuf, IN USHORT usFlag)
{
    pstMBuf->stTag.unL2Hdr.stLinkInfo.unL2Hdr.stEthHdr.usMarkFlag  &= (USHORT)(~usFlag);
    return;
}

/* 链路层头的二层协议类型的Get和Set */
static inline VOID MBUF_SET_ETH_L2TYPE (IN MBUF_S *pstMBuf, IN USHORT usL2Protocol)
{
    pstMBuf->stTag.unL2Hdr.stLinkInfo.unL2Hdr.stEthHdr.usL2Type = usL2Protocol;
    return;
}
static inline USHORT MBUF_GET_ETH_L2TYPE (IN MBUF_S *pstMBuf)
{
    return pstMBuf->stTag.unL2Hdr.stLinkInfo.unL2Hdr.stEthHdr.usL2Type;
}

/* 以太网帧格式的Get和Set */
static inline VOID MBUF_SET_ETH_PKTFMT (IN MBUF_S * pstMBuf, IN UCHAR ucPacketFmt)
{
    pstMBuf->stTag.unL2Hdr.stLinkInfo.unL2Hdr.stEthHdr.ucPktFmt = ucPacketFmt;
    return;
}
static inline UCHAR MBUF_GET_ETH_PKTFMT (IN MBUF_S * pstMBuf) 
{
    return pstMBuf->stTag.unL2Hdr.stLinkInfo.unL2Hdr.stEthHdr.ucPktFmt;
}

/* 获取MBUF对象报文的以太网信息结构指针,传入的MBUF指针必须有效 */
static inline MBUF_ETHERNET_HDR_S * MBUF_GET_ETHERNET_HDR (IN MBUF_S * pstMBuf)
{
    return (&(pstMBuf->stTag.unL2Hdr.stLinkInfo.unL2Hdr.stEthHdr));
}

/* 报文源MAC和目的MAC的Get和Set*/
static inline UCHAR *MBUF_GET_SOURCEMAC(IN MBUF_S * pstMBuf)
{
    return (pstMBuf->stTag.unL2Hdr.stLinkInfo.unL2Hdr.stEthHdr.aucSrcAddr);
}
static inline UCHAR *MBUF_GET_DESTMAC(IN MBUF_S * pstMBuf)
{
    return (pstMBuf->stTag.unL2Hdr.stLinkInfo.unL2Hdr.stEthHdr.aucDstAddr);
}

static inline VOID MBUF_SET_SOURCEMAC(IN MBUF_S * pstMBuf, IN UCHAR* pucSrcMac)
{
    UCHAR *pucMacAddr = pstMBuf->stTag.unL2Hdr.stLinkInfo.unL2Hdr.stEthHdr.aucSrcAddr;

    *((USHORT *)(VOID *)pucMacAddr) = *((USHORT *)(VOID *)pucSrcMac);
    *((USHORT *)(VOID *)pucMacAddr + 1) = *((USHORT *)(VOID *)pucSrcMac + 1);
    *((USHORT *)(VOID *)pucMacAddr + 2) = *((USHORT *)(VOID *)pucSrcMac + 2);

    return;
}
static inline VOID MBUF_SET_DESTMAC(IN MBUF_S * pstMBuf,IN UCHAR* pucDstMac)
{
    UCHAR *pucMacAddr = pstMBuf->stTag.unL2Hdr.stLinkInfo.unL2Hdr.stEthHdr.aucDstAddr;

    *((USHORT *)(VOID *)pucMacAddr) = *((USHORT *)(VOID *)pucDstMac);
    *((USHORT *)(VOID *)pucMacAddr + 1) = *((USHORT *)(VOID *)pucDstMac + 1);
    *((USHORT *)(VOID *)pucMacAddr + 2) = *((USHORT *)(VOID *)pucDstMac + 2);

    return;
}
#endif

#if 1   /* IP层信息 */
static inline UINT MBUF_GET_IP_PKTTYPE (IN MBUF_S *pstMBuf)
{
    return pstMBuf->stTag.unL3Hdr.stIpHdr.uiIpPktType;
}
#endif

#if 1   /* VRF信息 */

/* 报文的初始入口VPN索引的Get和Set */
static inline VOID MBUF_SET_RAWINVPNID (IN MBUF_S *pstMBuf, IN VRF_INDEX vrfIndexRawIn)
{
    pstMBuf->stTag.stVrf.vrfIndexRawIn = vrfIndexRawIn;
    return;
}

static inline VRF_INDEX MBUF_GET_RAWINVPNID (IN MBUF_S *pstMBuf)
{
    return (pstMBuf->stTag.stVrf.vrfIndexRawIn);
}

/* 报文的入口VPN索引的Get和Set */
static inline VOID MBUF_SET_INVPNID (IN MBUF_S *pstMBuf, IN VRF_INDEX vrfIndexIn)
{
    pstMBuf->stTag.stVrf.vrfIndexIn = vrfIndexIn;
    return;
}

static inline VRF_INDEX MBUF_GET_INVPNID (IN MBUF_S *pstMBuf)
{
    return (pstMBuf->stTag.stVrf.vrfIndexIn);
}

/* 报文的出口VPN索引的Get和Set */
static inline VOID MBUF_SET_OUTVPNID (IN MBUF_S *pstMBuf, IN VRF_INDEX vrfIndexOut)
{
    pstMBuf->stTag.stVrf.vrfIndexOut = vrfIndexOut;
    return;
}

static inline VRF_INDEX MBUF_GET_OUTVPNID (IN MBUF_S *pstMBuf)
{
    return (pstMBuf->stTag.stVrf.vrfIndexOut);
}
#endif

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__MBUF_UTL_H_*/


