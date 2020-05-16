/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "comp/comp_precver.h"

static COMP_PRECVER_S g_comp_precver = {
    .comp.comp_name = COMP_PRECVER_NAME,
    .precver_getnext = PRecver_GetNext,
    .precver_is_exit = PRecver_IsExit,
    .precver_run = PRecver_Run,
};

int PRecver_Comp_Init()
{
    COMP_Reg(&g_comp_precver.comp);
    return 0;
}

