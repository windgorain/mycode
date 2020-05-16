
#include "bs.h"

#include "utl/msgque_utl.h"
#include "utl/kf_utl.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnetc_mac_tbl.h"
#include "../inc/vnetc_protocol.h"
#include "../inc/vnetc_auth.h"
#include "../inc/vnetc_p_nodeinfo.h"
#include "../inc/vnetc_tp.h"
#include "../inc/vnetc_node.h"
#include "../inc/vnetc_context.h"

/* Debug 选项 */
#define _VNETC_PROTOCOL_DBG_PACKET 0x1

typedef struct
{
    CHAR *pcDoKey;
    PF_VNETC_PROTOCOL_DO_FUNC pfDoFunc;
}VNETC_PROTOCOL_DO_NODE_S;

static UINT g_ulVnetcProtocolDebugFlag = 0;

static KF_HANDLE g_hVnetcProtocolKf = NULL;

static VNETC_PROTOCOL_DO_NODE_S g_astVnetcProtocolDoTbl[] = 
{
    {"Auth", VNETC_Auth_Input},
    {"Logout", VNETC_Logout_PktInput},
    {"EnterDomain", VNETC_EnterDomain_Input},
    {"GetNodeInfo", VNETC_PeerInfo_Input},
    {"UpdateNodeInfo", VNETC_P_NodeInfo_Input},
};

static BS_STATUS vnetc_protocol_KfCallBack(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN HANDLE hRunHandle)
{
    VNETC_PROTOCOL_DO_NODE_S *pstDoNode = hUserHandle;
    VNETC_PROTOCOL_PACKET_INFO_S *pstPacketInfo = hRunHandle;

    return pstDoNode->pfDoFunc(hMime, pstPacketInfo);
}

static BS_STATUS vnetc_protocol_Input(IN UINT uiTpID, IN MBUF_S *pstMbuf)
{
    CHAR *pcData;
    VNETC_PROTOCOL_PACKET_INFO_S stPacketInfo;

    if (BS_OK != MBUF_MakeContinue(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf)))
    {
        BS_DBG_OUTPUT(g_ulVnetcProtocolDebugFlag, _VNETC_PROTOCOL_DBG_PACKET,
            ("VNETC-PROTOCOL:Make mbuf continue failed when process packet from TP 0x%08x.\r\n", uiTpID));
        return BS_NO_MEMORY;
    }

    pcData = MBUF_MTOD(pstMbuf);
    
    BS_DBG_OUTPUT(g_ulVnetcProtocolDebugFlag, _VNETC_PROTOCOL_DBG_PACKET,
        ("VNETC-PROTOCOL:Receive protocol packet from TP 0x%08x, data: %s.\r\n", uiTpID, pcData));

    stPacketInfo.uiTpID = uiTpID;
    stPacketInfo.pstMBuf = pstMbuf;

    return KF_RunString(g_hVnetcProtocolKf, ',', pcData, &stPacketInfo);
}

BS_STATUS VNETC_Protocol_SendData
(
    IN UINT uiTpID,
    IN VOID *pData,
    IN UINT ulDataLen
)
{
    MBUF_S *pstMbuf;

    pstMbuf = VNETC_Context_CreateMbufByCopyBuf(128, pData, ulDataLen);
    if (NULL == pstMbuf)
    {
        return(BS_ERR);
    }

    VNETC_Context_SetSrcNID(pstMbuf, VNETC_NODE_Self());

    if (g_ulVnetcProtocolDebugFlag & _VNETC_PROTOCOL_DBG_PACKET)
    {
        BS_DBG_OUTPUT(g_ulVnetcProtocolDebugFlag, _VNETC_PROTOCOL_DBG_PACKET,
            ("VNETC-PROTOCOL:Send packet to TP %x, packet:%s.\r\n",
            uiTpID, pData));
    }

    return VNETC_TP_PktOutput(uiTpID, pstMbuf);
}

BS_STATUS VNETC_Protocol_Input(IN UINT uiTpId, IN MBUF_S *pstMbuf)
{
    BS_STATUS eRet;

    eRet = vnetc_protocol_Input(uiTpId, pstMbuf);

    MBUF_Free(pstMbuf);

    return eRet;
}

BS_STATUS VNETC_Protocol_Init()
{
    UINT i;

    g_hVnetcProtocolKf = KF_Create("Protocol");
    if (NULL == g_hVnetcProtocolKf)
    {
        return BS_NO_MEMORY;
    }

    for (i=0; i<sizeof(g_astVnetcProtocolDoTbl)/sizeof(VNETC_PROTOCOL_DO_NODE_S); i++)
    {
        KF_AddFunc(g_hVnetcProtocolKf, g_astVnetcProtocolDoTbl[i].pcDoKey, vnetc_protocol_KfCallBack, &g_astVnetcProtocolDoTbl[i]);
    }

    return BS_OK;
}

/* debug protocol packet */
PLUG_API VOID VNETC_Protocol_DebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetcProtocolDebugFlag |= _VNETC_PROTOCOL_DBG_PACKET;
}

/* no debug protocol packet */
PLUG_API VOID VNETC_Protocol_NoDebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetcProtocolDebugFlag &= ~_VNETC_PROTOCOL_DBG_PACKET;
}

