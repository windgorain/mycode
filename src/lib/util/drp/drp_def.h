/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-7-26
* Description: 
* History:     
******************************************************************************/

#ifndef __DRP_DEF_H_
#define __DRP_DEF_H_

#include "utl/ss_utl.h"
#include "utl/mbuf_utl.h"
#include "utl/file_utl.h"
#include "utl/mempool_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 


typedef struct
{
    CHAR *pcStartTag;
    CHAR *pcEndTag;
    UINT uiStartTagLen;
    UINT uiEndTagLen;
    MEMPOOL_HANDLE hMemPool;
    DLL_HEAD_S stKeyList;
    SUNDAY_SKIP_TABLE_S stSundaySkipStart;
    SUNDAY_SKIP_TABLE_S stSundaySkipEnd;
}DRP_CTRL_S;

typedef struct
{
    DRP_CTRL_S *pstDrp;
    UINT uiReadOffset;
    MBUF_S *pstMbuf;
}DRP_FILE_S;

typedef BS_STATUS (*PF_DRP_CTX_OUTPUT)(IN VOID *pDrpCtx, IN UCHAR *pucData, IN UINT uiDataLen);

typedef struct
{
    PF_DRP_CTX_OUTPUT pfCtxOutput;
    VOID *pCtxData;
}DRP_CTX_S;


#ifdef __cplusplus
    }
#endif 

#endif 


