/*================================================================
*   Created by LiXingang
*   Description: 支持zone的示例
*
================================================================*/
#include "bs.h"
#include "utl/cff_utl.h"
#include "utl/plug_utl.h"
#include "utl/exec_utl.h"
#include "../../h/pwatcher_session.h"
#include "../../h/pwatcher_def.h"
#include "../../h/pwatcher_event.h"
#include "../h/pwatcher_ob_common.h"

typedef struct {
    UINT service_enable: 1; 
}PWATCHER_OB_EXAMPLE_SERVICE_S;

static int pwatcher_ob_example_input(UINT point, PWATCHER_PKT_DESC_S *pkt, void *data);

static PWATCHER_EV_OB_S g_pwatcher_ob_node = {
    .enabled=1,
    .ob_name="example",
    .event_func=pwatcher_ob_example_input
};

static PWATCHER_EV_POINT_S g_pwatcher_ob_points[] = {
    {.valid=1, .ob=&g_pwatcher_ob_node, .point=PWATCHER_EV_IP},
    {.valid=1, .ob=&g_pwatcher_ob_node, .point=PWATCHER_EV_ZONE},
    {.valid=0, .ob=&g_pwatcher_ob_node}
};

static PWATCHER_OB_EXAMPLE_SERVICE_S * pwatcher_ob_example_build_zone_service(PWATCHER_ZONE_S *zone)
{
    if (! zone) {
        return NULL;
    }

    if (zone->attr[PWATCHER_ZONE_ATTR_EXAMPLE]) {
        return zone->attr[PWATCHER_ZONE_ATTR_EXAMPLE];
    }

    PWATCHER_OB_EXAMPLE_SERVICE_S *svr = MEM_ZMalloc(sizeof(PWATCHER_OB_EXAMPLE_SERVICE_S));
    if (! svr) {
        return NULL;
    }

    zone->attr[PWATCHER_ZONE_ATTR_EXAMPLE] = svr;

    return svr;
}

static PWATCHER_OB_EXAMPLE_SERVICE_S * pwatcher_ob_example_get_service_by_env(void *env, int build, char *zone_name)
{
    PWATCHER_ZONE_S *zone = PWatcherZone_GetZoneByEnv(env);
    if (! zone) {
        return NULL;
    }

    if (! zone->attr[PWATCHER_ZONE_ATTR_EXAMPLE]) {
        if (build) {
            pwatcher_ob_example_build_zone_service(zone);
        }
    }

    if (zone_name) {
        strncpy(zone_name, zone->name, PWATCHER_ZONE_NAME_SIZE);
    }

    return zone->attr[PWATCHER_ZONE_ATTR_EXAMPLE];
}

static int pwatcherob_example_save_service(HANDLE hFile, PWATCHER_OB_EXAMPLE_SERVICE_S *svr)
{
    if (svr->service_enable) {
        CMD_EXP_OutputCmd(hFile, "ob example enable");
    }

    return 0;
}

static void pwatcher_ob_example_free_service(void *mem)
{
    PWATCHER_OB_EXAMPLE_SERVICE_S *svr = mem;
    MEM_Free(svr);
}

static void pwatcher_ob_example_zone_event(PWATCHER_ZONE_EV_S *ev)
{
    PWATCHER_OB_EXAMPLE_SERVICE_S *svr = ev->zone->attr[PWATCHER_ZONE_ATTR_EXAMPLE];

    if (! svr) {
        return;
    }

    switch (ev->ev_type) {
        case PWATCHER_ZONE_EV_SAVE:
            pwatcherob_example_save_service(ev->hFile, svr);
            break;
        case PWATCHER_ZONE_EV_DELETE_RCU:
            ev->zone->attr[PWATCHER_ZONE_ATTR_EXAMPLE] = NULL;
            pwatcher_ob_example_free_service(svr);
            break;
    }
}

static void pwatcher_ob_example_ip_input(PWATCHER_PKT_DESC_S *pkt_info)
{
    
}

static int pwatcher_ob_example_input(UINT point, PWATCHER_PKT_DESC_S *pkt_info, void *data)
{
    switch(point) {
        case PWATCHER_EV_IP:
            pwatcher_ob_example_ip_input(pkt_info);
            break;
        case PWATCHER_EV_ZONE:
            pwatcher_ob_example_zone_event(data);
            break;
        default:
            break;
    }

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


PLUG_API int PWatcherObTopn_CmdEnable(int argc, char **argv, void *env)
{
    PWATCHER_OB_EXAMPLE_SERVICE_S *svr = pwatcher_ob_example_get_service_by_env(env, 1, NULL);

    if (svr == NULL) {
        EXEC_OutString("Cat't get example handle \r\n");
        RETURN(BS_ERR);
    }

    if (argv[0][0] == 'n') {
        svr->service_enable = 0;
    } else {
        svr->service_enable = 1;
    }

    return 0;
}

PLUG_API int PWatcherObTopn_Save(HANDLE hFile)
{
    PWATCHEROB_SaveFilter(hFile);
    return 0;
}

