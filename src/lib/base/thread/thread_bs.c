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

    EXEC_OutString(" ThreadID   ThreadName\r\n"
        "--------------------------------------------------------------------------\r\n");

    ThreadNamed_InitIter(&iter);

    while (NULL != (info = ThreadNamed_GetNext(&iter))) {
        EXEC_OutInfo(" %-10u %-16s\r\n", info->thread_id, info->name);
    }

    EXEC_OutString("\r\n");

    return BS_OK;
}

THREAD_ID _thread_create
(
    IN CHAR  *pucName,
    IN THREAD_CREATE_PARAM_S *pstParam, 
    IN PF_THREAD_NAMED_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle 
)
{
    return ThreadNamed_Create(pucName, pstParam, pfFunc, pstUserHandle);
}

void _thread_reg_ob(THREAD_NAMED_OB_S *ob)
{
    ThreadNamed_RegOb(ob);
}

