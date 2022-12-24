/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/idkey_utl.h"
#include "utl/map_utl.h"
#include "utl/txt_utl.h"
#include "comp/comp_muc.h"
#include "../h/pwatcher_def.h"
#include "../h/pwatcher_zone.h"
#include "../h/pwatcher_rcu.h"
#include "../h/pwatcher_muc.h"
#include "../h/pwatcher_event.h"
#include "../h/pwatcher_event_data.h"

static int pwatcher_zone_event_in(UINT point, PWATCHER_PKT_DESC_S *pkt, void *data);

static IDKEY_S *g_pwatcher_zone_idkey;
static MAP_HANDLE g_pwatcher_zone_map; /* rule->zone的映射关系 */

typedef struct {
    PWATCHER_ZONE_S global_zone;
    IDKEY_S *idkey;
    MAP_HANDLE zone_map;  /* rule->zone的映射关系 */
}PWATCHER_ZONE_MUC_S;

static inline void pwatcher_zone_delete(PWATCHER_ZONE_S *zone);

static char *g_pwatcher_zone_type_names[] = {
#define _(a,b) b,
    PWATCHER_ZONE_TYPE_DEF
#undef _
    "unknown"
};

static PWATCHER_EV_OB_S g_pwatcher_ob_node = {
    .ob_name="zone",
    .event_func=pwatcher_zone_event_in
};

static PWATCHER_EV_POINT_S g_pwatcher_ob_points[] = {
    {.valid=1, .ob=&g_pwatcher_ob_node, .point=PWATCHER_EV_WORKER_TIMER},
    {.valid=1, .ob=&g_pwatcher_ob_node, .point=PWATCHER_EV_GLOBAL_TIMER},
    {.valid=0, .ob=&g_pwatcher_ob_node}
};

static MUTEX_S g_pwatcher_zone_lock;

static void pwatcher_zone_each_ev(IDKEY_KL_S *kl, INT64 id, void *data, void *ud)
{
    PWATCHER_ZONE_EV_S *ev = ud;
    ev->zone = data;

    PWatcherEvent_Notify(PWATCHER_EV_ZONE, NULL, ev);
}

static void pwatcher_zone_global_zone_ev(int muc_id, PWATCHER_ZONE_EV_S *ev)
{
    PWATCHER_ZONE_S *global_zone = PWatcherZone_GetGlobalZone(muc_id);
    if (global_zone == NULL) {
        return;
    }

    ev->zone = global_zone;

    PWatcherEvent_Notify(PWATCHER_EV_ZONE, NULL, ev);
}

static inline void pwatcher_zone_worker_timer_input(PWATCHER_TIMER_EV_S *ev)
{
    PWATCHER_ZONE_EV_S zone_ev;
    int muc_id = -1;

    zone_ev.ev_type = PWATCHER_ZONE_EV_WORKER_TIMER;
    zone_ev.worker_timer_ev = ev;

    while((muc_id = MUC_GetNext(muc_id)) >= 0) {
        pwatcher_zone_global_zone_ev(muc_id, &zone_ev);
        PWatcherZone_Walk(muc_id, pwatcher_zone_each_ev, &zone_ev);
    }
}

static inline void pwatcher_zone_global_timer_input(PWATCHER_TIMER_EV_S *ev)
{
    PWATCHER_ZONE_EV_S zone_ev;
    int muc_id = -1;

    zone_ev.ev_type = PWATCHER_ZONE_EV_GLOBAL_TIMER;

    while((muc_id = MUC_GetNext(muc_id)) >= 0) {
        pwatcher_zone_global_zone_ev(muc_id, &zone_ev);
        PWatcherZone_Walk(muc_id, pwatcher_zone_each_ev, &zone_ev);
    }
}

static int pwatcher_zone_event_in(UINT point, PWATCHER_PKT_DESC_S *pkt, void *data)
{
    switch (point) {
        case PWATCHER_EV_WORKER_TIMER:
            pwatcher_zone_worker_timer_input(data);
            break;
        case PWATCHER_EV_GLOBAL_TIMER:
            pwatcher_zone_global_timer_input(data);
            break;
    }

    return 0;
}

static void pwatcher_zone_idkey_free(void *data, void *ud)
{
    pwatcher_zone_delete(data);
}

static void pwatcher_zone_destroy_muc(PWATCHER_ZONE_MUC_S *node)
{
    if (node->idkey) {
        IDKEY_Destroy(node->idkey, pwatcher_zone_idkey_free, NULL);
    }
    if (node->zone_map) {
        MAP_Destroy(node->zone_map, NULL, NULL);
    }
    MEM_Free(node);
}

