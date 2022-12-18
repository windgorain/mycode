/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-10-25
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/mem_utl.h"
#include "utl/data2hex_utl.h"

void * MEM_FindOneOf(void *mem, UINT mem_len, void *to_finds, UINT to_finds_len)
{
    UCHAR *data = mem;
    UCHAR *end = data + mem_len;

    BS_DBGASSERT(NULL != data);

    while (data < end) {
        if (MEM_FindOne(to_finds, to_finds_len, *data)) {
            return data;
        }
        data ++;
    }

    return NULL;
}

void * MEM_FindOne(void *mem, UINT mem_len, UCHAR to_find)
{
    UCHAR *data = mem;
    UCHAR *end = data + mem_len;

    BS_DBGASSERT(NULL != data);

    while (data < end) {
        if (*data == to_find) {
            return data;
        }
        data ++;
    }

    return NULL;
}

void * MEM_Find(IN VOID *pMem, IN UINT ulMemLen, IN VOID *pMemToFind, IN UINT ulMemToFindLen)
{
    UCHAR *data = pMem;
    UCHAR *end;

    BS_DBGASSERT(NULL != pMem);
    BS_DBGASSERT(NULL != pMemToFind);
    BS_DBGASSERT(ulMemToFindLen > 0);
    
    if (ulMemToFindLen > ulMemLen) {
        return NULL;
    }

    end = data + (ulMemLen - ulMemToFindLen) + 1;

    for (; data < end; data++) {
        if (*data != *(UCHAR*)pMemToFind) {
            continue;
        }

        if (memcmp(data, pMemToFind, ulMemToFindLen) == 0) {
            return data;
        }
    }

    return NULL;
}

void * MEM_CaseFind(void *pMem, UINT ulMemLen, void *pMemToFind, UINT ulMemToFindLen)
{
    UCHAR *data = pMem;
    UCHAR *end;
    UCHAR ch;

    BS_DBGASSERT(NULL != pMem);
    BS_DBGASSERT(NULL != pMemToFind);
    BS_DBGASSERT(ulMemToFindLen > 0);
    
    if (ulMemToFindLen > ulMemLen) {
        return NULL;
    }

    end = data + (ulMemLen - ulMemToFindLen) + 1;

    UCHAR first = *(UCHAR*)pMemToFind;
    first = tolower(first);

    for (; data < end; data++) {
        ch = tolower(*data);
        if (ch != first) {
            continue;
        }

        if (MEM_CaseCmp(data, ulMemToFindLen, pMemToFind, ulMemToFindLen) == 0) {
            return data;
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

int MEM_CaseCmp(UCHAR *pucMem1, UINT uiMem1Len, UCHAR *pucMem2, UINT uiMem2Len)
{
    int c1;
    int c2;
    int i;
    UINT   uiLen   = MIN(uiMem1Len, uiMem2Len);
    UCHAR  *tmp1 = pucMem1;
    UCHAR  *tmp2 = pucMem2;

    for (i=0; i<uiLen; i++) {
        c1 = tolower(tmp1[i]);
        c2 = tolower(tmp2[i]);
        if (c1 != c2) {
            return (c1 - c2);
        }
    }

    if (uiMem1Len == uiMem2Len) {
        return 0;
    }

    if (uiMem1Len > uiMem2Len) {
        return 1;
    }

    return -1;
}

/* 打印内存字面值到buf中 */
int MEM_Sprint(IN UCHAR *pucMem, IN UINT uiLen, OUT char *buf, int buf_size)
{
    int tmp_len1, tmp_len2;
    UCHAR *d = pucMem;
    char info[24];
    int len = 0;
    int reserved_size = buf_size;
    int copyed_len = 0;
    UINT mem_len = uiLen;

    while (mem_len > 0) {
        tmp_len1 = MIN(16, mem_len);
        mem_len -= tmp_len1;
        while (tmp_len1 > 0) {
            tmp_len2 = MIN(4, tmp_len1);
            tmp_len1 -= tmp_len2;
            DH_Data2Hex(d, tmp_len2, info);
            info[tmp_len2 * 2] = ' ';
            info[(tmp_len2 * 2) + 1] = '\0';
            len = strlcpy(buf + copyed_len, info, reserved_size);
            if (len >= reserved_size) {
                RETURN(BS_OUT_OF_RANGE);
            }
            reserved_size -= len;
            copyed_len += len;
            d += tmp_len2;
        }
        len = strlcpy(buf + copyed_len, (char*)"\r\n", reserved_size);
        if (len >= reserved_size) {
            RETURN(BS_OUT_OF_RANGE);
        }
        reserved_size -= len;
        copyed_len += len;
    }

    return copyed_len;
}

void MEM_Print(UCHAR *pucMem, int len, PF_MEM_PRINT_FUNC print_func/* NULL使用缺省printf */)
{
    int tmp_len1, tmp_len2;
    UCHAR *d = pucMem;
    char info[16];
    PF_MEM_PRINT_FUNC func = print_func;

    if (! func) {
        func = (void*)printf;
    }

    while (len > 0) {
        tmp_len1 = MIN(16, len);
        len -= tmp_len1;
        while (tmp_len1 > 0) {
            tmp_len2 = MIN(4, tmp_len1);
            tmp_len1 -= tmp_len2;
            DH_Data2Hex(d, tmp_len2, info);
            info[tmp_len2 * 2] = ' ';
            info[(tmp_len2 * 2) + 1] = '\0';
            func(info);
            d += tmp_len2;
        }
        func((char*)"\r\n");
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

/* 按照ULONG格式一个个的赋值为0 */
void MEM_ZeroByUlong(void *data, int count)
{
    ULONG *a = data;
    int i;

    for (i=0; i<count; i++) {
        *a = 0;
        a++;
    }
}

/* 将内存中的src字符替换为dst, 返回替换了多少个字符 */
int MEM_ReplaceChar(void *data, int len, UCHAR src, UCHAR dst)
{
    int i;
    int count = 0;
    UCHAR *tmp = data;

    for (i=0; i<len; i++) {
        if (tmp[i] == src) {
            tmp[i] = dst;
            count ++;
        }
    }

    return count;
}

/* 将内存中的src字符替换为dst, 只替换一个. 返回替换了多少个字符 */
int MEM_ReplaceOneChar(void *data, int len, UCHAR src, UCHAR dst)
{
    int i;
    UCHAR *tmp = data;

    for (i=0; i<len; i++) {
        if (tmp[i] == src) {
            tmp[i] = dst;
            return 1;
        }
    }

    return 0;
}

/* 交换两块内存的内容 */
void MEM_Exchange(void *buf1, void *buf2, int len)
{
    unsigned char *d1 = buf1;
    unsigned char *d2 = buf2;
    unsigned char tmp;
    int i;

    for (i=0; i<len; i++) {
        tmp = d1[i];
        d1[i] = d2[i];
        d2[i] = tmp;
    }
}

