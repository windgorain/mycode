/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-11-3
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_MEM

#include "bs.h"

#include "utl/mutex_utl.h"
#include "utl/exec_utl.h"
#include "utl/num_utl.h"
#include "utl/txt_utl.h"

#define _MEM_MAX_LEVEL           8
#define _MEM_INVALID_LEVEL       0xffffffff
#define _MEM_DFT_CHECK_VALUE     0x0a0b0c0d
#define _MEM_MAX_SPLIT_MEM_SIZE (4*1024) /* 定义最大的分块内存的大小, 超过这个大小，申请Raw内存 */
#define _MEM_RESERVED_SPACE (sizeof(_MEM_HEAD_S) + sizeof(UINT64))  /* 之所以加sizeof(UINT),是因为需要用它作为tailcheck */

#define _MEM_GET_SPLIT_MEM_USRSIZE(ulLevel)    (32*(1<<(ulLevel)))
#define _MEM_GET_SPLIT_MEM_TOTLESIZE(ulLevel)    (_MEM_GET_SPLIT_MEM_USRSIZE(ulLevel) + _MEM_RESERVED_SPACE)
#define _MEM_GET_MEMPOOL_MEMSIZE(ulLevel,ulNum) ((ulNum) *  _MEM_GET_SPLIT_MEM_TOTLESIZE(ulLevel) + sizeof(_MEM_SPLITMEMPOOL_HEAD_S))
#define _MEM_GET_DFT_MEMPOOL_MEMSIZE(ulLevel)  _MEM_GET_MEMPOOL_MEMSIZE(ulLevel, g_ulMemDftSplitMemNum[ulLevel])

#define _MEM_GET_SIZE_BY_LEVEL(ulLevel) _MEM_GET_SPLIT_MEM_USRSIZE(ulLevel)

typedef struct
{
    UINT       uiHeadCheck1;
    DLL_NODE_S stLinkNode;
    CHAR       *pszFileName;
    UCHAR      bIsBusy:1;
    UCHAR      bIsRawMem:1;
    USHORT     usLine;
    UCHAR      ulLevel;         /* 级别 */
    UINT       ulSize;          /* 用户真实申请的内存大小 */
    UINT       uiHeadCheck2;
}_MEM_HEAD_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
    UINT      ulSize;       /* 包含头大小 */
    BOOL_T     bIsSplitMem;
}_MEM_RAWMEM_HEAD_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
    UINT      ulSize;       /* 包含头大小 */
}_MEM_SPLITMEMPOOL_HEAD_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
    CHAR *pszFileName;
    USHORT usLine;
    UINT ulCount;
}_MEM_FILELINE_COUNT_S;

static UINT g_ulMemDftSplitMemNum[_MEM_MAX_LEVEL] = {32, 32, 16, 16, 8, 8, 4, 4};
static DLL_HEAD_S g_stMemRawMemList;  /* 记录从操作系统申请到的内存 */
static DLL_HEAD_S g_stMemSplitMemPoolList[_MEM_MAX_LEVEL];    /* 记录分片内存池 */
static DLL_HEAD_S g_stMemFreeSplitMemList[_MEM_MAX_LEVEL];    /* 记录空闲的内存 */
static DLL_HEAD_S g_stMemBusySplitMemList[_MEM_MAX_LEVEL + 1];    /* 记录用户已经申请的内存. 最后一个记录Raw内存 */

static MUTEX_S g_stMemLock;

#ifdef SUPPORT_MEM_MANAGED
static VOID _mem_CheckFreeList(IN UINT uiLevel)
{
    DLL_NODE_S *pstDllNode;
    _MEM_HEAD_S *pstNode;

    if (uiLevel >= _MEM_MAX_LEVEL) {
        return;
    }

    DLL_SCAN(&g_stMemFreeSplitMemList[uiLevel], pstDllNode) {
        pstNode = container_of(pstDllNode, _MEM_HEAD_S, stLinkNode);
        if ((pstNode->bIsBusy == TRUE)
            || (pstNode->uiHeadCheck1 != _MEM_DFT_CHECK_VALUE)
            || (pstNode->uiHeadCheck2 != _MEM_DFT_CHECK_VALUE)) {
            printf(" Mem Err: %p(%d): level:%d,size:%d,HeadCheck1:0x%08x, HeadCheck2:0x%08x, Busy:%d.\r\n",
                pstNode->pszFileName,
                pstNode->usLine,
                uiLevel,
                pstNode->ulSize,
                pstNode->uiHeadCheck1,
                pstNode->uiHeadCheck2,
                pstNode->bIsBusy);
            BS_DBGASSERT(0);
        }
    }
}

