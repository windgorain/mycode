/* retcode所需要的宏 */
#define RETCODE_FILE_NUM VNET_RETCODE_FILE_NUM_CONTEXT

#include "bs.h"
    
#include "../inc/vnetc_context.h"

static VOID vnetc_context_FreeContext(IN HANDLE hContext)
{
    VNETC_CONTEXT_S *pstContext = hContext;

    if (NULL != pstContext)
    {
        MEM_Free(pstContext);
    }

    return;
}

static BS_STATUS vnetc_context_DupContext(IN HANDLE hPhyHandle, OUT HANDLE *phOutPhyHandle)
{
    VNETC_CONTEXT_S *pstContext = hPhyHandle;
    VNETC_CONTEXT_S *pstNewContext;

    *phOutPhyHandle = NULL;

    if (NULL == pstContext)
    {
        return BS_OK;
    }

    pstNewContext = MEM_ZMalloc(sizeof(VNETC_CONTEXT_S));
    if (NULL == pstNewContext)
    {
        RETURN(BS_NO_MEMORY);
    }

    *pstNewContext = *pstContext;

    *phOutPhyHandle = pstNewContext;

    return BS_OK;
}

VNETC_CONTEXT_S * VNETC_Context_AllocContext()
{
    VNETC_CONTEXT_S *pstContext;

    pstContext = MEM_ZMalloc(sizeof(VNETC_CONTEXT_S));

    return pstContext;
}

VNETC_CONTEXT_S * VNETC_Context_AddContextToMbuf(IN MBUF_S *pstMbuf)
{
    VNETC_CONTEXT_S *pstContext;
    MBUF_USER_CONTEXT_S *pstMbufContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);
    if (NULL != pstContext)
    {
        return pstContext;
    }

    pstContext = VNETC_Context_AllocContext();
    if (NULL == pstContext)
    {
        return NULL;
    }

    pstMbufContext = MBUF_GET_USER_CONTEXT(pstMbuf);
    BS_DBGASSERT(pstMbufContext->pUserContextData == NULL);

    pstMbufContext->pUserContextData = pstContext;
    pstMbufContext->pfFreeFunc = vnetc_context_FreeContext;
    pstMbufContext->pfDupFunc = vnetc_context_DupContext;

    return pstContext;
}

VOID VNETC_Context_SetRecvSesID(IN MBUF_S *pstMbuf, IN UINT uiSesID)
{
    VNETC_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    pstContext->stContext.uiRecvSesId = uiSesID;

    return;
}

UINT VNETC_Context_GetRecvSesID(IN MBUF_S *pstMbuf)
{
    VNETC_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    return pstContext->stContext.uiRecvSesId;
}

VOID VNETC_Context_SetSendSesID(IN MBUF_S *pstMbuf, IN UINT uiSesID)
{
    VNETC_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    pstContext->stContext.uiSendSesId = uiSesID;

    return;
}

UINT VNETC_Context_GetSendSesID(IN MBUF_S *pstMbuf)
{
    VNETC_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    return pstContext->stContext.uiSendSesId;
}

VOID VNETC_Context_SetSrcNID(IN MBUF_S *pstMbuf, IN UINT uiNodeID)
{
    VNETC_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    pstContext->stNidContext.uiSrcNID = uiNodeID;
}

UINT VNETC_Context_GetSrcNID(IN MBUF_S *pstMbuf)
{
    VNETC_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    return pstContext->stNidContext.uiSrcNID;
}

VOID VNETC_Context_SetDstNID(IN MBUF_S *pstMbuf, IN UINT uiNodeID)
{
    VNETC_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    pstContext->stNidContext.uiDstNID = uiNodeID;
}

UINT VNETC_Context_GetDstNID(IN MBUF_S *pstMbuf)
{
    VNETC_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    return pstContext->stNidContext.uiDstNID;
}

VNETC_PHY_CONTEXT_S * VNETC_Context_GetPhyContext(IN MBUF_S *pstMbuf)
{
    VNETC_CONTEXT_S *pstContext;

    pstContext = MBUF_GET_USER_CONTEXT_DATA(pstMbuf);

    BS_DBGASSERT(pstContext != NULL);
    
    return &pstContext->stPhyContext;
}

MBUF_S * VNETC_Context_CreateMbufByCopyBuf
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

    if (NULL == VNETC_Context_AddContextToMbuf(pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return NULL;
    }

    return pstMbuf;
}

MBUF_S * VNETC_Context_CreateMbufByCluster
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

    if (NULL == VNETC_Context_AddContextToMbuf(pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return NULL;
    }

    return pstMbuf;
}

