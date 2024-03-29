/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-5
* Description: IP范围表
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sprintf_utl.h"
#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/num_utl.h"
#include "utl/socket_utl.h"
#include "utl/ip_list.h"


static BS_STATUS _iplist_SingleString2Ip(IN LSTR_S *pstString, OUT UINT *puiIp)
{
    CHAR szTmp[32];
    UINT uiIp;

    LSTR_Strim(pstString, TXT_BLANK_CHARS, pstString);

    if ((pstString->uiLen == 0) || (pstString->uiLen >= sizeof(szTmp)))
    {
        return BS_ERR;
    }

    memcpy(szTmp, pstString->pcData, pstString->uiLen);
    szTmp[pstString->uiLen] = '\0';

    if (TRUE != Socket_IsIPv4(szTmp))
    {
        return BS_ERR;
    }

    uiIp = Socket_NameToIpHost(szTmp);

    *puiIp = uiIp;

    return BS_OK;
}



static VOID _iplist_ParseGroupString(IN IPLIST_S *pstList, IN LSTR_S *pstIpGroupString)
{
    LSTR_S stTrimed;
    CHAR *pcSplit;
    UINT uiBegin;
    UINT uiEnd;
    UINT uiTmp;
    LSTR_S stTmp;

    LSTR_Strim(pstIpGroupString, TXT_BLANK_CHARS, &stTrimed);

    if (stTrimed.uiLen == 0) {
        return;
    }
    
    pcSplit = TXT_MStrnchr(stTrimed.pcData, stTrimed.uiLen, "-/");
    if (NULL == pcSplit) {   
        if (BS_OK != _iplist_SingleString2Ip(&stTrimed, &uiBegin)) {
            return;
        }
        uiEnd = uiBegin;
    } else {
        stTmp.pcData = stTrimed.pcData;
        stTmp.uiLen = pcSplit - stTrimed.pcData;
        if (BS_OK != _iplist_SingleString2Ip(&stTmp, &uiBegin)) {
            return;
        }

        if (*pcSplit == '-') { 
            stTmp.pcData = pcSplit + 1;
            stTmp.uiLen = (stTrimed.pcData + stTrimed.uiLen) - stTmp.pcData;
            if (BS_OK != _iplist_SingleString2Ip(&stTmp, &uiEnd)) {
                return;
            }
        } else { 
            UCHAR prefix;
            UINT mask;
            stTmp.pcData = pcSplit + 1;
            stTmp.uiLen = (stTrimed.pcData + stTrimed.uiLen) - stTmp.pcData;
            prefix = LSTR_A2ui(&stTmp);
            mask = PREFIX_2_MASK(prefix);
            IpMask_2_Range(uiBegin, mask, &uiBegin, &uiEnd);
        }
    }

    if (uiBegin > uiEnd) {
        uiTmp = uiBegin;
        uiBegin = uiEnd;
        uiEnd = uiTmp;
    }

    IPList_AddRange(pstList, uiBegin, uiEnd);

    return;
}

static INT _iplist_Cmp(IN DLL_NODE_S *pstNode1, IN DLL_NODE_S *pstNode2, IN HANDLE hHandle)
{
    IPLIST_NODE_S *pstIpListNode1 = (VOID*)pstNode1;
    IPLIST_NODE_S *pstIpListNode2 = (VOID*)pstNode2;

    if (pstIpListNode1->uiBeginIp > pstIpListNode2->uiBeginIp)
    {
        return 1;
    }

    if (pstIpListNode1->uiBeginIp < pstIpListNode2->uiBeginIp)
    {
        return -1;
    }

    if (pstIpListNode1->uiEndIp < pstIpListNode2->uiEndIp)
    {
        return 1;
    }

    if (pstIpListNode1->uiEndIp > pstIpListNode2->uiEndIp)
    {
        return -1;
    }

    return 0;
}

VOID IPList_Finit(IN IPLIST_S *pstList)
{
    IPLIST_NODE_S *pstNode, *pstNodeTmp;

    DLL_SAFE_SCAN(&pstList->stList, pstNode, pstNodeTmp)
    {
        DLL_DEL(&pstList->stList, pstNode);
        MEM_Free(pstNode);
    }
}


