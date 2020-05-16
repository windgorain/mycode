/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-1-30
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_ITOR
    
#include "bs.h"

#include "utl/itor_utl.h"

typedef struct
{
    PF_ITOR_DEL_NOTIFY_FUNC pfDelNotifyFunc;
    USER_HANDLE_S stUserHandle;
    DLL_HEAD_S stListHead;
}_ITOR_INSTANCE_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
    HANDLE hHandle;
}_ITOR_S;

BS_STATUS ITOR_CreateInstance
(
    IN PF_ITOR_DEL_NOTIFY_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle,
    OUT HANDLE *phInstanceHandle
)
{
    _ITOR_INSTANCE_S *pstInstace;

    BS_DBGASSERT(NULL != phInstanceHandle);
    BS_DBGASSERT(NULL != pstUserHandle);

    pstInstace = MEM_ZMalloc(sizeof(_ITOR_INSTANCE_S));
    if (NULL == pstInstace)
    {
        RETURN(BS_NO_MEMORY);
    }

    pstInstace->pfDelNotifyFunc = pfFunc;
    pstInstace->stUserHandle = *pstUserHandle;
    DLL_INIT(&pstInstace->stListHead);

    *phInstanceHandle = pstInstace;

    return BS_OK;
}

BS_STATUS ITOR_DelInstace(IN HANDLE hInstanceId)
{
    _ITOR_INSTANCE_S *pstInstace = (_ITOR_INSTANCE_S*)hInstanceId;
    DLL_NODE_S *pstNode, *pstNodeTmp;

    if (NULL == pstInstace)
    {
        RETURN(BS_NULL_PARA);
    }

    DLL_SAFE_SCAN(&pstInstace->stListHead, pstNode, pstNodeTmp)
    {
        ITOR_DelItor(hInstanceId, pstNode);
    }
    
    MEM_Free(pstInstace);
    
    return BS_OK;
}

VOID ITOR_NotifyDelNode(IN HANDLE hInstanceId, IN VOID *pstNode2Del)
{
    _ITOR_INSTANCE_S *pstInstace = (_ITOR_INSTANCE_S*)hInstanceId;
    DLL_NODE_S *pstNode, *pstNodeTmp;

    if (NULL != pstInstace)
    {
        DLL_SAFE_SCAN(&pstInstace->stListHead, pstNode,pstNodeTmp)
        {
            pstInstace->pfDelNotifyFunc(pstNode2Del, hInstanceId, pstNode, &pstInstace->stUserHandle);
        }
    }
}

BS_STATUS ITOR_CreateItor(IN HANDLE hInstanceId, OUT HANDLE *phItorHandle)
{
    _ITOR_S *pstItor;
    _ITOR_INSTANCE_S *pstInstace = (_ITOR_INSTANCE_S*)hInstanceId;

    BS_DBGASSERT(NULL != pstInstace);

    pstItor = MEM_ZMalloc(sizeof(_ITOR_S));
    if (NULL == pstItor)
    {
        RETURN(BS_NO_MEMORY);
    }

    DLL_ADD(&pstInstace->stListHead, pstItor);

    *phItorHandle = pstItor;
    
    return BS_OK;
}

BS_STATUS ITOR_DelItor(IN HANDLE hInstance, IN HANDLE hItor)
{
    _ITOR_S *pstItor = (_ITOR_S*)hItor;
    _ITOR_INSTANCE_S *pstInstace = (_ITOR_INSTANCE_S*)hInstance;

    BS_DBGASSERT(NULL != pstInstace);

    if (NULL == pstItor)
    {
        RETURN(BS_NULL_PARA);
    }

    DLL_DEL(&pstInstace->stListHead, pstItor);
    MEM_Free(pstItor);

    return BS_OK;
}

BS_STATUS ITOR_SetHandle(IN HANDLE hItor, IN HANDLE hHandle)
{
    _ITOR_S *pstItor = (_ITOR_S*)hItor;

    BS_DBGASSERT(NULL != pstItor);

    pstItor->hHandle = hHandle;

    return BS_OK;
}

HANDLE ITOR_GetHandle(IN HANDLE hItor)
{
    _ITOR_S *pstItor = (_ITOR_S*)hItor;

    BS_DBGASSERT(NULL != pstItor);

    return pstItor->hHandle;
}

