/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-9-7
* Description: 
* History:     
******************************************************************************/


#define RETCODE_FILE_NUM RETCODE_FILE_NUM_UTL

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/mbuf_utl.h"
#include "utl/num_utl.h"
#include "utl/data2hex_utl.h"

#define _MBUF_NEAT_LEN  512

MBUF_CLUSTER_S * MBUF_CreateCluster ()
{
    MBUF_CLUSTER_S *pstCluster;

    pstCluster = MEM_ZMalloc(sizeof(MBUF_CLUSTER_S));
    if (NULL == pstCluster)
    {
        return NULL;
    }
    pstCluster->pucData = MEM_Malloc(_MBUF_POOL_DFT_CLUSTER_SIZE);
    if (NULL == pstCluster)
    {
        MEM_Free(pstCluster);
        return NULL;
    }
    pstCluster->ulSize = _MBUF_POOL_DFT_CLUSTER_SIZE;
    pstCluster->ulRefCnt = 1;

    return pstCluster;
}

VOID MBUF_FreeCluster (IN MBUF_CLUSTER_S *pstCluster)
{
    BS_DBGASSERT (NULL != pstCluster);

    MEM_Free(pstCluster->pucData);
    MEM_Free(pstCluster);
    return;
}

MBUF_MBLK_S * MBUF_CreateMblk()
{
    return MEM_ZMalloc(sizeof(MBUF_MBLK_S));
}

VOID MBUF_FreeMBlk (IN MBUF_MBLK_S *pstMblk)
{
    BS_DBGASSERT (NULL != pstMblk);

    MEM_Free(pstMblk);
    return;
}

static MBUF_S * _mbuf_GetMbufHead (IN UCHAR ucType)
{
    MBUF_S *pstMbuf;

    pstMbuf = MEM_ZMalloc(sizeof(MBUF_S));
    if (NULL == pstMbuf)
    {
        return NULL;
    }
    
    pstMbuf->ucType = ucType;
    DLL_INIT (&pstMbuf->stMblkHead);
    
    return pstMbuf;
}

static VOID _mbuf_FreeMbufHead (IN MBUF_S *pstMbuf)
{
    BS_DBGASSERT (NULL != pstMbuf);

    MEM_Free(pstMbuf);
    return;

}

static BS_STATUS mbuf_CopyUserData(IN MBUF_USER_CONTEXT_S *pstSrc, OUT MBUF_USER_CONTEXT_S *pstDst)
{
    VOID * pUserContext = NULL;
    BS_STATUS eRet;

    if (NULL != pstSrc->pfDupFunc)
    {
        eRet = pstSrc->pfDupFunc(pstSrc->pUserContextData, &pUserContext);
        if (BS_OK != eRet)
        {
            return eRet;
        }

        *pstDst = *pstDst;
        pstDst->pUserContextData = pUserContext;
    }

    return BS_OK;
}

static VOID mbuf_FreeUserData(IN MBUF_USER_CONTEXT_S *pstUserContext)
{
    if (NULL != pstUserContext->pfFreeFunc)
    {
        pstUserContext->pfFreeFunc(pstUserContext->pUserContextData);
    }
}

MBUF_S * MBUF_Create
(
    IN UCHAR ucType,
    IN UINT ulHeadSpaceLen
)
{
    MBUF_S *pstMbuf;
    MBUF_MBLK_S *pstMBlk;
    MBUF_CLUSTER_S *pstCluster;

    if (ulHeadSpaceLen > _MBUF_POOL_DFT_CLUSTER_SIZE)
    {
        return NULL;
    }

    pstMBlk = MBUF_CreateMblk(); 
    if (NULL == pstMBlk)
    {
        return NULL;
    }

    pstMbuf = _mbuf_GetMbufHead(ucType);
    if (NULL == pstMbuf)
    {
        MBUF_FreeMBlk (pstMBlk);
        BS_DBGASSERT (0);
        return NULL;
    }

    pstCluster = MBUF_CreateCluster();
    if (NULL == pstCluster)
    {
        _mbuf_FreeMbufHead(pstMbuf);
        MBUF_FreeMBlk (pstMBlk);
        BS_DBGASSERT (0);
        return NULL;
    }

    DLL_ADD_TO_HEAD ((DLL_HEAD_S*)&pstMbuf->stMblkHead, (DLL_NODE_S*)&pstMBlk->stMbufBlkLinkNode);

    pstMBlk->pstCluster = pstCluster;

    pstMBlk->pucData    = pstCluster->pucData + ulHeadSpaceLen;
    pstMBlk->ulLen      = 0;

    return pstMbuf;
}

VOID MBUF_Free (IN MBUF_S *pstMbuf)
{
    MBUF_MBLK_S *pstMblk;

    while (NULL != (pstMblk = DLL_Get((DLL_HEAD_S *)&pstMbuf->stMblkHead)))
    {
        MBUF_FreeCluster(pstMblk->pstCluster);
        MBUF_FreeMBlk (pstMblk);
    }

    mbuf_FreeUserData(&pstMbuf->stUserContext);

    _mbuf_FreeMbufHead (pstMbuf);

    return;
}


BS_STATUS _MBUF_Compress (IN MBUF_S *pstMbuf)
{
    MBUF_MBLK_S *pstMblk, *pstMblkNext, *pstMblkNextTmp;
    MBUF_CLUSTER_S *pstCluster;
    UINT  ulLen;

    DLL_SAFE_SCAN (&pstMbuf->stMblkHead, pstMblk, pstMblkNext)
    {
        if (NULL == pstMblkNext)
        {
            break;
        }

        if (MBUF_CLUSTER_REF (pstMblk->pstCluster) > 1)
        {
            pstCluster = MBUF_CreateCluster();
            if (! pstCluster) {
                RETURN(BS_NO_MEMORY);
            }
            memmove(pstCluster->pucData, pstMblk->pucData, pstMblk->ulLen);
            MBUF_FreeCluster (pstMblk->pstCluster);
            pstMblk->pstCluster = pstCluster;
            pstMblk->pucData = pstMblk->pstCluster->pucData;
        }
        else
        {
            pstCluster = pstMblk->pstCluster;
            
            if (pstMblk->pucData != pstCluster->pucData)
            {
                memmove(pstCluster->pucData, pstMblk->pucData, pstMblk->ulLen);
                pstMblk->pucData = pstMblk->pstCluster->pucData;
            }
        }

        
        ulLen = MIN(pstMblk->pstCluster->ulSize - pstMblk->ulLen, pstMblkNext->ulLen);
        MEM_Copy(pstMblk->pucData + pstMblk->ulLen, pstMblkNext->pucData, ulLen);
        pstMblk->ulLen += ulLen;
        pstMblkNext->ulLen -= ulLen;
        if (0 == pstMblkNext->ulLen)
        {
            pstMblkNextTmp = DLL_NEXT (&pstMbuf->stMblkHead, pstMblkNext);
            DLL_DEL (&pstMbuf->stMblkHead, pstMblkNext);
            MBUF_FreeCluster(pstMblkNext->pstCluster);
            MBUF_FreeMBlk(pstMblkNext);
            pstMblkNext = pstMblkNextTmp;
        }
    }

    return BS_OK;
}

