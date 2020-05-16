/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-4-13
* Description: 
* History:     
******************************************************************************/

#ifndef __RPC_INNER_H_
#define __RPC_INNER_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define _RPC_VER 1              /* RPC Version */
#define _RPC_ALIGN_MODE     4   /* 对齐方式:4字节对齐 */


/* RPC报文格式 */
typedef struct
{
    UCHAR ucVer;              /* 版本号 */
    UCHAR ucMsgType;          /* _RPC_TYPE_REQUEST or _RPC_TYPE_RESPONSE */
    UCHAR ucReturnType;       /* 只用于返回值, RPC_RETURN_TYPE_VOID等  */
    UCHAR ucReserved1;
    UINT ulTotalSize;        /* 本消息的总长度 */
    UINT ulParamNum;         /* 参数个数 */
    UINT ulRpcHeadDataLen;
    UINT ulFirstParamOffset; /* 第一个参数相对于MSG 的位移. 单位为字节 */
}_RPC_DATA_HEAD_S;

typedef struct
{
    UCHAR ucType;            /* RPC_PARAM_TYPE_UINT32等, RPC_PARAM_TYPE_VOID表示本参数无效 */
    UCHAR ucReserved1;
    UCHAR ucReserved2;
    UCHAR ucReserved3;
    UINT ulParamLen;        /* 参数长度 */
    UINT ulNextParamOffset; /* 下一个参数相对于当前参数的位移 */
}_RPC_DATA_PARAM_NODE_S;

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__RPC_INNER_H_*/