static VOID mem_checkBusyList(IN UINT uiLevel)
{
    DLL_NODE_S *pstDllNode;
    _MEM_HEAD_S *pstNode;
    UINT ulTailCheck;

    DLL_SCAN(&g_stMemBusySplitMemList[uiLevel], pstDllNode)
    {
        pstNode = container_of(pstDllNode, _MEM_HEAD_S, stLinkNode);

        ulTailCheck = *((UINT*)((VOID*)(((CHAR *)(pstNode + 1)) + pstNode->ulSize)));
        if ((pstNode->uiHeadCheck1 != _MEM_DFT_CHECK_VALUE)
            || (pstNode->uiHeadCheck2 != _MEM_DFT_CHECK_VALUE)
            || (ulTailCheck != _MEM_DFT_CHECK_VALUE)
            || (pstNode->bIsBusy != TRUE))
        {
            printf(" Mem Err: %s(%d): size:%d,HeadCheck1:0x%08x,HeadCheck2:0x%08x,TailCheck:0x%08x.\r\n",
                pstNode->pszFileName, pstNode->usLine, pstNode->ulSize,
                pstNode->uiHeadCheck1, pstNode->uiHeadCheck2, ulTailCheck);
            BS_DBGASSERT(0);
        }
    }
}

static VOID _MEMX_CheckMem(IN HANDLE hTimeHandle, IN USER_HANDLE_S *pstUserHandle)
{
    static UINT ulLevel = 0;
    
    MUTEX_P(&g_stMemLock);
    
    mem_checkBusyList(ulLevel);
    _mem_CheckFreeList(ulLevel);

    ulLevel ++;

    if (ulLevel > _MEM_MAX_LEVEL)
    {
        ulLevel = 0;
    }

    MUTEX_V(&g_stMemLock);
}
#endif

static VOID * _MEM_MallocRawMem(IN UINT ulSize, IN BOOL_T bIsForSplit)
{
    UINT ulRawSize = ulSize + sizeof(_MEM_RAWMEM_HEAD_S);
    _MEM_RAWMEM_HEAD_S *pstNode;

    pstNode = malloc(ulRawSize);
    if (NULL == pstNode)
    {
        return NULL;
    }

    pstNode->bIsSplitMem = bIsForSplit;
    pstNode->ulSize = ulRawSize;

    DLL_ADD(&g_stMemRawMemList, pstNode);

    return (VOID*)(pstNode+1);        
}

static VOID _MEM_FreeRawMem(IN VOID *pRawMem)
{
    _MEM_RAWMEM_HEAD_S *pstNode = (_MEM_RAWMEM_HEAD_S *)((VOID*)(((UCHAR*)pRawMem) - sizeof(_MEM_RAWMEM_HEAD_S)));

    DLL_DEL(&g_stMemRawMemList, pstNode);

    free(pstNode);
}

static VOID _MEM_Split(IN UCHAR *pucMem, IN UINT ulLevel, IN UINT ulNum)
{
    UCHAR *pucMemTmp;
    UINT i;
    _MEM_HEAD_S *pstMem;
    _MEM_SPLITMEMPOOL_HEAD_S *pstSplitMemPool;
    UINT ulSize = _MEM_GET_SPLIT_MEM_TOTLESIZE(ulLevel);

    if (0 == ulNum)
    {
        return;
    }

    pstSplitMemPool = (_MEM_SPLITMEMPOOL_HEAD_S *)((VOID*)pucMem);
    
    pstSplitMemPool->ulSize = ulSize * ulNum + sizeof(_MEM_SPLITMEMPOOL_HEAD_S);
    DLL_ADD(&g_stMemSplitMemPoolList[ulLevel], pstSplitMemPool);
    
    pucMemTmp = pucMem + sizeof(_MEM_SPLITMEMPOOL_HEAD_S);

    for (i=0; i<ulNum; i++)
    {
        pstMem = (_MEM_HEAD_S *)((VOID*)pucMemTmp);
        memset(pstMem, 0, sizeof(_MEM_HEAD_S));
        pstMem->pszFileName = "";
        pstMem->bIsRawMem = FALSE;
        pstMem->ulLevel = ulLevel;
        pstMem->bIsBusy = FALSE;
        pstMem->uiHeadCheck1 = _MEM_DFT_CHECK_VALUE;
        pstMem->uiHeadCheck2 = _MEM_DFT_CHECK_VALUE;
        DLL_ADD(&g_stMemFreeSplitMemList[ulLevel],  &pstMem->stLinkNode);
        pucMemTmp += ulSize;
    }
}