static BS_STATUS mbuf_PendBlock(IN MBUF_S *pstMbuf, IN UINT ulLen, IN BOOL_T bPendToHead)
{
    MBUF_MBLK_S *pstMblk;
    MBUF_CLUSTER_S *pstCluster;
    UINT ulLenTmp = ulLen;

    while (ulLenTmp > 0)
    {
        pstMblk = MBUF_CreateMblk();
        if (NULL == pstMblk)
        {
            RETURN(BS_NO_MEMORY);
        }

        pstCluster = MBUF_CreateCluster();
        if (NULL == pstCluster)
        {
            MBUF_FreeMBlk(pstMblk);
            RETURN(BS_NO_MEMORY);
        }

        pstMblk->pstCluster = pstCluster;

        if (ulLenTmp <= MBUF_CLUSTER_SIZE(pstMblk->pstCluster))
        {
            pstMblk->ulLen = ulLenTmp;
            pstMblk->pucData = pstCluster->pucData + pstCluster->ulSize - ulLenTmp;
            ulLenTmp = 0;
        }
        else
        {
            pstMblk->ulLen = MBUF_CLUSTER_SIZE(pstMblk->pstCluster);
            pstMblk->pucData = pstCluster->pucData;
            ulLenTmp -= MBUF_CLUSTER_SIZE(pstMblk->pstCluster);
        }

        if (bPendToHead)
        {
            DLL_ADD_TO_HEAD ((DLL_HEAD_S*)&pstMbuf->stMblkHead, pstMblk);
        }
        else
        {
            DLL_ADD((DLL_HEAD_S*)&pstMbuf->stMblkHead, pstMblk);
        }
    }

    MBUF_TOTAL_DATA_LEN (pstMbuf) += ulLen;

    return BS_OK;
}

BS_STATUS MBUF_ApppendBlock (IN MBUF_S *pstMbuf, IN UINT ulLen)
{
    return mbuf_PendBlock(pstMbuf, ulLen, FALSE);
}

BS_STATUS MBUF_PrependBlock (IN MBUF_S *pstMbuf, IN UINT ulLen)
{
    return mbuf_PendBlock(pstMbuf, ulLen, TRUE);
}

BS_STATUS MBUF_CutHead (IN MBUF_S *pstMbuf, IN UINT ulCutLen)
{
    UINT ulLen;
    MBUF_MBLK_S *pstMblk, *pstNextMblk;
    
    BS_DBGASSERT(NULL != pstMbuf);

    ulCutLen = MIN (ulCutLen, MBUF_TOTAL_DATA_LEN(pstMbuf));

    ulLen = ulCutLen;

    DLL_SAFE_SCAN (&pstMbuf->stMblkHead, pstMblk, pstNextMblk)
    {
        if (pstMblk->ulLen <= ulLen)
        {
            ulLen -= pstMblk->ulLen;
            
            DLL_DEL (&pstMbuf->stMblkHead, pstMblk);
            MBUF_FreeCluster (pstMblk->pstCluster);
            MBUF_FreeMBlk (pstMblk);
        }
        else
        {
            pstMblk->pucData += ulLen;
            pstMblk->ulLen -= ulLen;
            ulLen = 0;
        }

        if (0 == ulLen)
        {
            break;
        }
    }

    MBUF_TOTAL_DATA_LEN (pstMbuf) -= ulCutLen;

    return BS_OK;
}

BS_STATUS MBUF_CutTail (IN MBUF_S *pstMbuf, IN UINT ulCutLen)
{
    UINT ulLen = ulCutLen;
    MBUF_MBLK_S *pstMblk, *pstNextTmp;
    
    BS_DBGASSERT(NULL != pstMbuf);

    DLL_SAFE_SCAN_REVERSE (&pstMbuf->stMblkHead, pstMblk, pstNextTmp)
    {
        if (pstMblk->ulLen <= ulLen)
        {
            ulLen -= pstMblk->ulLen;
            
            DLL_DEL (&pstMbuf->stMblkHead, pstMblk);
            MBUF_FreeCluster (pstMblk->pstCluster);
            MBUF_FreeMBlk (pstMblk);
        }
        else
        {
            pstMblk->ulLen -= ulLen;
            ulLen = 0;
        }

        if (0 == ulLen)
        {
            break;
        }
    }

    MBUF_TOTAL_DATA_LEN (pstMbuf) -= ulCutLen;

    return BS_OK;
}

BS_STATUS MBUF_CutPart (IN MBUF_S *pstMbuf, IN UINT ulOffset, IN UINT ulCutLen)
{
    UINT ulLen = ulCutLen;
    MBUF_MBLK_S *pstMblk, *pstNextMblk;
    MBUF_CLUSTER_S *pstCluster;
    
    BS_DBGASSERT(NULL != pstMbuf);

    if (MBUF_TOTAL_DATA_LEN(pstMbuf) <= ulOffset)
    {
        BS_DBGASSERT(0);
        RETURN(BS_ERR);
    }

    if (MBUF_TOTAL_DATA_LEN(pstMbuf) - ulOffset < ulCutLen)
    {
        BS_DBGASSERT(0);
        RETURN(BS_ERR);
    }

    DLL_SAFE_SCAN (&pstMbuf->stMblkHead, pstMblk, pstNextMblk)
    {
        if (ulOffset >= pstMblk->ulLen)
        {
            ulOffset -= pstMblk->ulLen;
            continue;
        }

        if (ulOffset > 0)
        {
            if (ulLen >= (pstMblk->ulLen - ulOffset))
            {
                ulLen -= (pstMblk->ulLen - ulOffset);
                pstMblk->ulLen = ulOffset;
            }
            else
            {
                if (MBUF_CLUSTER_REF (pstMblk->pstCluster) > 1)
                {
                    pstCluster = MBUF_CreateCluster();
                    BS_DBGASSERT(NULL != pstCluster);

                    MEM_Copy (pstCluster->pucData, pstMblk->pucData, ulOffset);
                    MEM_Copy (pstCluster->pucData + ulOffset, pstMblk->pucData + ulOffset + ulLen,
                        pstMblk->ulLen - ulOffset - ulLen);
                    MBUF_FreeCluster (pstMblk->pstCluster);
                    pstMblk->pstCluster = pstCluster;
                    pstMblk->pucData = pstCluster->pucData;
                    pstMblk->ulLen -= ulLen;
                }
                else
                {
                    memmove(pstMblk->pucData + ulOffset, 
                        pstMblk->pucData + ulOffset + ulLen, pstMblk->ulLen - ulOffset - ulLen);
                    pstMblk->ulLen -= ulLen;
                }

                ulLen = 0;
            }

            ulOffset = 0;
        }
        else
        {
            if (pstMblk->ulLen <= ulLen)
            {
                ulLen -= pstMblk->ulLen;
                
                DLL_DEL (&pstMbuf->stMblkHead, pstMblk);
                MBUF_FreeCluster (pstMblk->pstCluster);
                MBUF_FreeMBlk (pstMblk);
            }
            else
            {
                pstMblk->pucData += ulLen;
                pstMblk->ulLen -= ulLen;
                ulLen = 0;
            }
        }

        if (0 == ulLen)
        {
            break;
        }
    }

    MBUF_TOTAL_DATA_LEN (pstMbuf) -= ulCutLen;

    return BS_OK;
}

