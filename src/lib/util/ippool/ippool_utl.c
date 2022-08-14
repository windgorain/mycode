/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-2
* Description: 
* History:     
******************************************************************************/

/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_IPPOOL

#include "bs.h"
    
#include "utl/bitmap1_utl.h"
#include "utl/num_utl.h"
#include "utl/ip_list.h"
#include "utl/large_bitmap.h"
#include "utl/ippool_utl.h"

typedef struct
{
    IPLIST_S stIpList;
    LBITMAP_HANDLE hUsedOrDenyBitmap;     /* 地址池位图, 用来表示地址池中哪些地址被使用或禁止使用了 */
    LBITMAP_HANDLE hDenyBitmap;     /* 地址池位图,  用来表示不能分配的IP */
}_IP_POOL_S;

static UINT ippool_GetFreeIP(IN _IP_POOL_S *pstIpPool)
{
    UINT uiBeginIP, uiEndIP;
    UINT uiIP;

    IPLIST_SCAN_BEGIN(&pstIpPool->stIpList, uiBeginIP, uiEndIP)
    {
        if (BS_OK == LBitMap_AllocByRange(pstIpPool->hUsedOrDenyBitmap, uiBeginIP, uiEndIP, &uiIP))
        {
            return uiIP;
        }
    }IPLIST_SCAN_END();

    return 0;
}

IPPOOL_HANDLE IPPOOL_Create()
{
    _IP_POOL_S *pstIpPool;

    pstIpPool = MEM_ZMalloc(sizeof(_IP_POOL_S));
    if (NULL == pstIpPool)
    {
        return NULL;
    }

    IPList_Init(&pstIpPool->stIpList);
    pstIpPool->hUsedOrDenyBitmap = LBitMap_Create(NULL);
    pstIpPool->hDenyBitmap = LBitMap_Create(NULL);

    if ((NULL == pstIpPool->hUsedOrDenyBitmap) || (NULL == pstIpPool->hDenyBitmap))
    {
        IPPOOL_Destory(pstIpPool);
        return NULL;
    }    

    return pstIpPool;
}

VOID IPPOOL_Destory(IN IPPOOL_HANDLE hIpPoolHandle)
{
    _IP_POOL_S *pstIpPool = (_IP_POOL_S *)hIpPoolHandle;

    if (pstIpPool == NULL)
    {
        return;
    }

    if (pstIpPool->hUsedOrDenyBitmap != NULL)
    {
        LBitMap_Destory(pstIpPool->hUsedOrDenyBitmap);
        pstIpPool->hUsedOrDenyBitmap = NULL;
    }

    if (pstIpPool->hDenyBitmap != NULL)
    {
        LBitMap_Destory(pstIpPool->hDenyBitmap);
        pstIpPool->hDenyBitmap = NULL;
    }

    IPList_Finit(&pstIpPool->stIpList);

    MEM_Free(pstIpPool);
}

/* 添加一个地址池 */
BS_STATUS IPPOOL_AddRange
(
    IN IPPOOL_HANDLE hIpPoolHandle,
    IN UINT uiBeginIP/* 主机序, 含有此IP */,
    IN UINT uiEndIP/* 主机序, 含有此IP */
)
{
    _IP_POOL_S *pstIpPool = (_IP_POOL_S *)hIpPoolHandle;

    return IPList_AddRange(&pstIpPool->stIpList, uiBeginIP, uiEndIP);
}

BS_STATUS IPPOOL_DelRange
(
    IN IPPOOL_HANDLE hIpPoolHandle,
    IN UINT uiBeginIP/* 主机序*/,
    IN UINT uiEndIP/* 主机序*/
)
{
    _IP_POOL_S *pstIpPool = (_IP_POOL_S *)hIpPoolHandle;

    return IPList_DelRange(&pstIpPool->stIpList, uiBeginIP, uiEndIP);
}

BS_STATUS IPPOOL_ModifyRange
(
    IN IPPOOL_HANDLE hIpPoolHandle,
    IN UINT uiOldBeginIP/* 主机序*/,
    IN UINT uiOldEndIP/* 主机序*/,
    IN UINT uiBeginIP/* 主机序*/,
    IN UINT uiEndIP/* 主机序*/
)
{
    _IP_POOL_S *pstIpPool = (_IP_POOL_S *)hIpPoolHandle;
    
    return IPList_ModifyRange(&pstIpPool->stIpList, uiOldBeginIP, uiOldEndIP, uiBeginIP, uiEndIP);
}

/* 判断是否和已经存在的地址池有重叠 */
BOOL_T IPPOOL_IsOverlap
(
    IN IPPOOL_HANDLE hIpPoolHandle,
    IN UINT uiBeginIP/* 主机序*/,
    IN UINT uiEndIP/* 主机序*/
)
{
    _IP_POOL_S *pstIpPool = (_IP_POOL_S *)hIpPoolHandle;
    
    return IPLIst_IsOverlap(&pstIpPool->stIpList, uiBeginIP, uiEndIP);
}