/* 
    申请Raw内存并且将他分片.
    ulLevel为要申请的级别
*/
static BS_STATUS _MEM_MallocRawSplit(IN UINT ulLevel, IN UINT ulNum)
{
    UCHAR *pucMem;
    UINT ulRawMemSize;

    ulRawMemSize = _MEM_GET_MEMPOOL_MEMSIZE(ulLevel, ulNum);

    pucMem = _MEM_MallocRawMem(ulRawMemSize, TRUE);
    if (NULL == pucMem)
    {
        RETURN(BS_NO_MEMORY);
    }

    _MEM_Split (pucMem, ulLevel, ulNum);

    return BS_OK;
}

/* 根据大小得到Level */
static UINT _MEM_GetLevelBySize(IN ULONG ulSize)
{
    INT i;

    for (i = _MEM_MAX_LEVEL-1; i>=0; i--)
    {
        if (ulSize > (UINT)_MEM_GET_SPLIT_MEM_USRSIZE(i))
        {
            i++;
            break;
        }
        else if (ulSize == (UINT)_MEM_GET_SPLIT_MEM_USRSIZE(i))
        {
            break;
        }

        /* 比最小的分片内存还小,则直接申请最小的分片内存 */
        if (i == 0)
        {
            break;
        }
    }

    if (i == _MEM_MAX_LEVEL)
    {
        return _MEM_INVALID_LEVEL;
    }

    return i;
}

static VOID * _privatemem_MallocMem(IN ULONG ulSize, IN CHAR *pszFileName, IN UINT ulLine)
{
    UINT ulLevel;
    BS_STATUS eRet;
    DLL_NODE_S *pstDllNode;
    _MEM_HEAD_S *pstNode;
    UCHAR *pucMem;
    
    if (ulSize == 0)
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    ulLevel = _MEM_GetLevelBySize(ulSize);

    if (_MEM_INVALID_LEVEL != ulLevel)
    {
        if (DLL_COUNT(&g_stMemFreeSplitMemList[ulLevel]) == 0)
        {
            eRet = _MEM_MallocRawSplit(ulLevel, g_ulMemDftSplitMemNum[ulLevel]);
            if (BS_OK != eRet)
            {
                return NULL;
            }
        }

        pstDllNode = DLL_Get(&g_stMemFreeSplitMemList[ulLevel]);
        DLL_ADD(&g_stMemBusySplitMemList[ulLevel], pstDllNode);

        pstNode = container_of(pstDllNode, _MEM_HEAD_S, stLinkNode);
    }
    else
    {
        pstNode = _MEM_MallocRawMem(ulSize + _MEM_RESERVED_SPACE, FALSE);
        if (NULL == pstNode)
        {
            return NULL;
        }
        Mem_Zero(pstNode, sizeof(_MEM_HEAD_S));
        pstNode->bIsRawMem = TRUE;
        DLL_ADD(&g_stMemBusySplitMemList[_MEM_MAX_LEVEL], &pstNode->stLinkNode);
    }

    BS_DBGASSERT(pstNode->bIsBusy == FALSE);

    pstNode->bIsBusy = TRUE;
    pstNode->ulSize = ulSize;
    pstNode->pszFileName = pszFileName;
    pstNode->usLine = ulLine;
    pstNode->uiHeadCheck1 = _MEM_DFT_CHECK_VALUE;
    pstNode->uiHeadCheck2 = _MEM_DFT_CHECK_VALUE;

    pucMem = (VOID*)pstNode;
    pucMem += sizeof(_MEM_HEAD_S);
    
    *((UINT*)(pucMem + ulSize)) = _MEM_DFT_CHECK_VALUE;

    return (VOID*)pucMem;
}