VOID MBUF_Neat (IN MBUF_S *pstMbuf)
{
    MBUF_MBLK_S *pstMblk, *pstMblkNext;

    BS_DBGASSERT (NULL != pstMbuf);

    DLL_SAFE_SCAN (&pstMbuf->stMblkHead, pstMblk, pstMblkNext)
    {
        if (NULL == pstMblkNext)
        {
            break;
        }

        
        if (MBUF_DATABLOCK_LEN (pstMblk) < _MBUF_NEAT_LEN)
        {
            if ((MBUF_DATABLOCK_LEN (pstMblk) < MBUF_DATABLOCK_LEN (pstMblkNext)) 
                && (MBUF_DATABLOCK_LEN (pstMblk)) <= MBUF_DATABLOCK_HEAD_FREE_LEN(pstMblkNext))
            {
                pstMblkNext->pucData -= MBUF_DATABLOCK_LEN (pstMblk);
                MEM_Copy (pstMblkNext->pucData, pstMblk->pucData, MBUF_DATABLOCK_LEN (pstMblk));
                pstMblkNext->ulLen += MBUF_DATABLOCK_LEN (pstMblk);
                DLL_DEL (&pstMbuf->stMblkHead, pstMblk);
                MBUF_FreeCluster (pstMblk->pstCluster);
                MBUF_FreeMBlk (pstMblk);
                continue;
            }
        }

        
        if ((MBUF_DATABLOCK_LEN (pstMblkNext) < _MBUF_NEAT_LEN)
            && (MBUF_DATABLOCK_TAIL_FREE_LEN(pstMblk) >= MBUF_DATABLOCK_LEN (pstMblkNext)))
        {
            MEM_Copy (pstMblk->pucData + pstMblk->ulLen, pstMblkNext->pucData, MBUF_DATABLOCK_LEN (pstMblkNext));
            pstMblk->ulLen += MBUF_DATABLOCK_LEN (pstMblkNext);
            pstMblk = DLL_NEXT (&pstMbuf->stMblkHead, pstMblkNext);
            DLL_DEL (&pstMbuf->stMblkHead, pstMblkNext);
            MBUF_FreeCluster (pstMblkNext->pstCluster);
            MBUF_FreeMBlk (pstMblkNext);
            pstMblkNext = pstMblk;
            continue;
        }
    }

    return;
}

static inline UCHAR * _MBUF_GetChar (IN MBUF_S *pstMbuf, IN UINT ulOffset, OUT MBUF_ITOR_S *pstItor)
{
    MBUF_MBLK_S *pstMblk;
    
    DLL_SCAN (&pstMbuf->stMblkHead, pstMblk)
    {
        if (ulOffset >= MBUF_DATABLOCK_LEN(pstMblk))
        {
            ulOffset -= MBUF_DATABLOCK_LEN(pstMblk);
            continue;
        }

        pstItor->pstMblk = pstMblk;
        pstItor->ulOffset = ulOffset;

        return pstMblk->pucData + ulOffset;        
    }

    return NULL;
}

static inline UCHAR * _MBUF_GetNextChar (IN MBUF_S *pstMbuf, INOUT MBUF_ITOR_S *pstItor)
{
    pstItor->ulOffset ++;

    while (pstItor->ulOffset >= MBUF_DATABLOCK_LEN(pstItor->pstMblk))
    {
        if (NULL == (pstItor->pstMblk = MBUF_GET_NEXT_DATABLOCK(pstMbuf,pstItor->pstMblk)))
        {
            return NULL;
        }

        pstItor->ulOffset = 0;
    }

    return pstItor->pstMblk->pucData + pstItor->ulOffset;
}

BS_STATUS _MBUF_MakeContinue (IN MBUF_S * pstMbuf, IN UINT ulLen)
{
    MBUF_MBLK_S *pstMblk, *pstMblkNext;
    UINT ulLenMove;
    UINT ulLenShouldContinue = ulLen;
    UCHAR *pucData;

    if (MBUF_TOTAL_DATA_LEN(pstMbuf) < ulLen)
    {
        RETURN(BS_OUT_OF_RANGE);
    }

    pstMblk = DLL_FIRST (&pstMbuf->stMblkHead);
    if (NULL == pstMblk)
    {
        RETURN(BS_EMPTY);
    }

    if (MBUF_CLUSTER_SIZE (pstMblk->pstCluster) < ulLen)
    {
        RETURN(BS_NOT_SUPPORT);
    }

    
    if (MBUF_DATABLOCK_TAIL_FREE_LEN (pstMblk) < ulLen - MBUF_DATABLOCK_LEN (pstMblk))
    {
        pucData = pstMblk->pucData;
        pstMblk->pucData -= (ulLen - MBUF_DATABLOCK_LEN (pstMblk) - MBUF_DATABLOCK_TAIL_FREE_LEN (pstMblk));
        memmove(pstMblk->pucData, pucData, MBUF_DATABLOCK_LEN(pstMblk));
    }

    ulLenShouldContinue = ulLen - MBUF_DATABLOCK_LEN(pstMblk);

    
    while (NULL != (pstMblkNext = DLL_NEXT(&pstMbuf->stMblkHead, pstMblk)))
    {
        ulLenMove = MIN (ulLenShouldContinue, MBUF_DATABLOCK_LEN (pstMblkNext));
        
        MEM_Copy (pstMblk->pucData + MBUF_DATABLOCK_LEN (pstMblk), pstMblkNext->pucData, ulLenMove);
        MBUF_DATABLOCK_LEN(pstMblk) += ulLenMove;

        pstMblkNext->pucData += ulLenMove;
        MBUF_DATABLOCK_LEN (pstMblkNext) -= ulLenMove;

        ulLenShouldContinue -= ulLenMove;

        if (MBUF_DATABLOCK_LEN(pstMblkNext) == 0)
        {
            DLL_DEL (&pstMbuf->stMblkHead, pstMblkNext);
            MBUF_FreeCluster (pstMblkNext->pstCluster);
            MBUF_FreeMBlk (pstMblkNext);
        }

        if (ulLenShouldContinue == 0)
        {
            break;
        }
    }
    
    return BS_OK;
}

