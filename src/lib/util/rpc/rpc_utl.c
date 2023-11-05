/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-4-13
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_RPCUTL

#include "bs.h"

#include "utl/rpc_utl.h"

#include "rpc_inner.h"


RPC_MSG_S * RPC_CreateMsg(IN UCHAR ucMsgType )
{
    RPC_MSG_S *pstMsg;

    pstMsg = MEM_ZMalloc(sizeof(RPC_MSG_S));
    if (NULL == pstMsg)
    {
        return NULL;
    }

    pstMsg->ucMsgType = ucMsgType;

    return pstMsg;
}

VOID RPC_FreeMsg(IN RPC_MSG_S * pstMsg)
{
    RPC_MSG_S *pstParam = pstMsg;
    UINT i;

    if (NULL == pstParam)
    {
        return;
    }

    if (pstParam->ucIsMemMalloc == 1)
    {
        MEM_Free(pstParam->pucRpcHeadDataValue);
        pstParam->ucIsMemMalloc = 0;
    }

    for (i=0; i<_RPC_MAX_PARAM_NUM; i++)
    {
        if (pstParam->astParams[i].ucIsMemMalloc == 1)
        {
            MEM_Free(pstParam->astParams[i].pucParam);
            pstParam->astParams[i].pucParam = NULL;
            pstParam->astParams[i].ucIsMemMalloc = 0;
        }
    }

    MEM_Free(pstParam);
}

UCHAR RPC_GetMsgType(IN RPC_MSG_S *pstMsg)
{
    RPC_MSG_S *pstParam = pstMsg;

    return pstParam->ucMsgType;
}

BS_STATUS RPC_SetHeadDataValue(IN RPC_MSG_S *pstMsg, IN VOID *pData, IN UINT ulDataLen)
{
    RPC_MSG_S *pstParam = pstMsg;

    pstParam->pucRpcHeadDataValue = MEM_Malloc(ulDataLen);
    if (NULL == pstParam->pucRpcHeadDataValue)
    {
        BS_WARNNING(("No memory!"));
        RETURN(BS_NO_MEMORY);
    }

    MEM_Copy(pstParam->pucRpcHeadDataValue, pData, ulDataLen);
    pstParam->ulRpcHeadDataLen = ulDataLen;
    pstParam->ucIsMemMalloc = 1;

    return BS_OK;
}

UCHAR * RPC_GetHeadDataValue(IN RPC_MSG_S *pstMsg)
{
    RPC_MSG_S *pstParam = pstMsg;

    return pstParam->pucRpcHeadDataValue;
}

BS_STATUS RPC_SetReturnType(IN RPC_MSG_S *pstMsg, IN UCHAR ucType)
{
    RPC_MSG_S *pstParam = pstMsg;

    pstParam->ucReturnType = ucType;

    return BS_OK;
}

UCHAR RPC_GetReturnType(IN RPC_MSG_S *pstMsg)
{
    RPC_MSG_S *pstParam = pstMsg;

    return pstParam->ucReturnType;
}

UINT RPC_GetParamNum(IN RPC_MSG_S *pstMsg)
{
    RPC_MSG_S *pstParam = pstMsg;

    return pstParam->ulParamNum;
}

BS_STATUS RPC_AddParam(IN RPC_MSG_S *pstMsg, IN UCHAR ucParamType, IN UINT ulParamLen, IN VOID *pParam)
{
    RPC_MSG_S *pstParam = pstMsg;
    UCHAR *pucTemp = NULL;

    if (pstParam->ulParamNum >= _RPC_MAX_PARAM_NUM)
    {
        BS_DBGASSERT(0);
        BS_WARNNING(("Too many param!"));
        RETURN(BS_FULL);
    }

    if (ulParamLen > 0)
    {
        pucTemp = MEM_Malloc(ulParamLen);
        if (NULL == pucTemp)
        {
            BS_WARNNING(("No memory!"));
            RETURN(BS_NO_MEMORY);
        }
        
        MEM_Copy(pucTemp, pParam, ulParamLen);
        pstParam->astParams[pstParam->ulParamNum].ucIsMemMalloc = 1;
    }

    pstParam->astParams[pstParam->ulParamNum].pucParam = pucTemp;
    pstParam->astParams[pstParam->ulParamNum].ucType = ucParamType;
    pstParam->astParams[pstParam->ulParamNum].ulParamLen = ulParamLen;

    pstParam->ulParamNum ++;

    return BS_OK;
}

BS_STATUS RPC_GetParamByIndex
(
    IN RPC_MSG_S *pstMsg,
    IN UINT ulIndex,
    OUT UCHAR *pucParamType,
    OUT UINT *pulParamLen,
    OUT VOID **ppParam
)
{
    RPC_MSG_S *pstParam = pstMsg;

    if (ulIndex >= pstParam->ulParamNum)
    {
        RETURN(BS_OUT_OF_RANGE);
    }

    *pucParamType = pstParam->astParams[ulIndex].ucType;
    *pulParamLen = pstParam->astParams[ulIndex].ulParamLen;
    *ppParam = pstParam->astParams[ulIndex].pucParam;

    return BS_OK;
}

