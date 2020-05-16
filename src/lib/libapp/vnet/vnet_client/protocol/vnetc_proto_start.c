
#include "bs.h"

#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_user_status.h"
#include "../inc/vnetc_master.h"
#include "../inc/vnetc_tp.h"
#include "../inc/vnetc_fsm.h"
#include "../inc/vnetc_auth.h"
#include "../inc/vnetc_phy.h"
#include "../inc/vnetc_ses_c2s.h"
#include "../inc/vnetc_p_addr_change.h"


static VOID vnetc_proto_Stop(IN USER_HANDLE_S *pstUserHandle)
{
    VNETC_P_AddrChange_Stop();
    VNETC_AUTH_Logout();
    VNETC_TP_Stop();
    VNETC_SesC2S_Close();
    VNETC_User_SetStatus(VNET_USER_STATUS_INIT, VNET_USER_REASON_NONE);
    VNETC_FSM_ChangeState(VNETC_FSM_STATE_INIT);
}

static VOID vnetc_proto_ReStart(IN USER_HANDLE_S *pstUserHandle)
{
    vnetc_proto_Stop(pstUserHandle);
    VNETC_Start();
}

BS_STATUS VNETC_Proto_Init()
{
    VNETC_Master_SetEvent(VNETC_MASTER_EVENT_STOP, vnetc_proto_Stop, NULL);
    VNETC_Master_SetEvent(VNETC_MASTER_EVENT_RESTART, vnetc_proto_ReStart, NULL);

    return BS_OK;
}

BS_STATUS VNETC_Proto_Stop()
{
    return VNETC_Master_EventInput(VNETC_MASTER_EVENT_STOP);
}

BS_STATUS VNETC_Proto_ReStart()
{
    return VNETC_Master_EventInput(VNETC_MASTER_EVENT_RESTART);
}