static void pwatcher_zone_init_global_zone(int muc_id, PWATCHER_ZONE_MUC_S *node)
{
    strlcpy(node->global_zone.name, "global", sizeof(node->global_zone.name));
    node->global_zone.muc_id = muc_id;
    node->global_zone.enable = 1;

    PWATCHER_ZONE_EV_S ev;
    ev.ev_type = PWATCHER_ZONE_EV_CREATE;
    ev.zone = &node->global_zone;
    PWatcherEvent_Notify(PWATCHER_EV_ZONE, NULL, &ev);
}

static void pwatcher_zone_destroy_global_zone(PWATCHER_ZONE_MUC_S *node)
{
    PWATCHER_ZONE_EV_S ev;

    ev.zone = &node->global_zone;

    ev.ev_type = PWATCHER_ZONE_EV_DELETE;
    PWatcherEvent_Notify(PWATCHER_EV_ZONE, NULL, &ev);

    ev.ev_type = PWATCHER_ZONE_EV_DELETE_RCU;
    PWatcherEvent_Notify(PWATCHER_EV_ZONE, NULL, &ev);
}

static int pwatcher_zone_create_muc_locked(int muc_id, PWATCHER_MUC_S *pwatcher_muc)
{
    PWATCHER_ZONE_MUC_S *node;

    if (pwatcher_muc->zone != NULL) {
        return 0;
    }

    node = MEM_ZMalloc(sizeof(PWATCHER_ZONE_MUC_S));
    if (node == NULL) {
        RETURN(BS_ERR);
    }

    IDKEY_PARAM_S idkey_param = {0};
    idkey_param.bucket_num = 8192;
    idkey_param.memcap = RcuEngine_GetMemcap();
    idkey_param.flag = IDKEY_FLAG_ID_MAP | IDKEY_FLAG_KEY_MAP;

    node->idkey = IDKEY_BaseCreate(&idkey_param);
    if (node->idkey == NULL) {
        pwatcher_zone_destroy_muc(node);
        RETURN(BS_NO_MEMORY);
    }

    MAP_PARAM_S map_param = {0};
    map_param.bucket_num = 8192;
    map_param.memcap = RcuEngine_GetMemcap();

    node->zone_map = MAP_HashCreate(&map_param);
    if (! node->zone_map) {
        pwatcher_zone_destroy_muc(node);
        RETURN(BS_ERR);
    }

    pwatcher_muc->zone = node;

    pwatcher_zone_init_global_zone(muc_id, node);

    return 0;
}

static int pwatcher_zone_create_muc(int muc_id, PWATCHER_MUC_S *pwatcher_muc)
{
    int ret;

    MUTEX_P(&g_pwatcher_zone_lock);
    ret = pwatcher_zone_create_muc_locked(muc_id, pwatcher_muc);
    MUTEX_V(&g_pwatcher_zone_lock);

    return ret;
}

static PWATCHER_ZONE_MUC_S * pwatcher_zone_get_muc(int muc_id)
{
    PWATCHER_MUC_S *pwatcher_muc = PWatcherMuc_Get(muc_id);
    if (! pwatcher_muc) {
        return NULL;
    }

    return pwatcher_muc->zone;
}

static PWATCHER_ZONE_MUC_S * pwatcher_zone_get_create_muc(int muc_id)
{
    PWATCHER_MUC_S *pwatcher_muc = PWatcherMuc_Get(muc_id);
    if (! pwatcher_muc) {
        return NULL;
    }

    if (pwatcher_muc->zone) {
        return pwatcher_muc->zone;
    }

    pwatcher_zone_create_muc(muc_id, pwatcher_muc);

    return pwatcher_muc->zone;
}

static inline void pwatcher_zone_delete(PWATCHER_ZONE_S *zone)
{
    PWatcherZone_DelRule(zone);

    PWATCHER_ZONE_EV_S ev;
    ev.ev_type = PWATCHER_ZONE_EV_DELETE;
    ev.zone = zone;
    PWatcherEvent_Notify(PWATCHER_EV_ZONE, NULL, &ev);
}

char * PWatcherZone_GetStrByType(int type)
{
    if (type >= PWATCHER_ZONE_TYPE_MAX) {
        return "unkonwn";
    }

    return g_pwatcher_zone_type_names[type];
}

