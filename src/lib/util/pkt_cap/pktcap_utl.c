
#include "bs.h"

#include "utl/sprintf_utl.h"
#include "utl/net.h"
#include "utl/mutex_utl.h"
#include "utl/plug_utl.h"
#include "utl/exec_utl.h"
#include "utl/pktcap_utl.h"

#include "pktcap_os_utl.h"

static _PKTCAP_FUNC_TBL_S g_stPktcapFuncTbl = {0};


static BS_STATUS _os_pktcap_Init()
{
	PLUG_ID ulPlugId = 0;
	UINT i;
	VOID ** pFunc;
    CHAR *pcDllFile = "";
	static BOOL_T bIsInit = FALSE;

    if (bIsInit) {
        return BS_OK;
    }

	bIsInit = TRUE;

#ifdef IN_WINDOWS
    pcDllFile = "wpcap.dll";
#endif

#ifdef IN_UNIXLIKE
    pcDllFile = "libpcap.so";
#endif

#ifdef IN_MAC
    pcDllFile = "libpcap.dylib";
#endif

    ulPlugId = PLUG_LOAD(pcDllFile);

    if (NULL == ulPlugId)
    {
        BS_WARNNING(("Can't load %s", pcDllFile));
		return BS_CAN_NOT_OPEN;
    }

	g_stPktcapFuncTbl.pfFindAllDevs = PLUG_GET_FUNC_BY_NAME(ulPlugId, "pcap_findalldevs");
	g_stPktcapFuncTbl.pfFreeAllDevs = PLUG_GET_FUNC_BY_NAME(ulPlugId, "pcap_freealldevs");
	g_stPktcapFuncTbl.pfOpenLive = PLUG_GET_FUNC_BY_NAME(ulPlugId, "pcap_open_live");
	g_stPktcapFuncTbl.pfDataLink = PLUG_GET_FUNC_BY_NAME(ulPlugId, "pcap_datalink");
	g_stPktcapFuncTbl.pfClose = PLUG_GET_FUNC_BY_NAME(ulPlugId, "pcap_close");
	g_stPktcapFuncTbl.pfSendPacket = PLUG_GET_FUNC_BY_NAME(ulPlugId, "pcap_sendpacket");
	g_stPktcapFuncTbl.pfCompile = PLUG_GET_FUNC_BY_NAME(ulPlugId, "pcap_compile");
	g_stPktcapFuncTbl.pfSetFilter = PLUG_GET_FUNC_BY_NAME(ulPlugId, "pcap_setfilter");
	g_stPktcapFuncTbl.pfDispatch = PLUG_GET_FUNC_BY_NAME(ulPlugId, "pcap_dispatch");
	g_stPktcapFuncTbl.pfLoop = PLUG_GET_FUNC_BY_NAME(ulPlugId, "pcap_loop");
	g_stPktcapFuncTbl.pfBreakLoop = PLUG_GET_FUNC_BY_NAME(ulPlugId, "pcap_breakloop");

	pFunc = (VOID**)&g_stPktcapFuncTbl;
	for (i=0; i<sizeof(g_stPktcapFuncTbl)/sizeof(VOID*); i++)
	{
		if (pFunc == NULL)
		{
			PLUG_FREE(ulPlugId);
			return BS_ERR;
		}
		pFunc ++;
	}

	return BS_OK;
}

static inline CHAR *pkctap_GetStr(IN CHAR *pcStr)
{
    if (NULL == pcStr)
    {
        return "";
    }

    return pcStr;
}