MBUF_S * MBUF_CreateByCopyBuf
(
    IN UINT ulReserveHeadSpace,
    IN void *pucBuf,
    IN UINT ulLen,
    IN UCHAR ucType 
)
{
    MBUF_S *pstMbuf;
    MBUF_MBLK_S *pstMblk;
    MBUF_CLUSTER_S *pstCluster;
    UINT ulLenRev = ulLen;
    UINT ulCopyLen;
    UCHAR *pucBufCopy = pucBuf;
    
    
    if (ulReserveHeadSpace >= _MBUF_POOL_DFT_CLUSTER_SIZE)
    {
        ulReserveHeadSpace = 0;
    }

    pstMbuf = _mbuf_GetMbufHead (ucType);
    if (NULL == pstMbuf)
    {
        return NULL;
    }

    while (ulLenRev > 0)
    {
        ulCopyLen = MIN (ulLenRev, _MBUF_POOL_DFT_CLUSTER_SIZE - ulReserveHeadSpace);
        ulLenRev -= ulCopyLen;

        pstMblk = MBUF_CreateMblk ();
        if (pstMblk == NULL)
        {
            MBUF_Free (pstMbuf);
            return NULL;
        }

        pstCluster = MBUF_CreateCluster();
        if (pstCluster == NULL)
        {
            MBUF_FreeMBlk(pstMblk);
            MBUF_Free (pstMbuf);
            return NULL;
        }
        
        MEM_Copy (pstCluster->pucData + ulReserveHeadSpace, pucBufCopy, ulCopyLen);
        pucBufCopy += ulCopyLen;
        
        pstMblk->pstCluster = pstCluster;
        pstMblk->pucData = pstCluster->pucData + ulReserveHeadSpace;
        pstMblk->ulLen = ulCopyLen;

        DLL_ADD ((DLL_HEAD_S*)&pstMbuf->stMblkHead, pstMblk);
        
        ulReserveHeadSpace = 0; 
    }

    MBUF_TOTAL_DATA_LEN (pstMbuf) = ulLen;

    return pstMbuf;
}

MBUF_S * MBUF_CreateByCluster
(
    IN MBUF_CLUSTER_S *pstCluster,
    IN UINT ulReserveHeadSpace,
    IN UINT ulLen,
    IN UCHAR ucType
)
{
    MBUF_S *pstMbuf;
    MBUF_MBLK_S *pstMblk;

    if (NULL == pstCluster)
    {
        BS_DBGASSERT (0);
        return NULL;
    }

    pstMbuf = _mbuf_GetMbufHead (ucType);
    if (NULL == pstMbuf)
    {
        return NULL;
    }
    
    pstMblk = MBUF_CreateMblk ();
    if (pstMblk == NULL)
    {
        MBUF_Free (pstMbuf);
        return NULL;
    }    

    pstMblk->pstCluster = pstCluster;
    pstMblk->pucData = pstCluster->pucData + ulReserveHeadSpace;
    pstMblk->ulLen = ulLen;

    DLL_ADD ((DLL_HEAD_S*)&pstMbuf->stMblkHead, pstMblk);
    
    MBUF_TOTAL_DATA_LEN (pstMbuf) = ulLen;

    return pstMbuf;
}

BS_STATUS MBUF_CopyFromMbufToBuf (IN MBUF_S *pstMbuf, IN UINT ulOffset, IN UINT ulLen, OUT VOID *buf)
{
    MBUF_MBLK_S *pstMblk;
    UINT ulLenRev = ulLen;
    UINT ulCopyLen;
    UINT ulCopyLenThis;
    
    if ((NULL == pstMbuf) || (NULL == buf))
    {
        RETURN(BS_NULL_PARA);
    }

    if (ulOffset + ulLen > MBUF_TOTAL_DATA_LEN(pstMbuf))
    {
        RETURN(BS_NO_SUCH);
    }

    ulCopyLen = 0;
    DLL_SCAN (&pstMbuf->stMblkHead, pstMblk)
    {
        if (ulOffset >= MBUF_DATABLOCK_LEN(pstMblk))
        {
            ulOffset -= MBUF_DATABLOCK_LEN(pstMblk);
            continue;
        }

        ulCopyLenThis = MIN (ulLenRev, MBUF_DATABLOCK_LEN(pstMblk) - ulOffset);

        MEM_Copy ((UCHAR*)buf + ulCopyLen, pstMblk->pucData + ulOffset, ulCopyLenThis);
        ulLenRev -= ulCopyLenThis;
        ulOffset = 0;
        if (ulLenRev == 0)
        {
            break;
        }
        ulCopyLen += ulCopyLenThis;
    }

    return BS_OK;
}

MBUF_MBLK_S * MBUF_MblkFragment (IN MBUF_MBLK_S *pstMblk, IN UINT ulLen)
{
    MBUF_MBLK_S *pstMblkTmp;
    MBUF_CLUSTER_S *pstCluster;

    if (ulLen >= MBUF_DATABLOCK_LEN (pstMblk))
    {
        return NULL;
    }
    
    pstMblkTmp = MBUF_CreateMblk ();
    if (NULL == pstMblkTmp)
    {
        return NULL;
    }

    
    pstCluster = MBUF_CreateCluster();
    if (NULL == pstCluster)
    {
        MBUF_FreeMBlk(pstMblkTmp);
        return NULL;
    }
    
    
    pstMblkTmp->pstCluster = pstCluster;
    pstMblkTmp->pucData = pstCluster->pucData + pstCluster->ulSize - ulLen;
    pstMblkTmp->ulLen   = ulLen;
    MEM_Copy (pstMblkTmp->pucData, pstMblk->pucData, ulLen);
    pstMblk->pucData += ulLen;
    pstMblk->ulLen   -= ulLen;

    return pstMblkTmp;
}


