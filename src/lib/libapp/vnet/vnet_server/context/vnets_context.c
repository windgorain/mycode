/* retcode所需要的宏 */
#define RETCODE_FILE_NUM VNET_RETCODE_FILE_NUM_CONTEXT

#include "bs.h"
    
#include "../inc/vnets_context.h"

static VOID vnets_context_FreeContext(IN HANDLE hContext)
{
    VNETS_CONTEXT_S *pstContext = hContext;

    if (NULL != pstContext)
    {
        MEM_Free(pstContext);
    }

    return;
}

static BS_STATUS vnets_context_DupContext(IN HANDLE hPhyHandle, OUT HANDLE *phOutPhyHandle)
{
    VNETS_CONTEXT_S *pstContext = hPhyHandle;
    VNETS_CONTEXT_S *pstNewContext;

    *phOutPhyHandle = NULL;

    if (NULL == pstContext)
    {
        return BS_OK;
    }

    pstNewContext = MEM_ZMalloc(sizeof(VNETS_CONTEXT_S));
    if (NULL == pstNewContext)
    {
        RETURN(BS_NO_MEMORY);
    }

    *pstNewContext = *pstContext;

    *phOutPhyHandle = pstNewContext;

    return BS_OK;
}

VNETS_CONTEXT_S * VNETS_Context_AllocContext()
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MEM_ZMalloc(sizeof(VNETS_CONTEXT_S));

    return pstContext;
}

VNETS_CONTEXT_S * VNETS_Context_AddContextToMbuf(IN MBUF_S *pstMbuf)
{
    VNETS_CONTEXT_S *pstContext;
    MBUF_USER_CONTEXT_S *pstMbufContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);
    if (NULL != pstContext)
    {
        return pstContext;
    }

    pstContext = VNETS_Context_AllocContext();
    if (NULL == pstContext)
    {
        return NULL;
    }

    pstMbufContext = MBUF_GET_USER_CONTEXT(pstMbuf);
    pstMbufContext->pUserContextData = pstContext;
    pstMbufContext->pfFreeFunc = vnets_context_FreeContext;
    pstMbufContext->pfDupFunc = vnets_context_DupContext;

    return pstContext;
}

BS_STATUS VNETS_Context_SetContextToMbuf(IN MBUF_S *pstMbuf, IN VNETS_CONTEXT_S *pstContext)
{
    VNETS_CONTEXT_S *pstContextTmp;

    pstContextTmp = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(NULL != pstContextTmp);

    *pstContextTmp = *pstContext;

    return BS_OK;
}

VNETS_CONTEXT_S * VNETS_Context_GetContext(IN MBUF_S *pstMbuf)
{
    return MBUF_GET_USER_CONTEXT_DATA(pstMbuf);
}

VOID VNETS_Context_SetRecvSesID(IN MBUF_S *pstMbuf, IN UINT uiSesID)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    pstContext->stContext.ulRecvSesId = uiSesID;

    return;
}

UINT VNETS_Context_GetRecvSesID(IN MBUF_S *pstMbuf)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    return pstContext->stContext.ulRecvSesId;
}

VOID VNETS_Context_SetSendSesID(IN MBUF_S *pstMbuf, IN UINT uiSesID)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    pstContext->stContext.ulSendSesId = uiSesID;

    return;
}

UINT VNETS_Context_GetSendSesID(IN MBUF_S *pstMbuf)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    return pstContext->stContext.ulSendSesId;
}

VOID VNETS_Context_SetDstNodeID(IN MBUF_S *pstMbuf, IN UINT uiNodeID)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    pstContext->stContext.uiDstNodeID = uiNodeID;

    return;
}

UINT VNETS_Context_GetDstNodeID(IN MBUF_S *pstMbuf)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    return pstContext->stContext.uiDstNodeID;
}

VOID VNETS_Context_SetSrcNodeID(IN MBUF_S *pstMbuf, IN UINT uiNodeID)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    pstContext->stContext.uiSrcNodeID = uiNodeID;

    return;
}

UINT VNETS_Context_GetSrcNodeID(IN MBUF_S *pstMbuf)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    return pstContext->stContext.uiSrcNodeID;
}

UINT VNETS_Context_GetFlag(IN MBUF_S *pstMbuf)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    return pstContext->stContext.uiFlag;
}

VOID VNETS_Context_SetFlag(IN MBUF_S *pstMbuf, IN UINT uiFlag)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    pstContext->stContext.uiFlag = uiFlag;
}

BOOL_T VNETS_Context_CheckFlag(IN MBUF_S *pstMbuf, IN UINT uiFlagBit)
{
    UINT uiFlag;

    uiFlag = VNETS_Context_GetFlag(pstMbuf);
    if ((uiFlag & uiFlagBit) == uiFlagBit)
    {
        return TRUE;
    }

    return FALSE;
}

VOID VNETS_Context_SetFlagBit(IN MBUF_S *pstMbuf, IN UINT uiFlagBit)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    pstContext->stContext.uiFlag |= uiFlagBit;
}

VOID VNETS_Context_ClrFlagBit(IN MBUF_S *pstMbuf, IN UINT uiFlagBit)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    pstContext->stContext.uiFlag &= (~uiFlagBit);
}

VOID VNETS_Context_SetPhyType(IN MBUF_S *pstMbuf, IN VNETS_PHY_TYPE_E enType)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    pstContext->stPhyContext.enType = enType;

    return;
}

VNETS_PHY_TYPE_E VNETS_Context_GetPhyType(IN MBUF_S *pstMbuf)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    return pstContext->stPhyContext.enType;
}

VNETS_PHY_CONTEXT_S * VNETS_Context_GetPhyContext(IN MBUF_S *pstMbuf)
{
    VNETS_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    return &(pstContext->stPhyContext);
}

MBUF_S * VNETS_Context_CreateMbufByCopyBuf
(
    IN UINT ulReserveHeadSpace,
    IN UCHAR *pucBuf,
    IN UINT ulLen
)
{
    MBUF_S *pstMbuf;

    pstMbuf = MBUF_CreateByCopyBuf(ulReserveHeadSpace, pucBuf, ulLen, MBUF_DATA_DATA);
    if (NULL == pstMbuf)
    {
        return NULL;
    }

    if (NULL == VNETS_Context_AddContextToMbuf(pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return NULL;
    }

    return pstMbuf;
}

MBUF_S * VNETS_Context_CreateMbufByCluster
(
    IN MBUF_CLUSTER_S *pstCluster,
    IN UINT ulReserveHeadSpace,
    IN UINT ulLen
)
{
    MBUF_S *pstMbuf;

    pstMbuf = MBUF_CreateByCluster(pstCluster, ulReserveHeadSpace, ulLen, MBUF_DATA_DATA);
    if (NULL == pstMbuf)
    {
        return NULL;
    }

    if (NULL == VNETS_Context_AddContextToMbuf(pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return NULL;
    }

    return pstMbuf;
}


