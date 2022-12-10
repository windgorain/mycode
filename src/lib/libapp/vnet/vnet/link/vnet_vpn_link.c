#include "bs.h"
#include "utl/aes_utl.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_vpn_link.h"

BS_STATUS VNET_VpnLink_Encrypt(IN MBUF_S *pstMbuf, IN BOOL_T bIsEnc)
{
    UINT uiPadLen;
    UCHAR *pucData;
    UINT uiLen;
    USHORT usLen;
    VNET_VPN_LINK_HEAD_S *pstLinkHead;

    if (bIsEnc) {
        if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(VNET_VPN_LINK_HEAD_S))) {
            RETURN(BS_ERR);
        }
        pstLinkHead = MBUF_MTOD (pstMbuf);
        usLen = MBUF_TOTAL_DATA_LEN(pstMbuf);
        pstLinkHead->usPktLen = htons(usLen);
    }

    uiPadLen = AES_CIPHER_PAD_LEN(MBUF_TOTAL_DATA_LEN(pstMbuf));
    if (uiPadLen > 0) {
        if (BS_OK != MBUF_Append(pstMbuf, uiPadLen)) {
            return BS_ERR;
        }
    }

    uiLen = MBUF_TOTAL_DATA_LEN(pstMbuf);

    if (BS_OK != MBUF_MakeContinue(pstMbuf, uiLen)) {
        return BS_ERR;
    }

    pucData = MBUF_MTOD(pstMbuf);

    int ret = AES_Cipher256(AES_GetSysKey(), AES_GetSysIv(), (void*)pucData, uiLen, pucData, uiLen, 1);
    if (ret < 0) {
        return ret;
    }

    if (FALSE == bIsEnc) {
        pstLinkHead = MBUF_MTOD (pstMbuf);
        uiPadLen = MBUF_TOTAL_DATA_LEN(pstMbuf) - ntohs(pstLinkHead->usPktLen);
        if (uiPadLen > 0) {
            MBUF_CutTail(pstMbuf, uiPadLen);
        }
    }

    return BS_OK;
}

