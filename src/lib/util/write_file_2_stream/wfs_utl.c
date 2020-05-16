/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-5-6
* Description: write file 2 stream
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_WFS

#include "bs.h"

#include "utl/ofbuf_utl.h"
#include "utl/file_utl.h"
#include "utl/ssltcp_utl.h"


#define _WFS_MAX_DATA_BLOCK_SIZE 1024

typedef struct
{
    FILE *fp;
    UINT uiSsltcpId;
    HANDLE hOfbuf;
    UINT64 uiFileLength;  /* 文件总长度 */
    UINT64 uiSendLen;     /* 已经发送的长度 */
}_WFS_CTRL_S;

HANDLE WFS_Create(IN CHAR *pszFileName, IN UINT uiSsltcpId)
{
    _WFS_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(_WFS_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstCtrl->hOfbuf = OFBUF_Create(_WFS_MAX_DATA_BLOCK_SIZE);
    if (NULL == pstCtrl->hOfbuf)
    {
        MEM_Free(pstCtrl);
        return NULL;
    }

    pstCtrl->fp = FILE_Open(pszFileName, FALSE, "rb");
    if (NULL == pstCtrl->fp)
    {
        OFBUF_Destory(pstCtrl->hOfbuf);
        MEM_Free(pstCtrl);
        return NULL;
    }

    if (BS_OK != FILE_GetSize(pszFileName, &pstCtrl->uiFileLength))
    {
        OFBUF_Destory(pstCtrl->hOfbuf);
        MEM_Free(pstCtrl);
        return NULL;
    }

    pstCtrl->uiSsltcpId = uiSsltcpId;

    return pstCtrl;
}

VOID WFS_Destory(IN HANDLE hWfsHandle)
{
    _WFS_CTRL_S *pstCtrl = (_WFS_CTRL_S *)hWfsHandle;

    if (pstCtrl->fp != NULL)
    {
        fclose(pstCtrl->fp);
    }

    if (pstCtrl->hOfbuf != NULL)
    {
        OFBUF_Destory(pstCtrl->hOfbuf);
    }

    MEM_Free(pstCtrl);
}


/* OK/AGAIN/ERR等 */
BS_STATUS WFS_Write(IN HANDLE hWfsHandle)
{
    _WFS_CTRL_S *pstCtrl = (_WFS_CTRL_S *)hWfsHandle;
    UINT uiLen;
    UCHAR *pucData;
    UCHAR *pucBuf;
    UINT uiWriteSize;
    BS_STATUS eRet;
    INT iReadLen;

    pucBuf = OFBUF_GetBuf(pstCtrl->hOfbuf);

    for (;;)
    {
        uiLen = OFBUF_GetDataLen(pstCtrl->hOfbuf);
        if (uiLen > 0) /* 缓冲区中有数据 */
        {
            pucData = OFBUF_GetData(pstCtrl->hOfbuf);

            eRet = SSLTCP_Write(pstCtrl->uiSsltcpId, pucData, uiLen, &uiWriteSize);
            if (BS_OK != eRet)
            {
                return eRet;
            }

            pstCtrl->uiSendLen += uiWriteSize;

            OFBUF_CutHead(pstCtrl->hOfbuf, uiWriteSize);
            if (uiLen > uiWriteSize)        /* 未发送完成，缓冲区中还有数据 */
            {
                return BS_AGAIN;
            }
        }

        if (NULL == pstCtrl->fp)    /* 文件中也没有可读数据了,已经被关闭.这种情况下说明已经发送完成了 */
        {
            return BS_OK;
        }

        iReadLen = fread(pucBuf, 1, _WFS_MAX_DATA_BLOCK_SIZE, pstCtrl->fp);
        if (iReadLen < 0)
        {
            return BS_ERR;
        }
        
        if (iReadLen < _WFS_MAX_DATA_BLOCK_SIZE)    /* 文件已经读完 */
        {
            fclose(pstCtrl->fp);
            pstCtrl->fp = NULL;
        }

        OFBUF_SetDataLen(pstCtrl->hOfbuf, (UINT)iReadLen);
    }

    return BS_OK;
}

/* 得到已经发送了多少数据 */
UINT64 WFS_GetWritedLen(IN HANDLE hWfsHandle)
{
    _WFS_CTRL_S *pstCtrl = (_WFS_CTRL_S *)hWfsHandle;

    return pstCtrl->uiSendLen;
}

/* 得到剩余多少数据待发送 */
UINT64 WFS_GetRemainLen(IN HANDLE hWfsHandle)
{
    _WFS_CTRL_S *pstCtrl = (_WFS_CTRL_S *)hWfsHandle;

    return pstCtrl->uiFileLength - pstCtrl->uiSendLen;
}


