/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/eth_utl.h"
#include "utl/eth_mbuf.h"
#include "utl/data2hex_utl.h"

BS_STATUS ETH_GetEthHeadInfoByMbuf(IN MBUF_S *pstMbuf, OUT ETH_PKT_INFO_S *pstHeadInfo)
{
    UINT uiScanLen = sizeof(ETH_HEADER_S) + sizeof(ETH_LLC_S) + sizeof(ETH_SNAP_S);

    uiScanLen = MIN(uiScanLen, MBUF_TOTAL_DATA_LEN(pstMbuf));

    if (BS_OK != MBUF_MakeContinue(pstMbuf, uiScanLen))
    {
        return BS_ERR;
    }

    return ETH_GetEthHeadInfo(MBUF_MTOD(pstMbuf), uiScanLen, pstHeadInfo);
}


BOOL_T ETH_IsMulticastPktByMbuf(IN MBUF_S *pstMbuf)
{
    ETH_HEADER_S *pstEth;

    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(ETH_HEADER_S)))
    {
        return FALSE;
    }

    pstEth = MBUF_MTOD(pstMbuf);

    if (MAC_ADDR_IS_MULTICAST(pstEth->stDMac.aucMac))
    {
        return TRUE;
    }

    return FALSE;
}

BS_STATUS ETH_PadPacket(IN MBUF_S *pstMBuf, IN BOOL_T bHaveEthHead)
{  
    UINT uiPadLen = 0; 
    UINT uiTotalDataSize = 0;
    UINT uiMinSize = ETH_MIN_PAYLOAD_LEN;

    BS_DBGASSERT(NULL != pstMBuf);

    if (bHaveEthHead == TRUE)
    {
        uiMinSize += sizeof(ETH_HEADER_S);
    }
    
    uiTotalDataSize = MBUF_TOTAL_DATA_LEN(pstMBuf);
    
    if (uiTotalDataSize < uiMinSize)
    {  
        uiPadLen = (uiMinSize - uiTotalDataSize);  
        if (BS_OK != MBUF_Append(pstMBuf, uiPadLen))
        {
            return BS_NO_MEMORY;
        }

        MBUF_Set(pstMBuf, 0, uiTotalDataSize, uiPadLen);
    }  

    return BS_OK;
}

