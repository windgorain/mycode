/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/atomic_utl.h"
#include "utl/txt_utl.h"
#include "utl/sem_utl.h"

#include "os_sem.h"

#define _SEM_MAX_NAME_LEN    16

typedef enum
{
    _SEM_TYPE_COUNTER = 0,
    _SEM_TYPE_MUTEX
}_SEM_TYPE_E;

typedef struct
{
    _SEM_TYPE_E eSemType;
    THREAD_ID ulOwnerOsTID;     /* 仅用于互斥信号量, 表示哪个OS线程持有此信号量 */
    UINT     ulOwnerCounter;   /* 仅用于互斥信号量, 表示当前持有者SEM_P了多少次了 */
    OS_SEM   osSem;
    volatile int count;
    CHAR     acName[_SEM_MAX_NAME_LEN+1];
}SEM_CTRL_S;

static SEM_HANDLE sem_Create(IN CHAR *pcName, IN INT iInitNum, IN _SEM_TYPE_E eType)
{
    UINT ulRet;
    SEM_CTRL_S *pstSem;
    
    if (strlen(pcName) > _SEM_MAX_NAME_LEN)
    {
        BS_WARNNING (("Too long sem name:%s", pcName));
        return NULL;
    }

    pstSem = MEM_ZMalloc(sizeof(SEM_CTRL_S));
    if (NULL == pstSem)
    {
        return NULL;
    }

    ulRet = _OSSEM_Create(pcName, 0, &pstSem->osSem);
    if (BS_OK != ulRet)
    {
        BS_WARNNING(("Can't create os sem!"));
        return NULL;
    }

    TXT_StrCpy(pstSem->acName, pcName);
    pstSem->count = iInitNum;
    pstSem->eSemType = eType;

    return pstSem;
}

/* 主要用于递归取互斥信号量. 如果不存在递归情况,建议使用计数器信号量, 效率比这个高一倍左右 */
SEM_HANDLE SEM_MCreate (IN CHAR *pcName)
{
    return sem_Create(pcName, 1, _SEM_TYPE_MUTEX);
}


/* 当lInitNum为1时, 就相当于不支持递归SEM_P的互斥信号量. 如果使用者不存在递归情况,建议使用它 */
SEM_HANDLE SEM_CCreate (IN CHAR *pcName, IN INT iInitNum)
{
    return sem_Create(pcName, iInitNum, _SEM_TYPE_COUNTER);
}

VOID SEM_Destory(IN SEM_HANDLE hSem)
{
    SEM_CTRL_S *pstCtrl = hSem;
    
    _OSSEM_Delete(&pstCtrl->osSem);    
}

static inline BS_STATUS sem_CPV
(
    IN SEM_CTRL_S *pstSemToP,
    IN SEM_CTRL_S *pstSemToV,
    IN BS_WAIT_E eWaitMode,
    IN UINT ulMilliseconds
)
{
    INT   iCount;
    BS_STATUS eRet;

    iCount = ATOM_DEC_FETCH (&pstSemToP->count);

    if (NULL != pstSemToV)
    {
        SEM_V(pstSemToV);
    }

    if (iCount < 0)
    {
        eRet = _OSSEM_P(&pstSemToP->osSem, eWaitMode, ulMilliseconds);
        if (BS_OK != eRet)
        {
            ATOM_INC_FETCH (&pstSemToP->count);
        }
    }
    else
    {
        eRet = BS_OK;
    }

    return eRet;
}

static inline BS_STATUS sem_MPV
(
    IN SEM_CTRL_S *pstSemToP,
    IN SEM_CTRL_S *pstSemToV,
    IN BS_WAIT_E eWaitMode,
    IN UINT ulMilliseconds
)
{
    THREAD_ID ulSelfOsTid;
    BS_STATUS eRet;
    INT   iCount;

    ulSelfOsTid = THREAD_GetSelfID();

    /* 持有此信号量的是本任务 */
    if (pstSemToP->ulOwnerOsTID == ulSelfOsTid)
    {
        pstSemToP->ulOwnerCounter ++;

        if (NULL != pstSemToV)
        {
            SEM_V(pstSemToV);
        }

        return BS_OK;
    }

    iCount = ATOM_DEC_FETCH (&pstSemToP->count);

    if (NULL != pstSemToV)
    {
        SEM_V(pstSemToV);
    }

    if (iCount == 0)    /* 原来没有人持有此信号量 */
    {
        pstSemToP->ulOwnerCounter = 1;
        pstSemToP->ulOwnerOsTID = ulSelfOsTid;
        eRet = BS_OK;
    }
    else
    {
        /* 持有此信号量的不是本任务 */
        eRet = _OSSEM_P (&pstSemToP->osSem, eWaitMode, ulMilliseconds);
        if (BS_OK != eRet)
        {
            ATOM_INC_FETCH(&pstSemToP->count);
        }
        else    /* 成功 */
        {
            pstSemToP->ulOwnerOsTID = ulSelfOsTid;
            pstSemToP->ulOwnerCounter = 1;
        }
    }

    return eRet;
}