BS_STATUS IPList_ParseString(IN IPLIST_S *pstList, IN LSTR_S *pstIpStrList)
{
    LSTR_S stGroupString;

    LSTR_SCAN_ELEMENT_BEGIN(pstIpStrList->pcData, pstIpStrList->uiLen, ',', &stGroupString)
    {
        _iplist_ParseGroupString(pstList, &stGroupString);
    }LSTR_SCAN_ELEMENT_END();

    return BS_OK;
}


BOOL_T IPLIst_IsOverlap(IN IPLIST_S *pstList, IN UINT uiStartIP, IN UINT uiEndIP)
{
    UINT uiStartIPTmp, uiEndIPTmp;

    IPLIST_SCAN_BEGIN(pstList, uiStartIPTmp, uiEndIPTmp)
    {
        if (NUM_AREA_IS_OVERLAP2(uiStartIPTmp, uiEndIPTmp, uiStartIP, uiEndIP))
        {
            return TRUE;
        }
    }IPLIST_SCAN_END();

    return FALSE;
}


BOOL_T IPList_IsIPInTheList(IN IPLIST_S *pstList, IN UINT uiIP)
{
    UINT uiStartIP, uiEndIP;

    IPLIST_SCAN_BEGIN(pstList, uiStartIP, uiEndIP)
    {
        if ((uiIP >= uiStartIP) && (uiIP <= uiEndIP))
        {
            return TRUE;
        }
    }IPLIST_SCAN_END();

    return FALSE;
}

