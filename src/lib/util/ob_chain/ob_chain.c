/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-3
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/ob_chain.h"


BS_STATUS OB_CHAIN_AddWithPri
(
    IN OB_CHAIN_S *pstHead,
    IN UINT uiPri,
    IN UINT_FUNC_X pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    OB_CHAIN_NODE_S *pstNode;
    OB_CHAIN_NODE_S *pstNodeTmp;

    pstNode = MEM_ZMalloc(sizeof (OB_CHAIN_NODE_S));
    if (NULL == pstNode)
    {
        return (BS_NO_MEMORY);
    }

    pstNode->pfFunc = pfFunc;
    pstNode->uiPri = uiPri;
    if (NULL != pstUserHandle)
    {
        pstNode->stUserHandle = *pstUserHandle;
    }

    DLL_SCAN(pstHead, pstNodeTmp)
    {
        if (uiPri < pstNodeTmp->uiPri)
        {
            DLL_INSERT_BEFORE(pstHead, pstNode, pstNodeTmp);
            return BS_OK;
        }
    }

    DLL_ADD(pstHead, pstNode);

    return BS_OK;
}

BS_STATUS OB_CHAIN_Add (IN OB_CHAIN_S *pstHead, IN UINT_FUNC_X pfFunc, IN USER_HANDLE_S *pstUserHandle)
{
    return OB_CHAIN_AddWithPri(pstHead, 0xffffffff, pfFunc, pstUserHandle);
}

BS_STATUS OB_CHAIN_Del (IN OB_CHAIN_S *pstHead, IN UINT_FUNC_X pfFunc)
{
    OB_CHAIN_NODE_S *pstNode;

    DLL_SCAN (pstHead, pstNode)
    {
        if (pstNode->pfFunc == pfFunc)
        {
            DLL_DEL (pstHead, pstNode);
            MEM_Free(pstNode);
            return BS_OK;
        }
    }

    return BS_OK;
}

BS_STATUS OB_CHAIN_DelAll(IN OB_CHAIN_S *pstHead)
{
    OB_CHAIN_NODE_S *pstNode, *pstNodeNext;

    DLL_SAFE_SCAN (pstHead, pstNode, pstNodeNext)
    {
        DLL_DEL (pstHead, pstNode);
        MEM_Free(pstNode);
    }

    return BS_OK;
}


