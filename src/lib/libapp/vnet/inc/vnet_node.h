/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-6-6
* Description: 
* History:     
******************************************************************************/

#ifndef __VNET_NODE_H_
#define __VNET_NODE_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define VNET_NID_TYPE_CLIENT 0
#define VNET_NID_TYPE_SERVER 1
#define VNET_NID_TYPE_VNIC   2
#define VNET_NID_TYPE_PCAP   3

#define VNET_NID_TYPE_MASK 0xf0000000
#define VNET_NID_MAKE(type,offset) (((UINT)(type)) << 28 | (offset))
#define VNET_NID_TYPE(uiNID) (((uiNID) >> 28) & 0xf)
#define VNET_NID_INDEX(uiNID) ((uiNID) & 0x0fffffff)

#define VNET_NID_SERVER VNET_NID_MAKE(VNET_NID_TYPE_SERVER, 0)

static inline CHAR * VNET_NODE_GetTypeStringByNID(IN UINT uiNID)
{
    CHAR *pcType = "";

    switch (VNET_NID_TYPE(uiNID))
    {
        case VNET_NID_TYPE_SERVER:
        {
            pcType = "S";
            break;
        }

        case VNET_NID_TYPE_VNIC:
        {
            pcType = "V";
            break;
        }

        case VNET_NID_TYPE_PCAP:
        {
            pcType = "P";
            break;
        }

        case VNET_NID_TYPE_CLIENT:
        {
            pcType = "C";
            break;
        }

        default:
        {
            break;
        }
    }

    return pcType;
}

#define VNET_NODE_PKT_FLAG_GIVE_DETECTER 0x1    /* 转交直连探测权 */
#define VNET_NODE_PKT_FLAG_NOT_ONLINE    0x8000 /* Not Online */


typedef struct
{
    USHORT usFlag;
    USHORT usProto;    /* 承载的上层协议 */
    UINT   uiDstNodeID;
    UINT   uiSrcNodeID;
    UINT   uiCookie;
}VNET_NODE_PKT_HEADER_S;

typedef enum
{
    VNET_NODE_PKT_PROTO_TP,              /* TP协议 */
    VNET_NODE_PKT_PROTO_DATA,            /* 数据报文 */

    VNET_NODE_PKT_PROTO_MAX
}VNET_NODE_PKT_PROTO_E;


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNET_NODE_H_*/