/* 打印网卡信息  */
static VOID _pktcap_GetNdisInfoString(IN pcap_if_t* pNdis, IN UINT uiSize, OUT CHAR *pcString)
{
    pcap_addr_t *pAddr = NULL;          // 网卡地址
    struct sockaddr_in *pstSinIp;
    struct sockaddr_in *pstSinMask;
    INT iLen;
    CHAR *pcTmp;
    UINT uiNewSize;

    iLen = snprintf(pcString, uiSize,
                    "Pcap Name: %s\r\n"
                    "Pcap Description: %s\r\n"
                    "Pcap Address:\r\n",
                    pNdis->name,
                    pkctap_GetStr(pNdis->description));
    
    pcTmp = pcString + iLen;
    uiNewSize = uiSize - iLen;
    if (uiNewSize <= 1)
    {
        return;
    }

    for(pAddr = pNdis->addresses; pAddr; pAddr = pAddr->next)
    {
        iLen = 0;

        /* 一张网卡可能有多个地址 */
        switch(pAddr->addr->sa_family)
        {
            case AF_INET:
            {
                pstSinIp = (struct sockaddr_in*)pAddr->addr;
                pstSinMask = (struct sockaddr_in*)pAddr->netmask;
				if (pstSinIp->sin_addr.s_addr != 0)
                {
                    iLen = BS_Snprintf(pcTmp, uiNewSize, " %pI4/%pI4\r\n", &pstSinIp->sin_addr, &pstSinMask->sin_addr);
                }
                break;
            }

            default:
            {
                break;
            }
        }

        pcTmp = pcTmp + iLen;
        uiNewSize = uiNewSize - iLen;
        if (uiNewSize <= 1)
        {
            break;
        }
    }

    return;
}

/* 打印网卡信息  */
static VOID _pktcap_PrintNdis(pcap_if_t* pNdis)
{
    CHAR szInfo[512];

    _pktcap_GetNdisInfoString(pNdis, sizeof(szInfo), szInfo);

    EXEC_OutInfo("%s\r\n", szInfo); 

    return;
}

/* 打印网卡信息 */
static VOID _pktcap_PrintfNdisInfo(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo)
{
    pcap_if_t *pNdis = hNdisInfo;
    pcap_if_t *pCurNdis = NULL;

    if (pNdis == NULL)
    {
        BS_DBGASSERT(0);
        return;
    }

    for(pCurNdis = pNdis; pCurNdis; pCurNdis = pCurNdis->next)
    {
        _pktcap_PrintNdis(pCurNdis);
    }
}

/* 根据Index获取Index */
static UINT _pktcap_GetNdisIndexByName(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo, IN CHAR *pcNdisName)
{
    pcap_if_t *pNdis = hNdisInfo;
    pcap_if_t *pstCurNdis = NULL;
    UINT uiIndex = PKTCAP_INVALID_INDEX;
    UINT i;

    if (pNdis == NULL)
    {
        BS_DBGASSERT(0);
        return PKTCAP_INVALID_INDEX;
    }

    i = 0;
    for(pstCurNdis = pNdis; pstCurNdis; pstCurNdis = pstCurNdis->next)
    {
        if (strcmp(pstCurNdis->name, pcNdisName) == 0)
        {
            uiIndex = i;
            break;
        }

        i++;
    }

    return uiIndex;
}

/* 根据Index获取网卡名 */
static CHAR * _pktcap_GetNdisNameByIndex(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo, IN UINT uiIndex)
{
    UINT i;
    pcap_if_t *pNdis = hNdisInfo;
    pcap_if_t *pCurNdis = NULL;

    if (pNdis == NULL)
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    i = 0;
    for(pCurNdis = pNdis; pCurNdis; pCurNdis = pCurNdis->next)
    {
        if (i == uiIndex)
        {
            break;
        }

        i++;
    }

    if (pCurNdis == NULL)
    {
        return NULL;
    }

    return pCurNdis->name;
}