MBUF_MBLK_S * MBUF_CreateMblkByCopyBuf (IN void *buf, IN UINT ulLen, IN UINT ulReservrdHeadSpace)
{
    UINT ulLenCopy;
    MBUF_MBLK_S *pstMblk;
    MBUF_CLUSTER_S *pstCluster;

    if (ulReservrdHeadSpace >= _MBUF_POOL_DFT_CLUSTER_SIZE)
    {
        ulReservrdHeadSpace = 0;
    }

    pstMblk = MBUF_CreateMblk ();
    if (NULL == pstMblk)
    {
        return NULL;
    }

    pstCluster = MBUF_CreateCluster();
    if (NULL == pstCluster)
    {
        MBUF_FreeMBlk(pstMblk);
        return NULL;
    }

    ulLenCopy = MIN (_MBUF_POOL_DFT_CLUSTER_SIZE - ulReservrdHeadSpace, ulLen);

    pstMblk->pstCluster = pstCluster;
    pstMblk->pucData = pstCluster->pucData + ulReservrdHeadSpace;
    MEM_Copy (pstMblk->pucData, buf, ulLenCopy);
    pstMblk->ulLen = ulLenCopy;

    return pstMblk;
}

BS_STATUS MBUF_CopyFromBufToMbuf
(
    IN MBUF_S *pstMbuf,
    IN UINT ulOffset,
    IN void *buf,
    IN UINT ulLen
)
{
    MBUF_MBLK_S *pstMblk, *pstMblkTmp;
    UINT ulOffsetRev;
    UINT ulLenRev;
    UCHAR *pucBufToCopy;
    UINT ulCopyLen;
    
    BS_DBGASSERT (NULL != pstMbuf);
    BS_DBGASSERT (NULL != buf);

    if (0 == ulLen)
    {
        RETURN(BS_BAD_PARA);
    }

    if (ulOffset > MBUF_TOTAL_DATA_LEN (pstMbuf))
    {
        RETURN(BS_NOT_SUPPORT);
    }

    ulOffsetRev = ulOffset;
    ulLenRev = ulLen;
    pucBufToCopy = buf;
    
    DLL_SCAN (&pstMbuf->stMblkHead, pstMblk)
    {
        if (ulLenRev == 0)
        {
            return BS_OK;
        }
        
        if (ulOffsetRev > MBUF_DATABLOCK_LEN (pstMblk))
        {
            ulOffsetRev -= MBUF_DATABLOCK_LEN (pstMblk);
            continue;
        }

        ulCopyLen = MIN(ulLenRev, MBUF_DATABLOCK_LEN(pstMblk) - ulOffsetRev);
        if (ulCopyLen > 0)
        {
            MEM_Copy(pstMblk->pucData + ulOffsetRev, pucBufToCopy, ulCopyLen);
            ulLenRev -= ulCopyLen;
            pucBufToCopy += ulCopyLen;
        }

        ulOffsetRev = 0;

        
        if ((ulLenRev > 0) && (DLL_NEXT(&pstMbuf->stMblkHead, pstMblk) == NULL))
        {
            ulCopyLen = MIN(ulLenRev, MBUF_DATABLOCK_TAIL_FREE_LEN(pstMblk));
            MEM_Copy(pstMblk->pucData + pstMblk->ulLen, pucBufToCopy, ulCopyLen);
            pstMblk->ulLen += ulCopyLen;
            ulLenRev -= ulCopyLen;
            pucBufToCopy += ulCopyLen;
            MBUF_TOTAL_DATA_LEN(pstMbuf) += ulCopyLen;
        }
    }

    if (ulLenRev > 0)
    {
        
        while (ulLenRev > 0)
        {
            pstMblkTmp = MBUF_CreateMblkByCopyBuf(pucBufToCopy, ulLenRev, 0);
            if (NULL == pstMblkTmp)
            {
                RETURN(BS_NO_MEMORY);
            }
            ulLenRev -= MBUF_DATABLOCK_LEN (pstMblkTmp);
            pucBufToCopy += MBUF_DATABLOCK_LEN (pstMblkTmp);
            MBUF_TOTAL_DATA_LEN(pstMbuf) += MBUF_DATABLOCK_LEN (pstMblkTmp);
            DLL_ADD((DLL_HEAD_S*)&pstMbuf->stMblkHead, pstMblkTmp);
        }
    }

    return BS_OK;
}

BS_STATUS MBUF_InsertBuf(IN MBUF_S *pstMbuf, IN UINT ulOffset, IN void *buf, IN UINT ulLen)
{
    MBUF_MBLK_S *pstMblk, *pstMblkTmp;
    UINT ulOffsetRev;
    UINT ulLenRev;
    UCHAR *pucBufToCopy;
    
    BS_DBGASSERT (NULL != pstMbuf);
    BS_DBGASSERT (NULL != buf);

    if (0 == ulLen)
    {
        RETURN(BS_BAD_PARA);
    }

    if (ulOffset > MBUF_TOTAL_DATA_LEN (pstMbuf))
    {
        RETURN(BS_NOT_SUPPORT);
    }

    ulOffsetRev = ulOffset;
    
    DLL_SCAN (&pstMbuf->stMblkHead, pstMblk)
    {
        if (ulOffsetRev >= MBUF_DATABLOCK_LEN (pstMblk))
        {
            ulOffsetRev -= MBUF_DATABLOCK_LEN (pstMblk);
            continue;
        }

        if (ulOffsetRev != 0)
        {
            pstMblkTmp = MBUF_MblkFragment (pstMblk, ulOffsetRev);
            BS_DBGASSERT (NULL != pstMblkTmp);
            DLL_INSERT_BEFORE((DLL_HEAD_S*)&pstMbuf->stMblkHead, pstMblkTmp, pstMblk);
            pstMblk = pstMblkTmp;
            ulOffsetRev = 0;
            continue;
        }

        if (ulLen <= MBUF_DATABLOCK_HEAD_FREE_LEN (pstMblk))
        {
            pstMblk->pucData -= ulLen;
            MEM_Copy (pstMblk->pucData, buf, ulLen);
            pstMblk->ulLen += ulLen;
        }
        else
        {
            
            ulLenRev = ulLen;
            pucBufToCopy = buf;
            while (ulLenRev > 0)
            {
                pstMblkTmp = MBUF_CreateMblkByCopyBuf(pucBufToCopy, ulLenRev, 0);
                BS_DBGASSERT (NULL != pstMblkTmp);
                ulLenRev -= MBUF_DATABLOCK_LEN (pstMblkTmp);
                pucBufToCopy += MBUF_DATABLOCK_LEN (pstMblkTmp);
                DLL_INSERT_BEFORE ((DLL_HEAD_S*)&pstMbuf->stMblkHead, pstMblkTmp, pstMblk);
            }
        }

        break;
    }

    MBUF_TOTAL_DATA_LEN(pstMbuf) += ulLen;

    return BS_OK;
}

