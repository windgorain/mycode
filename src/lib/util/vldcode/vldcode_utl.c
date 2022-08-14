/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-4-3
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_VLDCODE

#include "bs.h"

#include "utl/vldcode_utl.h"
#include "utl/hash_utl.h"
#include "utl/nap_utl.h"
#include "utl/rand_utl.h"
#include "utl/time_utl.h"

#define _VLDCODE_CODE_LEN  (4)  /* 校验码长度 */ 

/* 图片中每个字符的大小 */
#define _VLDCODE_BMP_WIDTH  32
#define _VLDCODE_BMP_HEIGHT 32

/* 验证码超时时间, 单位:秒 */
#define _VLDCODE_TIMEOUT_TIME 60

/* 两次遍历超时节点的最小时间间隔, 单位:秒 */
#define _VLDCODE_SCAN_FREE_TIME 3


typedef struct
{
    HASH_NODE_S stLinkNode;
    UINT ulClientId;     /* 用于标志客户端. 可以使用IP地址等 */
    time_t ulTime;
    CHAR szVldCode[_VLDCODE_CODE_LEN + 1];
}_VLDCODE_NODE_S;

typedef struct
{
    UINT ulMaxVldNum;
    UINT ulScanFreeTime;   /* 上一次遍历并释放超时节点的时间, 单位秒 */
    HANDLE hNapId;
    HANDLE hHashId;
}_VLDCODE_CTRL_S;

static UINT _VLDCODE_HashIndex(IN _VLDCODE_NODE_S *pstNode)
{
    return pstNode->ulClientId;
}

static INT  _VLDCODE_Cmp(IN _VLDCODE_NODE_S * pstNode1, IN _VLDCODE_NODE_S * pstNode2)
{
    return (INT)(pstNode1->ulClientId - pstNode2->ulClientId);
}

static inline VOID _VLDCODE_FreeTimeOutNode(IN _VLDCODE_CTRL_S *pstCtrl)
{
    _VLDCODE_NODE_S *pstNode = NULL;
    UINT uiIndex = NAP_INVALID_INDEX;
    time_t ulTimeNow= TM_NowInSec();

    while ((uiIndex = NAP_GetNextIndex(pstCtrl->hNapId, uiIndex))
            != NAP_INVALID_INDEX)
    {
        pstNode = NAP_GetNodeByIndex(pstCtrl->hNapId, uiIndex);

        if (ulTimeNow - pstNode->ulTime > _VLDCODE_TIMEOUT_TIME)
        {
            HASH_Del(pstCtrl->hHashId, (HASH_NODE_S*)pstNode);
            NAP_Free(pstCtrl->hNapId, pstNode);
        }
    }

    return;
}

HANDLE VLDCODE_CreateInstance(IN UINT ulMaxVldNum)
{
    _VLDCODE_CTRL_S *pstCtrl;

    if (ulMaxVldNum <= 0)
    {
        return 0;
    }

    pstCtrl = MEM_ZMalloc(sizeof(_VLDCODE_CTRL_S));
    if (NULL == pstCtrl)
    {
        return 0;
    }

    pstCtrl->hHashId = HASH_CreateInstance(NULL, ulMaxVldNum / 5 + 1, (PF_HASH_INDEX_FUNC)_VLDCODE_HashIndex);
    if (0 == pstCtrl->hHashId)
    {
        MEM_Free(pstCtrl);
        return 0;
    }

    NAP_PARAM_S param = {0};
    param.enType = NAP_TYPE_PTR_ARRAY;
    param.uiMaxNum = ulMaxVldNum;
    param.uiNodeSize = sizeof(_VLDCODE_NODE_S);

    pstCtrl->hNapId = NAP_Create(&param);
    if (pstCtrl->hNapId == NULL)
    {
        HASH_DestoryInstance(pstCtrl->hHashId);
        MEM_Free(pstCtrl);
        return NULL;
    }
    
    NAP_EnableSeq(pstCtrl->hNapId, 0, ulMaxVldNum);

    pstCtrl->ulMaxVldNum = ulMaxVldNum;

    return pstCtrl;
}

VOID VLDCODE_DelInstance(IN HANDLE hVldCodeInstance)
{
    _VLDCODE_CTRL_S *pstCtrl = (_VLDCODE_CTRL_S *)hVldCodeInstance;
    
    if (NULL == pstCtrl)
    {
        return;
    }

    if (0 != pstCtrl->hNapId)
    {
        NAP_Destory(pstCtrl->hNapId);
    }

    if (0 != pstCtrl->hHashId)
    {
        HASH_DestoryInstance(pstCtrl->hHashId);
    }

    MEM_Free(pstCtrl);
}

