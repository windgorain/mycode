/******************************************************************************
* Copyright (C), LiXingang
* Author:      lixingang  Version: 1.0  Date: 2017-2-9
* Description: data with length.
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"


void LDATA_StrimHead(LDATA_S *data, LDATA_S *rm_chars, OUT LDATA_S *out_data)
{
    BS_DBGASSERT(NULL != data);
    BS_DBGASSERT(NULL != out_data);
    BS_DBGASSERT(NULL != rm_chars);
    BS_DBGASSERT(NULL != data->data);

    if (0 == data->len) {
        out_data->data = data->data;
        out_data->len = 0;
        return;
    }

    UCHAR *tmp = data->data;
    UCHAR *end = tmp + data->len;

    while (tmp < end) {
        if(NULL == MEM_FindOne(rm_chars->data, rm_chars->len, *tmp)) {
            break;
        }
        tmp ++;
    }

    out_data->data = tmp;
    out_data->len = end - tmp;

    return;
}


void LDATA_StrimTail(LDATA_S *data, LDATA_S *rm_chars, OUT LDATA_S *out_data)
{
    BS_DBGASSERT(NULL != data);
    BS_DBGASSERT(NULL != out_data);
    BS_DBGASSERT(NULL != rm_chars);
    BS_DBGASSERT(NULL != data->data);

    if (0 == data->len) {
        out_data->data = NULL;
        out_data->len = 0;
        return;
    }

    UCHAR *tmp = (data->data + data->len) - 1;

    while (tmp >= data->data) {
        if (NULL == MEM_FindOne(rm_chars->data, rm_chars->len, *tmp)) {
            break;
        }
        tmp --;
    }

    out_data->data = data->data;
    out_data->len = (tmp + 1) - data->data;

    return;
}


void LDATA_Strim(LDATA_S *data, LDATA_S *rm_chars, OUT LDATA_S *out_data)
{
    LDATA_StrimHead(data, rm_chars, out_data);
    LDATA_StrimTail(out_data, rm_chars, out_data);
}

void * LDATA_Strchr(LDATA_S *data, UCHAR ch)
{
    BS_DBGASSERT(NULL != data);
    return MEM_FindOne(data->data, data->len, ch);
}


void * LDATA_ReverseChr(LDATA_S *data, UCHAR ch)
{
    BS_DBGASSERT(NULL != data);

    UCHAR *buf = data->data;
    UINT len = data->len;
    UINT i;

    if (0 == len) {
        return NULL;
    }

    for (i=len; i>0; i--) {
        if (buf[i-1] == ch) {
            return &buf[i-1];
        }
    }

    return NULL;
}


void * LDATA_Strstr(LDATA_S *data, LDATA_S *to_find)
{
    BS_DBGASSERT(NULL != data);
    BS_DBGASSERT(NULL != to_find);

    return MEM_Find(data->data, data->len, to_find->data, to_find->len);
}


void * LDATA_Stristr(LDATA_S *data, LDATA_S *to_find)
{
    BS_DBGASSERT(NULL != data);
    BS_DBGASSERT(NULL != to_find);

    return MEM_CaseFind(data->data, data->len, to_find->data, to_find->len);
}

static inline void ldata_split_from(LDATA_S *in, UCHAR *split_pos, OUT LDATA_S *out1, OUT LDATA_S *out2)
{
    if (NULL == split_pos) {
        out1->data = in->data;
        out1->len = in->len;
        out2->data = NULL;
        out2->len = 0;
    } else {
        out1->data = in->data;
        out1->len =  split_pos - in->data;
        out2->data = split_pos + 1;
        out2->len = in->len - (out1->len + 1);
    }
}


void LDATA_MSplit(LDATA_S *in, LDATA_S *split_chars, OUT LDATA_S *out1, OUT LDATA_S *out2)
{
    UCHAR *split = MEM_FindOneOf(in->data, in->len, split_chars->data, split_chars->len);
    ldata_split_from(in, split, out1, out2);
}


void LDATA_Split(LDATA_S *in, UCHAR split_char, OUT LDATA_S *out1, OUT LDATA_S *out2)
{
    UCHAR *split = LDATA_Strchr(in, split_char);
    ldata_split_from(in, split, out1, out2);
}


UINT LDATA_XSplit(LDATA_S *in, UCHAR split_char, OUT LDATA_S *outs, UINT count )
{
    LDATA_S stReserved;
    UINT i = 0;

    stReserved = *in;

    while ((i < count) && (stReserved.len > 0)) {
        LDATA_Split(&stReserved, split_char, &outs[i], &stReserved);
        i++;
    }

    return i;
}


int LDATA_Cmp(LDATA_S *data1, LDATA_S *data2)
{
    BS_DBGASSERT(NULL != data1);
    BS_DBGASSERT(NULL != data2);

    return MEM_Cmp(data1->data, data1->len, data2->data, data2->len);
}


int LDATA_CaseCmp(LDATA_S *data1, LDATA_S *data2)
{
    return MEM_CaseCmp(data1->data, data1->len, data2->data, data2->len);
}


