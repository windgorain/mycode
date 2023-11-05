/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/cff_utl.h"
#include "utl/plug_utl.h"
#include "../../h/pwatcher_session.h"
#include "../../h/pwatcher_def.h"
#include "../../h/pwatcher_event.h"
#include "../h/pwatcher_ob_common.h"

static int pwatcher_ob_example_input(UINT point, PWATCHER_PKT_DESC_S *pkt, void *data);

static PWATCHER_EV_OB_S g_pwatcher_ob_node = {
    .ob_name="example",
    .event_func=pwatcher_ob_example_input
};

static PWATCHER_EV_POINT_S g_pwatcher_ob_points[] = {
    {.valid=1, .ob=&g_pwatcher_ob_node, .point=PWATCHER_EV_IP},
    {.valid=0, .ob=&g_pwatcher_ob_node}
};

static int pwatcher_ob_example_input(UINT point, PWATCHER_PKT_DESC_S *pkt, void *data)
{
    

    return 0;
}

static int pwatcher_ob_example_init()
{
    PWatcherEvent_Reg(g_pwatcher_ob_points);

    return 0;
}

static void pwatcher_ob_example_finit()
{
    PWatcherEvent_UnReg(g_pwatcher_ob_points);
    Sleep(1000);
}

PLUG_API BOOL_T DllMain(PLUG_HDL hPlug, int reason, void *reserved)
{
    switch(reason) {
        case DLL_PROCESS_ATTACH:
            pwatcher_ob_example_init();
            break;

        case DLL_PROCESS_DETACH:
            pwatcher_ob_example_finit();
            break;
    }

    return TRUE;
}

PLUG_ENTRY 

PWATCHER_OB_FUNCTIONS 

