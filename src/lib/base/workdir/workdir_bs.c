/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2008-1-12
* Description: 有些线程需要修改当前工作路径.
*                 为了避免影响其他程序的工作，在更改期间需要锁定，不允许其他线程更改路径.
*                 但是更改路径的线程还可以再次更改. 
*                 直到恢复了原来的路径,其他线程才能够更改路径.
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sem_utl.h"
#include "utl/file_utl.h"

typedef struct
{
    DLL_NODE_S stLinkNode;  /* 必须是首节点 */
    CHAR szRestorePath[FILE_MAX_PATH_LEN + 1];
}_WORKDIR_STACK_S;

static DLL_HEAD_S g_stWorkDirStack = DLL_HEAD_INIT_VALUE(&g_stWorkDirStack);
static SEM_HANDLE g_hWorkDirSem = 0;

/* 设置当前路径,并把原来路径压栈 */
BS_STATUS WORKDIR_Push(IN CHAR *pszDir)
{
    _WORKDIR_STACK_S *pstWorkStackNode;

    if (NULL == (pstWorkStackNode = MEM_ZMalloc(sizeof(_WORKDIR_STACK_S))))
    {
        RETURN(BS_ERR);
    }

    SEM_P(g_hWorkDirSem, BS_WAIT, BS_WAIT_FOREVER);

    FILE_GET_CURRENT_DIRECTORY(pstWorkStackNode->szRestorePath, FILE_MAX_PATH_LEN);

    DLL_ADD(&g_stWorkDirStack, pstWorkStackNode);
    
    FILE_SET_CURRENT_DIRECTORY(pszDir);

    return BS_OK;
}

/* 恢复栈顶记录的目录,并且出栈 */
BS_STATUS WORKDIR_Pop()
{
    _WORKDIR_STACK_S *pstWorkStackNode;

    pstWorkStackNode = DLL_LAST(&g_stWorkDirStack);
    if (NULL == pstWorkStackNode)
    {
        RETURN(BS_EMPTY);
    }

    FILE_SET_CURRENT_DIRECTORY(pstWorkStackNode->szRestorePath);

    DLL_DEL(&g_stWorkDirStack, pstWorkStackNode);
    MEM_Free(pstWorkStackNode);

    SEM_V(g_hWorkDirSem);

    return BS_OK;    
}

/* 根据栈信息,恢复到最初的路径*/
BS_STATUS WORKDIR_PopAll()
{
    _WORKDIR_STACK_S *pstWorkStackNode, *pstWorkStackNodeNext;

    pstWorkStackNode = DLL_FIRST(&g_stWorkDirStack);
    if (NULL == pstWorkStackNode)
    {
        RETURN(BS_EMPTY);
    }

    FILE_SET_CURRENT_DIRECTORY(pstWorkStackNode->szRestorePath);

    DLL_SAFE_SCAN(&g_stWorkDirStack, pstWorkStackNode, pstWorkStackNodeNext)
    {
        DLL_DEL(&g_stWorkDirStack, pstWorkStackNode);
        MEM_Free(pstWorkStackNode);
        SEM_V(g_hWorkDirSem);
    }

    return BS_OK;
}

BS_STATUS WORKDIR_Init()
{
    g_hWorkDirSem =  SEM_MCreate("WorkdirSem");  /* 使用互斥信号量,保证只有一个线程改变工作路径 */
    if (g_hWorkDirSem == 0)
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}


