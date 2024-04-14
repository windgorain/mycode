/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-10-25
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/mem_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/ulc_user.h"

static inline void _mem_swap(void *buf1, void *buf2, int len)
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
    int print_len = 0;

    while (len > print_len) {
        print_len += MEM_Sprint(mem + print_len, len - print_len, info, sizeof(info));
        if (print_func) {
            print_func(info);
        } else {
            printf("%s", info);
        }
    }
}

void MEM_PrintCFormat(void *mem, int len, PF_MEM_PRINT_FUNC print_func/* NULL使用缺省printf */)
{
    char info[5*16+2];
    int print_len = 0;

    while (len > print_len) {
        print_len += MEM_SprintCFromat(mem + print_len, len - print_len, info, sizeof(info));
        if (print_func) {
            print_func(info);
        } else {
            printf("%s", info);
        }
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

/* 交换两块内存的内容, 交换双方长度相等 */
void MEM_Swap(void *buf1, void *buf2, int len)
{
    _mem_swap(buf1, buf2, len);
}

/* 根据off, 交换一块连续内存内的两片内存的位置. 比如: 1 | 2 3 交换为 2 3 | 1, |为off位置 */
/* off: 第二块内存的offset */
int MEM_SwapByOff(void *buf, int buf_len, int off)
{
    if (off >= buf_len) {
        return 0;
    }

    if (off * 2 == buf_len) { /* 需要交换的两块内存大小相等 */
        _mem_swap(buf, (char*)buf + off, off);
        return 0;
    }

    void *tmp = MEM_Malloc(off);
    if (! tmp) {
        RETURN(BS_NO_MEMORY);
    }

    memcpy(tmp, buf, off);

    memmove(buf, (char*)buf + off, buf_len - off);
    memcpy((char*)buf + (buf_len - off), tmp, off);

    MEM_Free(tmp);

    return 0;
}

/* move buf to (buf + offset) */
/* 移动一块连续内存中的数据到这块连续内存的新位置. 周围的数据会被合上之后再挤进新位置去 */
/* 使用者需要确保移动的起始和终止位置之间的内存合法连续的 */
int MEM_MoveData(void *data, S64 len, S64 offset)
{
    if (offset == 0) {
        return 0;
    }

    void *tmp = MEM_Malloc(len);
    if (! tmp) {
        RETURN(BS_NO_MEMORY);
    }

    memcpy(tmp, data, len);

    if (offset < 0) { /* 往前移动数据的情况 */
        char *d = (char*)data + offset;
        memmove(d + len, d, -offset);
    } else { /* 向后移动的情况 */
        char *d = (char*)data + len;
        memmove(data, d, offset);
    }

    memcpy((char*)data + offset, tmp, len);

    MEM_Free(tmp);

    return 0;
}

/* move buf to dst */
/* 移动一块连续内存中的数据到这块连续内存的新位置. 周围的数据会被合上之后再挤进新位置去 */
/* 使用者需要确保移动的起始和终止位置之间的内存合法连续的 */
int MEM_MoveDataTo(void *data, U64 len, void *dst)
{
    return MEM_MoveData(data, len, (S64)dst - (S64)data);
}

void MEM_CopyWithCheck(void *dst, void *src, U32 len)
{
    /* check是否重叠, 重叠了的话,告警,应该使用memmove */
    {
        char *d1_min = dst;
        char *d1_max = (d1_min + len) - 1;
        char *d2_min = src;
        char *d2_max = (d2_min + len) - 1;
        if (NUM_AREA_IS_OVERLAP(d1_min, d1_max, d2_min, d2_max) != FALSE) {
            BS_DBGASSERT(0);
        }
    }
    memcpy(dst, src, len);
}

