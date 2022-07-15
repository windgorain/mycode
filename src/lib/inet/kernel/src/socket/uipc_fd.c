/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-19
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/darray_utl.h"

#include "uipc_fd.h"
#include "uipc_fd_func.h"

typedef struct
{
    DARRAY_HANDLE hDArray;
}UPIC_FD_CTRL_S;



static UINT g_uiUpicFdResId = 0;

static VOID upic_fd_ThreadResFree(IN UINT uiResID, IN HANDLE hUserRes)
{
    
}

static UPIC_FD_CTRL_S * uipc_fd_Create()
{
    UPIC_FD_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(UPIC_FD_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstCtrl->hDArray = DARRAY_Create(0, 32);
    if (NULL == pstCtrl->hDArray)
    {
        MEM_Free(pstCtrl);
        return NULL;
    }

    return pstCtrl;
}

BS_STATUS UIPC_FD_Init()
{
    g_uiUpicFdResId = THREAD_AllocGroupUserResID(upic_fd_ThreadResFree);
    if (0 == g_uiUpicFdResId)
    {
        return BS_ERR;
    }

    return BS_OK;
}

INT UIPC_FD_FAlloc(OUT UPIC_FD_S **ppFp)
{
    UINT uiSelfTId;
    UPIC_FD_CTRL_S *pstCtrl;
    UPIC_FD_S *pstFp;
    UINT uiFd;

    uiSelfTId = THREAD_GetSelfID();

    pstCtrl = THREAD_GetGroupUserRes(uiSelfTId, g_uiUpicFdResId);
    if (NULL == pstCtrl)
    {
        pstCtrl = uipc_fd_Create();
        if (NULL == pstCtrl)
        {
            return -1;
        }

        THREAD_SetGroupUserRes(uiSelfTId, g_uiUpicFdResId, pstCtrl);
    }

    pstFp = MEM_ZMalloc(sizeof(UPIC_FD_S));
    if (NULL == pstFp)
    {
        return -1;
    }

    uiFd = DARRAY_Add(pstCtrl->hDArray, pstFp);
    if (uiFd == DARRAY_INVALID_INDEX)
    {
        MEM_Free(pstFp);
        return -1;
    }

    pstFp->pCtrl = pstCtrl;
    pstFp->uiTID = uiSelfTId;
    pstFp->iFd = (INT)uiFd;

    return (INT)uiFd;
}

VOID UIPC_FD_FClose(IN UPIC_FD_S *pstFp)
{
    UPIC_FD_CTRL_S *pstCtrl = pstFp->pCtrl;

    DARRAY_Set(pstCtrl->hDArray, (UINT)pstFp->iFd, NULL);

    MEM_Free(pstFp);
}

VOID UIPC_FD_Attach(IN UPIC_FD_S *pstFp, IN VOID *pSocket)
{
    pstFp->pSocket = pSocket;
}

VOID * UIPC_FD_GetSocket(IN UPIC_FD_S *pstFp)
{
    return pstFp->pSocket;
}

VOID * UIPC_FD_GetFp(IN INT iFd)
{
    UINT uiSelfTId;
    UPIC_FD_CTRL_S *pstCtrl;

    uiSelfTId = THREAD_GetSelfID();

    pstCtrl = THREAD_GetGroupUserRes(uiSelfTId, g_uiUpicFdResId);
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    return DARRAY_Get(pstCtrl->hDArray, iFd);
}

