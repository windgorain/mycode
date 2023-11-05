/*================================================================
*   Created by LiXingang
*   Description: 数据队列，放置len+data的队列
*
================================================================*/
#include "bs.h"
#include "utl/data_que.h"

void DataQue_InitWriter(DATA_QUE_S *ctrl, void *buf, int size)
{
    ctrl->writer_base = buf;
    ctrl->size = size;
    ctrl->read = 0;
    ctrl->write = 0;
}

void DataQue_InitReader(DATA_QUE_S *ctrl, void *buf)
{
    ctrl->reader_base = buf;
}


int DataQue_Used(DATA_QUE_S *ctrl)
{
    int len = ctrl->write - ctrl->read;

    if (len < 0) {
        len = ctrl->size + len;
    }

    return len;
}

int DataQue_EmptyLen(DATA_QUE_S *ctrl)
{
    int len = ctrl->read - ctrl->write;

    if (len < 0) {
        len = ctrl->size + len;
    }

    return len;
}

int DataQue_Push(DATA_QUE_S *ctrl, void *data, int len)
{
    DATA_QUE_DATA_S *data_node;
    int need_len = len + sizeof(DATA_QUE_DATA_S);
    UCHAR *start = ctrl->writer_base;
    int read = ctrl->read;

    if (ctrl->write >= read) {
        if (ctrl->write + need_len > ctrl->size) { 
            data_node = (void*)(start + ctrl->write);
            data_node->len = 0; 
            ctrl->write = 0;
        }
    }

    if (read > ctrl->write) {
        if (ctrl->write + need_len >= read) {
            return BS_FULL;
        }
    } 

    data_node = (void*)(start + ctrl->write);
    data_node->len = len;
    memcpy(data_node->data, data, len);
    ctrl->write += need_len;

    
    if (ctrl->size - ctrl->write <= sizeof(DATA_QUE_DATA_S)) {
        ctrl->write = 0;
    }

    return 0;
}

DATA_QUE_DATA_S * DataQue_Get(DATA_QUE_S *ctrl)
{
    DATA_QUE_DATA_S *data_node;
    UCHAR *start = ctrl->reader_base;
    int write = ctrl->write;

    if (ctrl->read == write) {
        return NULL;
    }

    data_node = (void*)(start + ctrl->read);
    if (data_node->len == 0) {
        ctrl->read = 0;
    }

    if (ctrl->read == write) {
        return NULL;
    }

    data_node = (void*)(start + ctrl->read);

    return data_node;
}

void DataQue_Pop(DATA_QUE_S *ctrl, DATA_QUE_DATA_S *node)
{
    int need_len = node->len + sizeof(DATA_QUE_DATA_S);
    ctrl->read += need_len;

    if ((ctrl->read >= ctrl->size)
            || (ctrl->size - ctrl->read <= sizeof(DATA_QUE_DATA_S))) {
        ctrl->read = 0;
    }
}

