/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-6-15
* Description: 
* History:     
******************************************************************************/

static BS_STATUS _SSLTCP_CreateSsl(IN UINT ulFamily, IN CHAR *pszSslPolicy, OUT UINT *hSslTcpId)
{
    UINT  ulIndexFrom1 = 0;
    HANDLE  hSslHandle;

    if (BS_OK != SSLNET_Create (ulFamily, pszSslPolicy, &hSslHandle))
    {
        RETURN(BS_ERR);
    }
    
    SPLX_P();
    ulIndexFrom1 = BITMAP1_GetFreeCycle(&stSslTcpBitMap);
    if (ulIndexFrom1 != 0)
    {
        BITMAP_SET(&stSslTcpBitMap, ulIndexFrom1);
    }
    SPLX_V();

    if (ulIndexFrom1 == 0)
    {
        SSLNET_Close(hSslHandle);
        BS_WARNNING(("No empty ssltcp"));
        RETURN(BS_NO_RESOURCE);
    }

    Mem_Zero(&g_stSslTcpCtrl[ulIndexFrom1-1], sizeof(_SSLTCP_CTRL_S));

    g_stSslTcpCtrl[ulIndexFrom1-1].pfWrite     =  SSLNET_Send;
    g_stSslTcpCtrl[ulIndexFrom1-1].pfRead      =  SSLNET_Recv;
    g_stSslTcpCtrl[ulIndexFrom1-1].pfClose     =  SSLNET_Close;
    g_stSslTcpCtrl[ulIndexFrom1-1].pfAccept    =  SSLNET_Accept;
    g_stSslTcpCtrl[ulIndexFrom1-1].pfListen    =  SSLNET_Listen;
    g_stSslTcpCtrl[ulIndexFrom1-1].pfConnect   =  SSLNET_Listen;
    g_stSslTcpCtrl[ulIndexFrom1-1].pfSetAsyn   =  SSLNET_SetAsyn;
    g_stSslTcpCtrl[ulIndexFrom1-1].pfUnSetAsyn =  SSLNET_UnSetAsyn;
    g_stSslTcpCtrl[ulIndexFrom1-1].pfGetHostIpPort =  SSLNET_GetHostIpPort;
    g_stSslTcpCtrl[ulIndexFrom1-1].pfGetPeerIpPort =  SSLNET_GetPeerIpPort;
    g_stSslTcpCtrl[ulIndexFrom1-1].hFileHandle        = hSslHandle;
    g_stSslTcpCtrl[ulIndexFrom1-1].ulSslTcpId  = ulIndexFrom1;
    g_stSslTcpCtrl[ulIndexFrom1-1].ulFamily    = ulFamily;

    *hSslTcpId = ulIndexFrom1;

    return BS_OK;
}

