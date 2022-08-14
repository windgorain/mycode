/******************************************************************************
* Copyright (C), LiXingang
* Author:      lixingang  Version: 1.0  Date: 2017-2-9
* Description: data with length.
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"

/* 消除头字符 */
void LDATA_StrimHead(LDATA_S *data, LDATA_S *rm_chars, OUT LDATA_S *out_data)
{
    BS_DBGASSERT(NULL != data);
    BS_DBGASSERT(NULL != out_data);
    BS_DBGASSERT(NULL != rm_chars);
    BS_DBGASSERT(NULL != data->pucData);

    if (0 == data->uiLen) {
        out_data->pucData = data->pucData;
        out_data->uiLen = 0;
        return;
    }

    UCHAR *tmp = data->pucData;
    UCHAR *end = tmp + data->uiLen;

    while (tmp < end) {
        if(NULL == MEM_FindOne(rm_chars->pucData, rm_chars->uiLen, *tmp)) {
            break;
        }
        tmp ++;
    }

    out_data->pucData = tmp;
    out_data->uiLen = end - tmp;

    return;
}

/* 消除尾字符 */
void LDATA_StrimTail(LDATA_S *data, LDATA_S *rm_chars, OUT LDATA_S *out_data)
{
    BS_DBGASSERT(NULL != data);
    BS_DBGASSERT(NULL != out_data);
    BS_DBGASSERT(NULL != rm_chars);
    BS_DBGASSERT(NULL != data->pucData);

    if (0 == data->uiLen) {
        out_data->pucData = NULL;
        out_data->uiLen = 0;
        return;
    }

    UCHAR *tmp = (data->pucData + data->uiLen) - 1;

    while (tmp >= data->pucData) {
        if (NULL == MEM_FindOne(rm_chars->pucData, rm_chars->uiLen, *tmp)) {
            break;
        }
        tmp --;
    }

    out_data->pucData = data->pucData;
    out_data->uiLen = (tmp + 1) - data->pucData;

    return;
}

/* 消除头尾的字符 */
void LDATA_Strim(LDATA_S *data, LDATA_S *rm_chars, OUT LDATA_S *out_data)
{
    LDATA_StrimHead(data, rm_chars, out_data);
    LDATA_StrimTail(out_data, rm_chars, out_data);
}

void * LDATA_Strchr(LDATA_S *data, UCHAR ch)
{
    BS_DBGASSERT(NULL != data);
    return MEM_FindOne(data->pucData, data->uiLen, ch);
}

/* 反向查找字符 */
void * LDATA_ReverseChr(LDATA_S *data, UCHAR ch)
{
    BS_DBGASSERT(NULL != data);

    UCHAR *buf = data->pucData;
    UINT uiLen = data->uiLen;
    UINT i;

    if (0 == uiLen) {
        return NULL;
    }

    for (i=uiLen; i>0; i--) {
        if (buf[i-1] == ch) {
            return &buf[i-1];
        }
    }

    return NULL;
}

/* 查找数据 */
void * LDATA_Strstr(LDATA_S *data, LDATA_S *to_find)
{
    BS_DBGASSERT(NULL != data);
    BS_DBGASSERT(NULL != to_find);

    return MEM_Find(data->pucData, data->uiLen, to_find->pucData, to_find->uiLen);
}

/* 忽略大小写查找 */
void * LDATA_Stristr(LDATA_S *data, LDATA_S *to_find)
{
    BS_DBGASSERT(NULL != data);
    BS_DBGASSERT(NULL != to_find);

    return MEM_CaseFind(data->pucData, data->uiLen, to_find->pucData, to_find->uiLen);
}

static inline void ldata_split_from(LDATA_S *in, UCHAR *split_pos, OUT LDATA_S *out1, OUT LDATA_S *out2)
{
    if (NULL == split_pos) {
        out1->pucData = in->pucData;
        out1->uiLen = in->uiLen;
        out2->pucData = NULL;
        out2->uiLen = 0;
    } else {
        out1->pucData = in->pucData;
        out1->uiLen =  split_pos - in->pucData;
        out2->pucData = split_pos + 1;
        out2->uiLen = in->uiLen - (out1->uiLen + 1);
    }
}

/* 将数据分隔成两个 */
void LDATA_MSplit(LDATA_S *in, LDATA_S *split_chars, OUT LDATA_S *out1, OUT LDATA_S *out2)
{
    UCHAR *split = MEM_FindOneOf(in->pucData, in->uiLen, split_chars->pucData, split_chars->uiLen);
    ldata_split_from(in, split, out1, out2);
}

/* 将数据分隔成两个 */
void LDATA_Split(LDATA_S *in, UCHAR split_char, OUT LDATA_S *out1, OUT LDATA_S *out2)
{
    UCHAR *split = LDATA_Strchr(in, split_char);
    ldata_split_from(in, split, out1, out2);
}

/* 将数据分隔成多个, 返回分割成了多少个 */
UINT LDATA_XSplit(LDATA_S *in, UCHAR split_char, OUT LDATA_S *outs,/* 指向数组 */ UINT count /* 数组中元素个数 */)
{
    LDATA_S stReserved;
    UINT i = 0;

    stReserved = *in;

    while ((i < count) && (stReserved.uiLen > 0)) {
        LDATA_Split(&stReserved, split_char, &outs[i], &stReserved);
        i++;
    }

    return i;
}

/* 数据比较 */
int LDATA_Cmp(LDATA_S *data1, LDATA_S *data2)
{
    BS_DBGASSERT(NULL != data1);
    BS_DBGASSERT(NULL != data2);

    return MEM_Cmp(data1->pucData, data1->uiLen, data2->pucData, data2->uiLen);
}

/* 忽略大小写比较 */
int LDATA_CaseCmp(LDATA_S *data1, LDATA_S *data2)
{
    return MEM_CaseCmp(data1->pucData, data1->uiLen, data2->pucData, data2->uiLen);
}