int PWatcherZone_GetTypeByStr(char *type_str)
{
    int i;

    for (i=0; i<PWATCHER_ZONE_TYPE_MAX; i++) {
        if (0 == strcmp(g_pwatcher_zone_type_names[i], type_str)) {
            return i;
        }
    }

    return -1;
}

/* 添加一个Zone, 并自动分配ZoneID */
int PWatcherZone_Add(PWATCHER_ZONE_S *zone)
{
    int name_size = strlen(zone->name) + 1;

    PWATCHER_ZONE_MUC_S *zone_muc = pwatcher_zone_get_create_muc(zone->muc_id);
    if (! zone_muc) {
        RETURN(BS_ERR);
    }

    IDKEY_S *ctrl = zone_muc->idkey;

    if (IDKEY_GetIDByKey(ctrl, zone->name, name_size) >= 0) {
        /* 已经存在对应的KEY */
        RETURN(BS_ALREADY_EXIST);
    }

    INT64 index = IDKEY_Add(ctrl, zone->name, name_size, zone, 0);
    if (index < 0) {
        RETURN(BS_ERR);
    }

    zone->zone_id = PWATCHER_ZONE_BUILD_ID(zone->zone_type, index);

    PWATCHER_ZONE_EV_S ev;
    ev.ev_type = PWATCHER_ZONE_EV_CREATE;
    ev.zone = zone;
    PWatcherEvent_Notify(PWATCHER_EV_ZONE, NULL, &ev);

    return 0;
}

void * PWatcherZone_DelByName(int muc_id, char *name)
{
    PWATCHER_ZONE_S *zone;

    PWATCHER_ZONE_MUC_S *zone_muc = pwatcher_zone_get_muc(muc_id);
    if (! zone_muc) {
        return NULL;
    }

    IDKEY_S *ctrl = zone_muc->idkey;

    zone = IDKEY_DelByKey(ctrl, name, strlen(name) + 1);
    if (! zone) {
        return NULL;
    }

    pwatcher_zone_delete(zone);

    return zone;
}

void * PWatcherZone_DelByID(int muc_id, UINT64 zone_id)
{
    UINT64 index = PWATCHER_ZONE_ID_2_INDEX(zone_id);

    PWATCHER_ZONE_MUC_S *zone_muc = pwatcher_zone_get_muc(muc_id);
    if (! zone_muc) {
        return NULL;
    }

    IDKEY_S *ctrl = zone_muc->idkey;

    PWATCHER_ZONE_S *zone = IDKEY_DelByID(ctrl, index);
    if (! zone) {
        return NULL;
    }

    pwatcher_zone_delete(zone);

    return zone;
}

PWATCHER_ZONE_S * PWatcherZone_GetZoneByName(int muc_id, char *name)
{
    PWATCHER_ZONE_MUC_S *zone_muc = pwatcher_zone_get_muc(muc_id);
    if (! zone_muc) {
        return NULL;
    }

    IDKEY_S *ctrl = zone_muc->idkey;

    return IDKEY_GetDataByKey(ctrl, name, strlen(name) + 1);
}

PWATCHER_ZONE_S * PWatcherZone_GetGlobalZone(int muc_id)
{
    PWATCHER_ZONE_MUC_S *zone_muc = pwatcher_zone_get_create_muc(muc_id);
    if (! zone_muc) {
        return NULL;
    }
    return &zone_muc->global_zone;
}

PWATCHER_ZONE_S * PWatcherZone_GetZoneByEnv(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    char *view_name = CmdExp_GetCurrentViewName(env);

    if (0 == strcmp(view_name, "pwatcher-zone")) {
        return PWatcherZone_GetZoneByName(muc_id, CMD_EXP_GetCurrentModeValue(env));
    }

    return PWatcherZone_GetGlobalZone(muc_id);
}

void PWatcherZone_Walk(int muc_id, PF_IDKEY_WALK_FUNC walk_func, void *ud)
{
    PWATCHER_ZONE_MUC_S *zone_muc = pwatcher_zone_get_muc(muc_id);
    if (! zone_muc) {
        return;
    }
    IDKEY_S *ctrl = zone_muc->idkey;

    IDKEY_Walk(ctrl, walk_func, ud);
}

static void pwatcher_zone_walk_type (IDKEY_KL_S *kl, INT64 id, void *data, void *ud)
{
    USER_HANDLE_S *uh = ud;
    PWATCHER_ZONE_S *zone = data;
    UINT type = HANDLE_UINT(uh->ahUserHandle[0]);

    if (zone->zone_type != type) {
        return;
    }

    PF_IDKEY_WALK_FUNC func = uh->ahUserHandle[1];
    func(kl, id, data, uh->ahUserHandle[2]);
}

