/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-7-4
* Description: 
*   
* History:     
******************************************************************************/

#ifndef __DSTREAM_UTL_H_
#define __DSTREAM_UTL_H_

#include "utl/vbuf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef enum
{
    DSTREAM_RECV_STATE_HEAD = 0,    /* 正在接收头 */
    DSTREAM_RECV_STATE_DATA         /* 正在接收体 */
}DSTREAM_RECV_STATE_E;

typedef struct
{
    VBUF_S stWriteVBuf;
    VBUF_S stReadVBuf;
    UINT uiRemainReadLen;   /* 剩下还要读取的长度 */
    INT iFd;
    UINT bitIsBlock:1;  /* 是否阻塞式的 */
    UINT enState:2;     /* 状态 */
}DSTREAM_S;

BS_STATUS DSTREAM_Init(IN DSTREAM_S *pstDStream);
VOID DSTREAM_Finit(IN DSTREAM_S *pstDStream);
VOID DSTREAM_SetFd(IN DSTREAM_S *pstDStream, IN INT iFd);
BS_STATUS DSTREAM_Write(IN DSTREAM_S *pstDStream, IN UCHAR *pucData, IN UINT uiDataLen);
/*
 返回值: BS_OK: 接收到了一个完整的记录
         BS_AGAIN: 未收完一个记录,需要继续接收
         其他: 错误
*/
BS_STATUS DSTREAM_Read(IN DSTREAM_S *pstDStream);
/* 获取数据 */
VBUF_S * DSTREAM_GetData(IN DSTREAM_S *pstDStream);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__DSTREAM_UTL_H_*/


