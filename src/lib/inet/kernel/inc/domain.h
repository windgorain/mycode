/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-15
* Description: 
* History:     
******************************************************************************/

#ifndef __DOMAIN_H_
#define __DOMAIN_H_

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

typedef struct domain_s
{
    UINT uiDomFamily;
    CHAR *pcDomName;
    PROTOSW_S *pstProtoswStart;
    PROTOSW_S *pstProtoswEnd;
    struct domain_s *pstDomNext;
    void    (*dom_dispose)(MBUF_S *); 
}DOMAIN_S;

extern int max_hdr;

PROTOSW_S * DOMAIN_FindProto(IN UINT uiFamily, IN USHORT usProtocol, IN USHORT usType);
PROTOSW_S * DOMAIN_FindType(IN UINT uiFamily, IN USHORT usType);

#ifdef __cplusplus
    }
#endif 

#endif 