static BS_STATUS vldcode_AddNode(IN _VLDCODE_CTRL_S *pstCtrl, IN UINT uiClientId)
{
    _VLDCODE_NODE_S *pstNode;
    time_t ulTimeNow;

    pstNode = NAP_ZAlloc(pstCtrl->hNapId);
    if (NULL == pstNode)
    {
        /* 避免频繁扫面节点 */
        ulTimeNow= TM_NowInSec();
        if (ulTimeNow - pstCtrl->ulScanFreeTime <= _VLDCODE_SCAN_FREE_TIME)
        {
            return BS_ERR;
        }
    
        /* 申请不到节点, 扫描并释放超时的节点 */
        pstCtrl->ulScanFreeTime = (UINT)ulTimeNow;
        _VLDCODE_FreeTimeOutNode(pstCtrl);
        pstNode = NAP_ZAlloc(pstCtrl->hNapId);
        if (NULL == pstNode)
        {
            return BS_ERR;
        }
    }

    pstNode->ulTime = TM_NowInSec();
    pstNode->ulClientId = uiClientId;

    HASH_Add(pstCtrl->hHashId, (HASH_NODE_S*)pstNode);

    return BS_OK;
}

/* 产生一个随机的ClientID. 返回0表示失败 */
UINT VLDCODE_RandClientId(IN HANDLE hVldCodeInstance)
{
    _VLDCODE_CTRL_S *pstCtrl = (_VLDCODE_CTRL_S *)hVldCodeInstance;
    _VLDCODE_NODE_S *pstNode = NULL;
    UINT uiClientId = 0;
    _VLDCODE_NODE_S stNodeToFind;

    do {
        uiClientId = RAND_Get();
        if (uiClientId != 0)
        {
            stNodeToFind.ulClientId = uiClientId;
            pstNode = (_VLDCODE_NODE_S*) HASH_Find(pstCtrl->hHashId, (PF_HASH_CMP_FUNC)_VLDCODE_Cmp, (HASH_NODE_S*)&stNodeToFind);
        }
    }while ((uiClientId == 0) || (NULL != pstNode));

    if (BS_OK != vldcode_AddNode(pstCtrl, uiClientId))
    {
        return 0;
    }

    return uiClientId;
}

VLDBMP_S * VLDCODE_GenVldBmp
(
    IN HANDLE hVldCodeInstance,
    INOUT UINT *puiClientId/* 当clientID为0时,表示要自动产生一个 */
)
{
    _VLDCODE_CTRL_S *pstCtrl = (_VLDCODE_CTRL_S *)hVldCodeInstance;
    _VLDCODE_NODE_S *pstNode = NULL;
    VLDBMP_S *pstBmp = NULL;
    UINT uiClientId;
    _VLDCODE_NODE_S stNodeToFind;
    VLDBMP_OPT_S stOpt = {0};

    stOpt.uiLineRadii = 1;

    if (NULL == puiClientId)
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    if (*puiClientId == 0)
    {
        *puiClientId = VLDCODE_RandClientId(hVldCodeInstance);
        if (*puiClientId == 0)
        {
            return NULL;
        }
    }

    uiClientId = *puiClientId;

    Mem_Zero(&stNodeToFind, sizeof(_VLDCODE_NODE_S));
    stNodeToFind.ulClientId = uiClientId;

    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstNode = (_VLDCODE_NODE_S*) HASH_Find(pstCtrl->hHashId, (PF_HASH_CMP_FUNC)_VLDCODE_Cmp, (HASH_NODE_S*)&stNodeToFind);
    if (NULL == pstNode)
    {
        if (BS_OK != vldcode_AddNode(pstCtrl, uiClientId))
        {
            return NULL;
        }
    }

    VLDBMP_GenCode(_VLDCODE_CODE_LEN, pstNode->szVldCode);
    pstBmp = VLDBMP_CreateBmp(pstNode->szVldCode, _VLDCODE_BMP_WIDTH,
        _VLDCODE_BMP_HEIGHT, &stOpt);
    if (NULL == pstBmp)
    {
        HASH_Del(pstCtrl->hHashId, (HASH_NODE_S*)pstNode);
        NAP_Free(pstCtrl->hNapId, pstNode);
        return NULL;
    }

    return pstBmp;
}

VLDCODE_RET_E VLDCODE_Check(IN HANDLE hVldCodeInstance, IN UINT ulClientId, IN CHAR *pszCode)
{
    _VLDCODE_CTRL_S *pstCtrl = (_VLDCODE_CTRL_S *)hVldCodeInstance;
    _VLDCODE_NODE_S *pstNode = NULL;
    _VLDCODE_NODE_S stNodeToFind;
    VLDCODE_RET_E eRet;

    if ((NULL == pstCtrl) || (NULL == pszCode))
    {
        return VLDCODE_NOT_FOUND;
    }

    Mem_Zero(&stNodeToFind, sizeof(_VLDCODE_NODE_S));
    stNodeToFind.ulClientId = ulClientId;

    pstNode = (_VLDCODE_NODE_S*) HASH_Find(pstCtrl->hHashId, (PF_HASH_CMP_FUNC)_VLDCODE_Cmp, (HASH_NODE_S*)&stNodeToFind);
    if (NULL == pstNode)
    {
        return VLDCODE_NOT_FOUND;
    }

    eRet = VLDCODE_VALID;
    if (TM_NowInSec() - pstNode->ulTime > _VLDCODE_TIMEOUT_TIME)
    {
        eRet = VLDCODE_TIMEOUT;
    }
    else if (stricmp(pstNode->szVldCode, pszCode) != 0)
    {
        eRet = VLDCODE_INVALID;
    }

    HASH_Del(pstCtrl->hHashId, (HASH_NODE_S*)pstNode);
    NAP_Free(pstCtrl->hNapId, pstNode);

    return eRet;
}


