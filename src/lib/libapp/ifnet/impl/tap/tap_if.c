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
#include "tap_pkt.h"

#define TAP_IF_MAX 16

typedef struct {
    char index[IF_MAX_NAME_INDEX_LEN + 1];
    IF_INDEX ifindex;
    TUN_FD tap_fd;
}TAP_IF_S;

static TAP_IF_S g_tap_ifs[TAP_IF_MAX];
static UINT g_uiTunIfType;
static UINT g_uiTunIfUserDataIndex = 0;

static BS_STATUS tap_IfCreate(IF_INDEX ifIndex)
{
    CHAR szIfName[IF_MAX_NAME_LEN + 1];
    CHAR *pcIndex;
    int i;

    IFNET_GetIfName(ifIndex, szIfName);
    pcIndex = IF_GetPhyIndexStringByIfName(szIfName);

    for (i=0; i<TAP_IF_MAX; i++) {
        if (g_tap_ifs[i].ifindex == IF_INVALID_INDEX) {
            strlcpy(g_tap_ifs[i].index, pcIndex, IF_MAX_NAME_INDEX_LEN + 1);
            g_tap_ifs[i].ifindex = ifIndex;
            break;
        }
    }

    if (i >= TAP_IF_MAX) {
        EXEC_OutInfo("Create if failed because no resource\r\n");
        RETURN(BS_NO_RESOURCE);
    }

    g_tap_ifs[i].tap_fd = TAP_Open(NULL, 0);
    if (g_tap_ifs[i].tap_fd < 0) {
        g_tap_ifs[i].index[0] = '\0';
        g_tap_ifs[i].ifindex = IF_INVALID_INDEX;
        EXEC_OutInfo("Create if failed because can't open tap\r\n");
        RETURN(BS_CAN_NOT_OPEN);
    }

    IFNET_SetUserData(ifIndex, g_uiTunIfUserDataIndex, UINT_HANDLE(i));

    TAP_PKT_AddFd(g_tap_ifs[i].tap_fd, i);

    return BS_OK;
}

static VOID tap_IfDelete(IF_INDEX ifIndex)
{
    int i;

    for (i=0; i<TAP_IF_MAX; i++) {
        if (g_tap_ifs[i].ifindex == ifIndex) {
            g_tap_ifs[i].index[0] = '\0';
            g_tap_ifs[i].ifindex = IF_INVALID_INDEX;
            break;
        }
    }

    return;
}

static int tap_IfTypeCtl(IF_INDEX ifIndex, int cmd, void *data)
{
    int ret = 0;

    switch (cmd) {
        case IF_TYPE_CTL_CMD_CREATE: {
            ret = tap_IfCreate(ifIndex);
            break;
        }
        case IF_TYPE_CTL_CMD_DELETE: {
            tap_IfDelete(ifIndex);
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

static int tap_PhyOutput(UINT ifIndex, MBUF_S *pstMbuf)
{
    HANDLE hId;
    int id;

    if (BS_OK != IFNET_GetUserData(ifIndex, g_uiTunIfUserDataIndex, (HANDLE*)&hId)) {
        MBUF_Free(pstMbuf);
        return BS_OK;
    }

    id = HANDLE_UINT(hId);

    TUN_Write(g_tap_ifs[id].tap_fd, MBUF_MTOD(pstMbuf), MBUF_TOTAL_DATA_LEN(pstMbuf));

    MBUF_Free(pstMbuf);

    return BS_OK;
}

static BS_STATUS tap_RegIf()
{
    IF_PHY_PARAM_S phy_param = {0};
    IF_TYPE_PARAM_S type_param = {0};
    
    phy_param.pfPhyOutput = tap_PhyOutput;
    IFNET_SetPhyType("phy.tap", &phy_param);

    type_param.pcProtoType = IF_PROTO_IP_TYPE_MAME;
    type_param.pcLinkType = IF_ETH_LINK_TYPE_MAME;
    type_param.uiFlag = 0;
    type_param.pfTypeCtl = tap_IfTypeCtl;
    g_uiTunIfType = IFNET_AddIfType("if.tap", &type_param);

    g_uiTunIfUserDataIndex = IFNET_AllocUserDataIndex();

	return BS_OK;
}

int TAP_PhyInput(int id, MBUF_S *mbuf)
{
    if (g_tap_ifs[id].ifindex == IF_INVALID_INDEX) {
        MBUF_Free(mbuf);
        return BS_NO_SUCH;
    }

    return IFNET_LinkInput(g_tap_ifs[id].ifindex, mbuf);
}

BS_STATUS TAP_IF_Init()
{
    tap_RegIf();

    return BS_OK;
}


