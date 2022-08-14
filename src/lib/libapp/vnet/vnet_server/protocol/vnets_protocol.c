
#include "bs.h"

#include "utl/msgque_utl.h"
#include "utl/kf_utl.h"

#include "../inc/vnets_mac_tbl.h"
#include "../inc/vnets_protocol.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_node.h"
#include "../inc/vnets_node_ctrl.h"
#include "../inc/vnets_p_nodeinfo.h"
#include "../inc/vnets_tp.h"
#include "../inc/vnets_vpn_link.h"
#include "../inc/vnets_master.h"

#define VNETS_PROTOCOL_NEED_AUTHED 0x1

/* Debug 选项 */
#define _VNETS_PROTOCOL_DBG_PACKET 0x1
#define _VNETS_PROTOCOL_DBG_ERROR  0x2

typedef struct
{
    UINT uiFlag;
    CHAR *pcDoKey;
    PF_VNETS_PROTOCOL_DO_FUNC pfDoFunc;
}VNETS_PROTOCOL_DO_NODE_S;

static UINT g_ulVnetsProtocolDebugFlag = 0;
static KF_HANDLE g_hVnetsProtocolKf = NULL;

static VNETS_PROTOCOL_DO_NODE_S g_astVnetsProtocolDoTbl[] = 
{
    {0, "Auth", VNETS_Auth_Input},
    {VNETS_PROTOCOL_NEED_AUTHED, "Logout", VNETS_Logout_Recv},
    {VNETS_PROTOCOL_NEED_AUTHED, "EnterDomain", VNETS_EnterDomain_Input},
    {VNETS_PROTOCOL_NEED_AUTHED, "GetNodeInfo", VNETS_PeerInfo_Input},
    {VNETS_PROTOCOL_NEED_AUTHED, "UpdateNodeInfo", VNETS_P_NodeInfo_Input},
};

BS_STATUS VNETS_Protocol_SendData
(
    IN UINT uiTpID,
    IN CHAR *pcData,
    IN UINT ulDataLen
)
{
    MBUF_S *pstMbuf;

    pstMbuf = VNETS_Context_CreateMbufByCopyBuf(128, (void*)pcData, ulDataLen);
    if (NULL == pstMbuf)
    {
        return(BS_ERR);
    }

    if (g_ulVnetsProtocolDebugFlag & _VNETS_PROTOCOL_DBG_PACKET)
    {
        BS_DBG_OUTPUT(g_ulVnetsProtocolDebugFlag, _VNETS_PROTOCOL_DBG_PACKET,
            ("VNETS-PROTOCOL:Send packet to tp 0x%08x, packet:%s.\r\n",
               uiTpID, pcData));
    }

    VNETS_Context_SetSrcNodeID(pstMbuf, VNETS_NODE_Self());

    return VNETS_TP_Output(uiTpID, pstMbuf);
}

static BS_STATUS vnets_protocol_Input(IN UINT uiTpID, IN MBUF_S *pstMbuf)
{
    CHAR *pcData;
    VNETS_PROTOCOL_PACKET_INFO_S stPacketInfo;

    if (BS_OK != MBUF_MakeContinue(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf)))
    {
        BS_DBG_OUTPUT(g_ulVnetsProtocolDebugFlag, _VNETS_PROTOCOL_DBG_ERROR,
            ("VNETS-PROTOCOL:Make mbuf continue failed when process packet from TP 0x%08x.\r\n", uiTpID));
        return(BS_BAD_PARA);
    }

    pcData = MBUF_MTOD(pstMbuf);
    
    BS_DBG_OUTPUT(g_ulVnetsProtocolDebugFlag, _VNETS_PROTOCOL_DBG_PACKET,
        ("VNETS-PROTOCOL:Receive protocol packet from TP 0x%08x, data: %s.\r\n", uiTpID, pcData));

    stPacketInfo.uiTpID = uiTpID;
    stPacketInfo.pstMBuf = pstMbuf;

    return KF_RunString(g_hVnetsProtocolKf, ',', pcData, &stPacketInfo);
}

BS_STATUS VNETS_Protocol_Input(IN UINT uiTpID, IN MBUF_S *pstMbuf)
{
    BS_STATUS eRet;

    eRet = vnets_protocol_Input(uiTpID, pstMbuf);

    MBUF_Free(pstMbuf);

    return eRet;
}

static BS_STATUS vnets_protocol_KfCallBack(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN HANDLE hRunHandle)
{
    VNETS_PROTOCOL_DO_NODE_S *pstDoNode = hUserHandle;
    VNETS_PROTOCOL_PACKET_INFO_S *pstPacketInfo = hRunHandle;

    if (pstDoNode->uiFlag & VNETS_PROTOCOL_NEED_AUTHED)
    {
        if (FALSE == VNETS_Context_CheckFlag(pstPacketInfo->pstMBuf, VNETS_CONTEXT_FLAG_ONLINE))
        {
            VNETS_NodeCtrl_ReplyOffline(VNETS_Context_GetRecvSesID(pstPacketInfo->pstMBuf));
            return BS_NO_PERMIT;
        }
    }

    return pstDoNode->pfDoFunc(hMime, pstPacketInfo);
}

BS_STATUS VNETS_Protocol_Init()
{
    UINT i;

    g_hVnetsProtocolKf = KF_Create("Protocol");
    if (NULL == g_hVnetsProtocolKf)
    {
        return BS_NO_MEMORY;
    }

    for (i=0; i<sizeof(g_astVnetsProtocolDoTbl)/sizeof(VNETS_PROTOCOL_DO_NODE_S); i++)
    {
        KF_AddFunc(g_hVnetsProtocolKf, g_astVnetsProtocolDoTbl[i].pcDoKey, vnets_protocol_KfCallBack, &g_astVnetsProtocolDoTbl[i]);
    }

    return BS_OK;
}


/* debug protocol packet */
PLUG_API VOID VNETS_Protocol_DebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetsProtocolDebugFlag |= _VNETS_PROTOCOL_DBG_PACKET;
}

/* no debug protocol packet */
PLUG_API VOID VNETS_Protocol_NoDebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetsProtocolDebugFlag &= ~_VNETS_PROTOCOL_DBG_PACKET;
}

VOID VNETS_Protocol_NoDebugAll()
{
    g_ulVnetsProtocolDebugFlag = 0;
}

