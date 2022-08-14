/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-12-3
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_FUNCTBL

#include "bs.h"

#include "utl/sem_utl.h"
#include "utl/txt_utl.h"
    


/* define */
#define _FUNCTBL_HASH_SIZE 256
#define _FUNCTBL_MAX_ARGS_NUM 10   /* 函数最多的参数个数 */

/* struct */
typedef struct
{
    DLL_NODE_S stLinkNode;  /* 必须是第一个成员 */
    CHAR *pszFuncName;
    CHAR *pszFmt;
    UINT ulRetType;   /*0 - VOID, 1 - string, 2 - UINT, 3 - BOOL*/
    void *pfFunc;
}_FUNCTBL_NODE_S;

/* vars */
static DLL_HEAD_S g_astFuncHashTbl[_FUNCTBL_HASH_SIZE];
static MUTEX_S g_functbl_lock;

UINT _FUNCTBL_GetHashIndex(IN CHAR *pszFuncName)
{
    UINT ulHashIndex;
    CHAR *pcCh;

    pcCh = pszFuncName;

    ulHashIndex = 0;
    
    while (*pcCh != '\0')
    {
        ulHashIndex += *pcCh;
        pcCh ++;
    }

    return (ulHashIndex % _FUNCTBL_HASH_SIZE);
}

BS_STATUS FUNCTBL_Add(CHAR *pszFuncName, void *pfFunc, UINT ulRetType, CHAR *pszFmt)
{
    UINT ulHashIndex;
    _FUNCTBL_NODE_S *pstNode;
    
    BS_DBGASSERT(NULL != pszFuncName);

    pstNode = MEM_ZMalloc(sizeof(_FUNCTBL_NODE_S));
    if (NULL == pstNode)
    {
        RETURN(BS_NO_MEMORY);
    }
    
    pstNode->pszFuncName = pszFuncName;
    pstNode->pszFmt = pszFmt;
    pstNode->ulRetType = ulRetType;
    pstNode->pfFunc = pfFunc;

    ulHashIndex = _FUNCTBL_GetHashIndex(pszFuncName);

    MUTEX_P(&g_functbl_lock);
    DLL_ADD(&g_astFuncHashTbl[ulHashIndex], pstNode);
    MUTEX_V(&g_functbl_lock);

    return BS_OK;
}

BS_STATUS FUNCTBL_Del(IN CHAR *pszFuncName)
{
    UINT ulHashIndex;
    _FUNCTBL_NODE_S *pstNode;
    
    BS_DBGASSERT(NULL != pszFuncName);

    ulHashIndex = _FUNCTBL_GetHashIndex(pszFuncName);

    MUTEX_P(&g_functbl_lock);
    DLL_SCAN(&g_astFuncHashTbl[ulHashIndex], pstNode)
    {
        if (strcmp(pstNode->pszFuncName, pszFuncName) == 0)
        {
            DLL_DEL(&g_astFuncHashTbl[ulHashIndex], pstNode);
            break;
        }
    }
    MUTEX_V(&g_functbl_lock);

    return BS_OK;
}

void * FUNCTBL_GetFunc(IN CHAR *pszFuncName, OUT UINT *pulRetType, OUT CHAR *pszFmt)
{
    UINT ulHashIndex;
    _FUNCTBL_NODE_S *pstNode;
    void *pfFunc = NULL;
    
    BS_DBGASSERT(NULL != pszFuncName);

    ulHashIndex = _FUNCTBL_GetHashIndex(pszFuncName);

    MUTEX_P(&g_functbl_lock);
    DLL_SCAN(&g_astFuncHashTbl[ulHashIndex], pstNode)
    {
        if (strcmp(pstNode->pszFuncName, pszFuncName) == 0)
        {
            pfFunc = pstNode->pfFunc;
            if (NULL != pszFmt)
            {
                TXT_StrCpy(pszFmt, pstNode->pszFmt);
            }
            if (NULL != pulRetType)
            {
                *pulRetType = pstNode->ulRetType;
            }
            break;
        }
    }
    MUTEX_V(&g_functbl_lock);

    return pfFunc;
}


HANDLE FUNCTBL_Call(IN CHAR *pszFuncName, IN UINT ulArgsCount, ...)
{
    U64 args[_FUNCTBL_MAX_ARGS_NUM];
    UINT i;
    U64 ret;
    PF_FUNCTBL_FUNC_X pfFunc;
	va_list list;

    if (ulArgsCount > _FUNCTBL_MAX_ARGS_NUM)
    {
        BS_WARNNING(("Args count is too large!"));
        return UINT_HANDLE(BS_ERR);
    }

    pfFunc = FUNCTBL_GetFunc(pszFuncName, NULL, NULL);

	if (NULL == pfFunc) {
		return UINT_HANDLE(BS_NO_SUCH);
	}

	va_start(list, ulArgsCount);
    for (i=0; i<ulArgsCount; i++) {
        args[i] = (ULONG)va_arg(list, VOID*);
    }
	va_end(list);

    ret = pfFunc(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
    return (void*)(ULONG)ret;
}

static void functbl_init()
{
    UINT i;

    MUTEX_Init(&g_functbl_lock);

    for (i=0; i<_FUNCTBL_HASH_SIZE; i++)
    {
        DLL_INIT(&g_astFuncHashTbl[i]);
    }
}

CONSTRUCTOR(init) {
    functbl_init();
}

