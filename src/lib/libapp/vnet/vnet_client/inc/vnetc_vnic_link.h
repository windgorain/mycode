/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-22
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_VNIC_LINK_H_
#define __VNETC_VNIC_LINK_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


BS_STATUS VNET_VNIC_LinkInput (IN UINT ulIfIndex, IN MBUF_S *pstMbuf);
BS_STATUS VNET_VNIC_LinkOutput (IN UINT ulIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoTyp);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_VNIC_LINK_H_*/