static VOID _privatemem_FreeMem(IN VOID *pMem)
{
    _MEM_HEAD_S *pstNode;

    pstNode = (_MEM_HEAD_S*)(((UCHAR*)pMem) - sizeof(_MEM_HEAD_S));

    MUTEX_P(&g_stMemLock);

    BS_DBGASSERT(MUTEX_GetCount(&g_stMemLock) == 1);

    BS_DBGASSERT(pstNode->bIsBusy == TRUE);

    pstNode->bIsBusy = FALSE;

    if (pstNode->bIsRawMem == TRUE) {
        DLL_DEL(&g_stMemBusySplitMemList[_MEM_MAX_LEVEL], &pstNode->stLinkNode);
        _MEM_FreeRawMem(pstNode);
    } else {
        DLL_DEL(&g_stMemBusySplitMemList[pstNode->ulLevel], &pstNode->stLinkNode);
        DLL_ADD(&g_stMemFreeSplitMemList[pstNode->ulLevel], &pstNode->stLinkNode);
    }

    MUTEX_V(&g_stMemLock);
}

VOID * PrivateMEMX_MallocMem(IN ULONG ulSize, IN CHAR *pszFileName, IN UINT ulLine)
{
    VOID *pMem = NULL;

    MUTEX_P(&g_stMemLock);

    BS_DBGASSERT(MUTEX_GetCount(&g_stMemLock) == 1);

    pMem = _privatemem_MallocMem(ulSize, pszFileName, ulLine);

    MUTEX_V(&g_stMemLock);

    return pMem;
}

/* 根据用户内存地址得到Raw Mem原始地址 */
_MEM_RAWMEM_HEAD_S * _MEM_GetRawMemHeaderByUserMemPtr(IN VOID *pMemPtr)
{
    return (_MEM_RAWMEM_HEAD_S*)(((UCHAR*)pMemPtr) - sizeof(_MEM_HEAD_S) - sizeof(_MEM_RAWMEM_HEAD_S));
}

static VOID * _MEM_GetHead(IN VOID * pMem)
{
    _MEM_RAWMEM_HEAD_S *pstRawHead;
    _MEM_SPLITMEMPOOL_HEAD_S *pstSplitPoolHead;
    _MEM_HEAD_S *pstHead;
    UINT ulSplitMemSize;
    UINT ulinWhich;
    UINT i;

    /* 查找Raw 内存链表 */
    DLL_SCAN (&g_stMemRawMemList, pstRawHead)
    {
        if (NUM_IN_RANGE(pMem, pstRawHead + 1, pstRawHead->ulSize + (UCHAR *)(pstRawHead)))
        {
            break;
        }
    }

    if (NULL == pstRawHead)
    {
        return NULL;
    }

    /* 如果是RAW内存,则是一块内存. 直接返回. 否则,是分片内存 */
    if (pstRawHead->bIsSplitMem == FALSE)
    {
        return (VOID*)(pstRawHead + 1);
    }

    for (i=0; i<_MEM_MAX_LEVEL; i++)
    {
        DLL_SCAN(&g_stMemSplitMemPoolList[i], pstSplitPoolHead)
        {
            if (NUM_IN_RANGE(pMem, pstSplitPoolHead + 1, pstSplitPoolHead->ulSize -1 + (UCHAR *)(pstSplitPoolHead)))
            {
                break;
            }
        }

        if (pstSplitPoolHead != NULL)
        {
            break;
        }
    }

    if (NULL == pstSplitPoolHead) {
        return NULL;
    }

    ulSplitMemSize = _MEM_GET_SPLIT_MEM_TOTLESIZE(i);

    /* 计算pMem在第几片分片内存上 */
    ulinWhich = ((UCHAR *)pMem - (UCHAR *)(pstSplitPoolHead + 1)) / ulSplitMemSize;

    /* 定位Head */
    pstHead = (_MEM_HEAD_S*)((((UCHAR*)pstSplitPoolHead) + sizeof(_MEM_SPLITMEMPOOL_HEAD_S)) + ulinWhich * ulSplitMemSize);

    return pstHead;
}

