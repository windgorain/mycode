/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-10-25
* Description: 
* History:     
******************************************************************************/

#ifndef _MEM_UTL_H
#define _MEM_UTL_H

#include "utl/num_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define MEM_Set(pBuf, ucTo, uiLen) memset(pBuf, ucTo, uiLen)
#define Mem_Zero(pMem,ulSize)  MEM_Set(pMem, 0, ulSize)

#if defined(USE_BS)
#define SUPPORT_MEM_MANAGED
#endif

#define MEM_Malloc(uiSize)  _mem_Malloc(uiSize, __FILE__,  __LINE__)
#define MEM_Free(pMem)  _mem_Free((VOID*)(pMem), __FILE__, __LINE__)
#define MEM_ZMalloc(ulSize)  _mem_MallocWithZero(ulSize, __FILE__,  __LINE__)
#define MEM_MallocAndCopy(pSrc,uiSrcLen,uiMallocLen) _mem_MallocAndCopy(pSrc,uiSrcLen,uiMallocLen,__FILE__,__LINE__)

#define MEM_ZMallocAndCopy(pSrc,uiSrcLen,uiMallocLen) ({ \
        char *_mem = MEM_MallocAndCopy(pSrc,uiSrcLen, uiMallocLen); \
        if (_mem && (uiMallocLen > uiSrcLen)) \
            memset(_mem + uiSrcLen, 0, uiMallocLen - uiSrcLen); \
        (void*)_mem; })

#define MEM_Realloc(pSrc,uiSrcLen,uiMallocLen) _mem_Realloc(pSrc,uiSrcLen,uiMallocLen,__FILE__,__LINE__)

#define MEM_ZRealloc(pSrc,uiSrcLen,uiMallocLen) ({ \
        char *_mem = MEM_Realloc(pSrc,uiSrcLen, uiMallocLen); \
        if (_mem && (uiMallocLen > uiSrcLen)) \
            memset(_mem + uiSrcLen, 0, uiMallocLen - uiSrcLen); \
        (void*)_mem; })

#define MEM_Dup(data, len) MEM_MallocAndCopy(data, len, len)

void * _mem_Malloc(IN UINT uiSize, const char *pcFileName, IN UINT uiLine);
void _mem_Free(IN VOID *pMem, const char *pcFileName, IN UINT uiLine);
void * _mem_Realloc(void *old_mem, UINT old_size, UINT new_size, char *filename, UINT line);

static inline VOID MEM_Copy(IN VOID *pucDest, IN VOID *pucSrc, IN UINT ulLen)
{
#ifdef IN_DEBUG
    {
        char *d1_min = pucDest;
        char *d1_max = d1_min + ulLen;
        char *d2_min = pucSrc;
        char *d2_max = d2_min + ulLen;
        if (NUM_AREA_IS_OVERLAP(d1_min, d1_max, d2_min, d2_max) != FALSE) {
            assert(0);
        }
    }
#endif
    memcpy(pucDest, pucSrc, ulLen);
}

static inline VOID * _mem_MallocWithZero(IN UINT uiSize, const char *pszFileName, IN UINT ulLine)
{
    VOID *pMem;

    (VOID)pszFileName;
    (VOID)ulLine;

    pMem = _mem_Malloc(uiSize, pszFileName, ulLine);
    if (pMem) {
        Mem_Zero(pMem, uiSize);
    }

    return pMem;
}

static inline VOID * _mem_MallocAndCopy
(
    IN VOID *pSrc,
    IN UINT uiSrcLen,
    IN UINT uiMallocLen,
    const CHAR *pcFileName,
    IN UINT uiLine
)
{
    VOID *pMem;
    UINT uiCopyLen;

    (VOID)pcFileName;
    (VOID)uiLine;

    uiCopyLen = MIN(uiSrcLen, uiMallocLen);

    pMem = _mem_Malloc(uiMallocLen, pcFileName, uiLine);
    if (NULL == pMem)
    {
        return NULL;
    }

    if (uiCopyLen != 0)
    {
        MEM_Copy(pMem, pSrc, uiCopyLen);
    }

    return pMem;
}

void * _mem_rcu_malloc(IN UINT uiSize, const char *file, int line);
#define MEM_RcuMalloc(size) _mem_rcu_malloc(size, __FILE__, __LINE__)

void * _mem_rcu_zmalloc(IN UINT uiSize, const char *file, int line);
#define MEM_RcuZMalloc(size) _mem_rcu_zmalloc(size, __FILE__, __LINE__)

void * _mem_rcu_dup(void *mem, int size, const char *file, int line);
#define MEM_RcuDup(mem, size) _mem_rcu_dup(mem, size, __FILE__, __LINE__)

void MEM_RcuFree(IN VOID *pMem);

void * MEM_Find(IN VOID *pMem, IN UINT ulMemLen, IN VOID *pMemToFind, IN UINT ulMemToFindLen);

void * MEM_CaseFind(void *pMem, UINT ulMemLen, void *pMemToFind, UINT ulMemToFindLen);

void * MEM_FindOne(void *mem, UINT mem_len, UCHAR to_find);

void * MEM_FindOneOf(void *mem, UINT mem_len, void *to_finds, UINT to_finds_len);

INT MEM_Cmp(IN UCHAR *pucMem1, IN UINT uiMem1Len, IN UCHAR *pucMem2, IN UINT uiMem2Len);

int MEM_CaseCmp(UCHAR *pucMem1, UINT uiMem1Len, UCHAR *pucMem2, UINT uiMem2Len);

typedef struct
{
    UCHAR *pucPattern;     /* 模式串 */
    UINT uiPatternLen;     /* 模式串长度 */
    UINT uiPatternCmpLen;  /* 已经匹配了模式串多长 */
    UINT uiCmpMemOffset;   /* 正在和模式串匹配的区段的总Offset */
    UINT uiPreMemTotleSize;/* 之前已经扫描过的所有数据块的长度 */
}MEM_FIND_INFO_S;

VOID MEM_DiscreteFindInit(INOUT MEM_FIND_INFO_S *pstFindInfo, IN UCHAR *pucPattern, IN UINT uiPatternLen);
/* 在不连续缓冲区中查找数据 */
BS_STATUS MEM_DiscreteFind
(
    INOUT MEM_FIND_INFO_S *pstFindInfo,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiFindOffset
);

/* 将内存中的内容反序 */
void MEM_Invert(void *in, int len, void *out);
/* 是否全0 */
int MEM_IsZero(void *data, int size);
/* 是否全部是0xff */
int MEM_IsFF(void *data, int size);

void MEM_ZeroByUlong(void *data, int count);

int MEM_Sprint(IN UCHAR *pucMem, IN UINT uiLen, OUT char *buf, int buf_size);

typedef void (*PF_MEM_PRINT_FUNC)(const char *fmt, ...);
void MEM_Print(UCHAR *pucMem, int len, PF_MEM_PRINT_FUNC print_func/* NULL使用缺省printf */);

/* 将内存中的src字符替换为dst, 返回替换了多少个字符 */
int MEM_ReplaceChar(void *data, int len, UCHAR src, UCHAR dst);

/* 将内存中的src字符替换为dst, 返回替换了多少个字符 */
int MEM_ReplaceOneChar(void *data, int len, UCHAR src, UCHAR dst);

/* 交换两块内存的内容 */
void MEM_Exchange(void *buf1, void *buf2, int len);

#ifdef __cplusplus
}
#endif
#endif //MEM_UTL_H_
