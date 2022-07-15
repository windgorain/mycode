/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-2-25
* Description: 
* History:     
******************************************************************************/

#ifndef __VS_PROTOSW_H_
#define __VS_PROTOSW_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef INT (*PF_VS_ATTACH)(IN VOID *pstSocket, IN INT uiProto);

typedef struct
{
    PF_VS_ATTACH pfAttach;
}VS_USER_REQUEST_S;

typedef struct
{
    VS_DOMAIN_S *pstDomain;

    USHORT usType;
    USHORT usProtocol;
    USHORT usFlag;

    VS_USER_REQUEST_S *pstUsrRequest;
}VS_PROTOSW_S;


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VS_PROTOSW_H_*/