VOID PrivateMEMX_FreeMem(IN VOID *pMem, IN CHAR *pszFileName, IN UINT ulLine)
{
    _MEM_HEAD_S *pstNode;
    UINT ulTailCheck;

    BS_DBGASSERT(NULL != pMem);

    if (! _MEM_GetHead(pMem)) {
        printf("File=%s(%d)\r\n", pszFileName, ulLine);
        BS_DBGASSERT(0);
        return;
    }

    pstNode = (_MEM_HEAD_S*)(((UCHAR*)pMem) - sizeof(_MEM_HEAD_S));
    ulTailCheck = *((UINT*)(((CHAR *)(pstNode + 1)) + pstNode->ulSize));

    if ((pstNode->uiHeadCheck1 != _MEM_DFT_CHECK_VALUE)
        || (pstNode->uiHeadCheck2 != _MEM_DFT_CHECK_VALUE)
        || (ulTailCheck != _MEM_DFT_CHECK_VALUE))
    {
        printf(" Mem Err: %s(%d),size:%d,headcheck1:0x%08x,headcheck2:0x%08x,tailcheck:0x%08x,caller:%s(%d)\r\n",
            pstNode->pszFileName, pstNode->usLine, pstNode->ulSize,
            pstNode->uiHeadCheck1, pstNode->uiHeadCheck2, ulTailCheck,
            pszFileName, ulLine);

        BS_DBGASSERT(0);

        return;
    }

    _privatemem_FreeMem(pMem);
}

#ifdef IN_WINDOWS
UINT WIN_GetFatherEbp()
{
    UINT ulCurrentEbp;
    UINT ulFatherEbp;

    /* 得到当前Ebp */
    __asm
    {
        mov ulCurrentEbp, ebp
    }

    /* 得到调用者Ebp */
    ulCurrentEbp = *((UINT*)ulCurrentEbp);

    /* 得到调用者父函数Ebp */
    ulFatherEbp = *((UINT*)ulCurrentEbp);

    return ulFatherEbp;
}

UINT WIN_GetCurrentThreadStackBase()
{
    TEB_S * pstTeb;

    pstTeb = (TEB_S *)NtCurrentTeb();

    return (UINT)pstTeb->Tib.StackBase;
}

/* 得不到,则返回0 */
UINT WIN_GetPreEbp(IN UINT ulStackBase, IN UINT ulCurrentEbp)
{
    UINT ulPreEbp;

    ulPreEbp = *((UINT*)ulCurrentEbp);

    if ((ulPreEbp > ulStackBase) || (ulPreEbp < ulCurrentEbp))
    {
        return 0;
    }

    return ulPreEbp;
}

#endif

BOOL_T MEM_IsStackOverFlow(IN UCHAR *pMem, IN UINT ulLen)
{
#if 0
#ifdef IN_WINDOWS
    {
        UINT ulEbp;
        UINT ulStackBase;

        ulStackBase = WIN_GetCurrentThreadStackBase();
        ulEbp = WIN_GetFatherEbp();

        while (ulEbp != 0)
        {
            if (ulEbp > (UINT)pMem)
            {
                if(NUM_IN_RANGE(ulEbp, pMem, pMem + ulLen - 1))
                {
                    return TRUE;
                }

                return FALSE;
            }

            ulEbp = WIN_GetPreEbp(ulStackBase, ulEbp);
        }

        return FALSE;
    }
#endif
#endif

    return FALSE;
}

/* 计算从pMem开始到ulLen这段区间,有没有越界 */
BOOL_T MEM_IsOverFlow(IN VOID *pMem, IN UINT ulLen)
{
    _MEM_HEAD_S *pstHead;

    pstHead = _MEM_GetHead(pMem);
    if (NULL == pstHead)    /* 不是动态内存,检查是否覆盖本线程的堆栈 */
    {
        if (TRUE ==  MEM_IsStackOverFlow(pMem, ulLen))
        {
            BS_DBGASSERT(0);
            return TRUE;
        }

        return FALSE;
    }

    if (pstHead->bIsBusy == FALSE)
    {
        BS_DBGASSERT(0);
        return TRUE;
    }

    if (! NUM_IN_RANGE((UCHAR*)pMem + ulLen, pstHead + 1, ((UCHAR*)(pstHead + 1)) + pstHead->ulSize))
    {
        BS_DBGASSERT(0);
        return TRUE;
    }

    return FALSE;
}