/* 返回地址, 主机序 */
UINT IPPOOL_AllocIP
(
    IN IPPOOL_HANDLE hIpPoolHandle,
    IN UINT uiRequestIp/* 优先分配这个IP,如果不行,则自动分配一个. 主机序 */
)
{
    _IP_POOL_S *pstIpPool = (_IP_POOL_S *)hIpPoolHandle;

    if (pstIpPool == NULL)
    {
        return 0;
    }

    if ((uiRequestIp != 0)
        && (TRUE == IPList_IsIPInTheList(&pstIpPool->stIpList, uiRequestIp))
        && (FALSE == LBitMap_IsBitSetted(pstIpPool->hUsedOrDenyBitmap, uiRequestIp)))
    {
        LBitMap_SetBit(pstIpPool->hUsedOrDenyBitmap, uiRequestIp);
        return uiRequestIp;
    }

    return ippool_GetFreeIP(pstIpPool);
}


/* 申请特定IP */
BS_STATUS IPPOOL_AllocSpecIP(IN IPPOOL_HANDLE hIpPoolHandle, IN UINT uiSpecIp/* 主机序 */)
{
    _IP_POOL_S *pstIpPool = (_IP_POOL_S *)hIpPoolHandle;

    if ((pstIpPool == NULL) || (uiSpecIp == 0))
    {
        return BS_ERR;
    }

    if (TRUE != IPList_IsIPInTheList(&pstIpPool->stIpList, uiSpecIp))
    {
        return BS_OUT_OF_RANGE;
    }

    if (TRUE == LBitMap_IsBitSetted(pstIpPool->hUsedOrDenyBitmap, uiSpecIp))
    {
        return BS_ALREADY_EXIST;
    }

    LBitMap_SetBit(pstIpPool->hUsedOrDenyBitmap, uiSpecIp);

    return BS_OK;
}


VOID IPPOOL_FreeIP(IN IPPOOL_HANDLE hIpPoolHandle, IN UINT uiIP/* 主机序 */)
{
    _IP_POOL_S *pstIpPool = (_IP_POOL_S *)hIpPoolHandle;
    
    LBitMap_ClrBit(pstIpPool->hUsedOrDenyBitmap, uiIP);

    return;
}

/* 将IP设置为禁止分配 */
VOID IPPOOL_Deny(IN IPPOOL_HANDLE hIpPoolHandle, IN UINT uiDenyIp /* 主机序 */)
{
    _IP_POOL_S *pstIpPool = (_IP_POOL_S *)hIpPoolHandle;

    LBitMap_SetBit(pstIpPool->hUsedOrDenyBitmap, uiDenyIp);
    LBitMap_SetBit(pstIpPool->hDenyBitmap, uiDenyIp);
}

VOID IPPOOL_Permit(IN IPPOOL_HANDLE hIpPoolHandle, IN UINT uiIP /* 主机序 */)
{
    _IP_POOL_S *pstIpPool = (_IP_POOL_S *)hIpPoolHandle;\

    if (LBitMap_IsBitSetted(pstIpPool->hDenyBitmap, uiIP))
    {
        LBitMap_ClrBit(pstIpPool->hUsedOrDenyBitmap, uiIP);
        LBitMap_ClrBit(pstIpPool->hDenyBitmap, uiIP);
    }
}

VOID IPPOOL_PermitAll(IN IPPOOL_HANDLE hIpPoolHandle)
{
    _IP_POOL_S *pstIpPool = (_IP_POOL_S *)hIpPoolHandle;
    UINT uiIP = 0;

    while (BS_OK == LBitMap_GetNextBusyBit(pstIpPool->hDenyBitmap, uiIP, &uiIP))
    {
        LBitMap_ClrBit(pstIpPool->hUsedOrDenyBitmap, uiIP);
        LBitMap_ClrBit(pstIpPool->hDenyBitmap, uiIP);
    }
}

VOID IPPOOL_WalkBusy
(
    IN IPPOOL_HANDLE hIpPoolHandle,
    IN PF_IPPOOL_WALK_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    _IP_POOL_S *pstIpPool = (_IP_POOL_S *)hIpPoolHandle;
    UINT uiIp = 0;

    while (BS_OK == LBitMap_GetNextBusyBit(pstIpPool->hUsedOrDenyBitmap, uiIp, &uiIp))
    {
        if (! LBitMap_IsBitSetted(pstIpPool->hDenyBitmap, uiIp))
        {
            pfFunc(uiIp, hUserHandle);
        }
    }
}

