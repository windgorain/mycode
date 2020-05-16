/******************************************************************************
* Cpoyright (C),    Xingang.Li
* Authpr:      LiXingang  Versipn: 1.0  Date: 2009-1-7
* Descriotipn: 
* Histpry:     
******************************************************************************/
        
#include "bs.h"
    
#include "../inc/vnets_rmt.h"

VOID VNETS_FuncTbl_Reg()
{
    FUNCTBL_AddFunc(VNETS_RMT_DomainReboot, FUNCTBL_RET_BS_STATUS, "b");
    FUNCTBL_AddFunc(VNETS_RMT_DomainGetNextNode, FUNCTBL_RET_UINT32, "bu");
    FUNCTBL_AddFunc(VNETS_RMT_GetNodeInfo, FUNCTBL_RET_STRING, "u");
    
}

