/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-5-4
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_IPMACTBL
    
#include "bs.h"

#include "utl/hash_utl.h"
#include "utl/ipmac_tbl.h"


#define _IPMAC_TBL_HASH_BUCKET_NUM 1024


typedef struct
{
    HASH_S * hHashTbl;
}_IPMAC_TBL_CTRL_S;

static UINT _IPMAC_TBL_GetHashIndex (IN VOID *pstHashNode)
{
    IPMAC_TBL_NODE_S * pstNode = pstHashNode;

    return pstNode->uiIp;
}

static INT _IPMAC_TBL_CmpNode(IN IPMAC_TBL_NODE_S *pstNode1, IN IPMAC_TBL_NODE_S *pstNode2)
{
    return pstNode1->uiIp - pstNode2->uiIp;
}

IPMAC_HANDLE IPMAC_TBL_CreateInstance()
{
    _IPMAC_TBL_CTRL_S *pstIpMacTbl;

    pstIpMacTbl = MEM_ZMalloc(sizeof(_IPMAC_TBL_CTRL_S));
    if (NULL == pstIpMacTbl)
    {
        return NULL;
    }

    pstIpMacTbl->hHashTbl = HASH_CreateInstance(NULL, _IPMAC_TBL_HASH_BUCKET_NUM, _IPMAC_TBL_GetHashIndex);
    if (NULL == pstIpMacTbl->hHashTbl)
    {
        MEM_Free(pstIpMacTbl);
        return NULL;
    }

    return pstIpMacTbl;
}

VOID IPMAC_TBL_DelInstance(IN IPMAC_HANDLE hInstance)
{
    _IPMAC_TBL_CTRL_S *pstTbl = hInstance;

    if (NULL != pstTbl)
    {
        HASH_DestoryInstance(pstTbl->hHashTbl);
        MEM_Free(pstTbl);
    }
}

IPMAC_TBL_NODE_S * IPMAC_TBL_Find(IN IPMAC_HANDLE hInstance, IN UINT uiIp)
{
    _IPMAC_TBL_CTRL_S *pstTbl = hInstance;
    IPMAC_TBL_NODE_S stToFind;

    stToFind.uiIp = uiIp;

    return (IPMAC_TBL_NODE_S *) HASH_Find(pstTbl->hHashTbl, (PF_HASH_CMP_FUNC)_IPMAC_TBL_CmpNode, (HASH_NODE_S*)&stToFind);
}

IPMAC_TBL_NODE_S * IPMAC_TBL_Add(IN IPMAC_HANDLE hInstance, IN UINT uiIp, IN MAC_ADDR_S *pstMac)
{
    _IPMAC_TBL_CTRL_S *pstTbl = hInstance;
    IPMAC_TBL_NODE_S *pstNode = NULL;
    
    pstNode = IPMAC_TBL_Find(hInstance, uiIp);

    if (NULL == pstNode)
    {
        pstNode = MEM_ZMalloc(sizeof(IPMAC_TBL_NODE_S));
        if (NULL == pstNode)
        {
            return NULL;
        }

        pstNode->uiIp = uiIp;
        pstNode->stMac = *pstMac;
        HASH_Add(pstTbl->hHashTbl, (HASH_NODE_S*)pstNode);
    }
    else
    {
        pstNode->stMac = *pstMac;
    }

    return pstNode;
}

VOID IPMAC_TBL_Del(IN IPMAC_HANDLE hInstance, IN UINT uiIp)
{
    _IPMAC_TBL_CTRL_S *pstTbl = hInstance;
    IPMAC_TBL_NODE_S *pstNode;
    
    pstNode = IPMAC_TBL_Find(hInstance, uiIp);

    if (NULL == pstNode)
    {
        return;
    }

    HASH_Del(pstTbl->hHashTbl, (HASH_NODE_S*)pstNode);
    MEM_Free(pstNode);
}

static int ipmac_tbl_walkNode(IN void *hHashId, IN VOID *pstNode, IN VOID * pUserHandle)
{
    USER_HANDLE_S *pstUserHandle = pUserHandle;
    PF_IPMAC_TBL_WALK_FUNC pfFunc;

    pfFunc = pstUserHandle->ahUserHandle[0];

    pfFunc((IPMAC_TBL_NODE_S*)pstNode, pstUserHandle->ahUserHandle[1]);

    return 0;
}

VOID IPMAC_TBL_Walk(IN IPMAC_HANDLE hInstance, IN PF_IPMAC_TBL_WALK_FUNC pfFunc, IN VOID *pUserHandle)
{
    _IPMAC_TBL_CTRL_S *pstTbl = hInstance;
    USER_HANDLE_S stHandle;

    stHandle.ahUserHandle[0] = pfFunc;
    stHandle.ahUserHandle[1] = pUserHandle;

    HASH_Walk(pstTbl->hHashTbl, ipmac_tbl_walkNode, &stHandle);
}