UCHAR * RPC_CreateDataByMsg(IN RPC_MSG_S *pstMsg)
{
    RPC_MSG_S *pstParam = pstMsg;
    _RPC_DATA_HEAD_S *pstDataHead;
    _RPC_DATA_PARAM_NODE_S *pstParamNode;
    UINT ulMallocLen;  
    UINT i;
    UCHAR *pucData;
    UINT ulParamOffset = 0;

    
    ulMallocLen = sizeof(_RPC_DATA_HEAD_S); 
    ulMallocLen += NUM_UP_ALIGN(pstParam->ulRpcHeadDataLen, _RPC_ALIGN_MODE);
    ulMallocLen += sizeof(_RPC_DATA_PARAM_NODE_S) * pstParam->ulParamNum;
    for (i=0; i<pstParam->ulParamNum; i++)
    {
        ulMallocLen += NUM_UP_ALIGN(pstParam->astParams[i].ulParamLen, _RPC_ALIGN_MODE);
    }

    pstDataHead = MEM_Malloc(ulMallocLen);
    if (NULL == pstDataHead)
    {
        BS_WARNNING(("No memory!"));
        return NULL;
    }
    Mem_Zero(pstDataHead, sizeof(_RPC_DATA_HEAD_S));

    
    pstDataHead->ucVer = _RPC_VER;
    pstDataHead->ucMsgType = pstParam->ucMsgType;
    pstDataHead->ucReturnType = pstParam->ucReturnType;
    pstDataHead->ulTotalSize = htonl(ulMallocLen);
    pstDataHead->ulParamNum = htonl(pstParam->ulParamNum);
    pstDataHead->ulRpcHeadDataLen = htonl(pstParam->ulRpcHeadDataLen);
    if (pstParam->ulParamNum != 0)
    {
        ulParamOffset = sizeof(_RPC_DATA_HEAD_S)
            + NUM_UP_ALIGN(pstParam->ulRpcHeadDataLen, _RPC_ALIGN_MODE);
        pstDataHead->ulFirstParamOffset = htonl(ulParamOffset);
    }

    
    pucData = (UCHAR*)(pstDataHead + 1);
    if (pstParam->ulRpcHeadDataLen != 0)
    {
        MEM_Copy(pucData, pstParam->pucRpcHeadDataValue, pstParam->ulRpcHeadDataLen);
    }

    
    pucData = (UCHAR*)pstDataHead;
    pucData += ulParamOffset;
    for (i=0; i<pstParam->ulParamNum; i++)
    {
        pstParamNode = (_RPC_DATA_PARAM_NODE_S *)pucData;
        pstParamNode->ucType = pstParam->astParams[i].ucType;
        pstParamNode->ulParamLen = htonl(pstParam->astParams[i].ulParamLen);
        ulParamOffset = sizeof(_RPC_DATA_PARAM_NODE_S)
            + NUM_UP_ALIGN(pstParam->astParams[i].ulParamLen, _RPC_ALIGN_MODE);
        pstParamNode->ulNextParamOffset = htonl(ulParamOffset);
        if (pstParam->astParams[i].ulParamLen > 0)
        {
            MEM_Copy(pucData + sizeof(_RPC_DATA_PARAM_NODE_S),
                pstParam->astParams[i].pucParam, pstParam->astParams[i].ulParamLen);
        }
        pucData += ulParamOffset;
    }

    return (UCHAR *)pstDataHead;
}

RPC_MSG_S * RPC_CreateMsgByData(IN UCHAR *pucData)
{
    _RPC_DATA_HEAD_S *pstDataHead = (_RPC_DATA_HEAD_S *)pucData;
    _RPC_DATA_PARAM_NODE_S *pstParamNode;
    RPC_MSG_S *pstMsg;
    UCHAR *pucDataTemp;
    UINT i;
    UINT ulParamNum;

    pstMsg = RPC_CreateMsg(pstDataHead->ucMsgType);
    if (NULL == pstMsg)
    {
        BS_WARNNING(("No memory!"));
        return NULL;
    }

    RPC_SetReturnType(pstMsg, pstDataHead->ucReturnType);
    if (BS_OK != RPC_SetHeadDataValue(pstMsg, (UCHAR *)(pstDataHead + 1), ntohl(pstDataHead->ulRpcHeadDataLen)))
    {
        RPC_FreeMsg(pstMsg);
        return NULL;
    }

    ulParamNum = ntohl(pstDataHead->ulParamNum);
    pucDataTemp = (UCHAR *)pstDataHead;
    pucDataTemp += ntohl(pstDataHead->ulFirstParamOffset);

    for (i=0; i<ulParamNum; i++)
    {
        pstParamNode = (_RPC_DATA_PARAM_NODE_S *)pucDataTemp;
        if (BS_OK != RPC_AddParam(pstMsg, pstParamNode->ucType, ntohl(pstParamNode->ulParamLen), pstParamNode + 1))
        {
            RPC_FreeMsg(pstMsg);
            return NULL;
        }
        pucDataTemp += ntohl(pstParamNode->ulNextParamOffset);
    }

    return pstMsg;
}

UINT RPC_GetTotalSizeOfData(IN UCHAR *pucData)
{
    _RPC_DATA_HEAD_S *pstDataHead = (_RPC_DATA_HEAD_S*)pucData;
    UINT ulTotalSize;

    ulTotalSize = pstDataHead->ulTotalSize;
    
    return ntohl(ulTotalSize);
}

VOID RPC_FreeData(IN UCHAR *pucData)
{
    MEM_Free(pucData);
}