static BS_STATUS _pktcap_GetNdisInfoByName
(
    IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo,
    IN CHAR *pcNdisName,
    OUT PKTCAP_NDIS_INFO_S *pstInfo
)
{
    UINT i;
    pcap_if_t *pNdis = hNdisInfo;
    pcap_if_t *pstCurNdis = NULL;
    pcap_addr_t *pstAddr = NULL;
    struct sockaddr_in *pstSin;

    Mem_Zero(pstInfo, sizeof(PKTCAP_NDIS_INFO_S));

    if (pNdis == NULL)
    {
        BS_DBGASSERT(0);
        return BS_BAD_PARA;
    }

    for(pstCurNdis = pNdis; pstCurNdis; pstCurNdis = pstCurNdis->next)
    {
        if (strcmp(pstCurNdis->name, pcNdisName) == 0)
        {
            break;
        }
    }

    if (pstCurNdis == NULL)
    {
        return BS_NO_SUCH;
    }

    i = 0;
    for(pstAddr = pstCurNdis->addresses; pstAddr; pstAddr = pstAddr->next)
    {
        switch(pstAddr->addr->sa_family)
        {
            case AF_INET:
            {
                if (pstAddr->netmask)
                {
                    pstSin = (struct sockaddr_in*)pstAddr->addr;
                    pstInfo->astAddr[i].uiMask = NET_GET_ADDR_BY_SOCKADDR(pstSin);
                }

                if (pstAddr->addr)
                {
                    pstSin = (struct sockaddr_in*)pstAddr->addr;
                    pstInfo->astAddr[i].uiAddr = NET_GET_ADDR_BY_SOCKADDR(pstSin);

                    i++;
                }

                break;
            }

            default:
            {
                break;
            }
        }

        if (i >= PKTCAP_NDIS_MAX_ADDR_NUM)
        {
            break;
        }
    }

    return BS_OK;
}

static VOID _pktcap_PacketIn(UCHAR *param, const struct pcap_pkthdr *pstHeader, const UCHAR *pucPktData)
{
    PKTCAP_PKT_INFO_S stPktInfo;
    USER_HANDLE_S *pstUserHandle = (USER_HANDLE_S*)param;
    PF_PKT_CAP_PACKET_IN_FUNC pfFunc;

    pfFunc = pstUserHandle->ahUserHandle[0];

    memset(&stPktInfo, 0, sizeof(stPktInfo));
    stPktInfo.ts = pstHeader->ts;
    stPktInfo.uiPktRawLen = pstHeader->len;
    stPktInfo.uiPktCaptureLen = pstHeader->caplen;

    pfFunc((UCHAR*)pucPktData, &stPktInfo, pstUserHandle->ahUserHandle[1]);

    return;
}


/*  获取网卡信息 */
static PKTCAP_NIDS_INFO_HANDLE _pktcap_GetNdisInfo()
{
    pcap_if_t *pNdis = NULL;  // 网卡信息保存链表
    CHAR errbuf[PCAP_ERRBUF_SIZE];  // 错误信息

	if (BS_OK != _os_pktcap_Init())
	{
		return NULL;
	}

    /* 1. 获取网卡链表 */
    if(-1 == g_stPktcapFuncTbl.pfFindAllDevs(&pNdis, errbuf))
    {
        return NULL;
    }

    return pNdis;
}

/* 释放网卡信息 */
static VOID _pktcap_FreeNdisInfo(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo)
{
    pcap_if_t *pNdis = hNdisInfo;

    if (pNdis == NULL)
    {
        BS_DBGASSERT(0);
        return;
    }

    /* 释放网卡链表 */
    g_stPktcapFuncTbl.pfFreeAllDevs(pNdis);
}

