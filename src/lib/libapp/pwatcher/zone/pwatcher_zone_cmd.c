/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/idkey_utl.h"
#include "utl/map_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "../h/pwatcher_zone.h"
#include "../h/pwatcher_rcu.h"
#include "../h/pwatcher_event.h"
#include "../h/pwatcher_event_data.h"

static void pwatcher_zone_del_all_sub_zone(PWATCHER_ZONE_S *zone);

static void pwatcher_zone_notify_save_ev(HANDLE hFile, PWATCHER_ZONE_S *zone)
{
    if (! zone) {
        return;
    }

    
    PWATCHER_ZONE_EV_S ev;
    ev.ev_type = PWATCHER_ZONE_EV_SAVE;
    ev.zone = (void*)zone;
    ev.hFile = hFile;
    PWatcherEvent_Notify(PWATCHER_EV_ZONE, NULL, &ev);
}

static int pwatcher_zone_save(HANDLE hFile, PWATCHER_ZONE_S *zone)
{
    if (zone->description[0]) {
        CMD_EXP_OutputCmd(hFile, "description %s", zone->description);
    }

    if (zone->father_zone) {
        PWATCHER_ZONE_S *father_zone = zone->father_zone;
        CMD_EXP_OutputCmd(hFile, "father-zone %s", father_zone->name);
    }

    pwatcher_zone_notify_save_ev(hFile, zone);

    if (! zone->enable) {
        CMD_EXP_OutputCmd(hFile, "no enable");
    }

    return 0;
}

static void pwatcher_zone_process_each(IDKEY_KL_S *kl, INT64 id, void *data, void *ud)
{
    PWATCHER_ZONE_S *zone = data;
    HANDLE hFile = ud;

    if (0 == CMD_EXP_OutputMode(hFile, "zone %s type %s", zone->name, PWatcherZone_GetStrByType(zone->zone_type))) {
        pwatcher_zone_save(hFile, zone);
        CMD_EXP_OutputModeQuit(hFile);
    }
}

static int pwatcher_zone_create(int muc_id, char *type, char *name, void *env)
{
    PWATCHER_ZONE_S *zone;

    int zone_type = PWatcherZone_GetTypeByStr(type);
    if (zone_type < 0) {
        EXEC_OutString("Zone type not exist \r\n");
        RETURN(BS_ERR);
    }

    zone = PWatcherZone_GetZoneByName(muc_id, name);
    if (zone) {
        if (zone->zone_type != zone_type) {
            EXEC_OutString("Zone name has exist \r\n");
            RETURN(BS_ERR);
        } else {
            return 0;
        }
    }

    zone = MEM_ZMalloc(sizeof(PWATCHER_ZONE_S));
    if (! zone) {
        EXEC_OutString("No memory \r\n");
        RETURN(BS_ERR);
    }

    strlcpy(zone->name, name, PWATCHER_ZONE_NAME_SIZE);
    zone->zone_type = zone_type;
    zone->muc_id = muc_id;
    zone->enable = 1;

    if (0 != PWatcherZone_Add(zone)) {
        EXEC_OutString("Add zone failed \r\n");
        MEM_Free(zone);
        RETURN(BS_ERR);
    }

    return 0;
}

static int pwatcher_zone_enter(int argc, char **argv, void *env)
{
    PWATCHER_ZONE_S *zone;
    char *name = argv[1];
    char cmd[256];
    int muc_id = CmdExp_GetEnvMucID(env);

    zone = PWatcherZone_GetZoneByName(muc_id, name);
    if (! zone) {
        EXEC_OutString("Create zone need type \r\n");
        RETURN(BS_ERR);
    }

    snprintf(cmd, sizeof(cmd), "%s %s type %s", argv[0], argv[1], PWatcherZone_GetStrByType(zone->zone_type));

    return CmdExp_RunEnvCmd(cmd, env);
}

static void pwatcher_zone_free_rcu(void *rcu_node)
{
    PWATCHER_ZONE_S *zone = rcu_node;

    PWATCHER_ZONE_EV_S ev;
    ev.ev_type = PWATCHER_ZONE_EV_DELETE_RCU;
    ev.zone = (void*)zone;
    PWatcherEvent_Notify(PWATCHER_EV_ZONE, NULL, &ev);

    MEM_Free(zone);
}

static void pwatcher_zone_del_sub(IDKEY_KL_S *kl, INT64 id, void *data, void *ud)
{
    PWATCHER_ZONE_S *zone = data;
    if (zone->father_zone != ud) {
        return;
    }

    pwatcher_zone_del_all_sub_zone(zone);
    PWatcherZone_DelByID(zone->muc_id, zone->zone_id);
    RcuEngine_Call(&zone->rcu_node, pwatcher_zone_free_rcu);
}


