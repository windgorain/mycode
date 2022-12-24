/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-4-13
* Description: 
* History:     
******************************************************************************/

#ifndef __RPC_UTL_H_
#define __RPC_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/* 消息类型 */
#define RPC_MSG_TYPE_REQUEST    0
#define RPC_MSG_TYPE_RESPONSE   1
#define RPC_MSG_TYPE_ERRINFO    2

/* 参数类型 */
#define RPC_PARAM_TYPE_VOID         0   /* 无效参数 */
#define RPC_PARAM_TYPE_UINT32        1
#define RPC_PARAM_TYPE_STRING       2

/* 返回值类型 */
#define RPC_RETURN_TYPE_VOID        0
#define RPC_RETURN_TYPE_UINT32       1
#define RPC_RETURN_TYPE_STRING      2
#define RPC_RETURN_TYPE_BOOL        3

/* 支持最多参数个数 */
#define _RPC_MAX_PARAM_NUM  16

/* 参数节点结构 */
typedef struct
{
    UCHAR ucType;           /* RPC_PARAM_TYPE_UINT32等, RPC_PARAM_TYPE_VOID表示本参数无效 */
    UCHAR ucIsMemMalloc;    /* 是否申请内存: 1-申请了内存; 0-未申请内存 */
    UCHAR ucReserved1;
    UCHAR ucReserved2;
    UINT ulParamLen;       /* 参数长度 */
    UCHAR *pucParam;        /* 参数内容 */
}RPC_PARAM_NODE_S;

/* 用户RFTB 参数结构 */
typedef struct
{
    UCHAR ucMsgType;            /* _RPC_TYPE_REQUEST or _RPC_TYPE_RESPONSE */
    UCHAR ucReturnType;         /* 只用于返回值, RPC_RETURN_TYPE_VOID等  */
    UCHAR ucIsMemMalloc;        /* pucRpcHeadDataValue是否申请了内存 */
    UCHAR ucReserved1;
    UINT ulRpcHeadDataLen;        /* pucRpcHeadDataValue 长度 */
    UCHAR *pucRpcHeadDataValue;         /* 函数名/ 返回值 */
    UINT ulParamNum;           /* 参数个数 */
    RPC_PARAM_NODE_S astParams[_RPC_MAX_PARAM_NUM];
}RPC_MSG_S;

RPC_MSG_S * RPC_CreateMsg(IN UCHAR ucMsgType);
VOID RPC_FreeMsg(IN RPC_MSG_S * pstMsg);
UCHAR RPC_GetMsgType(IN RPC_MSG_S *pstMsg);
BS_STATUS RPC_SetHeadDataValue(IN RPC_MSG_S *pstMsg, IN VOID *pData, IN UINT ulDataLen);
UCHAR * RPC_GetHeadDataValue(IN RPC_MSG_S *pstMsg);
BS_STATUS RPC_SetReturnType(IN RPC_MSG_S *pstMsg, IN UCHAR ucType);
UCHAR RPC_GetReturnType(IN RPC_MSG_S *pstMsg);
UINT RPC_GetParamNum(IN RPC_MSG_S *pstMsg);
BS_STATUS RPC_AddParam(IN RPC_MSG_S *pstMsg, IN UCHAR ucParamType, IN UINT ulParamLen, IN VOID *pParam);
BS_STATUS RPC_GetParamByIndex
(
    IN RPC_MSG_S *pstMsg,
    IN UINT ulIndex,
    OUT UCHAR *pucParamType,
    OUT UINT *pulParamLen,
    OUT VOID **ppParam
);
UCHAR * RPC_CreateDataByMsg(IN RPC_MSG_S *pstMsg);
RPC_MSG_S * RPC_CreateMsgByData(IN UCHAR *pucData);
UINT RPC_GetTotalSizeOfData(IN UCHAR *pucData);
VOID RPC_FreeData(IN UCHAR *pucData);


/* 客户端 */
HANDLE RPCC_UDP_Create(IN UINT ulIp/* 主机序 */, IN USHORT usPort/* 主机序*/);

RPC_MSG_S * RPCC_Send(IN HANDLE hRpcHandle, IN RPC_MSG_S *pstRpcMsg);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__RPC_UTL_H_*/


