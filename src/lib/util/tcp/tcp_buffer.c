/*================================================================
*   Created by LiXingang
*   Description: 将tcp payload缓存起来
*
================================================================*/
#include "bs.h"
#include "utl/tcp_utl.h"
#include "utl/hole_data.h"
#include "utl/tcp_buffer.h"


typedef struct {
    unsigned int payload_begin_sn; /* tcp数据的起始sn(不包含syn), 主机序 */
    unsigned int buffer_begin_sn; /* 本次缓冲的起始sn, 主机序 */
    HOLE_DATA_S holedata;
    UINT failed:1;  /* 重组失败 */
    UCHAR data[0];
}TCPB_NODE_S;

int TCPB_Init(OUT TCPB_S *tcpb, int max_count, int max_buff_size)
{
    NAP_PARAM_S param = {0};
    int node_size = sizeof(TCPB_NODE_S) + max_buff_size
        + BITMAP_NEED_BYTES(max_buff_size);

    param.enType = NAP_TYPE_HASH;
    param.uiMaxNum = max_count;
    param.uiNodeSize = node_size;
    tcpb->hNap = NAP_Create(&param);
    if (tcpb->hNap == NULL) {
        RETURN(BS_NO_MEMORY);
    }

    tcpb->max_buff_size = max_buff_size;

    return 0;
}

void TCPB_Final(TCPB_S *tcpb)
{
    if (tcpb->hNap) {
        NAP_Destory(tcpb->hNap);
        tcpb->hNap = NULL;
    }
    tcpb->max_buff_size = 0;
}

TCPB_S * TCPB_New(int max_count, int max_buff_size)
{
    TCPB_S *ctrl = MEM_ZMalloc(sizeof(TCPB_S));
    if (ctrl) {
        if (0 != TCPB_Init(ctrl, max_count, max_buff_size)) {
            MEM_Free(ctrl);
            return NULL;
        }
    }

    return ctrl;
}

void TCPB_Delete(TCPB_S *ctrl)
{
    TCPB_Final(ctrl);
    MEM_Free(ctrl);
}

void * TCPB_Find(TCPB_S *tcpb, UINT index)
{
    return NAP_GetNodeByIndex(tcpb->hNap, index);
}

static void tcbp_InitNode(TCPB_S *tcpb, TCPB_NODE_S *node, int payload_sn)
{
    memset(node, 0, sizeof(TCPB_NODE_S));
    HoleData_Init(&node->holedata, node->data, tcpb->max_buff_size,
            node->data + tcpb->max_buff_size);
    node->buffer_begin_sn = 0;
    node->payload_begin_sn = payload_sn;
}

static void tcbp_FinalNode(TCPB_S *tcpb, TCPB_NODE_S *node)
{
    HoleData_Final(&node->holedata);
}

void * TCPB_AlocSpec(TCPB_S *tcpb, UINT index, UINT payload_sn)
{
    TCPB_NODE_S *node;

    node = NAP_AllocByIndex(tcpb->hNap, index);
    if (node == NULL) {
        return NULL;
    }

    tcbp_InitNode(tcpb, node, payload_sn);

    return node;
}

void * TCPB_Alloc(TCPB_S *tcpb, UINT payload_sn)
{
    TCPB_NODE_S *node;

    node = NAP_Alloc(tcpb->hNap);
    if (node == NULL) {
        return NULL;
    }

    tcbp_InitNode(tcpb, node, payload_sn);

    return node;
}

void TCPB_Free(TCPB_S *tcpb, UINT index)
{
    TCPB_NODE_S *node = NAP_GetNodeByIndex(tcpb->hNap, index);

    if (node) {
        tcbp_FinalNode(tcpb, node);
        NAP_Free(tcpb->hNap, node);
    }
}

void TCPB_Alloc_SetPayloadSn(TCPB_S *tcpb, void *tcpb_node, UINT payload_sn)
{
    TCPB_NODE_S *node = tcpb_node;
    if (node) {
        node->payload_begin_sn = payload_sn;
    }
}

static int tcpb_Save(TCPB_S *tcpb, TCPB_NODE_S *node, UINT sn,
        UCHAR *payload, int payload_len)
{
    int buff_offset;

    if (HoleData_GetFilledSize(&node->holedata) == 0) {
        /* 空的缓冲区,直接从开始位置开始填 */
        buff_offset = 0;
        node->buffer_begin_sn = sn;
    } else {
        buff_offset = (int)sn - (int)node->buffer_begin_sn;
        if (buff_offset < 0) {
            return 0;
        }
        if (buff_offset >= tcpb->max_buff_size) {
            return (BS_OUT_OF_RANGE);
        }
    }

    int saved_len = HoleData_Input(&node->holedata, payload, buff_offset, payload_len);

    return saved_len;
}

