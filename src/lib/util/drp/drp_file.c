/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-7-26
* Description: 动态replace
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/ss_utl.h"
#include "utl/drp_utl.h"
    
#include "drp_def.h"

#define _DRP_FILE_NOT_FOUND_KEY_STR "Can't get drp key: "
#define _DRP_FILE_NOT_FOUND_KEY_LEN (sizeof(_DRP_FILE_NOT_FOUND_KEY_STR) - 1)


static BS_STATUS drp_file_NotFoundKey(IN CHAR *pcKey, IN UINT uiKeyLen, INOUT MBUF_S *pstMbuf)
{
    BS_STATUS eRet = BS_OK;

    eRet |= MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf), _DRP_FILE_NOT_FOUND_KEY_STR, _DRP_FILE_NOT_FOUND_KEY_LEN);
    eRet |= MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf), pcKey, uiKeyLen);
    eRet |= MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf), " ", 1);

    return eRet;
}

static BS_STATUS drp_file_CtxOutput(IN VOID *pDrpCtx, IN UCHAR *pucData, IN UINT uiDataLen)
{
    DRP_CTX_S *pstCtx = pDrpCtx;
    MBUF_S *pstMbuf = pstCtx->pCtxData;

    return MBUF_AddTailData(pstMbuf, pucData, uiDataLen);
}

static BS_STATUS drp_file_AddDynamicData
(
    IN DRP_CTRL_S *pstDrp,
    IN CHAR *pcKey,
    IN UINT uiKeyLen,
    INOUT MBUF_S *pstMbuf,
    IN HANDLE hUserHandle
)
{
    LSTR_S stKey;
    DRP_CTX_S stCtx;
    DRP_NODE_S *pstNode;

    stKey.pcData = pcKey;
    stKey.uiLen = uiKeyLen;

    LSTR_Strim(&stKey, TXT_BLANK_CHARS, &stKey);

    stCtx.pfCtxOutput = drp_file_CtxOutput;
    stCtx.pCtxData = pstMbuf;    

    pstNode = DRP_Find(pstDrp, stKey.pcData, stKey.uiLen);
    if (NULL == pstNode)
    {
        return drp_file_NotFoundKey(pcKey, uiKeyLen, pstMbuf);
    }
    else
    {
        return pstNode->pfSourceFunc(pstDrp, &stKey, &stCtx, hUserHandle, pstNode->hUserHandle2);
    }
}

static MBUF_S * drp_file_Rewrite(DRP_CTRL_S *pstDrp, IN CHAR *pcFile, IN HANDLE hUserHandle)
{
    FILE_MEM_S *pstFileMMap;
    CHAR *pcStatic;
    UINT uiStaticLen;
    UINT uiRemainLen;
    CHAR *pcStart;
    CHAR *pcEnd;
    CHAR *pcKeyStart;
    UINT uiKeyLen;
    MBUF_S *pstMBuf;
    BS_STATUS eRet = BS_OK;

    pstFileMMap = FILE_Mem(pcFile);
    if (NULL == pstFileMMap)
    {
        return NULL;
    }

    pstMBuf = MBUF_Create(MBUF_DATA_DATA, 0);
    if (NULL == pstMBuf)
    {
        FILE_MemFree(pstFileMMap);
        return NULL;
    }

    pcStatic = (CHAR*)pstFileMMap->pucFileData;
    uiRemainLen = (UINT)pstFileMMap->uiFileLen;

    while (uiRemainLen > 0)
    {
        /* 查找标签起始位置 */
        pcStart = (CHAR*)Sunday_SearchFast((UCHAR*)pcStatic, uiRemainLen, (UCHAR*)pstDrp->pcStartTag, pstDrp->uiStartTagLen, &pstDrp->stSundaySkipStart);

        if (NULL == pcStart)
        {
            eRet = MBUF_CopyFromBufToMbuf(pstMBuf, MBUF_TOTAL_DATA_LEN(pstMBuf), pcStatic, uiRemainLen);
            break;
        }

        /* Copy前面的静态数据 */
        uiStaticLen = pcStart - pcStatic;
        if (uiStaticLen > 0)
        {
            eRet = MBUF_CopyFromBufToMbuf(pstMBuf, MBUF_TOTAL_DATA_LEN(pstMBuf), pcStatic, uiStaticLen);
            if (eRet != BS_OK)
            {
                break;
            }
        }

        pcKeyStart = pcStart + pstDrp->uiStartTagLen;
        uiRemainLen -= uiStaticLen;
        uiRemainLen -= pstDrp->uiStartTagLen;

        /* 查找结束标签 */
        pcEnd = (CHAR*)Sunday_SearchFast((UCHAR*)pcKeyStart, uiRemainLen, (UCHAR*)pstDrp->pcEndTag, pstDrp->uiEndTagLen, &pstDrp->stSundaySkipEnd);
        if (NULL == pcEnd)
        {
            uiKeyLen = uiRemainLen;
            uiRemainLen = 0;
            pcStatic = NULL;
        }
        else
        {
            uiKeyLen = pcEnd - pcKeyStart;
            uiRemainLen -= (uiKeyLen + pstDrp->uiEndTagLen);
            pcStatic = pcEnd + pstDrp->uiEndTagLen;
        }

        /* 根据Key添加动态数据 */
        if (BS_OK != drp_file_AddDynamicData(pstDrp, pcKeyStart, uiKeyLen, pstMBuf, hUserHandle))
        {
            break;
        }
    }

    if (eRet != BS_OK)
    {
        MBUF_Free(pstMBuf);
        pstMBuf = NULL;
    }

    FILE_MemFree(pstFileMMap);

    return pstMBuf;
}

