/*================================================================
* Author：LiXingang. Data: 2019.07.30
* Description: TCP 乱序报文重组
*              在丢包率较高的网络中,inline模式它会导致网络性能降低,
================================================================*/
#include "bs.h"
#include "utl/tcp_utl.h"
#include "utl/tcp_reassemble.h"

typedef struct {
    DLL_NODE_S link_node;
    void *pkt;
    int pkt_sn;
    USHORT len;
    UCHAR next;
}TCP_REASSEMBLE_PKT_S;

static BOOL_T tcpreassemble_IsHaveThisPkt(TCP_REASSEMBLE_FLOW_S *flow, int sn)
{
    int offset;
    TCP_REASSEMBLE_PKT_S *pktnode;

    DLL_SCAN(&flow->pkts_list, pktnode) {
        offset = sn - pktnode->pkt_sn;
        if (offset < 0) {
            break;
        }
        if (offset == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

static int tcpreassemble_CmpPkt(DLL_NODE_S *pstNode1, DLL_NODE_S *pstNode2, HANDLE hUserHandle)
{
    TCP_REASSEMBLE_PKT_S *pktnode1 = (void*)pstNode1;
    TCP_REASSEMBLE_PKT_S *pktnode2 = (void*)pstNode2;

    return pktnode1->pkt_sn - pktnode2->pkt_sn;
}

static int tcpreassemmble_InsertPkt(TCP_REASSEMBLE_S *ctrl, TCP_REASSEMBLE_FLOW_S *flow, TCP_REASSEMBLE_PKTINFO_S *pktinfo)
{
    void *pkt = pktinfo->pkt;
    TCP_REASSEMBLE_PKT_S *pktnode;

    if (DLL_COUNT(&flow->pkts_list) > ctrl->max_segs_per_flow) {
        RETURN(BS_OUT_OF_RANGE);
        return -1;
    }

    pktnode = MEM_ZMalloc(sizeof(TCP_REASSEMBLE_PKT_S));
    if (! pktnode) {
        /* 内存申请失败,重组失败 */
        RETURN(BS_NO_MEMORY);
    }

    if (ctrl->dup_pkt) {
        pkt = ctrl->dup_pkt(pkt);
        if (pkt == NULL) {
            MEM_Free(pktnode);
            /* 内存申请失败,重组失败 */
            RETURN(BS_NO_MEMORY);
        }
    }

    pktnode->pkt = pkt;
    pktnode->pkt_sn = pktinfo->sn;
    pktnode->len = pktinfo->tcp_payload_len;

    DLL_SortAdd(&flow->pkts_list, &pktnode->link_node, tcpreassemble_CmpPkt, NULL);

    return 0;
}

static void tcpreassemble_FreePkt(TCP_REASSEMBLE_S *ctrl,
        TCP_REASSEMBLE_PKT_S *pktnode)
{
    if (ctrl->free_dup_pkt) {
        ctrl->free_dup_pkt(pktnode->pkt);
    }
    MEM_Free(pktnode);
}

static void tcpreassemble_TryReassemble(TCP_REASSEMBLE_S *ctrl,
        TCP_REASSEMBLE_FLOW_S *flow,
        PF_TCP_REASSEMBLE_OUT pkt_out)
{
    int offset;
    TCP_REASSEMBLE_PKT_S *pktnode;

    while ((pktnode = DLL_FIRST(&flow->pkts_list))) {
        offset = flow->sn - pktnode->pkt_sn;
        if (offset < 0) {
            break;
        }
        DLL_DEL(&flow->pkts_list, pktnode);
        flow->sn = pktnode->pkt_sn + pktnode->len;
        pkt_out(pktnode->pkt);
        tcpreassemble_FreePkt(ctrl, pktnode);
    }
}

void TcpR_Init(TCP_REASSEMBLE_S *ctrl)
{
#define TCP_REASSEMBLE_DFT_MAX_SEGS 128

    memset(ctrl, 0, sizeof(TCP_REASSEMBLE_S));
    ctrl->max_segs_per_flow = TCP_REASSEMBLE_DFT_MAX_SEGS;
}

void TcpR_Final(TCP_REASSEMBLE_S *ctrl)
{
    memset(ctrl, 0, sizeof(TCP_REASSEMBLE_S));
}

/* 在缓存报文的时候可以选择复制报文 */
void TcpR_SetDupPkt(TCP_REASSEMBLE_S *ctrl,
        PF_TCP_REASSEMBLE_DUP_PKT dup_pkt,
        PF_TCP_REASSEMBLE_FREE_DUPPED_PKT free_dup_pkt)
{
    ctrl->dup_pkt = dup_pkt;
    ctrl->free_dup_pkt = free_dup_pkt;
}

void TcpR_InitFlow(TCP_REASSEMBLE_FLOW_S *flow, int init_sn)
{
    memset(flow, 0, sizeof(TCP_REASSEMBLE_FLOW_S));
    flow->sn = init_sn;
    DLL_INIT(&flow->pkts_list);
}

void TcpR_ResetFlow(TCP_REASSEMBLE_S *ctrl, TCP_REASSEMBLE_FLOW_S *flow, int init_sn)
{
    TcpR_Flush(ctrl, flow, NULL);
    memset(flow, 0, sizeof(TCP_REASSEMBLE_FLOW_S));
    flow->sn = init_sn;
    DLL_INIT(&flow->pkts_list);
}

void TcpR_FinalFlow(TCP_REASSEMBLE_S *ctrl, TCP_REASSEMBLE_FLOW_S *flow)
{
    TcpR_Flush(ctrl, flow, NULL);
    memset(flow, 0, sizeof(TCP_REASSEMBLE_FLOW_S));
}

int TcpR_Input(TCP_REASSEMBLE_S *ctrl, TCP_REASSEMBLE_FLOW_S *flow,
        TCP_REASSEMBLE_PKTINFO_S *pktinfo, PF_TCP_REASSEMBLE_OUT pkt_out)
{
    int offset;

    offset = pktinfo->sn - flow->sn;

    if (offset < 0) { /* 重复报文 */
        return 0;
    }

    if (tcpreassemble_IsHaveThisPkt(flow, pktinfo->sn)) {
        return 0;
    }

    if (offset > 0) { /* 乱序报文 */
        return tcpreassemmble_InsertPkt(ctrl, flow, pktinfo);
    }

    if (offset == 0) { /* 命中需要的序号 */
        pkt_out(pktinfo->pkt);
        flow->sn += pktinfo->tcp_payload_len;
        if (pktinfo->flag & (TCP_FLAG_SYN | TCP_FLAG_FIN)) {
            flow->sn ++;
        }
    }

    tcpreassemble_TryReassemble(ctrl, flow, pkt_out);

    return 0;
}

/* 即使没有重组成功，也将缓冲的报文都发出 */
void TcpR_Flush(TCP_REASSEMBLE_S *ctrl,
        TCP_REASSEMBLE_FLOW_S *flow, PF_TCP_REASSEMBLE_OUT pkt_out/* 可以为NULL */)
{
    TCP_REASSEMBLE_PKT_S *pktnode;

    while ((pktnode = DLL_Get(&flow->pkts_list))) {
        if (pkt_out) {
            pkt_out(pktnode->pkt);
        }
        tcpreassemble_FreePkt(ctrl, pktnode);
    }
}