static int tcpb_SaveAndSend(TCPB_S *tcpb, TCPB_NODE_S *node, UINT sn,
        UCHAR *payload, int payload_len, PF_TCPB_OUTPUT output, void *ud)
{
    TCPB_BUF_S buf = {0};
    int last_continue_len = HoleData_GetContineLen(&node->holedata);
    int saved_len = tcpb_Save(tcpb, node, sn, payload, payload_len);
    int current_continue_len = HoleData_GetContineLen(&node->holedata);

    if (last_continue_len != current_continue_len) {
        buf.offset = node->buffer_begin_sn - node->payload_begin_sn;
        buf.data = node->data;
        buf.len = current_continue_len;
        buf.need_save = 0;

        output(&buf, ud);

        if (buf.need_save == 0) {
            HoleData_Reset(&node->holedata);
        } else if (saved_len < payload_len) {
            /* 缓冲区已经放不下现在的报文了, 清除缓冲区 */
            HoleData_Reset(&node->holedata);
            return BS_FULL;
        }
    }

    return saved_len;
}

static int tcpb_SendAndSave(TCPB_S *tcpb, TCPB_NODE_S *node, UINT sn,
        UCHAR *payload, int payload_len, PF_TCPB_OUTPUT output, void *ud)
{
    TCPB_BUF_S buf = {0};

    BS_DBGASSERT(HoleData_GetFilledSize(&node->holedata) == 0);

    buf.offset = sn - node->payload_begin_sn;
    buf.data = payload;
    buf.len = payload_len;
    buf.need_save = 0;

    output(&buf, ud);

    if (buf.need_save) {
        UINT need_save_len = (UINT)payload_len - buf.save_offset;
        UINT saved_len = tcpb_Save(tcpb, node, sn + buf.save_offset, payload + buf.save_offset, need_save_len);
        if (saved_len < need_save_len) {
            return BS_FULL;
        }
    }

    return 0;
}

static int tcpb_Input(TCPB_S *tcpb, TCPB_NODE_S *node, UINT sn,
        UCHAR *payload, int payload_len, PF_TCPB_OUTPUT output, void *ud)
{
    UINT sn_tmp = sn;
    UCHAR *payload_tmp = payload;
    int payload_len_tmp = payload_len;
    int ret = 0;

    while (payload_len_tmp > 0) {
        if (HoleData_GetFilledSize(&node->holedata) == 0) {
            ret = tcpb_SendAndSave(tcpb, node, sn_tmp, payload_tmp, payload_len_tmp, output, ud);
            if (ret < 0) {
                break;
            }
            payload_len_tmp = 0;
        } else {
            ret = tcpb_SaveAndSend(tcpb, node, sn_tmp, payload_tmp, payload_len_tmp, output, ud);
            if (ret <= 0) {
                break;
            }
            sn_tmp += ret;
            payload_tmp += ret;
            payload_len_tmp -= ret;
        }
    }

    return ret;
}

static void tcpb_FailedSend(TCPB_S *tcpb, TCPB_NODE_S *node, PF_TCPB_OUTPUT output, void *ud)
{
    TCPB_BUF_S buf = {0};
    int continue_len = HoleData_GetContineLen(&node->holedata);

    if (continue_len > 0) {
        buf.offset = node->buffer_begin_sn - node->payload_begin_sn;
        buf.data = node->data;
        buf.len = continue_len;
        buf.need_save = 0;
        buf.failed = 1;

        output(&buf, ud);
    }
}

/* sn为主机序 */
int TCPB_Input(TCPB_S *tcpb, void *tcpb_node, UINT sn,
        void *payload, int payload_len, PF_TCPB_OUTPUT output, void *ud)
{
    TCPB_NODE_S *node = tcpb_node;

    BS_DBGASSERT(NULL != node);

    if (node->failed) {
        return BS_ERR;
    }

    int ret = tcpb_Input(tcpb, node, sn, payload, payload_len, output, ud);
    if (ret < 0) {
        node->failed = 1;
        /* 最后一次通知,只能缓存这么多了,后面再也不会通知了 */
        tcpb_FailedSend(tcpb, tcpb_node, output, ud);
    }

    return ret;
}

void TCPB_SetSaveOffset(TCPB_BUF_S *tcpbuf, UINT offset)
{
    if (tcpbuf->need_save) {
        tcpbuf->save_offset = MIN(offset, tcpbuf->save_offset);
    } else {
        tcpbuf->save_offset = offset;
    }
    tcpbuf->need_save = 1;
}

