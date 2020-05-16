/*================================================================
*   Created：2018.10.03 LiXingang All rights reserved.
*   Description：
*
================================================================*/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/sif_utl.h"
#include "utl/fib_utl.h"
#include "utl/ipfwd_service.h"
#include "utl/vf_utl.h"
#include "utl/arp_utl.h"
#include "utl/bit_opt.h"
#include "utl/msgque_utl.h"
#include "comp/comp_pcap.h"
#include "comp/comp_if.h"

typedef struct
{
    IF_INDEX ifIndex;
    UINT uiUsePcapIndex;
}_AIF_PCAP_CTRL_S;

static UINT g_uiAifPcapIfUserDataIndex = 0;
static UINT g_uiAifPcapIfType;

static BS_STATUS _aifpcap_PhyOutput(IN UINT uiIfIndex, IN MBUF_S *pstMbuf)
{
    _AIF_PCAP_CTRL_S *pstCtrl;

    if ((BS_OK != CompIf_GetUserData(uiIfIndex, g_uiAifPcapIfUserDataIndex, (HANDLE*)&pstCtrl))
        || (NULL == pstCtrl) || (pstCtrl->uiUsePcapIndex == PCAP_INVALID_INDEX))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return COMP_PCAP_SendPkt(pstCtrl->uiUsePcapIndex, pstMbuf);
}

static BS_STATUS _aifpcap_PhyIoctl(IN IF_INDEX ifIndex, IN UINT uiCmd/* IF_PHY_IOCTL_CMD_E */, INOUT VOID *pData)
{
    _AIF_PCAP_CTRL_S *pstCtrl;
    BS_STATUS eRet = BS_OK;

    if ((BS_OK != CompIf_GetUserData(ifIndex, g_uiAifPcapIfUserDataIndex, (HANDLE*)&pstCtrl))
        || (NULL == pstCtrl) || (pstCtrl->uiUsePcapIndex == PCAP_INVALID_INDEX))
    {
        return BS_ERR;
    }

    switch(uiCmd)
    {
        case IF_PHY_IOCTL_CMD_SET_SHUTDOWN:
        {
            BOOL_T bStart = FALSE;
            if (pData == NULL)
            {
                bStart = TRUE;
            }
            eRet = COMP_PCAP_Ioctl(pstCtrl->uiUsePcapIndex, PCAP_IOCTL_CMD_SET_START, UINT_HANDLE(bStart));
            break;
        }
        case IF_PHY_IOCTL_CMD_GET_PRIVATE_INFO:
        {
            eRet = COMP_PCAP_Ioctl(pstCtrl->uiUsePcapIndex, PCAP_IOCTL_GET_PRIVATE_INFO, pData);
            break;
        }

        default:
        {
            eRet = BS_ERR;
            break;
        }
    }

    return eRet;
}

static VOID _aifpcap_PhyPktIn
(
    IN UCHAR *pucPktData,
    IN PKTCAP_PKT_INFO_S *pstPktInfo,
    IN USER_HANDLE_S *pstUserHandle
)
{
    MBUF_S *pstMbuf;
    MAC_ADDR_S stSrcMac;
    ETH_HEADER_S * pstEthHeader;
    _AIF_PCAP_CTRL_S *pstPcapCtrl;

    if (pstPktInfo->uiPktCaptureLen != pstPktInfo->uiPktRawLen)
    {
        return;
    }

    if (pstPktInfo->uiPktCaptureLen < sizeof(ETH_HEADER_S))
    {
        return;
    }

    pstPcapCtrl = pstUserHandle->ahUserHandle[0];

    CompIf_Ioctl(pstPcapCtrl->ifIndex, IFNET_CMD_GET_MAC, &stSrcMac);
    pstEthHeader = (VOID*)pucPktData;

    /* 目的MAC地址不是自己且不是广播 */
    if ( (! MAC_ADDR_IS_EQUAL(stSrcMac.aucMac, pstEthHeader->stDMac.aucMac))
        && (! MAC_ADDR_IS_BROADCAST(pstEthHeader->stDMac.aucMac)))
    {
        return;
    }

    pstMbuf = MBUF_CreateByCopyBuf(200, pucPktData, pstPktInfo->uiPktCaptureLen, MBUF_DATA_DATA);
    if (NULL == pstMbuf)
    {
        return;
    }

    MBUF_SET_RECV_IF_INDEX(pstMbuf, pstPcapCtrl->ifIndex);

    CompIf_LinkInput(pstPcapCtrl->ifIndex, pstMbuf);
}

static BS_STATUS _aifpcap_SetPcap(IN IF_INDEX ifIndex,
        IN _AIF_PCAP_CTRL_S *pstCtrl, IN UINT uiPcapIndex)
{
    USER_HANDLE_S stUserHandle;
    BOOL_T bStart;

    pstCtrl->uiUsePcapIndex = uiPcapIndex;

    COMP_PCAP_Ioctl(uiPcapIndex, PCAP_IOCTL_CMD_GET_START, &bStart);
    if (bStart == FALSE) {
        CompIf_Ioctl(ifIndex, IFNET_CMD_SET_SHUTDOWN, UINT_HANDLE(TRUE));
    } else {
        CompIf_Ioctl(ifIndex, IFNET_CMD_SET_SHUTDOWN, UINT_HANDLE(FALSE));
    }

    stUserHandle.ahUserHandle[0] = UINT_HANDLE(pstCtrl);
    COMP_PCAP_RegService(uiPcapIndex, ETH_P_IP,
            _aifpcap_PhyPktIn, &stUserHandle);
    COMP_PCAP_RegService(uiPcapIndex, ETH_P_ARP,
            _aifpcap_PhyPktIn, &stUserHandle);

    return BS_OK;
}

