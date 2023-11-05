/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-9-10
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/my_ip_helper.h"

typedef struct
{
    PF_MY_IP_HELPER_NOTIFY_ADDR_CHANGE pfFunc;
    USER_HANDLE_S stUserHandle;
}_MY_IP_HELPER_ADDR_CHANGE_NOTIFY_S;

static _MY_IP_HELPER_ADDR_CHANGE_NOTIFY_S g_stMyIpHelperAddrChangeNotify;
static BOOL_T g_bMyIpHelperAddrChangeNotifyStart = FALSE;

#ifdef IN_WINDOWS

static BS_STATUS _os_my_ip_helper_Main(IN USER_HANDLE_S *pstUserHandle)
{
    DWORD dwError;
    HANDLE hHandle = pstUserHandle->ahUserHandle[0];
    
    while(1)
    {
        dwError=WaitForSingleObject(hHandle, 100000);

        if( dwError == WAIT_OBJECT_0) 
        {
            if (g_stMyIpHelperAddrChangeNotify.pfFunc != NULL)
            {
                g_stMyIpHelperAddrChangeNotify.pfFunc(&g_stMyIpHelperAddrChangeNotify.stUserHandle);
            }
        }
    }
}

static VOID _os_my_ip_helper_StartAddrChangeNotify()
{
    OVERLAPPED overLapped;
    HANDLE h2; 
    HANDLE hHandle;
    USER_HANDLE_S stUserHandle;
    
    hHandle=CreateEvent(NULL, 
        FALSE, 
        FALSE, 
        "IPChang.data" 
        ); 
    
    overLapped.hEvent=hHandle; 
    NotifyAddrChange(&h2,&overLapped);

    
    stUserHandle.ahUserHandle[0] = hHandle;

    THREAD_Create("MyIpHelper", NULL, _os_my_ip_helper_Main, &stUserHandle);
}

#endif

BS_STATUS My_Ip_Helper_RegAddrNotify
(
    IN PF_MY_IP_HELPER_NOTIFY_ADDR_CHANGE pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    BOOL_T bNeedStart = FALSE;
    
    if (NULL != pstUserHandle)
    {
        g_stMyIpHelperAddrChangeNotify.stUserHandle = *pstUserHandle;
    }

    g_stMyIpHelperAddrChangeNotify.pfFunc = pfFunc;

    SPLX_P();
    if (g_bMyIpHelperAddrChangeNotifyStart == FALSE)
    {
        g_bMyIpHelperAddrChangeNotifyStart = TRUE;
        bNeedStart = TRUE;
    }
    SPLX_V();

    if (bNeedStart)
    {
        _os_my_ip_helper_StartAddrChangeNotify();
    }

    return BS_OK;
}


