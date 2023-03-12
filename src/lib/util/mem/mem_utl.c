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

/* 按照C输入格式打印内存字面值到buf中.
return: 实际打印了多少字节内存的字面值 */
int MEM_SprintCFromat(void *mem, UINT mem_len, OUT char *buf, int buf_size)
{
    UCHAR *d = mem;
    char info[64];
    int len;
    int reserved_size = buf_size;
    int copyed_len = 0;
    int print_len = 0;

    /* 6: length("0xaa,\n") */
    while ((mem_len > print_len) && (reserved_size > 6)) {
        sprintf(info, "0x%02x,", *d);
        len = strlcpy(buf + copyed_len, info, reserved_size);
        reserved_size -= len;
        copyed_len += len;
        print_len ++;
        d ++;
        if ((print_len) && ((print_len % 16) == 0)) {
            len = strlcpy(buf + copyed_len, (char*)"\n", reserved_size);
            reserved_size -= len;
            copyed_len += len;
        }
    }

    if ((print_len) && ((print_len % 16) != 0)) {
        strlcpy(buf + copyed_len, (char*)"\n", reserved_size);
    }

    return print_len;
}

/* 打印内存字面值到buf中.
return: 实际打印了多少字节内存的字面值 */
int MEM_Sprint(void *mem, UINT mem_len, OUT char *buf, int buf_size)
{
    UCHAR *d = mem;
    char info[64];
    int len = 0;
    int reserved_size = buf_size;
    int copyed_len = 0;
    int print_len = 0;

    /* 3: length("aa ") */
    while ((mem_len > print_len) && (reserved_size > 3)) {
        sprintf(info, "%02x ", *d);
        len = strlcpy(buf + copyed_len, info, reserved_size);
        reserved_size -= len;
        copyed_len += len;
        print_len ++;
        d ++;
        if ((print_len) && ((print_len % 16) == 0)) {
            buf[copyed_len - 1] = '\n';
        }
    }

    if ((print_len) && ((print_len % 16) != 0)) {
        buf[copyed_len - 1] = '\n';
    }

    return print_len;
}

void MEM_Print(void *mem, int len, PF_MEM_PRINT_FUNC print_func/* NULL使用缺省printf */)
{
    char info[3*16+1];
    PF_MEM_PRINT_FUNC func = print_func;
    int print_len = 0;

    if (! func) {
        func = (void*)printf;
    }

    while (len > print_len) {
        print_len += MEM_Sprint(mem + print_len, len - print_len, info, sizeof(info));
        func(info);
    }
}

void MEM_PrintCFormat(void *mem, int len, PF_MEM_PRINT_FUNC print_func/* NULL使用缺省printf */)
{
    char info[5*16+2];
    PF_MEM_PRINT_FUNC func = print_func;
    int print_len = 0;

    if (! func) {
        func = (void*)printf;
    }

    while (len > print_len) {
        print_len += MEM_SprintCFromat(mem + print_len, len - print_len, info, sizeof(info));
        func(info);
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
void MEM_Swap(void *buf1, void *buf2, int len)
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

