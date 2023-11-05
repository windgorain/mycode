
#include "bs.h"

#include "utl/eth_utl.h"
#include "utl/data2hex_utl.h"

/* 或得ETH链路层头信息 */
BS_STATUS ETH_GetEthHeadInfo(IN UCHAR *pucData, IN UINT uiDataLen, OUT ETH_PKT_INFO_S *pstHeadInfo)
{
    USHORT usOffSet;
    ETH_HEADER_S *pstHeader;
    ETH_VLAN_HEAD_S *pstVlan;
    ETH_LLC_S* pstLlc;
    ETH_SNAP_S *pstSnap;
    USHORT usPktLenOrType;
    UCHAR ucPktFmt = PKTFMT_OTHERS;
    USHORT usType = ETHERTYPE_OTHER;
    USHORT usVlan = ETH_INVALID_VLAN_ID;

    if (uiDataLen <= sizeof(ETH_HEADER_S))
    {
        return BS_ERR;
    }

    pstHeader = (ETH_HEADER_S*)pucData;

    usPktLenOrType = ntohs(pstHeader->usProto);

    usOffSet = sizeof(ETH_HEADER_S);

    if (ETH_IS_PKTTYPE(usPktLenOrType))
    {
        if (ETHERTYPE_VLAN == usPktLenOrType)
        {
            pstVlan = (ETH_VLAN_HEAD_S *)((UCHAR*)pstHeader + usOffSet);
            usVlan = ntohs(pstVlan->usVlanTci) & 0xFFF;

            usType = ntohs(pstVlan->usProto);
            usOffSet += sizeof(ETH_VLAN_HEAD_S);
            ucPktFmt = PKTFMT_ETHII_ENCAP;
        }
        else if(ETHERTYPE_ISIS2 != usPktLenOrType)
        {   
            usType = usPktLenOrType;
            ucPktFmt = PKTFMT_ETHII_ENCAP;
        }
        else
        {  
            
            usType = usPktLenOrType;
            usOffSet += ETH_LLC_LEN;
            ucPktFmt = PKTFMT_LLC_ENCAP;
        }
    }
    else if (ETH_IS_PKTLEN(usPktLenOrType))
    {
        if (uiDataLen < usOffSet + sizeof(ETH_LLC_S))
        {
            return BS_ERR;
        }
        
        pstLlc = (ETH_LLC_S*)pucData;

        
        if ((0xaa == pstLlc->ucDSAP ) && (0xaa == pstLlc->ucSSAP))
        {
            usOffSet += (ETH_LLC_LEN + ETH_SNAP_LEN);
            ucPktFmt = PKTFMT_SNAP_ENCAP;

            if (uiDataLen < usOffSet)
            {
                return BS_ERR;
            }

            pstSnap = (ETH_SNAP_S *)(pstLlc + 1);
            usType = ntohs(pstSnap->usType);
        }
        
        else if ((0xff == pstLlc->ucDSAP) && (0xff == pstLlc->ucSSAP))
        {
            ucPktFmt = PKTFMT_8023RAW_ENCAP;
        }
        
        else if ((0xfe == pstLlc->ucDSAP) && 
                 (0xfe == pstLlc->ucSSAP) && 
                 (0x03 == pstLlc->ucCtrl))
        {
            ucPktFmt = PKTFMT_LLC_ENCAP;
            usType = ETHERTYPE_ISIS;
            usOffSet += ETH_LLC_LEN;
        }
        
        else
        {
            usOffSet += ETH_LLC_LEN;
            ucPktFmt = PKTFMT_LLC_ENCAP;
        }
    }
    else
    {
        
    }

    if (usOffSet >= uiDataLen)
    {
        return BS_ERR;
    }

    pstHeadInfo->ucPktFmt = ucPktFmt;
    pstHeadInfo->usType = usType;
    pstHeadInfo->usHeadLen = usOffSet;
    pstHeadInfo->usPktLenOrType = usPktLenOrType;
    pstHeadInfo->usVlanId = usVlan;

    return BS_OK;
}


int ETH_ToEthPkt(void *data, int len, OUT void **new_data, OUT int *new_len)
{
    ETH_PKT_INFO_S eth_info;
    int ret;
    int cut_len = 0;
    ETH_HEADER_S *eth_hdr;

    ret = ETH_GetEthHeadInfo(data, len, &eth_info);
    if (BS_OK != ret) {
        return ret;
    }

    if (eth_info.ucPktFmt == PKTFMT_ETHII_ENCAP) {
        *new_data = data;
        *new_len = len;
        return BS_OK;
    }

    if (eth_info.usHeadLen < sizeof(ETH_HEADER_S)) {
        return BS_ERR;
    }

    eth_hdr = data;
    if (eth_info.usHeadLen > sizeof(ETH_HEADER_S)) {
        cut_len = eth_info.usHeadLen - sizeof(ETH_HEADER_S);
        eth_hdr = (void*)((char*)data + cut_len);
        memmove(data, eth_hdr, cut_len);
    }

    eth_hdr->usProto = htons(eth_info.usType);
    *new_data = eth_hdr;
    *new_len = len - cut_len;

    return BS_OK;
}

VOID ETH_Mac2String(IN UCHAR *pucMac, IN CHAR cSplit, OUT CHAR szMacString[ETH_MAC_ADDR_STRING_LEN + 1])
{
    UINT i;
    CHAR *pcString = szMacString;

    for (i=0; i<6; i++)
    {    
        if (i != 0)
        {
            pcString[0] = cSplit;
            pcString ++;
        }

        UCHAR_2_HEX(pucMac[i], pcString);

        pcString += 2;
    }
}

BS_STATUS ETH_String2Mac(IN CHAR *pcMacString, OUT UCHAR *pucMac)
{
    UINT i;
    CHAR *pcString = pcMacString;
    UCHAR *pucChar;

    if (strlen(pcMacString) < 17)
    {
        return BS_BAD_PARA;
    }

    pucChar= pucMac;
    
    for (i=0; i<6; i++)
    {    
        if (i != 0)
        {
            pcString ++;
        }

        if (BS_OK != HEX_2_UCHAR(pcString, pucChar))
        {
            return BS_ERR;
        }

        pcString += 2;
        pucChar ++;
    }

    return BS_OK;
}