BS_STATUS SEM_P (IN SEM_HANDLE hSem, IN BS_WAIT_E eWaitMode, IN UINT ulMilliseconds)
{
    SEM_CTRL_S *pstSem = hSem;

    if (pstSem->eSemType == _SEM_TYPE_COUNTER)
    {
        return sem_CPV (pstSem, NULL, eWaitMode, ulMilliseconds);
    }
    else if (pstSem->eSemType == _SEM_TYPE_MUTEX)
    {
        return sem_MPV (pstSem, NULL, eWaitMode, ulMilliseconds);
    }
    else
    {
        BS_DBGASSERT (0);
        return (BS_ERR);
    }
}

/* 在取一个信号量的同时，释放另外一个信号量 */
BS_STATUS SEM_PV
(
    IN SEM_HANDLE hSemToP,
    IN SEM_HANDLE hSemToV,
    IN BS_WAIT_E eWaitMode,
    IN UINT ulMilliseconds
)
{
    SEM_CTRL_S *pstSemToP = hSemToP;
    SEM_CTRL_S *pstSemToV = hSemToV;

    if (pstSemToP->eSemType == _SEM_TYPE_COUNTER)
    {
        return sem_CPV (pstSemToP, pstSemToV, eWaitMode, ulMilliseconds);
    }
    else if (pstSemToP->eSemType == _SEM_TYPE_MUTEX)
    {
        return sem_MPV (pstSemToP, pstSemToV, eWaitMode, ulMilliseconds);
    }
    else
    {
        BS_DBGASSERT (0);
        return (BS_ERR);
    }
}

static inline BS_STATUS sem_CV (IN SEM_CTRL_S *pstSem)
{
    INT   lCount;

    lCount = ATOM_INC_FETCH (&pstSem->count);

    if (lCount <= 0)
    {
        return _OSSEM_V (&pstSem->osSem);
    }
    else
    {
        return BS_OK;
    }
}

static inline BS_STATUS sem_MV (IN SEM_CTRL_S *pstSem)
{
    INT iCount;

    BS_DBGASSERT (pstSem->count != 1);     /* 根本就没有人SEM_P, 走到这里说明有问题, 有人多SEM_V了 */
    BS_DBGASSERT (pstSem->ulOwnerOsTID == THREAD_GetSelfID());  /* 互斥信号量, 不用于同步, 所以SEM_V和SEM_P必然是同一个任务 */

    pstSem->ulOwnerCounter --;

    if (pstSem->ulOwnerCounter > 0)
    {
        /* 本任务还持有此信号量 */
        return BS_OK;        
    }

    pstSem->ulOwnerOsTID = 0;

    iCount = ATOM_INC_FETCH (&pstSem->count);

    if (iCount <= 0)
    {
        return _OSSEM_V (&pstSem->osSem);
    }
    else
    {
        return BS_OK;
    }
}

BS_STATUS SEM_V(IN SEM_HANDLE hSem)
{
    SEM_CTRL_S *pstSem = hSem;

    if (pstSem->eSemType == _SEM_TYPE_COUNTER)
    {
        return sem_CV (pstSem);
    }
    else if (pstSem->eSemType == _SEM_TYPE_MUTEX)
    {
        return sem_MV (pstSem);
    }
    else
    {
        BS_DBGASSERT (0);
        return (BS_ERR);
    }
}


static inline BS_STATUS sem_CVAll(IN SEM_CTRL_S *pstSem)
{
    INT   lCount;

    while (1)
    {
        lCount = ATOM_INC_FETCH (&pstSem->count);

        if (lCount <= 0)
        {
            _OSSEM_V (&pstSem->osSem);
        }
        else
        {
            break;
        }
    }

    return BS_OK;
}


BS_STATUS SEM_VAll(IN SEM_HANDLE hSem)
{
    SEM_CTRL_S *pstSem = hSem;

    if (pstSem->eSemType == _SEM_TYPE_COUNTER)
    {
        return sem_CVAll(pstSem);
    }
    else
    {
        BS_DBGASSERT (0);
        return (BS_ERR);
    }
}

/* 有多少阻塞在上面 */
INT SEM_CountPending(IN SEM_HANDLE hSem)
{
    SEM_CTRL_S *pstSem = hSem;
    INT iPending;

    iPending = pstSem->count;

    if (iPending < 0)
    {
        return - iPending;
    }

    return 0;
}
