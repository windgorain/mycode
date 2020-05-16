/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-10-25
* Description: 
* History:     
******************************************************************************/
 

#include "bs.h"
#include "utl/mem_utl.h"

VOID * MEM_Find(IN VOID *pMem, IN UINT ulMemLen, IN VOID *pMemToFind, IN UINT ulMemToFindLen)
{
    UINT ulOffset;

    BS_DBGASSERT(NULL != pMem);
    BS_DBGASSERT(NULL != pMemToFind);
    BS_DBGASSERT(ulMemToFindLen > 0);
    
    if (ulMemToFindLen > ulMemLen)
    {
        return NULL;
    }

    for (ulOffset=0; ulOffset<=(ulMemLen-ulMemToFindLen); ulOffset++)
    {
        if (memcmp((UCHAR*)pMem + ulOffset, pMemToFind, ulMemToFindLen) == 0)
        {
            return (UCHAR*)pMem + ulOffset;
        }
    }

    return NULL;
}

INT MEM_Cmp(IN UCHAR *pucMem1, IN UINT uiMem1Len, IN UCHAR *pucMem2, IN UINT uiMem2Len)
{
    UINT uiCmpLen = MIN(uiMem1Len, uiMem2Len);
    UINT i;
    INT iCmp;

    for (i=0; i<uiCmpLen; i++)
    {
        iCmp = pucMem1[i] - pucMem2[i];
        if (iCmp != 0)
        {
            return iCmp;
        }
    }

    iCmp = uiMem1Len - uiMem2Len;

    return iCmp;
}

VOID MEM_Print(IN UCHAR *pucMem, IN UINT uiLen)
{
    UINT i;

    for (i=0; i<uiLen; i++)
    {
        if (i % 16 == 0)
        {
            printf ("\r\n");
        }

        printf (" %02x", pucMem[i]);
    }
}

VOID MEM_DiscreteFindInit(INOUT MEM_FIND_INFO_S *pstFindInfo, IN UCHAR *pucPattern, IN UINT uiPatternLen)
{
    Mem_Zero(pstFindInfo, sizeof(MEM_FIND_INFO_S));

    pstFindInfo->pucPattern = pucPattern;
    pstFindInfo->uiPatternLen = uiPatternLen;
}

/* 在不连续缓冲区中查找数据 */
BS_STATUS MEM_DiscreteFind
(
    INOUT MEM_FIND_INFO_S *pstFindInfo,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiFindOffset
)
{
    UCHAR *pucTmp;
    UCHAR *pucEnd;
    BOOL_T bFound = FALSE;

    pucTmp = pucData;
    pucEnd = pucData + uiDataLen;

    while (pucTmp < pucEnd)
    {
        if (*pucTmp != pstFindInfo->pucPattern[pstFindInfo->uiPatternCmpLen])
        {
            pstFindInfo->uiPatternCmpLen = 0;
        }
        else
        {
            if (pstFindInfo->uiPatternCmpLen == 0)
            {
                pstFindInfo->uiCmpMemOffset = pstFindInfo->uiPreMemTotleSize + (pucTmp - pucData);
            }

            pstFindInfo->uiPatternCmpLen ++;

            if (pstFindInfo->uiPatternCmpLen == pstFindInfo->uiPatternLen)
            {
                *puiFindOffset = pstFindInfo->uiCmpMemOffset;
                bFound = TRUE;
                break;
            }
        }

        pucTmp ++;
    }

    if (bFound == TRUE)
    {
        return BS_OK;
    }

    pstFindInfo->uiPreMemTotleSize += uiDataLen;

    return BS_NOT_FOUND;
}

/* 将一块内存中的指定偏移和长度的数据进行移动 */
VOID MEM_Move(IN VOID *pData, IN ULONG ulOldOffset, IN ULONG ulDataLen, IN ULONG ulNewOffset)
{
    ULONG i;
    UCHAR *pucData = pData;
    UCHAR *pucSrc;
    UCHAR *pucDst;

    if (ulOldOffset == ulNewOffset)
    {
        return;
    }

    if (ulOldOffset > ulNewOffset)
    {
        MEM_Copy(pucData + ulNewOffset, pucData + ulOldOffset, ulDataLen);
    }
    else
    {
        pucSrc = pucData + ulOldOffset + ulDataLen - 1;
        pucDst = pucData + ulNewOffset + ulDataLen - 1;
        for (i=ulDataLen; i>0; i--)
        {
            *pucDst = *pucSrc;
            pucDst --;
            pucSrc --;
        }
    }

    return;
}

/* 将内存中的内容反序 */
void MEM_Invert(void *in, int len, void *out)
{
    int i;
    unsigned char * inc = (unsigned char*)in + (len -1);
    unsigned char * outc = out;

    for (i=0; i<len; i++) {
        *outc = *inc;
        outc ++;
        inc --;
    }
}

/* 是否全0 */
int MEM_IsZero(void *data, int size)
{
    int i;
    unsigned char *tmp = data;

    for (i=0; i<size; i++) {
        if (tmp[i] != 0) {
            return 0;
        }
    }

    return 1;
}

/* 是否全部是0xff */
int MEM_IsFF(void *data, int size)
{
    int i;
    unsigned char *tmp = data;

    for (i=0; i<size; i++) {
        if (tmp[i] != 0xff) {
            return 0;
        }
    }

    return 1;
}
