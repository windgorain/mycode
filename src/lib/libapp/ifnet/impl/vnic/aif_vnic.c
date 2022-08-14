/*================================================================
*   Created：2018.10.04 LiXingang All rights reserved.
*   Description：
*
================================================================*/
#include "bs.h"

#include "utl/if_utl.h"
#include "utl/eth_utl.h"
#include "utl/my_ip_helper.h"
#include "app/if_pub.h"
#include "comp/comp_vnic.h"
#include "comp/comp_wan.h"
#include "comp/comp_if.h"

#ifdef IN_WINDOWS

static IF_INDEX g_ifAifVnicIfIndex;

static BS_STATUS _aif_vnic_PhyOutput(IN UINT uiIfIndex, IN MBUF_S *pstMbuf)
{
    return Vnic_Output(pstMbuf);
}

static VOID _aif_vnic_Recver(IN MBUF_S *pstMbuf)
{
    IFNET_LinkInput(g_ifAifVnicIfIndex, pstMbuf);
}

BS_STATUS AIF_VNIC_Load()
{
    IF_LINK_PARAM_S *pstLinkParam;
    IF_PHY_PARAM_S stPhyParam = {0};
    IF_TYPE_PARAM_S stTypeParam = {0};

    stPhyParam.pfPhyOutput = _aif_vnic_PhyOutput;
    IFNET_SetPhyType(IF_VNIC_PHY_TYPE_MAME, &stPhyParam);

    pstLinkParam = IFNET_GetLinkType(IF_ETH_LINK_TYPE_MAME);
    if (NULL == pstLinkParam)
    {
        return BS_ERR;
    }

    memset(&stTypeParam, 0, sizeof(stTypeParam));
    stTypeParam.pcProtoType = IF_PROTO_IP_TYPE_MAME;
    stTypeParam.pcLinkType = IF_ETH_LINK_TYPE_MAME;
    stTypeParam.pcPhyType = IF_VNIC_PHY_TYPE_MAME;
    stTypeParam.uiFlag = IF_TYPE_FLAG_STATIC;
    IFNET_AddIfType(IF_VNIC_IF_TYPE_MAME, &stTypeParam);

    g_ifAifVnicIfIndex = IFNET_CreateIf(IF_VNIC_IF_TYPE_MAME);
    if (g_ifAifVnicIfIndex == IF_INVALID_INDEX)
    {
        return BS_ERR;
    }

    Vnic_RegRecver(_aif_vnic_Recver);

    Vnic_Start();

    return BS_OK;
}

#endif
