/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-4-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/netinfo_utl.h"

extern NETINFO_S * _NETINFO_GetNetInfo();

NETINFO_S * NETINFO_GetAllInfo()
{
    return _NETINFO_GetNetInfo();
}

VOID NETINFO_Free(IN NETINFO_S *pstNetInfo)
{
    MEM_Free(pstNetInfo);
}

BS_STATUS NETINFO_GetAdapterInfo(IN CHAR *pcAdapterName, OUT NETINFO_ADAPTER_S *pstAdapterInfo)
{
    NETINFO_S *pstNetInfo;
    UINT i;
    BS_STATUS eRet = BS_NO_SUCH;

#ifdef IN_WINDOWS
    pcAdapterName = pcAdapterName + sizeof("\\Device\\NPF_") - 1;
#endif

    pstNetInfo = NETINFO_GetAllInfo();
    if (NULL == pstNetInfo)
    {
        return BS_NO_SUCH;
    }

    for (i=0; i<pstNetInfo->uiAdapterNum; i++)
    {
        if (strcmp(pcAdapterName, pstNetInfo->astAdapter[i].szAdapterName) == 0)
        {
            *pstAdapterInfo = pstNetInfo->astAdapter[i];
            eRet = BS_OK;
            break;
        }
    }

    NETINFO_Free(pstNetInfo);

    return eRet;
}