MBUF_MBLK_S * MBUF_FindMblk (IN MBUF_S *pstMbuf, IN UINT ulOffset, OUT UINT *pulOffsetInThisMblk)
{
    MBUF_MBLK_S *pstMblk;
    UINT ulOffsetToStart = ulOffset;

    DLL_SCAN (&pstMbuf->stMblkHead, pstMblk)
    {
        if (ulOffsetToStart >= MBUF_DATABLOCK_LEN (pstMblk))
        {
            ulOffsetToStart -= MBUF_DATABLOCK_LEN (pstMblk);
            continue;
        }

        *pulOffsetInThisMblk = ulOffsetToStart;
        return pstMblk;
    }

    return NULL;
}

#if 0
MBUF_S * MBUF_ReferenceCopy (IN MBUF_S *pstMbuf, IN UINT ulOffset, IN UINT ulLen)
{
    MBUF_S *pstMbufDst;
    MBUF_MBLK_S *pstMblkDst, *pstMblk;
    UINT ulOffsetToStart;
    UINT ulCopyLen;

    if (NULL == pstMbuf)
    {
        return NULL;
    }
    
    pstMblk = MBUF_FindMblk (pstMbuf, ulOffset, &ulOffsetToStart);
    if (NULL == pstMblk)
    {
        return NULL;
    }

    pstMbufDst = _mbuf_GetMbufHead (pstMbuf->ucType);
    if (NULL == pstMbufDst)
    {
        return NULL;
    }

    ulCopyLen = MIN (ulLen, MBUF_TOTAL_DATA_LEN(pstMbuf) - ulOffset);
    MBUF_TOTAL_DATA_LEN (pstMbufDst) = ulCopyLen;

    while (ulCopyLen > 0)
    {
        pstMblkDst = MBUF_CreateMblk();
        if (NULL == pstMblkDst)
        {
            MBUF_Free (pstMbufDst);
            return NULL;
        }
        pstMblkDst->pstCluster = pstMblk->pstCluster;
        ATOM_INC_FETCH (pstMblkDst->pstCluster->ulRefCnt);
        pstMblkDst->pucData = pstMblk->pucData + ulOffsetToStart;
        pstMblkDst->ulLen = pstMblk->ulLen - ulOffsetToStart;
        ulOffsetToStart = 0;
        ulCopyLen -= MBUF_DATABLOCK_LEN (pstMblkDst);
        DLL_ADD ((DLL_HEAD_S*)&pstMbufDst->stMblkHead, pstMblkDst);
    }

    if (BS_OK != mbuf_CopyUserData(&pstMbuf->stUserInfo, &pstMbufDst->stUserInfo))
    {
        MBUF_Free(pstMbufDst);
        pstMbufDst = NULL;
    }

    return pstMbufDst;
}
#endif

MBUF_S * MBUF_ReferenceCopy (IN MBUF_S *pstMbuf, IN UINT ulOffset, IN UINT ulLen)
{
    return MBUF_RawCopy(pstMbuf, ulOffset, ulLen, 0);
}

MBUF_S * MBUF_RawCopy (IN MBUF_S *pstMbuf, IN UINT ulOffset, IN UINT ulLen, IN UINT ulReservedHeadSpace)
{
    MBUF_S *pstMbufDst;
    MBUF_MBLK_S *pstMblkDst, *pstMblk;
    UINT ulCopyLen;
    UINT ulOffsetToStart;

    if (NULL == pstMbuf)
    {
        return NULL;
    }

    pstMblk = MBUF_FindMblk (pstMbuf, ulOffset, &ulOffsetToStart);
    if (NULL == pstMblk)
    {
        return NULL;
    }

    pstMbufDst = _mbuf_GetMbufHead (pstMbuf->ucType);
    if (NULL == pstMbufDst)
    {
        return NULL;
    }

    ulCopyLen = MIN (ulLen, MBUF_TOTAL_DATA_LEN(pstMbuf) - ulOffset);
    MBUF_TOTAL_DATA_LEN (pstMbufDst) = ulCopyLen;

    if (ulReservedHeadSpace >= MBUF_CLUSTER_SIZE (pstMblk->pstCluster))
    {
        ulReservedHeadSpace = 0;
    }
    
    while (ulCopyLen > 0)
    {
        pstMblkDst = MBUF_CreateMblkByCopyBuf(pstMblk->pucData + ulOffsetToStart,
            MIN((pstMblk->ulLen - ulOffsetToStart), ulCopyLen), ulReservedHeadSpace);
        if (NULL == pstMblkDst)
        {
            MBUF_Free (pstMbufDst);
            return NULL;
        }

        ulCopyLen -= MBUF_DATABLOCK_LEN (pstMblkDst);
        ulReservedHeadSpace = 0;
        DLL_ADD ((DLL_HEAD_S*)&pstMbufDst->stMblkHead, pstMblkDst);
        if ((pstMblk->ulLen - ulOffsetToStart) <= MBUF_DATABLOCK_LEN (pstMblkDst))
        {
            pstMblk = DLL_NEXT (&pstMbuf->stMblkHead, pstMblk);
            ulOffsetToStart = 0;
        }
        else
        {
            ulOffsetToStart += MBUF_DATABLOCK_LEN (pstMblkDst);
        }
    }

    if (BS_OK != mbuf_CopyUserData(&pstMbuf->stUserContext, &pstMbufDst->stUserContext))
    {
        MBUF_Free(pstMbufDst);
        pstMbufDst = NULL;
    }

    return pstMbufDst;
}


