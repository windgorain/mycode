/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2012-9-10
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#ifdef IN_WINDOWS

#include "utl/txt_utl.h"
#include "utl/my_ip_helper.h"

typedef struct
{
    DLL_NODE_S stLinkNode;
    UINT uiIP;
    UINT uiIpId;
}_MY_IP_HELPER_ADDED_IP_INFO_S;

static DLL_HEAD_S g_stMyIpHelperAddedIpList = DLL_HEAD_INIT_VALUE(&g_stMyIpHelperAddedIpList);

static VOID _my_ip_helper_PrintAdapters()
{
    IP_ADAPTER_ADDRESSES *pstAddresses;
    IP_ADAPTER_ADDRESSES *pstCurrAddresses;
    ULONG ulOutBufLen = 0;

    if (GetAdaptersAddresses(AF_INET, 0, NULL, NULL, &ulOutBufLen)
        != ERROR_BUFFER_OVERFLOW)
	{
        return;
    }

    pstAddresses = (IP_ADAPTER_ADDRESSES*) malloc(ulOutBufLen);
    if (NULL == pstAddresses)
    {
        return;
    }

    if (GetAdaptersAddresses(AF_INET, 0, NULL, pstAddresses,  &ulOutBufLen) != NO_ERROR)
    {
        free(pstAddresses);
        return;
    }

    pstCurrAddresses = pstAddresses;

    while (pstCurrAddresses)
    {
        printf("\tFriendly name: %s\n", pstCurrAddresses->FriendlyName);
        printf("\tDescription: %s\n", pstCurrAddresses->Description);
        pstCurrAddresses = pstCurrAddresses->Next;
    }

    free(pstAddresses);

	return;
}

static BOOL_T _my_ip_helper_GetAdapterName
(
    IN CHAR *pcAdapterGuid,
    OUT VOID *pAdapterNameUni,
    IN UINT uiSize
)
{
    _snwprintf (pAdapterNameUni, uiSize, L"\\DEVICE\\TCPIP_%S", pcAdapterGuid);

    return TRUE;
}

static IP_ADAPTER_INFO * _my_ip_helper_GetAdaptersInfo()
{
    DWORD dwInfoSize = 0;
    IP_ADAPTER_INFO *pstAdaptersInfo;

    GetAdaptersInfo(NULL, &dwInfoSize);

    if ((pstAdaptersInfo = MEM_Malloc(dwInfoSize)) == NULL)
    {
        return NULL;
    }

    // 获取网卡信息
    if (NO_ERROR != GetAdaptersInfo(pstAdaptersInfo, &dwInfoSize))
    {

        MEM_Free(pstAdaptersInfo);
        return NULL;
    }

    return pstAdaptersInfo;
}

static VOID _my_ip_helper_FreeAdaptersInfo(IN IP_ADAPTER_INFO *pstInfo)
{
    MEM_Free(pstInfo);
}

BS_STATUS My_IP_Helper_GetAdapterIndex(IN CHAR *pcAdapterGuid, OUT UINT *puiIndex)
{
    UCHAR szAdapterNameUnicode[256];

    if (FALSE == _my_ip_helper_GetAdapterName(pcAdapterGuid, szAdapterNameUnicode, sizeof(szAdapterNameUnicode)))
    {
        return BS_ERR;
    }

    if (NO_ERROR != GetAdapterIndex ((VOID*)szAdapterNameUnicode, puiIndex))
    {
        return BS_ERR;
    }

    return BS_OK;
}

static VOID _my_ip_helper_RecordAddedIp(IN UINT uiIp, IN UINT uiIpId)
{
    _MY_IP_HELPER_ADDED_IP_INFO_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(_MY_IP_HELPER_ADDED_IP_INFO_S));
    if (NULL == pstNode)
    {
        return;
    }

    pstNode->uiIP = uiIp;
    pstNode->uiIpId = uiIpId;

    DLL_ADD(&g_stMyIpHelperAddedIpList, pstNode);
}

VOID My_IP_Helper_DeleteAllIpAddress(IN UINT uiAdapterIndex)
{
    My_IP_Helper_DeleteIpAddress(uiAdapterIndex, 0);
}


