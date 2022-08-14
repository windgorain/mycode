/******************************************************************************
* Copyright (C), LiXingang
* Author:      Xingang.Li  Version: 1.0  Date: 2012-3-29
* Description: 
* History:     
******************************************************************************/
#include "ndis.h"

#include "vndis_def.h"
#include "vndis_pub.h"
#include "vndis_que.h"
#include "vndis_dev.h"
#include "vndis_adapter.h"
#include "vndis_instance.h"
#include "vndis_mem.h"

#define VNDIS_INSTANCE_ADAPTER_NAME "vndis"

typedef struct
{
    LIST_ENTRY      stAdapterList;
    NDIS_SPIN_LOCK  stLock;
}VNDIS_INSTANCE_LIST_S;


static VNDIS_INSTANCE_LIST_S g_stVndisInstanceList;


VOID VNDIS_Instance_Init()
{
    NdisZeroMemory(&g_stVndisInstanceList, sizeof(g_stVndisInstanceList));

    NdisAllocateSpinLock(&g_stVndisInstanceList.stLock);
    NdisInitializeListHead(&g_stVndisInstanceList.stAdapterList);

    return;
}

VOID VNDIS_Instance_UnInit()
{
    NdisFreeSpinLock(&g_stVndisInstanceList.stLock);

    return;
}

VOID VNDIS_Instance_Add(IN VNDIS_ADAPTER_S *pstAdapter)
{
    NdisAcquireSpinLock(&g_stVndisInstanceList.stLock);
    InsertHeadList(&g_stVndisInstanceList.stAdapterList, &pstAdapter->stListNode);
    NdisReleaseSpinLock(&g_stVndisInstanceList.stLock);
}

VOID VNDIS_Instance_Del(IN VNDIS_ADAPTER_S *pstAdapter)
{
    NdisAcquireSpinLock(&g_stVndisInstanceList.stLock);
    RemoveEntryList(&(pstAdapter->stListNode));
    NdisReleaseSpinLock(&g_stVndisInstanceList.stLock); 
}

VNDIS_ADAPTER_S * VNDIS_Instance_Search(IN PDEVICE_OBJECT pstDeviceObject)
{
    VNDIS_ADAPTER_S *pstAdapter;
    VNDIS_ADAPTER_S *pstAdapterFound = NULL;

    NdisAcquireSpinLock(&g_stVndisInstanceList.stLock);
    
    for (pstAdapter = (VNDIS_ADAPTER_S*) g_stVndisInstanceList.stAdapterList.Flink;
         pstAdapter != (VNDIS_ADAPTER_S*)&g_stVndisInstanceList.stAdapterList;
         pstAdapter = (VNDIS_ADAPTER_S*)pstAdapter->stListNode.Flink)
    {
        if (pstAdapter->stDev.pstDeviceObject == pstDeviceObject)
        {
            pstAdapterFound = pstAdapter;
            VNDIS_Adapter_IncUsing(pstAdapterFound);
            break;
        }
    }
    
    NdisReleaseSpinLock(&g_stVndisInstanceList.stLock);

    return pstAdapterFound;
}

VNDIS_ADAPTER_S * VNDIS_Instance_SearchByAdapter(IN VNDIS_ADAPTER_S *pstAdapterToFound)
{
    VNDIS_ADAPTER_S *pstAdapter;
    VNDIS_ADAPTER_S *pstAdapterFound = NULL;

    NdisAcquireSpinLock(&g_stVndisInstanceList.stLock);
    
    for (pstAdapter = (VNDIS_ADAPTER_S*) g_stVndisInstanceList.stAdapterList.Flink;
         pstAdapter != (VNDIS_ADAPTER_S*)&g_stVndisInstanceList.stAdapterList;
         pstAdapter = (VNDIS_ADAPTER_S*)pstAdapter->stListNode.Flink)
    {
        if (pstAdapter == pstAdapterToFound)
        {
            pstAdapterFound = pstAdapter;
            VNDIS_Adapter_IncUsing(pstAdapterFound);
            break;
        }
    }
    
    NdisReleaseSpinLock(&g_stVndisInstanceList.stLock);

    return pstAdapterFound;
}

