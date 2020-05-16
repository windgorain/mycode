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
    HANDLE_FUNC_X pfFunc;
}_FUNCTBL_NODE_S;

/* vars */
static DLL_HEAD_S g_astFuncHashTbl[_FUNCTBL_HASH_SIZE];
static SEM_HANDLE g_hSem = 0;

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

BS_STATUS FUNCTBL_Add(IN CHAR *pszFuncName, IN HANDLE_FUNC_X pfFunc, IN UINT ulRetType, IN CHAR *pszFmt)
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

    SEM_P(g_hSem, BS_WAIT, BS_WAIT_FOREVER);
    DLL_ADD(&g_astFuncHashTbl[ulHashIndex], pstNode);
    SEM_V(g_hSem);

    return BS_OK;
}

BS_STATUS FUNCTBL_Del(IN CHAR *pszFuncName)
{
    UINT ulHashIndex;
    _FUNCTBL_NODE_S *pstNode;
    
    BS_DBGASSERT(NULL != pszFuncName);

    ulHashIndex = _FUNCTBL_GetHashIndex(pszFuncName);

    SEM_P(g_hSem, BS_WAIT, BS_WAIT_FOREVER);
    DLL_SCAN(&g_astFuncHashTbl[ulHashIndex], pstNode)
    {
        if (strcmp(pstNode->pszFuncName, pszFuncName) == 0)
        {
            DLL_DEL(&g_astFuncHashTbl[ulHashIndex], pstNode);
            break;
        }
    }
    SEM_V(g_hSem);

    return BS_OK;
}

HANDLE_FUNC_X FUNCTBL_GetFunc(IN CHAR *pszFuncName, OUT UINT *pulRetType, OUT CHAR *pszFmt)
{
    UINT ulHashIndex;
    _FUNCTBL_NODE_S *pstNode;
    HANDLE_FUNC_X pfFunc = NULL;
    
    BS_DBGASSERT(NULL != pszFuncName);

    ulHashIndex = _FUNCTBL_GetHashIndex(pszFuncName);

    SEM_P(g_hSem, BS_WAIT, BS_WAIT_FOREVER);
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
    SEM_V(g_hSem);

    return pfFunc;
}


HANDLE FUNCTBL_Call(IN CHAR *pszFuncName, IN UINT ulArgsCount, ...)
{
    VOID * pArgs[_FUNCTBL_MAX_ARGS_NUM];
    UINT i;
    HANDLE_FUNC_X pfFunc;
	va_list list;

    if (ulArgsCount > _FUNCTBL_MAX_ARGS_NUM)
    {
        BS_WARNNING(("Args count is too large!"));
        return UINT_HANDLE(BS_ERR);
    }

    pfFunc = FUNCTBL_GetFunc(pszFuncName, NULL, NULL);

	if (NULL == pfFunc)
	{
		return UINT_HANDLE(BS_NO_SUCH);
	}

	va_start(list, ulArgsCount);
    
    for (i=0; i<ulArgsCount; i++)
    {
        pArgs[i] = va_arg(list, VOID*);
    }
    
	va_end(list);

    switch (ulArgsCount)
    {
        case 0:
            return ((HANDLE_FUNC)(pfFunc))();
            break;

        case 1:
            return pfFunc(pArgs[0]);
            break;

        case 2:
            return pfFunc(pArgs[0], pArgs[1]);
            break;

        case 3:
            return pfFunc(pArgs[0], pArgs[1], pArgs[2]);
            break;

        case 4:
            return pfFunc(pArgs[0], pArgs[1], pArgs[2], pArgs[3]);
            break;

        case 5:
            return pfFunc(pArgs[0], pArgs[1], pArgs[2], pArgs[3], pArgs[4]);
            break;

        case 6:
            return pfFunc(pArgs[0], pArgs[1], pArgs[2], pArgs[3], pArgs[4], pArgs[5]);
            break;

        case 7:
            return pfFunc(pArgs[0], pArgs[1], pArgs[2], pArgs[3], pArgs[4], pArgs[5], pArgs[6]);
            break;

        case 8:
            return pfFunc(pArgs[0], pArgs[1], pArgs[2], pArgs[3], pArgs[4], pArgs[5], pArgs[6], pArgs[7]);
            break;

        case 9:
            return pfFunc(pArgs[0], pArgs[1], pArgs[2], pArgs[3], pArgs[4], pArgs[5], pArgs[6], pArgs[7], pArgs[8]);
            break;

        case 10:
            return pfFunc(pArgs[0], pArgs[1], pArgs[2], pArgs[3], pArgs[4], pArgs[5], pArgs[6], pArgs[7], pArgs[8], pArgs[9]);
            break;

        default:
            BS_WARNNING(("Can't support args num: %d args!", ulArgsCount));
            return UINT_HANDLE(BS_ERR);
    }
}

BS_STATUS FUNCTBL_Init()
{
    UINT i;

    for (i=0; i<_FUNCTBL_HASH_SIZE; i++)
    {
        DLL_INIT(&g_astFuncHashTbl[i]);
    }

    if (0 == (g_hSem = SEM_CCreate("FuncTblSem", 1)))
    {
        BS_WARNNING(("Can't create sem for funcTbl!"));
        RETURN(BS_NO_RESOURCE);
    }

    return BS_OK;
}