VOID My_IP_Helper_DeleteIpAddress
(
    IN UINT uiAdapterIndex, 
    IN UINT uiIp/* 网络序, 0表示删除所有IP */
)
{
    IP_ADAPTER_INFO *pstAdaptersInfo;
    IP_ADAPTER_INFO *pAdapt;
    IP_ADDR_STRING *paddr;
    CHAR szIpSz[16];

    if (uiIp != 0)
    {
        snprintf(szIpSz, sizeof(szIpSz), "%pI4", &uiIp);
    }

    pstAdaptersInfo = _my_ip_helper_GetAdaptersInfo();
    if (NULL == pstAdaptersInfo)
    {
        return;
    }

    pAdapt = pstAdaptersInfo;

    while (pAdapt)
    {
        if (uiAdapterIndex == pAdapt->Index)
        {
            break;
        }

        pAdapt = pAdapt->Next;
    }

    if (NULL == pAdapt)
    {
        _my_ip_helper_FreeAdaptersInfo(pstAdaptersInfo);
        return;
    }

    paddr = &pAdapt->IpAddressList;
    while(paddr)
    {
        if ((uiIp == 0) || (strcmp(szIpSz, paddr->IpAddress.String) == 0))
        {
            DeleteIPAddress(paddr->Context);
            paddr = paddr->Next;
        }
    }

    FlushIpNetTable(uiAdapterIndex);

    _my_ip_helper_FreeAdaptersInfo(pstAdaptersInfo);
}

BS_STATUS My_IP_Helper_AddIPAddress
(
    IN UINT uiAdapterIndex,
    IN UINT uiIp,   /* 网络序 */
    IN UINT uiMask  /* 网络序 */
)
{
    ULONG ulIpId = 0;
    ULONG ulInstance = 0;
    DWORD uiRet;

	uiRet = AddIPAddress(uiIp, uiMask, uiAdapterIndex, &ulIpId, &ulInstance);
    if (NO_ERROR != uiRet)
    {
        return BS_ERR;
    }

    _my_ip_helper_RecordAddedIp(uiIp, ulIpId);

    return BS_OK;
}

BS_STATUS My_IP_Helper_GetIPAddress
(
    IN CHAR *pcAdapterGuid,
    OUT UINT *puiIp,
    OUT UINT *puiMask
)
{
    UINT uiSize = 0;
    MIB_IPADDRTABLE * pIPAddrTable = NULL;
    UINT i;
    UINT uiAdapterIndex;

    *puiIp = 0;
    *puiMask = 0;

    if (BS_OK != My_IP_Helper_GetAdapterIndex(pcAdapterGuid, &uiAdapterIndex))
    {
        return BS_NO_SUCH;
    }

    if (GetIpAddrTable(pIPAddrTable, &uiSize, 0) != ERROR_INSUFFICIENT_BUFFER)
    {
        return BS_ERR;
    }

    pIPAddrTable = (MIB_IPADDRTABLE *) malloc(uiSize);
    if (NULL == pIPAddrTable)
    {
        return BS_NO_MEMORY;
    }

    if (GetIpAddrTable(pIPAddrTable, &uiSize, 0 ) != NO_ERROR)
    { 
        free(pIPAddrTable);
        return BS_ERR;
    }

    for (i=0; i < (int) pIPAddrTable->dwNumEntries; i++)
    {
        if (pIPAddrTable->table[i].dwIndex == uiAdapterIndex)
        {
            *puiIp = pIPAddrTable->table[i].dwAddr;
            *puiMask = pIPAddrTable->table[i].dwMask;
            break;
        }
    }

    free(pIPAddrTable);

    return BS_OK;
}

VOID My_IP_Helper_DeleteArpsOfAdapter(IN UINT uiAdapterIndex)
{
    FlushIpNetTable(uiAdapterIndex);
}

MIB_IPFORWARDTABLE * My_IP_Helper_GetRouteTbl()
{
    MIB_IPFORWARDTABLE *pIpForwardTable = NULL;
    MIB_IPFORWARDROW *pRow = NULL;
    DWORD dwSize = 0;
    BOOL bOrder = FALSE;
    DWORD dwStatus = 0;
    BS_STATUS eRet = BS_NOT_FOUND;
    
    dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    if (dwStatus == ERROR_INSUFFICIENT_BUFFER)
    {
        pIpForwardTable = MEM_Malloc(dwSize);
        if (pIpForwardTable == NULL)
        {
            return NULL;
        }

        dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    }

    if (dwStatus != ERROR_SUCCESS)
    {
        if (pIpForwardTable) 
        {
            MEM_Free(pIpForwardTable);
        }
        return NULL;
    }

    return pIpForwardTable;
}