/* 打开网卡 */
static PKTCAP_NDIS_HANDLE _pktcap_OpenNdis
(
    IN CHAR *pcNdisName,
    IN UINT uiFlag,
    IN UINT uiTimeOutTime/* ms, 0表示永不超时 */
)
{
    pcap_t *fp;
    char errBuf[PCAP_ERRBUF_SIZE];

    if (NULL == pcNdisName)
    {
        BS_DBGASSERT(0);
        return NULL;
    }

	if (BS_OK != _os_pktcap_Init())
	{
		return NULL;
	}

    /* 1. Open网卡 */
    fp = g_stPktcapFuncTbl.pfOpenLive(pcNdisName, // 网卡信息中的name
                        65536,      // 报文捕获时，返回整个报文的内容
                        uiFlag,
                        uiTimeOutTime,       // 超时时间
                        errBuf);

    if(NULL == fp)
    {
        return NULL;
    }

    if(g_stPktcapFuncTbl.pfDataLink(fp) != DLT_EN10MB)
    {
        g_stPktcapFuncTbl.pfClose(fp);
        return NULL;
    }

    return fp;
}


/* 关闭网卡 */
static VOID _pktcap_CloseNdis(IN PKTCAP_NDIS_HANDLE hNdisHandle)
{
    pcap_t *fp = hNdisHandle;

    if (NULL != fp)
    {
        g_stPktcapFuncTbl.pfClose(fp);
    }
}

/* 报文发送 */
static BOOL_T _pktcap_SendPacket
(
    IN PKTCAP_NDIS_HANDLE hNdisHandle,
    IN UCHAR *pucPkt,
    IN UINT uiPktLen
)
{
    pcap_t *fp = hNdisHandle;

    if (NULL == fp)
    {
        BS_DBGASSERT(0);
        return FALSE;
    }

    if(0 != g_stPktcapFuncTbl.pfSendPacket(fp, pucPkt, uiPktLen))
    {
        return FALSE;
    }

    return TRUE;
}

/* 过滤规则设置 */
static BOOL_T _pktcap_SetFilter(IN PKTCAP_NDIS_HANDLE hNdisHandle, IN CHAR* pcFilter)
{
    struct bpf_program fcode;   /* 过滤器 */
    pcap_t *fp = hNdisHandle;

    if ((NULL == fp) || (NULL == pcFilter))
    {
        BS_DBGASSERT(0);
        return FALSE;
    }

    /*1.  编译过滤器（将过滤文本编译成底层驱动识别的数据格式） */
    if(-1 == g_stPktcapFuncTbl.pfCompile(fp,       /* 网卡 */
                          &fcode,   /* 过滤器 */
                          pcFilter,    /* 过滤文本 */
                          1,        /* 优化 */
                          0))     /* 不设置子网过滤 */
    {
        return FALSE;
    }

    /* 2. 将过滤器绑定在网卡上 */
    if(g_stPktcapFuncTbl.pfSetFilter(fp, &fcode)<0)
    {
        return FALSE;
    }

    return TRUE;
}

/* 支持超时方式的抓包 */
static INT _pktcap_Dispatch
(
    IN PKTCAP_NDIS_HANDLE hNdisHandle,
    IN PF_PKT_CAP_PACKET_IN_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    pcap_t *fp = hNdisHandle;
    USER_HANDLE_S stUserHandle;

    if (NULL == fp)
    {
        BS_DBGASSERT(0);
        return -1;
    }

    stUserHandle.ahUserHandle[0] = pfFunc;
    stUserHandle.ahUserHandle[1] = hUserHandle;
    
    /* 开始捕获报文*/
	return g_stPktcapFuncTbl.pfDispatch(fp, 0, _pktcap_PacketIn, (UCHAR*)&stUserHandle);
}

static INT _pktcap_Loop
(
    IN PKTCAP_NDIS_HANDLE hNdisHandle,
    IN PF_PKT_CAP_PACKET_IN_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    pcap_t *fp = hNdisHandle;
    USER_HANDLE_S stUserHandle;

    if (NULL == fp)
    {
        BS_DBGASSERT(0);
        return -1;
    }

    stUserHandle.ahUserHandle[0] = pfFunc;
    stUserHandle.ahUserHandle[1] = hUserHandle;
    
    /* 开始捕获报文*/
	return g_stPktcapFuncTbl.pfLoop(fp, 0, _pktcap_PacketIn, (UCHAR*)&stUserHandle);
}

