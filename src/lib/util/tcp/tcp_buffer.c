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
    UCHAR data[0];
}TCPB_NODE_S;

int TCPB_Init(OUT TCPB_S *tcpb, int max_count, int max_buff_size)
{
    int node_size = sizeof(TCPB_NODE_S) + max_buff_size
        + BITMAP_NEED_BYTES(max_buff_size);

    tcpb->hNap = NAP_Create(NAP_TYPE_PTR_ARRAY, max_count, node_size, 0);
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

void TCB_Alloc_SetPayloadSn(TCPB_S *tcpb, void *tcpb_node, UINT payload_sn)
{
    TCPB_NODE_S *node = tcpb_node;
    if (node) {
        node->payload_begin_sn = payload_sn;
    }
}

static int tcpb_Input(TCPB_S *tcpb, TCPB_NODE_S *node, UINT sn,
        UCHAR *payload, int payload_len, PF_TCPB_OUTPUT output, void *ud)
{
    int buff_offset;
    int saved_len;

    if (HoleData_GetFilledSize(&node->holedata) == 0) {
        /* 空的缓冲区,直接从开始位置开始填 */
        buff_offset = 0;
        node->buffer_begin_sn = sn;
    } else {
        buff_offset = (int)sn - (int)node->buffer_begin_sn;
        if (buff_offset >= tcpb->max_buff_size) {
            RETURN(BS_OUT_OF_RANGE);
        }
    }

    int last_continue_len = HoleData_GetContineLen(&node->holedata);
    saved_len = HoleData_Input(&node->holedata,
            payload, buff_offset, payload_len);
    int current_continue_len = HoleData_GetContineLen(&node->holedata);
    if (last_continue_len != current_continue_len) {
        UINT tcp_offset = node->buffer_begin_sn - node->payload_begin_sn;
        output(tcp_offset, node->data, current_continue_len, ud);
    }

    if (HoleData_IsFinished(&node->holedata)) {
        /* 缓冲区已满, 清除缓冲区 */
        HoleData_Reset(&node->holedata);
        if (saved_len < payload_len) {
            return tcpb_Input(tcpb, node, sn+saved_len, payload + saved_len,
                    payload_len - saved_len, output, ud);
        }
    }

    return 0;
}

/* sn为主机序 */
int TCPB_Input(TCPB_S *tcpb, void *tcpb_node, UINT sn,
        void *payload, int payload_len, PF_TCPB_OUTPUT output, void *ud)
{
    TCPB_NODE_S *node = tcpb_node;
    if (node == NULL) {
        RETURN(BS_NOT_FOUND);
    }

    return tcpb_Input(tcpb, node, sn, payload, payload_len, output, ud);
}

