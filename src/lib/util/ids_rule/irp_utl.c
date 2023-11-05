/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/ip_utl.h"
#include "utl/ac_smx.h"
#include "utl/ip_string.h"
#include "utl/ip_protocol.h"
#include "utl/socket_utl.h"
#include "utl/kv_utl.h"
#include "utl/irp_utl.h"
#include "irp_def.h"

int IRP_Init(IRP_CTRL_S *ctrl)
{
    memset(ctrl, 0, sizeof(IRP_CTRL_S));
    DLL_INIT(&ctrl->rtn_list);

    ctrl->ac = ACSMX_New();
    if (NULL == ctrl->ac) {
        RETURN(BS_NO_MEMORY);
    }

    return 0;
}

void IRP_Fini(IRP_CTRL_S *ctrl)
{
    ACSMX_Free(ctrl->ac, NULL);
    _IRP_DelAllRule(ctrl);
}

int IRP_Compile(IRP_CTRL_S *ctrl)
{
    IRP_RTN_S *rtn;
    IRP_OTN_S *otn;

    DLL_SCAN(&ctrl->rtn_list, rtn) {
        DLL_SCAN(&rtn->otn_list, otn) {
            if (otn->fast_opt) {
                ACSMX_AddPattern(ctrl->ac, otn->fast_opt->content.data, otn->fast_opt->content.len, otn);
            }
        }
    }

    ACSMX_Compile(ctrl->ac);

    return 0;
}