VOID My_IP_Helper_FreeRouteTbl(IN MIB_IPFORWARDTABLE *pstRouteTbl)
{
    if (NULL != pstRouteTbl)
    {
        MEM_Free(pstRouteTbl);
    }

    return;
}

MIB_IPFORWARDROW *My_IP_Helper_FindDftRouteByTbl(IN MIB_IPFORWARDTABLE *pstRouteTbl)
{
    MIB_IPFORWARDROW * pstRow;
    
    MY_IP_HELPER_SCAN_ROUTE_TBL_START(pstRouteTbl, pstRow)
    {
        if ((pstRow->dwForwardDest == 0) && (pstRow->dwForwardMask == 0))
        {
            return pstRow;
        }
    }MY_IP_HELPER_SCAN_ROUTE_TBL_END();

    return NULL;
}

unsigned int My_IP_Helper_GetDftGateway()
{
    MIB_IPFORWARDTABLE * tbl = My_IP_Helper_GetRouteTbl();
    MIB_IPFORWARDROW *find;
    unsigned int gw = 0;

    if (tbl == NULL) {
        return 0;
    }

    find = My_IP_Helper_FindDftRouteByTbl(tbl);
    if (NULL != find) {
        gw = find->dwForwardNextHop;
    }

    My_IP_Helper_FreeRouteTbl(tbl);

    return gw;
}

VOID My_IP_Helper_DelRoute2(IN MIB_IPFORWARDROW *pstRoute)
{
    DeleteIpForwardEntry(pstRoute);
}

VOID My_IP_Helper_DelRoute
(
    IN UINT uiDstIp/* 网络序 */,
    IN UINT uiMask/* 网络序 */,
    IN UINT uiNextHop/* 网络序 */,
    IN UINT uiOutIfIndex
)
{
    MIB_IPFORWARDROW stRoute = {0};

    stRoute.dwForwardDest = uiDstIp & uiMask;
    stRoute.dwForwardMask = uiMask;
    stRoute.dwForwardNextHop = uiNextHop;
    stRoute.dwForwardIfIndex = uiOutIfIndex;
    stRoute.dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;
    stRoute.dwForwardProto = RouteProtocolNetMgmt;
    stRoute.dwForwardAge = 0x16;
    stRoute.dwForwardMetric1 = 0x11e;

    My_IP_Helper_DelRoute2(&stRoute);
}

BS_STATUS My_IP_Helper_ModifyRoute(IN MIB_IPFORWARDROW *pstRoute)
{
    DWORD dwStatus;
    
    dwStatus = SetIpForwardEntry(pstRoute);
    if (dwStatus == NO_ERROR)
    {
        return BS_OK;
    }

    return BS_ERR;
}

BS_STATUS My_IP_Helper_AddRoute2(IN MIB_IPFORWARDROW *pstRoute)
{
    DWORD dwStatus;
    
    dwStatus = CreateIpForwardEntry(pstRoute);
    if (dwStatus == NO_ERROR)
    {
        return BS_OK;
    }

    return BS_ERR;
}

BS_STATUS My_IP_Helper_AddRoute
(
    IN UINT uiDstIp/* 网络序 */,
    IN UINT uiMask/* 网络序 */,
    IN UINT uiNextHop/* 网络序 */,
    IN UINT uiOutIfIndex
)
{
    MIB_IPFORWARDROW stRoute = {0};
    BS_STATUS eRet;

    stRoute.dwForwardDest = uiDstIp & uiMask;
    stRoute.dwForwardMask = uiMask;
    stRoute.dwForwardNextHop = uiNextHop;
    stRoute.dwForwardIfIndex = uiOutIfIndex;
    stRoute.dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;
    stRoute.dwForwardProto = RouteProtocolNetMgmt;
    stRoute.dwForwardAge = 0x16;
    stRoute.dwForwardMetric1 = 0x11e;

    eRet = My_IP_Helper_AddRoute2(&stRoute);

    return eRet;
}