static void pwatcher_zone_del_all_sub_zone(PWATCHER_ZONE_S *zone)
{
    PWatcherZone_Walk(zone->muc_id, pwatcher_zone_del_sub, zone);
}

static void pwatcher_zone_detach_sub(IDKEY_KL_S *kl, INT64 id, void *data, void *ud)
{
    PWATCHER_ZONE_S *zone = data;
    if (zone->father_zone != ud) {
        return;
    }

    zone->father_zone = NULL;
}


static void pwatcher_zone_detach_all_sub_zone(PWATCHER_ZONE_S *zone)
{
    PWatcherZone_Walk(zone->muc_id, pwatcher_zone_detach_sub, zone);
}


PLUG_API int PWatcherZone_CmdEnterZone(int argc, char **argv, void *env)
{
    int ret;
    int muc_id = CmdExp_GetEnvMucID(env);

    if (argc >= 4) {
        ret = pwatcher_zone_create(muc_id, argv[3], argv[1], env);
        if (ret == 0) {
            CmdExp_SetCurrentModeValue(env, argv[1]);
        }
    } else {
        ret = pwatcher_zone_enter(argc, argv, env);
    }

    if (ret != 0) {
        return ret;
    }

    return 0;
}


PLUG_API int PWatcherZone_CmdNoZone(int argc, char **argv, void *env)
{
    char *name = argv[2];
    int muc_id = CmdExp_GetEnvMucID(env);
    PWATCHER_ZONE_S *zone = PWatcherZone_DelByName(muc_id, name);

    if (! zone) {
        return -1;
    }

    if (argc >= 4) {
        pwatcher_zone_del_all_sub_zone(zone);
    } else {
        pwatcher_zone_detach_all_sub_zone(zone);
    }

    RcuEngine_Call(&zone->rcu_node, pwatcher_zone_free_rcu);

    return 0;
}


PLUG_API int PWatcherZone_CmdDescription(int argc, char **argv, void *env)
{
    PWATCHER_ZONE_S *zone = PWatcherZone_GetZoneByEnv(env);

    if (! zone) {
        RETURN(BS_ERR);
    }

    strlcpy(zone->description, argv[1], sizeof(zone->description));

    return 0;
}


PLUG_API int PWatcherZone_CmdFatherZone(int argc, char **argv, void *env)
{
    PWATCHER_ZONE_S *zone = PWatcherZone_GetZoneByEnv(env);
    int muc_id = CmdExp_GetEnvMucID(env);

    if (! zone) {
        RETURN(BS_ERR);
    }

    if (argv[0][0] == 'n') {
        zone->father_zone = NULL;
        return 0;
    }

    char *father_name = argv[1];
    PWATCHER_ZONE_S *father_zone = PWatcherZone_GetZoneByName(muc_id, father_name);
    if (! father_zone) {
        EXEC_OutString("Father zone is not exist. \r\n");
        RETURN(BS_ERR);
    }

    if (father_zone->zone_type >= zone->zone_type) {
        EXEC_OutString("Not support this father type. \r\n");
        RETURN(BS_ERR);
    }

    zone->father_zone = father_zone;

    return 0;
}


PLUG_API int PWatcherZone_CmdEnable(int argc, char **argv, void *env)
{
    PWATCHER_ZONE_S *zone = PWatcherZone_GetZoneByEnv(env);
    int enable;

    if (argv[0][0] == 'n') {
        enable = 0;
    } else {
        enable = 1;
    }

    zone->enable = enable;

    return 0;
}

static void pwatcher_zone_show_each(IDKEY_KL_S *kl, INT64 id, void *data, void *ud)
{
    PWATCHER_ZONE_S *zone = data;

    EXEC_OutInfo("%-10s %-10s %-8u \r\n",
            PWatcherZone_GetStrByType(zone->zone_type), zone->name, zone->enable);
}


PLUG_API int PWatcherZone_Show(int argc, char **argv, void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);

    EXEC_OutString("Type       Name       Enable \r\n"
            "-------------------------------------------- \r\n");

    PWatcherZone_WalkByTypeOrder(muc_id, pwatcher_zone_show_each, NULL);

    return 0;
}

PLUG_API int PWatcherZone_Save(HANDLE hFile)
{
    int muc_id = CmdExp_GetMucIDBySaveHandle(hFile);

    pwatcher_zone_notify_save_ev(hFile, PWatcherZone_GetGlobalZone(muc_id));

    PWatcherZone_WalkByTypeOrder(muc_id, pwatcher_zone_process_each, hFile);
    return 0;
}