static BS_STATUS _aifpcap_IfCreate(IN IF_INDEX ifIndex)
{
    _AIF_PCAP_CTRL_S *pstPcap;
    CHAR szIfName[IF_MAX_NAME_LEN + 1];
    CHAR *pcIndex;
    UINT uiPcapIndex;
    NETINFO_ADAPTER_S stInfo;

    pstPcap = MEM_ZMalloc(sizeof(_AIF_PCAP_CTRL_S));
    if (NULL == pstPcap) {
        return BS_NO_MEMORY;
    }

    CompIf_GetIfName(ifIndex, szIfName);
    pcIndex = IF_GetPhyIndexStringByIfName(szIfName);
    uiPcapIndex = TXT_Str2Ui(pcIndex);

    if (BS_OK != COMP_PCAP_Ioctl(uiPcapIndex, PCAP_IOCTL_CMD_GET_NET_INFO,
                &stInfo)) {
        return BS_ERR;
    }

    CompIf_Ioctl(ifIndex, IFNET_CMD_SET_MAC, &stInfo.stMacAddr);

    pstPcap->ifIndex = ifIndex;

    CompIf_SetUserData(ifIndex, g_uiAifPcapIfUserDataIndex, pstPcap);

    _aifpcap_SetPcap(ifIndex, pstPcap, uiPcapIndex);

    return BS_OK;
}

static VOID _aifpcap_IfDelete(IN IF_INDEX ifIndex)
{
    _AIF_PCAP_CTRL_S *pstPcap;

    if ((BS_OK != CompIf_GetUserData(ifIndex, g_uiAifPcapIfUserDataIndex,
                    (HANDLE*)&pstPcap)) || (NULL == pstPcap)) {
        return;
    }

    CompIf_SetUserData(ifIndex, g_uiAifPcapIfUserDataIndex, NULL);

    MEM_Free(pstPcap);

    return;
}

static int _aifpcap_IfTypeCtl(IF_INDEX ifIndex, int cmd, void *data)
{
    int ret = 0;

    switch (cmd) {
        case IF_TYPE_CTL_CMD_CREATE: {
            ret = _aifpcap_IfCreate(ifIndex);
            break;
        }
        case IF_TYPE_CTL_CMD_DELETE: {
            _aifpcap_IfDelete(ifIndex);
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

static BS_STATUS _aifpcap_RegPcapIf()
{
    IF_PHY_PARAM_S stPhyParam = {0};
    IF_TYPE_PARAM_S stTypeParam = {0};

    stPhyParam.pfPhyOutput = _aifpcap_PhyOutput;
    stPhyParam.pfPhyIoctl = _aifpcap_PhyIoctl;
    CompIf_SetPhyType(IF_PCAP_PHY_TYPE_MAME, &stPhyParam);

    stTypeParam.pcProtoType = IF_PROTO_IP_TYPE_MAME;
    stTypeParam.pcLinkType = IF_ETH_LINK_TYPE_MAME;
    stTypeParam.pcPhyType = IF_PCAP_PHY_TYPE_MAME;
    stTypeParam.uiFlag = IF_TYPE_FLAG_RUNNING_STATIC;
    stTypeParam.pfTypeCtl = _aifpcap_IfTypeCtl;
    g_uiAifPcapIfType = CompIf_AddIfType(IF_PCAP_IF_TYPE_MAME, &stTypeParam);

    return BS_OK;
}


static IF_INDEX _aifpcap_AutoCratePcapIf(IN UINT uiPcapIndex, IN CHAR *pcIfName)
{
    IF_INDEX ifIndex;

    ifIndex = CompIf_CreateIfByName(IF_PCAP_IF_TYPE_MAME, pcIfName);
    if (ifIndex == IF_INVALID_INDEX)
    {
        return IF_INVALID_INDEX;
    }

    return ifIndex;
}

BS_STATUS AIF_PCAP_Load()
{
    CompIf_Init();
    COMP_PCAP_Init();

    return BS_OK;
}

int AIF_PCAP_AfterRegCmd0()
{
    _aifpcap_RegPcapIf();
    g_uiAifPcapIfUserDataIndex = CompIf_AllocUserDataIndex();

    return 0;
}

BS_STATUS AIF_PCAP_AutoCreateInterface()
{
    UINT i;
    IF_INDEX ifIndex;
    CHAR szAdapterName[NETINFO_ADATER_NAME_MAX_LEN + 1];
    CHAR szIfName[32];

    for (i=0; i<PCAP_MAX_NUM; i++)
    {
        if (BS_OK != COMP_PCAP_Ioctl(i, PCAP_IOCTL_CMD_GET_NAME, szAdapterName))
        {
            continue;
        }

        if (szAdapterName[0] == '\0')
        {
            continue;
        }

        sprintf(szIfName, "if.pcap-%d", i);

        ifIndex = CompIf_GetIfIndex(szIfName);
        if (ifIndex == IF_INVALID_INDEX)
        {
            _aifpcap_AutoCratePcapIf(i, szIfName);
        }
    }

    return BS_OK;
}