static VOID _pktcap_BreakLoop(IN PKTCAP_NDIS_HANDLE hNdisHandle)
{
    pcap_t *fp = hNdisHandle;

    if (NULL == fp)
    {
        BS_DBGASSERT(0);
        return;
    }

    g_stPktcapFuncTbl.pfBreakLoop(fp);

    return;
}

/*  获取网卡信息 */
PKTCAP_NIDS_INFO_HANDLE PKTCAP_GetNdisInfoList()
{
    return _pktcap_GetNdisInfo();
}

PKTCAP_NIDS_INFO_HANDLE PKTCAP_FindNdisByNameInNdisInfoList(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfoList, IN CHAR *pcName)
{
    PKTCAP_NIDS_INFO_HANDLE hNdisCurrent;
    
    hNdisCurrent = hNdisInfoList;

    while(hNdisCurrent)
    {
        if (strcmp(pcName, PKTCAP_GetNdisInfoName(hNdisCurrent)) == 0)
        {
            return hNdisCurrent;
        }
        hNdisCurrent = PKTCAP_GetNextNdisInfo(hNdisCurrent);
    }

    return NULL;
}

PKTCAP_NIDS_INFO_HANDLE PKTCAP_GetNextNdisInfo(IN PKTCAP_NIDS_INFO_HANDLE hCurrentNdisInfo)
{
    pcap_if_t *pstCurrentNdisInfo = hCurrentNdisInfo;

    if (NULL == pstCurrentNdisInfo)
    {
        return NULL;
    }

    return pstCurrentNdisInfo->next;
}

CHAR * PKTCAP_GetNdisInfoDesc(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo)
{
    pcap_if_t *pstNdisInfo = hNdisInfo;

    return pstNdisInfo->description;
}

CHAR * PKTCAP_GetNdisInfoName(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo)
{
    pcap_if_t *pstNdisInfo = hNdisInfo;

    return pstNdisInfo->name;
}

BOOL_T PKTCAP_IsNdisExist(IN CHAR *pcName)
{
    PKTCAP_NIDS_INFO_HANDLE hNdis, hNdisCurrent;
    
    hNdis = PKTCAP_GetNdisInfoList();
    hNdisCurrent = hNdis;

    while(hNdisCurrent)
    {
        if (strcmp(pcName, PKTCAP_GetNdisInfoName(hNdisCurrent)) == 0)
        {
            return TRUE;
        }
        hNdisCurrent = PKTCAP_GetNextNdisInfo(hNdisCurrent);
    }

    PKTCAP_FreeNdisInfoList(hNdis);

    return FALSE;
}

VOID PKTCAP_GetNdisInfoString(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo, IN UINT uiSize, OUT CHAR *pcString)
{
    _pktcap_GetNdisInfoString(hNdisInfo, uiSize, pcString);
}

/* 打印网卡信息 */
VOID PKTCAP_PrintfNdisInfo(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo)
{
    _pktcap_PrintfNdisInfo(hNdisInfo);
}

/* 根据Name获取Index */
UINT PKTCAP_GetNdisIndexByName(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo, IN CHAR *pcNdisName)
{
    return _pktcap_GetNdisIndexByName(hNdisInfo, pcNdisName);
}

/* 根据Index获取网卡名 */
CHAR * PKTCAP_GetNdisNameByIndex(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo, IN UINT uiIndex)
{
    return _pktcap_GetNdisNameByIndex(hNdisInfo, uiIndex);
}

BS_STATUS PKTCAP_GetNdisInfoByNameEx
(
    IN PKTCAP_NIDS_INFO_HANDLE hNdisInfoList,
    IN CHAR *pcNdisName,
    OUT PKTCAP_NDIS_INFO_S *pstInfo
)
{
    return _pktcap_GetNdisInfoByName(hNdisInfoList, pcNdisName, pstInfo);
}