BS_STATUS My_IP_Helper_SetRoute2(IN MIB_IPFORWARDROW *pstRoute)
{
    BS_STATUS eRet;
    
    eRet = My_IP_Helper_ModifyRoute(pstRoute);
    if (eRet == BS_OK)
    {
        return BS_OK;
    }

    return My_IP_Helper_AddRoute2(pstRoute);
}

BS_STATUS My_IP_Helper_SetRoute
(
    IN UINT uiDstIp/* 网络序 */,
    IN UINT uiMask/* 网络序 */,
    IN UINT uiNextHop/* 网络序 */,
    IN UINT uiOutIfIndex
)
{
    MIB_IPFORWARDROW stRoute = {0};
    BS_STATUS eRet;

    stRoute.dwForwardDest = uiDstIp & uiMask;
    stRoute.dwForwardMask = uiMask;
    stRoute.dwForwardNextHop = uiNextHop;
    stRoute.dwForwardIfIndex = uiOutIfIndex;
    stRoute.dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;
    stRoute.dwForwardProto = RouteProtocolNetMgmt;
    stRoute.dwForwardAge = 0x16;
    stRoute.dwForwardMetric1 = 0x11e;

    eRet = My_IP_Helper_SetRoute2(&stRoute);

    return eRet;
}

/* 根据下一条删除所有路由 */
BS_STATUS My_IP_Helper_DelAllRouteByNexthop(IN UINT uiNexthop /* 网络序 */)
{
    MIB_IPFORWARDROW *pstIpRoute;
    MIB_IPFORWARDROW *pstIpRouteFound = NULL;
    MIB_IPFORWARDTABLE *pstRouteTbl;
    BS_STATUS eRet = BS_NOT_FOUND;

    pstRouteTbl = My_IP_Helper_GetRouteTbl();
    if (NULL == pstRouteTbl)
    {
        return BS_NO_MEMORY;
    }

    MY_IP_HELPER_SCAN_ROUTE_TBL_START(pstRouteTbl, pstIpRoute)
    {
        if (pstIpRoute->dwForwardNextHop == uiNexthop)
        {
            My_IP_Helper_DelRoute2(pstIpRoute);
        }
    }MY_IP_HELPER_SCAN_ROUTE_TBL_END();

    My_IP_Helper_FreeRouteTbl(pstRouteTbl);

    return eRet;
}

/* 将出接口是指定接口的路由全部删除 */
BS_STATUS My_IP_Helper_DelAllRouteByAdapterindex(IN UINT uiAdapterIndex)
{
    MIB_IPFORWARDROW *pstIpRoute;
    MIB_IPFORWARDROW *pstIpRouteFound = NULL;
    MIB_IPFORWARDTABLE *pstRouteTbl;
    BS_STATUS eRet = BS_NOT_FOUND;

    pstRouteTbl = My_IP_Helper_GetRouteTbl();
    if (NULL == pstRouteTbl)
    {
        return BS_NO_MEMORY;
    }

    MY_IP_HELPER_SCAN_ROUTE_TBL_START(pstRouteTbl, pstIpRoute)
    {
        if (pstIpRoute->dwForwardIfIndex == uiAdapterIndex)
        {
            My_IP_Helper_DelRoute2(pstIpRoute);
        }
    }MY_IP_HELPER_SCAN_ROUTE_TBL_END();

    My_IP_Helper_FreeRouteTbl(pstRouteTbl);

    return eRet;
}

/* 计算指定出接口的路由条数 */
UINT My_IP_Helper_CountRouteByAdapterIndex(IN UINT uiAdapterIndex)
{
    MIB_IPFORWARDROW *pstIpRoute;
    MIB_IPFORWARDROW *pstIpRouteFound = NULL;
    MIB_IPFORWARDTABLE *pstRouteTbl;
    BS_STATUS eRet = BS_NOT_FOUND;
    UINT uiCount = 0;

    pstRouteTbl = My_IP_Helper_GetRouteTbl();
    if (NULL == pstRouteTbl)
    {
        return 0;
    }

    MY_IP_HELPER_SCAN_ROUTE_TBL_START(pstRouteTbl, pstIpRoute)
    {
        if (pstIpRoute->dwForwardIfIndex == uiAdapterIndex)
        {
            uiCount ++;
        }
    }MY_IP_HELPER_SCAN_ROUTE_TBL_END();

    My_IP_Helper_FreeRouteTbl(pstRouteTbl);

    return uiCount;
}

#endif
