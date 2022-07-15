/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-12-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "inet/in_pub.h"


IN_REG_TBL_S g_stInRegTbl;

VOID IN_Reg(IN IN_REG_TBL_S *pstTbl)
{
    g_stInRegTbl = *pstTbl;
}

BS_STATUS IN_IP_Output
(
    IN MBUF_S *pstMBuf,
    IN MBUF_S *pstMOpt,
    IN LONG lFlags,
    IN IPMOPTIONS_S *pstIpMo
)
{
    return g_stInRegTbl.pfIpOutput(pstMBuf, pstMOpt, lFlags, pstIpMo);
}

/* 源地址选择 */
BS_STATUS IN_IPAddr_SelectSrcAddr
(
    IN UINT uiDstAddr,
    IN VRF_INDEX vrfIndex, 
    IN IF_INDEX ifIndexOut,
    IN UINT uiNextHop,
    OUT UINT *puiSrcAddr
)
{
    return g_stInRegTbl.pfSelectSrcAddr(uiDstAddr, vrfIndex, ifIndexOut, uiNextHop, puiSrcAddr);
}

/* 在VRF中查找和目的地址网段匹配的地址 */
BS_STATUS IN_IPAddr_GetAddrInVrf
(
    IN VRF_INDEX vrfIndex,
    IN UINT uiIPAddr, 
    IN UINT uiAddrType,
    OUT IPADDR_INFO_S *pstAddrInfo
)
{
    return g_stInRegTbl.pfGetAddrInVrf(vrfIndex, uiIPAddr, uiAddrType, pstAddrInfo);
}

BS_STATUS IN_IPAddr_MatchBestNetInVrf
(
    IN VRF_INDEX vrfIndex,
    IN UINT uiIPAddr, 
    IN UINT uiAddrType, 
    OUT IPADDR_INFO_S *pstAddrInfo
)
{
    return g_stInRegTbl.pfMatchBestNetInVrf(vrfIndex, uiIPAddr, uiAddrType, pstAddrInfo);
}

/* 在VRF内找一个任意地址 */
BS_STATUS IN_IPAddr_GetFwdAddrInVrf
(
    IN VRF_INDEX vrfIndex,
    OUT IPADDR_INFO_S *pstAddrInfo
)
{
    return g_stInRegTbl.pfGetFwdAddrInVrf(vrfIndex, pstAddrInfo);
}

/* 在接口上获取一个合适的地址 */
BS_STATUS IN_IPAddr_GetPriorAddr
(
    IN IF_INDEX ifIndex,
    IN UINT uiAddrType, 
    OUT IPADDR_INFO_S *pstAddrInfo
)
{
    return g_stInRegTbl.pfGetPriorAddr(ifIndex, uiAddrType, pstAddrInfo);
}



