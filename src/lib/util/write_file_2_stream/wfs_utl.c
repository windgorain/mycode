/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-5-6
* Description: write file 2 stream
* History:     
******************************************************************************/

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
    UINT64 uiFileLength;  
    UINT64 uiSendLen;     
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
        if (uiLen > 0) 
        {
            pucData = OFBUF_GetData(pstCtrl->hOfbuf);

            eRet = SSLTCP_Write(pstCtrl->uiSsltcpId, pucData, uiLen, &uiWriteSize);
            if (BS_OK != eRet)
            {
                return eRet;
            }

            pstCtrl->uiSendLen += uiWriteSize;

            OFBUF_CutHead(pstCtrl->hOfbuf, uiWriteSize);
            if (uiLen > uiWriteSize)        
            {
                return BS_AGAIN;
            }
        }

        if (NULL == pstCtrl->fp)    
        {
            return BS_OK;
        }

        iReadLen = fread(pucBuf, 1, _WFS_MAX_DATA_BLOCK_SIZE, pstCtrl->fp);
        if (iReadLen < 0)
        {
            return BS_ERR;
        }
        
        if (iReadLen < _WFS_MAX_DATA_BLOCK_SIZE)    
        {
            fclose(pstCtrl->fp);
            pstCtrl->fp = NULL;
        }

        OFBUF_SetDataLen(pstCtrl->hOfbuf, (UINT)iReadLen);
    }

    return BS_OK;
}


UINT64 WFS_GetWritedLen(IN HANDLE hWfsHandle)
{
    _WFS_CTRL_S *pstCtrl = (_WFS_CTRL_S *)hWfsHandle;

    return pstCtrl->uiSendLen;
}


UINT64 WFS_GetRemainLen(IN HANDLE hWfsHandle)
{
    _WFS_CTRL_S *pstCtrl = (_WFS_CTRL_S *)hWfsHandle;

    return pstCtrl->uiFileLength - pstCtrl->uiSendLen;
}