BS_STATUS PKTCAP_GetNdisInfoByName
(
    IN CHAR *pcNdisName,
    OUT PKTCAP_NDIS_INFO_S *pstInfo
)
{
    PKTCAP_NIDS_INFO_HANDLE hNdisInfo;
    BS_STATUS eRet;

    hNdisInfo = PKTCAP_GetNdisInfoList();
    if (NULL == hNdisInfo)
    {
        return BS_ERR;
    }

    eRet = PKTCAP_GetNdisInfoByNameEx(hNdisInfo, pcNdisName, pstInfo);

    PKTCAP_FreeNdisInfoList(hNdisInfo);

    return eRet;
}


/* 释放网卡信息 */
VOID PKTCAP_FreeNdisInfoList(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo)
{
    _pktcap_FreeNdisInfo(hNdisInfo);
}

INT PKTCAP_GetDataLinkTypeByName(IN CHAR *pcDevName)
{
    pcap_t *fp;
    char errBuf[PCAP_ERRBUF_SIZE];
    INT iType;
    
    /* 1. Open网卡 */
    fp = g_stPktcapFuncTbl.pfOpenLive(pcDevName, // 网卡信息中的name
                        65536,      // 报文捕获时，返回整个报文的内容
                        0,
                        1000,       // 超时时间
                        errBuf);

    if(NULL == fp)
    {
        return -1;
    }

    iType = g_stPktcapFuncTbl.pfDataLink(fp);

    g_stPktcapFuncTbl.pfClose(fp);

    return iType;
}

BOOL_T PKTCAP_IsEthDataLink(IN INT iDataLink)
{
    if (DLT_EN10MB == iDataLink)
    {
        return TRUE;
    }

    return FALSE;
}

/* 打开网卡 */
PKTCAP_NDIS_HANDLE PKTCAP_OpenNdis(IN CHAR *pcNdisName, IN UINT uiFlag/* PKTCAP_FLAG_X */, IN UINT uiTimeOutTime/* ms, 0表示永不超时 */)
{
    return _pktcap_OpenNdis(pcNdisName, uiFlag, uiTimeOutTime);
}

INT PKTCAP_GetDataLinkType(IN PKTCAP_NDIS_HANDLE hNdisHandle)
{
    return g_stPktcapFuncTbl.pfDataLink(hNdisHandle);
}

/* 关闭网卡 */
VOID PKTCAP_CloseNdis(IN PKTCAP_NDIS_HANDLE hNdisHandle)
{
    _pktcap_CloseNdis(hNdisHandle);
}

/* 报文发送 */
BOOL_T PKTCAP_SendPacket
(
    IN PKTCAP_NDIS_HANDLE hNdisHandle,
    IN UCHAR *pucPkt,
    IN UINT uiPktLen
)
{
    return _pktcap_SendPacket(hNdisHandle, pucPkt, uiPktLen);
}

/* 过滤规则设置 */
BOOL_T PKTCAP_SetFilter(IN PKTCAP_NDIS_HANDLE hNdisHandle, IN CHAR* pcFilter)
{
    return _pktcap_SetFilter(hNdisHandle, pcFilter);
}

/* 支持超时方式的抓包 */
INT PKTCAP_Dispatch
(
    IN PKTCAP_NDIS_HANDLE hNdisHandle,
    IN PF_PKT_CAP_PACKET_IN_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    return _pktcap_Dispatch(hNdisHandle, pfFunc, hUserHandle);
}

INT PKTCAP_Loop
(
    IN PKTCAP_NDIS_HANDLE hNdisHandle,
    IN PF_PKT_CAP_PACKET_IN_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    return _pktcap_Loop(hNdisHandle, pfFunc, hUserHandle);
}

VOID PKTCAP_BreakLoop(IN PKTCAP_NDIS_HANDLE hNdisHandle)
{
    _pktcap_BreakLoop(hNdisHandle);
}