MBUF_S * MBUF_Fragment (IN MBUF_S *pstMbuf, IN UINT ulLen)
{
    MBUF_S *pstMbufDst;
    MBUF_MBLK_S *pstMblk, *pstMblkTmp;
    UINT ulLenShouldMove;
    
    if (NULL == pstMbuf)
    {
        return NULL;
    }

    ulLen = MIN (ulLen, MBUF_TOTAL_DATA_LEN(pstMbuf));
    ulLenShouldMove = ulLen;

    pstMbufDst = _mbuf_GetMbufHead (pstMbuf->ucType);
    if (NULL == pstMbufDst)
    {
        return NULL;
    }

    while (NULL != (pstMblk = DLL_FIRST(&pstMbuf->stMblkHead)))
    {
        if (ulLenShouldMove >= MBUF_DATABLOCK_LEN(pstMblk))
        {
            DLL_DEL ((DLL_HEAD_S*)&pstMbuf->stMblkHead, pstMblk);
            DLL_ADD ((DLL_HEAD_S*)&pstMbufDst->stMblkHead, pstMblk);
            ulLenShouldMove -= MBUF_DATABLOCK_LEN(pstMblk);
        }
        else
        {
            pstMblkTmp = MBUF_MblkFragment (pstMblk, ulLenShouldMove);
            BS_DBGASSERT (NULL != pstMblkTmp);            
            DLL_ADD ((DLL_HEAD_S*)&pstMbufDst->stMblkHead, pstMblkTmp);
            ulLenShouldMove = 0;
        }

        if (0 == ulLenShouldMove)
        {
            break;
        }
    }

    MBUF_TOTAL_DATA_LEN(pstMbuf)    -= ulLen;
    MBUF_TOTAL_DATA_LEN(pstMbufDst) = ulLen;
    mbuf_CopyUserData(&pstMbuf->stUserContext, &pstMbufDst->stUserContext);

    return pstMbufDst;
}

BS_STATUS MBUF_AddCluster (IN MBUF_S *pstMbufDst, IN MBUF_CLUSTER_S *pstCluster, IN UINT ulOffset, IN UINT ulDataLen)
{
    MBUF_MBLK_S *pstMblk;
    
    BS_DBGASSERT (NULL != pstMbufDst);
    BS_DBGASSERT (NULL != pstCluster);

    pstMblk = MBUF_CreateMblk();
    if (NULL == pstMblk)
    {
        RETURN(BS_NO_MEMORY);
    }

    pstMblk->pstCluster = pstCluster;
    pstMblk->pucData = pstCluster->pucData + ulOffset;
    pstMblk->ulLen = ulDataLen;
    DLL_ADD((DLL_HEAD_S*)&pstMbufDst->stMblkHead, pstMblk);
    MBUF_TOTAL_DATA_LEN(pstMbufDst) += ulDataLen;

    return BS_OK;
}


INT MBUF_NFind (IN MBUF_S *pstMbuf, IN UINT ulStartOffset, IN UINT ulLen, IN UCHAR *pMemToFind, IN UINT ulMemLen)
{
    UCHAR *pucData;
    UINT ulOffset;
    MBUF_ITOR_S stItor;
    MBUF_ITOR_S stItorTmp;
    UINT i;
    
    BS_DBGASSERT (NULL != pstMbuf);
    BS_DBGASSERT (NULL != pMemToFind);
    BS_DBGASSERT (0 != ulMemLen);

    ulOffset = ulStartOffset;

    BS_DBGASSERT(MBUF_TOTAL_DATA_LEN(pstMbuf) >= ulStartOffset + ulLen);

    if (ulLen < ulMemLen)
    {
        return -1;
    }

    for (pucData = MBUF_GetChar(pstMbuf, ulOffset, &stItor); 
        ulLen >= ulMemLen;
        pucData = _MBUF_GetNextChar (pstMbuf, &stItor))
    {
        stItorTmp = stItor;

        for (i=0; i<ulMemLen; i++)
        {
            if (*pucData != pMemToFind[i])
            {
                break;
            }

            pucData = MBUF_GetNextChar (pstMbuf, &stItorTmp);
        }

        if (i == ulMemLen)  
        {
            return (INT)ulOffset;
        }

        ulOffset++;
        ulLen--;
    }

    return -1;
}


INT MBUF_Find (IN MBUF_S *pstMbuf, IN UINT ulStartOffset, IN UCHAR *pMemToFind, IN UINT ulMemLen)
{
    return MBUF_NFind(pstMbuf, ulStartOffset, MBUF_TOTAL_DATA_LEN(pstMbuf) - ulStartOffset, pMemToFind, ulMemLen);
}

INT MBUF_FindByte(IN MBUF_S *pstMbuf, IN UINT ulStartOffset, IN UCHAR ucByteToFind)
{
    UCHAR aucToFind[2];

    aucToFind[0] = ucByteToFind;
    return MBUF_Find(pstMbuf, ulStartOffset, aucToFind, 1);
}

BS_STATUS MBUF_Set(IN MBUF_S *pstMbuf, IN UCHAR ucToSet, IN UINT ulOffset, IN UINT ulSetLen)
{
    UCHAR * pucChar;
    MBUF_ITOR_S stMbufItor;
    UINT i;

    for (i=0, pucChar=MBUF_GetChar(pstMbuf, ulOffset, &stMbufItor);
        i < ulSetLen;
        i++, pucChar = MBUF_GetNextChar(pstMbuf, &stMbufItor))
    {
        if (pucChar == NULL)
        {
            BS_DBGASSERT(0);
            RETURN(BS_OUT_OF_RANGE);
        }

        *pucChar = ucToSet;
    }

    return BS_OK;    
}


UCHAR * MBUF_GetContinueMem
(
    IN MBUF_S *pstMbuf,
    IN UINT ulOffset,
    IN UINT ulLen,
    OUT HANDLE *phMemHandle
)
{
    UCHAR *pucMem;

    *phMemHandle = NULL;
    
    if (MBUF_TOTAL_DATA_LEN(pstMbuf) < ulOffset + ulLen)
    {
        BS_DBGASSERT(0);
        return NULL;
    }
    
    
    if (BS_OK == MBUF_MakeContinue(pstMbuf, ulOffset + ulLen))
    {
        pucMem =  MBUF_MTOD(pstMbuf);
        return pucMem  + ulOffset;
    }

    return MBUF_GetContinueMemRaw(pstMbuf, ulOffset, ulLen, phMemHandle);
}


void* MBUF_GetContinueMemRaw
(
    IN MBUF_S *pstMbuf,
    IN UINT ulOffset,
    IN UINT ulLen,
    OUT HANDLE *phMemHandle
)
{
    UCHAR *pucMem;

    *phMemHandle = NULL;
    
    if (MBUF_TOTAL_DATA_LEN(pstMbuf) < ulOffset + ulLen)
    {
        BS_DBGASSERT(0);
        return NULL;
    }
    
    pucMem = MEM_Malloc(ulLen + 1);
    if (NULL == pucMem)
    {
        return NULL;
    }

    *phMemHandle = pucMem;

    MBUF_CopyFromMbufToBuf(pstMbuf, ulOffset, ulLen, pucMem);

    pucMem[ulLen] = 0;

    return pucMem;    
}

VOID MBUF_FreeContinueMem(IN HANDLE hMemHandle)
{
    if (hMemHandle != 0)
    {
        MEM_Free(hMemHandle);
    }
}