DRP_FILE DRP_FileOpen(IN DRP_HANDLE hDrp, IN CHAR *pcFile, IN HANDLE hUserHandle)
{
    DRP_FILE_S *pstFile;
    DRP_CTRL_S *pstDrp;

    pstDrp = hDrp;

    pstFile = MEMPOOL_ZAlloc(pstDrp->hMemPool, sizeof(DRP_FILE_S));
    if (NULL == pstFile)
    {
        return NULL;
    }

    pstFile->pstMbuf = drp_file_Rewrite(pstDrp, pcFile, hUserHandle);
    if (NULL == pstFile->pstMbuf)
    {
        MEMPOOL_Free(pstDrp->hMemPool, pstFile);
        return NULL;
    }

    pstFile->pstDrp = hDrp;

    return pstFile;
}

VOID DRP_FileClose(IN DRP_FILE hFile)
{
    DRP_FILE_S *pstFile = hFile;

    if (NULL != pstFile)
    {
        if (NULL != pstFile->pstMbuf)
        {
            MBUF_Free(pstFile->pstMbuf);
        }

        MEMPOOL_Free(pstFile->pstDrp->hMemPool, pstFile);
    }
}

INT DRP_FileRead(IN DRP_FILE hFile, OUT UCHAR *pucData, IN UINT uiDataLen)
{
    DRP_FILE_S *pstFile = hFile;
    UINT uiReadSize;
    UINT64 uiRemainSize;

    if ((uiDataLen == 0)
        || (pstFile->uiReadOffset >= MBUF_TOTAL_DATA_LEN(pstFile->pstMbuf)))
    {
        return -1;
    }

    uiRemainSize = MBUF_TOTAL_DATA_LEN(pstFile->pstMbuf) - pstFile->uiReadOffset;

    uiReadSize = (INT) MIN(uiRemainSize, (UINT64)uiDataLen);

    (VOID) MBUF_CopyFromMbufToBuf(pstFile->pstMbuf, pstFile->uiReadOffset, uiReadSize, pucData);

    pstFile->uiReadOffset += uiReadSize;

    return (INT)uiReadSize;
}

BOOL_T DRP_FileEOF(IN DRP_FILE hFile)
{
    DRP_FILE_S *pstFile = hFile;

    if (pstFile->uiReadOffset >= MBUF_TOTAL_DATA_LEN(pstFile->pstMbuf))
    {
        return TRUE;
    }

    return FALSE;
}

UINT64 DRP_FileLength(IN DRP_FILE hFile)
{
    DRP_FILE_S *pstFile = hFile;
    
    return (UINT64)MBUF_TOTAL_DATA_LEN(pstFile->pstMbuf);
}


