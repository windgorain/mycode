/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2008-3-23
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_SOCKSES

#include "bs.h"

#include "utl/sem_utl.h"
#include "utl/hookapi.h"
#include "utl/rbuf.h"
#include "utl/sock_ses.h"

typedef struct
{
    SEM_HANDLE hSem;
    DLL_HEAD_S stSesList;
}_SOCK_SES_HEAD_S;


BS_STATUS SockSes_AddNode(IN HANDLE hSockSesId, IN UINT ulSslTcpSideA, IN UINT ulSslTcpSideB)
{
    _SOCK_SES_HEAD_S *pstSesHead = (_SOCK_SES_HEAD_S *)hSockSesId;
    SOCK_SES_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(SOCK_SES_S));
    if (NULL == pstNode)
    {
        RETURN(BS_NO_MEMORY);
    }    

    RBUF_Create(1024, &pstNode->stSideA.hRbufId);
    RBUF_Create(1024, &pstNode->stSideB.hRbufId);

    pstNode->stSideA.ulSslTcpId = ulSslTcpSideA;
    pstNode->stSideB.ulSslTcpId = ulSslTcpSideB;

    if (0 != pstSesHead->hSem)
    {
        SEM_P(pstSesHead->hSem, BS_WAIT, BS_WAIT_FOREVER);
    }

    DLL_ADD((DLL_HEAD_S*)&pstSesHead->stSesList, pstNode);

    if (0 != pstSesHead->hSem)
    {
        SEM_V(pstSesHead->hSem);
    }

    return BS_OK;
}

BS_STATUS SockSes_DelNode(IN HANDLE hSockSesId, IN SOCK_SES_S *pstDelNode)
{
    _SOCK_SES_HEAD_S *pstSesHead = (_SOCK_SES_HEAD_S *)hSockSesId;

    if (0 != pstSesHead->hSem)
    {
        SEM_P(pstSesHead->hSem, BS_WAIT, BS_WAIT_FOREVER);
    }

    DLL_DEL(&pstSesHead->stSesList, pstDelNode);

    if (0 != pstSesHead->hSem)
    {
        SEM_V(pstSesHead->hSem);
    }

    RBUF_Delete(pstDelNode->stSideA.hRbufId);
    RBUF_Delete(pstDelNode->stSideB.hRbufId);

    MEM_Free(pstDelNode);

    return BS_OK;
}

static SOCK_SES_S * _SockSes_GetNode(IN HANDLE hSockSesId, IN BOOL_T bIsByA, IN UINT ulSslTcpId)
{
    _SOCK_SES_HEAD_S *pstSesHead = (_SOCK_SES_HEAD_S *)hSockSesId;
    SOCK_SES_S *pstNode = NULL, *pstNodeTmp;
    SOCK_SES_SIDE_S *pstSideNode;
    
    if (0 != pstSesHead->hSem)
    {
        SEM_P(pstSesHead->hSem, BS_WAIT, BS_WAIT_FOREVER);
    }

    DLL_SCAN(&pstSesHead->stSesList, pstNodeTmp)
    {
        pstSideNode = bIsByA == TRUE ? &pstNodeTmp->stSideA : &pstNodeTmp->stSideB;

        if (pstSideNode->ulSslTcpId == ulSslTcpId)
        {
            pstNode = pstNodeTmp;
            break;
        }
    }    

    if (0 != pstSesHead->hSem)
    {
        SEM_V(pstSesHead->hSem);
    }

    return pstNode;
}

SOCK_SES_S * SockSes_GetNodeBySideA(IN HANDLE hSockSesId, IN UINT ulSslTcpSideA)
{
    return _SockSes_GetNode(hSockSesId, TRUE, ulSslTcpSideA);
}

SOCK_SES_S * SockSes_GetNodeBySideB(IN HANDLE hSockSesId, IN UINT ulSslTcpSideA)
{
    return _SockSes_GetNode(hSockSesId, FALSE, ulSslTcpSideA);
}

VOID SockSes_UnLock(IN HANDLE hSockSesId)
{
    _SOCK_SES_HEAD_S *pstSesHead = (_SOCK_SES_HEAD_S *)hSockSesId;

    if (0 != pstSesHead->hSem)
    {
        SEM_V(pstSesHead->hSem);
    }
}


/* 初始化Socket 重定向模块 */
BS_STATUS SockSes_CreateInstance(IN BOOL_T bNeedSem, OUT HANDLE *phSockSesId)
{
    _SOCK_SES_HEAD_S *pstSesHead;

    pstSesHead = MEM_ZMalloc(sizeof(_SOCK_SES_HEAD_S));
    if (NULL == pstSesHead)
    {
        RETURN(BS_NO_MEMORY);
    }

    if (bNeedSem == TRUE)
    {
        pstSesHead->hSem = SEM_CCreate("SockSes", 1);
        if (pstSesHead->hSem == 0)
        {
            MEM_Free(pstSesHead);
            RETURN(BS_ERR);
        }
    }

    *phSockSesId = pstSesHead;

    return BS_OK;
}

VOID SockSes_DeleteInstance(IN HANDLE hSockSesId)
{
    _SOCK_SES_HEAD_S *pstSesHead = (_SOCK_SES_HEAD_S *)hSockSesId;
    SOCK_SES_S *pstNode, *pstNodeTmp;

    SEM_Destory(pstSesHead->hSem);
    pstSesHead->hSem = 0;

    DLL_SAFE_SCAN(&pstSesHead->stSesList, pstNode, pstNodeTmp)
    {
        SockSes_DelNode(hSockSesId, pstNode);
    }

    MEM_Free(pstSesHead);
}