void PWatcherZone_WalkType(int muc_id, UINT type, PF_IDKEY_WALK_FUNC walk_func, void *ud)
{
    USER_HANDLE_S uh;
    uh.ahUserHandle[0] = UINT_HANDLE(type);
    uh.ahUserHandle[1] = walk_func;
    uh.ahUserHandle[2] = ud;

    PWatcherZone_Walk(muc_id, pwatcher_zone_walk_type, &uh);
}

/* 按照Type顺序遍历 */
void PWatcherZone_WalkByTypeOrder(int muc_id, PF_IDKEY_WALK_FUNC walk_func, void *ud)
{
    UINT i;
    for (i=0; i<PWATCHER_ZONE_TYPE_MAX; i++) {
        PWatcherZone_WalkType(muc_id, i, walk_func, ud);
    }
}

int PWatcherZone_ChangeRule(PWATCHER_ZONE_S *zone, void *new_rule)
{
    UINT new_type = *(UINT*)new_rule;
    UINT old_type = *(UINT*)zone->rule_data;

    if (new_type != old_type) {
        BS_DBGASSERT(0);
        RETURN(BS_ERR);
    }

    if (zone->rule_setted) {
        if (memcmp(new_rule, zone->rule_data, zone->rule_key_size) == 0) {
            return 0;
        }
    }

    if (MAP_Get(g_pwatcher_zone_map, new_rule, zone->rule_key_size)) {
        RETURN(BS_ALREADY_EXIST);
    }

    MAP_Del(g_pwatcher_zone_map, zone->rule_data, zone->rule_key_size);

    memcpy(zone->rule_data, new_rule, zone->rule_key_size);

    int ret = MAP_Add(g_pwatcher_zone_map, zone->rule_data, zone->rule_key_size, zone, 0);
    if (ret != 0) {
        return ret;
    }

    zone->rule_setted = 1;

    return 0;
}

void * PWatcherZone_DelRule(PWATCHER_ZONE_S *zone)
{
    if (zone->rule_setted == 0) {
        return NULL;
    }

    zone->rule_setted = 0;

    return MAP_Del(g_pwatcher_zone_map, zone->rule_data, zone->rule_key_size);
}

void * PWatcherZone_GetZoneByRule(void *rule, int rule_len)
{
    return MAP_Get(g_pwatcher_zone_map, rule, rule_len);
}

/* 如果存在father zone, 则必须同时匹配father */
BOOL_T PWatcherZone_CheckPktFatherZone(void *pkt_desc, PWATCHER_ZONE_S *zone)
{
    PWATCHER_PKT_DESC_S *pkt = pkt_desc;
    int i;

    if (! zone->father_zone) {
        return TRUE;
    }

    for (i=0; i<pkt->zone_num; i++) {
        if (zone->father_zone == pkt->zone[i]) {
            return TRUE;
        }
    }

    return FALSE;
}

void PWatcherZone_MucDestroy(void *pwatcher_muc_info)
{
    PWATCHER_MUC_S *pwatcher_muc = pwatcher_muc_info;
    PWATCHER_ZONE_MUC_S *node = pwatcher_muc->zone;

    if (NULL == node) {
        return;
    }

    pwatcher_zone_destroy_global_zone(node);
    pwatcher_zone_destroy_muc(node);
    pwatcher_muc->zone = NULL;
}

int PWatcherZone_Init()
{
    IDKEY_PARAM_S idkey_param = {0};
    MAP_PARAM_S map_param = {0};

    MUTEX_Init(&g_pwatcher_zone_lock);

    idkey_param.bucket_num = 8192;
    idkey_param.memcap = RcuEngine_GetMemcap();
    idkey_param.flag = IDKEY_FLAG_ID_MAP | IDKEY_FLAG_KEY_MAP;

    g_pwatcher_zone_idkey = IDKEY_BaseCreate(&idkey_param);
    if (!g_pwatcher_zone_idkey) {
        RETURN(BS_NO_MEMORY);
    }

    map_param.bucket_num = 8192;
    map_param.memcap = RcuEngine_GetMemcap();

    g_pwatcher_zone_map = MAP_HashCreate(&map_param);
    if (! g_pwatcher_zone_map) {
        RETURN(BS_ERR);
    }

    PWatcherEvent_Reg(g_pwatcher_ob_points);
    g_pwatcher_ob_node.enabled = 1;

    return 0;
}