#ifdef SUPPORT_MEM_MANAGED
static BS_STATUS mem_Init()
{
    UINT i;
    UINT ulRawMemSize = 0;
    UCHAR *pucMem = NULL;
    
    DLL_INIT(&g_stMemRawMemList);
    MUTEX_Init(&g_stMemLock);

    for (i=0; i<_MEM_MAX_LEVEL; i++)
    {
        DLL_INIT(&g_stMemSplitMemPoolList[i]);
        DLL_INIT(&g_stMemFreeSplitMemList[i]);
        DLL_INIT(&g_stMemBusySplitMemList[i]);

        ulRawMemSize += _MEM_GET_DFT_MEMPOOL_MEMSIZE(i);
    }

    DLL_INIT(&g_stMemBusySplitMemList[_MEM_MAX_LEVEL]); /* 初始化Raw内存Busy 链表 */

    if (ulRawMemSize == 0)
    {
        return BS_OK;
    }

    pucMem = _MEM_MallocRawMem(ulRawMemSize, TRUE);
    if (NULL == pucMem)
    {
        RETURN(BS_NO_MEMORY);
    }

    for (i=0; i<_MEM_MAX_LEVEL; i++)
    {
        _MEM_Split (pucMem, i, g_ulMemDftSplitMemNum[i]);
        pucMem += _MEM_GET_DFT_MEMPOOL_MEMSIZE(i);
    }

    return BS_OK;
}
#endif

BS_STATUS MEM_Init()
{
    int ret = 0;
#ifdef SUPPORT_MEM_MANAGED
    ret = mem_Init();
#endif
    return ret;
}

BS_STATUS MEM_Run()
{
#ifdef SUPPORT_MEM_MANAGED
    int ret;
    static MTIMER_S g_stMemMTimer;

    /* 启动检查内存错误的定时器 */
    ret = MTimer_Add(&g_stMemMTimer, 1000, TIMER_FLAG_CYCLE,
            _MEMX_CheckMem, NULL);
    if (ret < 0) {
        BS_DBGASSERT(0);
        RETURN(BS_ERR);
    }

#endif

    return BS_OK;
}

/* 按文件和行号统计内存的个数 */
static inline VOID _MEM_CountByFileLine(IN DLL_HEAD_S *pstDllHead, IN _MEM_HEAD_S *pstMemHead)
{
    _MEM_FILELINE_COUNT_S *pstNode;

    /* 查找是否已经存在,如果存在,则count加一即可 */
    DLL_SCAN(pstDllHead, pstNode)
    {
        if ((pstNode->usLine == pstMemHead->usLine)
            && ((pstNode->pszFileName == pstMemHead->pszFileName) 
                || (0 == strcmp(pstNode->pszFileName, pstMemHead->pszFileName))))
        {
            pstNode->ulCount ++;
            return;
        }
    }

    /* 没有找到,则新建一条表项 */
    pstNode = malloc(sizeof(_MEM_FILELINE_COUNT_S));    /* 不使用MEM_Malloc 是为了避免死锁 */
    if (NULL == pstNode)
    {
        return;
    }
    memset(pstNode, 0, sizeof(_MEM_FILELINE_COUNT_S));

    pstNode->pszFileName = pstMemHead->pszFileName;  /* 无需拷贝, fileName是静态内存,不会被释放 */
    pstNode->usLine = pstMemHead->usLine;
    pstNode->ulCount = 1;

    DLL_ADD_TO_HEAD(pstDllHead, pstNode);
}

static inline VOID _MEM_FreeCountFileLine(IN DLL_HEAD_S *pstDllHead)
{
    _MEM_FILELINE_COUNT_S *pstNode, *pstNodeTmp;

    DLL_SAFE_SCAN(pstDllHead,pstNode,pstNodeTmp)
    {
        DLL_DEL(pstDllHead, pstNode);
        free(pstNode);
    }
}

static inline VOID _MEM_ShowLevelOfMemStat(IN UINT ulLevel)
{
    DLL_NODE_S *pstDllNode;
    _MEM_HEAD_S *pstHead;
    DLL_HEAD_S stDllHead;
    _MEM_FILELINE_COUNT_S *pstNode;
    UINT ulTotalCount;

    if (ulLevel >= _MEM_MAX_LEVEL)
    {
        return;
    }

    DLL_INIT(&stDllHead);

    MUTEX_P(&g_stMemLock);

    ulTotalCount = DLL_COUNT(&g_stMemBusySplitMemList[ulLevel]);
    
    DLL_SCAN(&g_stMemBusySplitMemList[ulLevel], pstDllNode)
    {
        pstHead = container_of(pstDllNode, _MEM_HEAD_S, stLinkNode);
        _MEM_CountByFileLine(&stDllHead, pstHead); 
    }

    MUTEX_V(&g_stMemLock);

    EXEC_OutInfo(" Memory size:%d      Total count:%d\r\n", _MEM_GET_SIZE_BY_LEVEL(ulLevel), ulTotalCount);

    EXEC_OutString(" Count   Filename(line)\r\n"
        "--------------------------------------------------------------------------\r\n");

    /* 输出统计结果 */
    DLL_SCAN(&stDllHead, pstNode)
    {
        EXEC_OutInfo(" %-7d %s(%d)\r\n", pstNode->ulCount, pstNode->pszFileName, pstNode->usLine);
    }

    EXEC_OutString("\r\n");

    _MEM_FreeCountFileLine(&stDllHead);
}