UCHAR * MBUF_GetChar (IN MBUF_S *pstMbuf, IN UINT ulOffset, OUT MBUF_ITOR_S *pstItor)
{
    return _MBUF_GetChar(pstMbuf, ulOffset, pstItor);
}

UCHAR * MBUF_GetNextChar (IN MBUF_S *pstMbuf, INOUT MBUF_ITOR_S *pstItor)
{
    return _MBUF_GetNextChar(pstMbuf, pstItor);
}



UINT MBUF_DelBlankSideByIndex(IN MBUF_S *pstMbuf, IN UINT ulStartOffset, INOUT UINT *pulOffset)
{
    MBUF_ITOR_S stItor;
    UINT ulOffsetTmp;
    UINT ulCount;
    UCHAR *pucChar;
    UINT ulCutLen = 0;

    ulOffsetTmp = *pulOffset - 1;
    ulCount = 0;

    if (*pulOffset > 0)
    {
        while (ulOffsetTmp >= ulStartOffset)
        {
            pucChar = MBUF_GetChar(pstMbuf, ulOffsetTmp, &stItor);
            if (!TXT_IS_BLANK_OR_RN(*pucChar))
            {
                break;
            }
            ulCount++;
            ulOffsetTmp--;
        }

        if (ulCount > 0)
        {
            ulCutLen += ulCount;
            MBUF_CutPart(pstMbuf, *pulOffset - ulCount, ulCount);
            *pulOffset -= ulCount;
        }
    }

    ulOffsetTmp = *pulOffset + 1;
    ulCount = 0;

    while (ulOffsetTmp < MBUF_TOTAL_DATA_LEN(pstMbuf))
    {
        pucChar = MBUF_GetChar(pstMbuf, ulOffsetTmp, &stItor);
        if (!TXT_IS_BLANK_OR_RN(*pucChar))
        {
            break;
        }
        ulCount++;
        ulOffsetTmp++;
    }

    if (ulCount > 0)
    {
        ulCutLen += ulCount;
        MBUF_CutPart(pstMbuf, *pulOffset + 1, ulCount);
    }

    return ulCutLen;
}


BS_STATUS MBUF_Append(IN MBUF_S *pstMbuf, IN UINT ulLen)
{
    MBUF_MBLK_S *pstMblk;
    UINT uiTailFreeLen;
    
    BS_DBGASSERT(NULL != pstMbuf);

    pstMblk = (MBUF_MBLK_S *)DLL_FIRST (&pstMbuf->stMblkHead);
    if (NULL == pstMblk)
    {
        return MBUF_ApppendBlock (pstMbuf, ulLen);
    }

    uiTailFreeLen = (UINT)(ULONG)(pstMblk->pstCluster->pucData + pstMblk->pstCluster->ulSize)
                    - (UINT)(ULONG)(pstMblk->pucData + pstMblk->ulLen);

    if (uiTailFreeLen < ulLen)
    {
        return MBUF_ApppendBlock (pstMbuf, ulLen);
    }

    pstMblk->ulLen += ulLen;
    MBUF_TOTAL_DATA_LEN (pstMbuf) += ulLen;

    return BS_OK;
}


BS_STATUS MBUF_AppendData(IN MBUF_S *pstMbuf, IN UCHAR *pucData, IN UINT uiDataLen)
{
    return MBUF_CopyFromBufToMbuf(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf), pucData, uiDataLen);
}



BS_STATUS MBUF_Prepend (IN MBUF_S *pstMbuf, IN UINT ulLen)
{
    MBUF_MBLK_S *pstMblk;
    
    BS_DBGASSERT(NULL != pstMbuf);

    pstMblk = (MBUF_MBLK_S *)DLL_FIRST (&pstMbuf->stMblkHead);
    if (NULL == pstMblk)
    {
        return MBUF_PrependBlock (pstMbuf, ulLen);
    }

    if ((UINT)(pstMblk->pucData - pstMblk->pstCluster->pucData) < ulLen)
    {
        return MBUF_PrependBlock (pstMbuf, ulLen);
    }

    pstMblk->pucData -= ulLen;
    pstMblk->ulLen   += ulLen;
    MBUF_TOTAL_DATA_LEN (pstMbuf) += ulLen;

    return BS_OK;
}


BS_STATUS MBUF_PrependData(IN MBUF_S *pstMbuf, IN UCHAR *pucData, IN UINT uiDataLen)
{
    BS_STATUS eRet;

    eRet = MBUF_Prepend(pstMbuf, uiDataLen);
    if (eRet != BS_OK)
    {
        return eRet;
    }

    return MBUF_CopyFromBufToMbuf(pstMbuf, 0, pucData, uiDataLen);
}


VOID MBUF_PrintAsString(IN MBUF_S *pstMbuf)
{
    CHAR *pszMbufString;
    UINT ulMbufLen;

    ulMbufLen = MBUF_TOTAL_DATA_LEN(pstMbuf);
    
    pszMbufString = MEM_Malloc(ulMbufLen + 1);
    if (NULL == pszMbufString)
    {
        return;
    }

    MBUF_CopyFromMbufToBuf(pstMbuf, 0, ulMbufLen, pszMbufString);
    pszMbufString[ulMbufLen] = '\0';

    printf("MBufLen:%d, Type:%d\r\n-----------------------\r\n%s\r\n-----------------------\r\n",
        ulMbufLen, pstMbuf->ucType, pszMbufString);

    MEM_Free(pszMbufString);
}

VOID MBUF_PrintAsHex(IN MBUF_S *pstMbuf)
{
    CHAR *pcMbufData;
    CHAR *pcMbufHex;
    UINT ulMbufLen;

    ulMbufLen = MBUF_TOTAL_DATA_LEN(pstMbuf);
    
    pcMbufHex = MEM_Malloc(ulMbufLen * 2 + 1);
    if (NULL == pcMbufHex)
    {
        return;
    }

    ulMbufLen = MBUF_TOTAL_DATA_LEN(pstMbuf);
    
    pcMbufData = MEM_Malloc(ulMbufLen);
    if (NULL == pcMbufData)
    {
        MEM_Free(pcMbufHex);
        return;
    }

    MBUF_CopyFromMbufToBuf(pstMbuf, 0, ulMbufLen, pcMbufData);

    DH_Data2HexString((void*)pcMbufData, ulMbufLen, pcMbufHex);

    printf("MBufLen:%d, Type:%d\r\n-----------------------\r\n%s\r\n-----------------------\r\n",
        ulMbufLen, pstMbuf->ucType, pcMbufHex);

    MEM_Free(pcMbufHex);
    MEM_Free(pcMbufData);
}

