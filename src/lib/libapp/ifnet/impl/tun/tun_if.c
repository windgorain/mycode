/*================================================================
* Author：LiXingang. Data: 2019.08.19
* Description: 
*
================================================================*/
#include "bs.h"
#include "utl/sif_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/tun_utl.h"
#include "utl/rcu_utl.h"
#include "comp/comp_if.h"
#include "tun_pkt.h"

#define TUN_IF_MAX 16

typedef struct {
    char index[IF_MAX_NAME_INDEX_LEN + 1];
    IF_INDEX ifindex;
    TUN_FD tun_fd;
}TUN_IF_S;

static TUN_IF_S g_tun_ifs[TUN_IF_MAX];
static UINT g_uiTunIfType;
static UINT g_uiTunIfUserDataIndex = 0;

static BS_STATUS tun_IfCreate(IF_INDEX ifIndex)
{
    CHAR szIfName[IF_MAX_NAME_LEN + 1];
    CHAR *pcIndex;
    int i;

    IFNET_GetIfName(ifIndex, szIfName);
    pcIndex = IF_GetPhyIndexStringByIfName(szIfName);

    for (i=0; i<TUN_IF_MAX; i++) {
        if (g_tun_ifs[i].ifindex == IF_INVALID_INDEX) {
            strlcpy(g_tun_ifs[i].index, pcIndex, IF_MAX_NAME_INDEX_LEN + 1);
            g_tun_ifs[i].ifindex = ifIndex;
            break;
        }
    }

    if (i >= TUN_IF_MAX) {
        EXEC_OutInfo("Create if failed because no resource\r\n");
        RETURN(BS_NO_RESOURCE);
    }

    g_tun_ifs[i].tun_fd = TUN_Open(NULL, 0);
    if (g_tun_ifs[i].tun_fd < 0) {
        g_tun_ifs[i].index[0] = '\0';
        g_tun_ifs[i].ifindex = IF_INVALID_INDEX;
        EXEC_OutInfo("Create if failed because can't open tun\r\n");
        RETURN(BS_CAN_NOT_OPEN);
    }

    IFNET_SetUserData(ifIndex, g_uiTunIfUserDataIndex, UINT_HANDLE(i));

    TUN_PKT_AddFd(g_tun_ifs[i].tun_fd, i);

    return BS_OK;
}

static VOID tun_IfDelete(IF_INDEX ifIndex)
{
    int i;

    for (i=0; i<TUN_IF_MAX; i++) {
        if (g_tun_ifs[i].ifindex == ifIndex) {
            g_tun_ifs[i].index[0] = '\0';
            g_tun_ifs[i].ifindex = IF_INVALID_INDEX;
            break;
        }
    }

    return;
}

static int tun_IfTypeCtl(IF_INDEX ifIndex, int cmd, void *data)
{
    int ret = 0;

    switch (cmd) {
        case IF_TYPE_CTL_CMD_CREATE: {
            ret = tun_IfCreate(ifIndex);
            break;
        }
        case IF_TYPE_CTL_CMD_DELETE: {
            tun_IfDelete(ifIndex);
            break;
        }
        default: {
            ERROR_SET(BS_NOT_SUPPORT);
            ret = BS_NOT_SUPPORT;
            break;
        }
    }

    return ret;
}

static BS_STATUS tun_LinkOutput
(
    IN IF_INDEX ifIndex,
    IN MBUF_S *pstMbuf,
    IN USHORT usProtoType
)
{
    HANDLE hId;
    int id;

    if (BS_OK != IFNET_GetUserData(ifIndex, g_uiTunIfUserDataIndex, (HANDLE*)&hId)) {
        MBUF_Free(pstMbuf);
        return BS_OK;
    }

    id = HANDLE_UINT(hId);

    TUN_Write(g_tun_ifs[id].tun_fd, MBUF_MTOD(pstMbuf), MBUF_TOTAL_DATA_LEN(pstMbuf));

    MBUF_Free(pstMbuf);

    return BS_OK;
}

static BS_STATUS tun_RegIf()
{
    IF_LINK_PARAM_S stLinkParam = {0};
    IF_TYPE_PARAM_S stTypeParam = {0};
    
    stLinkParam.pfLinkOutput = tun_LinkOutput;
    IFNET_SetLinkType("link.tun", &stLinkParam);

    stTypeParam.pcProtoType = IF_PROTO_IP_TYPE_MAME;
    stTypeParam.pcLinkType = "link.tun";
    stTypeParam.uiFlag = 0;
    stTypeParam.pfTypeCtl = tun_IfTypeCtl;
    g_uiTunIfType = IFNET_AddIfType("if.tun", &stTypeParam);

    g_uiTunIfUserDataIndex = IFNET_AllocUserDataIndex();

	return BS_OK;
}

int TUN_LinkInput(int id, MBUF_S *mbuf)
{
    if (g_tun_ifs[id].ifindex == IF_INVALID_INDEX) {
        MBUF_Free(mbuf);
        return BS_NO_SUCH;
    }

    return IFNET_LinkInput(g_tun_ifs[id].ifindex, mbuf);
}

BS_STATUS TUN_IF_Init()
{
    tun_RegIf();

    return BS_OK;
}


