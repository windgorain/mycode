/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-2-25
* Description: 
* History:     
******************************************************************************/

#ifndef __VS_DOMAIN_H_
#define __VS_DOMAIN_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct
{
    DLL_NODE_S stLinkNode;
    UINT uiDomFamily;
    UINT uiProtoSwCount;
    PROTOSW_S *pstProtoswArry;
}VS_DOMAIN_S;

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VS_DOMAIN_H_*/


