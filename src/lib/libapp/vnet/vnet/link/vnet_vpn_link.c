#include "bs.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_vpn_link.h"

BS_STATUS VNET_VpnLink_Encrypt(IN MBUF_S *pstMbuf, IN BOOL_T bIsEnc)
{
    DES_cblock stPwIov = {21,31,41,51,22,32,42,52};
    UINT uiPadLen;
    UCHAR *pucData;
    UINT uiLen;
    USHORT usLen;
    VNET_VPN_LINK_HEAD_S *pstLinkHead;

    if (bIsEnc)
    {
        if (BS_OK != MBUF_MakeContinue (pstMbuf, sizeof(VNET_VPN_LINK_HEAD_S)))
        {
            return(BS_ERR);
        }

        pstLinkHead = MBUF_MTOD (pstMbuf);
        usLen = MBUF_TOTAL_DATA_LEN(pstMbuf);
        pstLinkHead->usPktLen = htons(usLen);
    }

    uiPadLen = DES_CIPHER_PAD_LEN(MBUF_TOTAL_DATA_LEN(pstMbuf));
    if (uiPadLen > 0)
    {
        if (BS_OK != MBUF_Append(pstMbuf, uiPadLen))
        {
            return BS_ERR;
        }
    }

    uiLen = MBUF_TOTAL_DATA_LEN(pstMbuf);

    if (BS_OK != MBUF_MakeContinue(pstMbuf, uiLen))
    {
        return BS_ERR;
    }

    pucData = MBUF_MTOD(pstMbuf);

    DES_Ede3CbcEncrypt(pucData, pucData, uiLen,
        DES_GetSysKey1(), DES_GetSysKey2(), DES_GetSysKey3(), &stPwIov, bIsEnc);

    if (FALSE == bIsEnc)
    {
        pstLinkHead = MBUF_MTOD (pstMbuf);
        uiPadLen = MBUF_TOTAL_DATA_LEN(pstMbuf) - ntohs(pstLinkHead->usPktLen);
        if (uiPadLen > 0)
        {
            MBUF_CutTail(pstMbuf, uiPadLen);
        }
    }

    return BS_OK;
}

