/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-2-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/exec_utl.h"

BS_STATUS THREAD_Display()
{
    THREAD_NAMED_ITER_S iter;
    THREAD_NAMED_INFO_S *info;

    EXEC_OutString(" ThreadName\r\n"
        "--------------------------------------------------------------------------\r\n");

    ThreadNamed_InitIter(&iter);

    while (NULL != (info = ThreadNamed_GetNext(&iter))) {
        EXEC_OutInfo(" %-16s\r\n", info->name);
    }

    EXEC_OutString("\r\n");

    return BS_OK;
}

THREAD_ID THREAD_Create
(
    IN CHAR  *pucName,
    IN THREAD_CREATE_PARAM_S *pstParam, /* 可以为NULL */
    IN PF_THREAD_NAMED_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle /* 可以为NULL */
)
{
    return ThreadNamed_Create(pucName, pstParam, pfFunc, pstUserHandle);
}

THREAD_ID THREAD_GetSelfID()
{
    return ThreadUtl_GetSelfID();
}