/* show memory status */
BS_STATUS MEM_ShowStat(IN UINT ulArgc, IN CHAR **argv)
{
    UINT i;

    EXEC_OutString(" MemSize  BusyCount  FreeCount\r\n"
        "--------------------------------------------------------------------------\r\n");
    
    /* 输出分块内存统计结果 */
    for (i=0; i<_MEM_MAX_LEVEL; i++)
    {
        EXEC_OutInfo(" %-7d  %-9d  %-9d\r\n",
            _MEM_GET_SPLIT_MEM_USRSIZE(i), DLL_COUNT(&g_stMemBusySplitMemList[i]), DLL_COUNT(&g_stMemFreeSplitMemList[i]));
    }

    /* 输出Raw 内存统计结果 */
    EXEC_OutInfo(" >%-6d  %-9d  %-9d\r\n",
        _MEM_GET_SPLIT_MEM_USRSIZE(_MEM_MAX_LEVEL-1), DLL_COUNT(&g_stMemBusySplitMemList[_MEM_MAX_LEVEL]), 0);
    
    EXEC_OutString("\r\n");

    return BS_OK;
}

/* show memory size {32|64...4096} */
BS_STATUS MEM_ShowSizeOfMemStat(IN UINT ulArgc, IN CHAR **argv)
{
    UINT ulSize = 32;
    UINT ulLevel;

    if (BS_OK != TXT_Atoui(argv[3], &ulSize))
    {
        RETURN(BS_ERR);
    }

    ulLevel = _MEM_GetLevelBySize(ulSize);

    _MEM_ShowLevelOfMemStat(ulLevel);

    return BS_OK;
}

/* show memory size all */
BS_STATUS MEM_ShowAllSizeOfMem(IN UINT ulArgc, IN CHAR **argv)
{
    UINT ulLevel;

    for (ulLevel = 0; ulLevel < _MEM_MAX_LEVEL; ulLevel++)
    {
        _MEM_ShowLevelOfMemStat(ulLevel);
    }

    return BS_OK;
}

/* show memory row */
BS_STATUS MEM_ShowRawMem(IN UINT ulArgc, IN CHAR **argv)
{
    DLL_NODE_S *pstDllNode;
    _MEM_HEAD_S *pstHead;

    EXEC_OutString("--------------------------------------------------------------------------\r\n");

    MUTEX_P(&g_stMemLock);
    
    /* 输出分块内存统计结果 */
    DLL_SCAN(&g_stMemBusySplitMemList[_MEM_MAX_LEVEL], pstDllNode)
    {
        pstHead = container_of(pstDllNode, _MEM_HEAD_S, stLinkNode);
        EXEC_OutInfo(" File:%s \r\n Line:%d \r\n Size:%d\r\n", 
            pstHead->pszFileName, pstHead->usLine, pstHead->ulSize);
    }
    
    MUTEX_V(&g_stMemLock);
    
    EXEC_OutString("\r\n");

    return BS_OK;
}


VOID * MEM_RcuMalloc(IN UINT uiSize)
{
    UINT uiNewSize;
    UCHAR *pucMem;

    uiNewSize = sizeof(RCU_NODE_S) + uiSize;

    pucMem = MEM_Malloc(uiNewSize);
    if (NULL == pucMem)
    {
        return NULL;
    }

    return pucMem + sizeof(RCU_NODE_S);
}

static VOID _mem_RcuFreeCallBack(IN VOID *pstRcuNode)
{
    MEM_Free(pstRcuNode);
}

VOID MEM_RcuFree(IN VOID *pMem)
{
    UCHAR *pucMem = pMem;

    pucMem -= sizeof(RCU_NODE_S);

    RcuBs_Free((RCU_NODE_S*)pucMem, _mem_RcuFreeCallBack);
}