BS_STATUS IPList_AddRange(IN IPLIST_S *pstList, IN UINT uiBeginIP, IN UINT uiEndIP)
{
    IPLIST_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(IPLIST_NODE_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    pstNode->uiBeginIp = uiBeginIP;
    pstNode->uiEndIp = uiEndIP;

    DLL_ADD(&pstList->stList, pstNode);

    return BS_OK;
}


BS_STATUS IPList_DelRange(IN IPLIST_S *pstList, IN UINT uiBeginIP, IN UINT uiEndIP)
{
    IPLIST_NODE_S *pstNode;

    DLL_SCAN(&pstList->stList, pstNode)
    {
        if ((pstNode->uiBeginIp == uiBeginIP) && (pstNode->uiEndIp == uiEndIP))
        {
            DLL_DEL(&pstList->stList, pstNode);
            return BS_OK;
        }
    }

    return BS_NO_SUCH;
}

BS_STATUS IPList_ModifyRange
(
    IN IPLIST_S *pstList,
    IN UINT uiOldBeginIP,
    IN UINT uiOldEndIP,
    IN UINT uiBeginIP,
    IN UINT uiEndIP
)
{
    IPLIST_NODE_S *pstNode;
    
    DLL_SCAN(&pstList->stList, pstNode)
    {
        if ((pstNode->uiBeginIp == uiOldBeginIP) && (pstNode->uiEndIp == uiOldEndIP))
        {
            pstNode->uiBeginIp = uiBeginIP;
            pstNode->uiEndIp = uiEndIP;
            return BS_OK;
        }
    }

    return BS_NOT_FOUND;
}


VOID IPLIst_Cat(INOUT IPLIST_S *pstListDst, INOUT IPLIST_S *pstListSrc)
{
    DLL_Cat(&pstListDst->stList, &pstListSrc->stList);
}


VOID IPList_Sort(IN IPLIST_S *pstList)
{
    DLL_Sort(&pstList->stList, _iplist_Cmp, 0);
}


VOID IPList_Compress(IN IPLIST_S *pstList)
{
    IPLIST_NODE_S *pstNode, *pstNodeNext;
    
    
    IPList_Sort(pstList);

    
    pstNode = DLL_FIRST(&pstList->stList);

    if (NULL == pstNode)
    {
        return;
    }

    while(1)
    {
        pstNodeNext = DLL_NEXT(&pstList->stList, pstNode);
        if (NULL == pstNodeNext)
        {
            break;
        }

        if (pstNodeNext->uiEndIp <= pstNode->uiEndIp)
        {
            
            DLL_DEL(&pstList->stList, pstNodeNext);
            MEM_Free(pstNodeNext);
            continue;
        }

        if (pstNodeNext->uiBeginIp <= pstNode->uiEndIp)
        {
            pstNodeNext->uiBeginIp = pstNode->uiEndIp + 1;
        }

        pstNode = pstNodeNext;
    }

    return;
}


VOID IPList_Continue(IN IPLIST_S *pstList)
{
    IPLIST_NODE_S *pstNode, *pstNodeNext;

    
    IPList_Compress(pstList);

    
    pstNode = DLL_FIRST(&pstList->stList);

    if (NULL == pstNode)
    {
        return;
    }

    while(1)
    {
        pstNodeNext = DLL_NEXT(&pstList->stList, pstNode);
        if (NULL == pstNodeNext)
        {
            break;
        }

        if (pstNodeNext->uiBeginIp == pstNode->uiEndIp + 1)
        {
            pstNode->uiEndIp = pstNodeNext->uiEndIp;
            DLL_DEL(&pstList->stList, pstNodeNext);
            MEM_Free(pstNodeNext);
            continue;
        }

        pstNode = pstNodeNext;
    }

    return;
}


BS_STATUS IPList_Range2SubNetList
(
    INOUT IPLIST_S *pstList,
    IN UINT uiBeginIP,
    IN UINT uiEndIP
)
{
    UINT uiMask;
    UINT uiBeginTmp = uiBeginIP;
    UINT uiStartIp;
    UINT uiStopIp;
    BS_STATUS eRet = BS_OK;

    while (uiBeginTmp <= uiEndIP)
    {
        uiMask = Range_GetFirstMask(uiBeginTmp, uiEndIP);
        IpMask_2_Range(uiBeginTmp, uiMask, &uiStartIp, &uiStopIp);
        eRet |= IPList_AddRange(pstList, uiStartIp, uiStopIp);
        uiBeginTmp = uiStopIp + 1;
    }

    return eRet;
}


BS_STATUS IPLIst_SplitBySubNet(INOUT IPLIST_S *pstList)
{
    IPLIST_S stTmpList;
    IPLIST_NODE_S *pstNode;
    BS_STATUS eRet = BS_OK;
    
    IPList_Init(&stTmpList);

    DLL_SCAN(&pstList->stList, pstNode)
    {
        eRet |= IPList_Range2SubNetList(&stTmpList, pstNode->uiBeginIp, pstNode->uiEndIp);
    }

    if (eRet == BS_OK)
    {
        IPList_Finit(pstList);
        IPLIst_Cat(pstList, &stTmpList);
    }

    IPList_Finit(&stTmpList);

    return eRet;
}

BS_STATUS IPLIst_ToString(IN IPLIST_S *pstList, OUT CHAR *pcString, IN UINT uiSize)
{
    IPLIST_NODE_S *pstNode;
    CHAR *pcTmp = pcString;
    UINT uiSizeTmp = uiSize;
    UINT uiBeginIp, uiEndIp;
    INT iLen;

    DLL_SCAN(&pstList->stList, pstNode)
    {
        uiBeginIp = htonl(pstNode->uiBeginIp);
        uiEndIp = htonl(pstNode->uiEndIp);

        if (uiBeginIp == uiEndIp)
        {        
            iLen = BS_Snprintf(pcTmp, uiSizeTmp, "%pI4", &uiBeginIp);
        }
        else
        {
            iLen = BS_Snprintf(pcTmp, uiSizeTmp, "%pI4-%pI4", &uiBeginIp, &uiEndIp);
        }

        if ((UINT)iLen >= uiSizeTmp)
        {
            break;
        }

        pcTmp += iLen;
        uiSizeTmp -= iLen;

        if (DLL_NEXT(&pstList->stList, pstNode) != NULL)
        {
            iLen = scnprintf(pcTmp, uiSizeTmp, ",");
            if ((UINT)iLen >= uiSizeTmp)
            {
                break;
            }
            pcTmp += iLen;
            uiSizeTmp -= iLen;
        }
    }

    return BS_OK;
}

