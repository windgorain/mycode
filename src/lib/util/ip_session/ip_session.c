/*================================================================
* Author：LiXingang. Data: 2019.07.25
* Description：
*
================================================================*/
#include "bs.h"
#include "utl/cuckoo_hash.h"
#include "utl/ob_utl.h"
#include "utl/box_utl.h"
#include "utl/vclock_utl.h"
#include "utl/ip_session.h"

#define IP_SESSION_DFT_TIMEOUT_STEPS 300

int IPSession_Init(IP_SESSION_CTRL_S *ctrl, CUCKOO_HASH_NODE_S *hash_table,
        UINT bucket_num, UINT bucket_depth)
{
    int ret;

    ret = IPTupBox_Init(&ctrl->ip_tup_box, hash_table, bucket_num,
            bucket_depth);
    if (ret != BS_OK) {
        return ret;
    }

    ctrl->timeout_tick = IP_SESSION_DFT_TIMEOUT_STEPS;

    VCLOCK_InitInstance(&ctrl->vclock, FALSE);

    OB_INIT(&ctrl->ob_list);

    return BS_OK;
}

void IPSession_Fini(IP_SESSION_CTRL_S *ctrl)
{
    Box_Fini(&ctrl->ip_tup_box, NULL, NULL);
}

void IPSession_SetTimeoutSteps(IP_SESSION_CTRL_S *ctrl, unsigned int tick)
{
    ctrl->timeout_tick = tick;
}

IP_SESSION_CTRL_S * IPSession_Create(UINT bucket_num, UINT bucket_depth)
{
    IP_SESSION_CTRL_S *ctrl;
    CUCKOO_HASH_NODE_S *hash_table;

    ctrl = MEM_ZMalloc(sizeof(IP_SESSION_CTRL_S));
    if (! ctrl) {
        return NULL;
    }
    hash_table = 
        MEM_ZMalloc(sizeof(CUCKOO_HASH_NODE_S)*bucket_num*bucket_depth);
    if (! hash_table) {
        MEM_Free(ctrl);
        return NULL;
    }

    IPSession_Init(ctrl, hash_table, bucket_num, bucket_depth);

    return ctrl;
}

void IPSession_Destroy(IP_SESSION_CTRL_S *ctrl)
{
    IPSession_Fini(ctrl);
    MEM_Free(ctrl->ip_tup_box.hash.table);
    MEM_Free(ctrl);
}

void IPSession_RegOb(IP_SESSION_CTRL_S *ctrl, OB_S *ob)
{
    OB_Add(&ctrl->ob_list, ob);
}

void IPSession_UnregOb(IP_SESSION_CTRL_S *ctrl, OB_S *ob)
{
    OB_Del(&ctrl->ob_list, ob);
}

static void ipsession_NotifyBeforeAdd(IP_SESSION_CTRL_S *ctrl, IP_SESSION_S *session)
{
    OB_NOTIFY2(&ctrl->ob_list, session, IP_SESSION_EVENT_BEFORE_ADD);
}

static void ipsession_NotifyAfterAdd(IP_SESSION_CTRL_S *ctrl, IP_SESSION_S *session)
{
    OB_NOTIFY2(&ctrl->ob_list, session, IP_SESSION_EVENT_AFTER_ADD);
}

static void ipsession_NotifyBeforeDel(IP_SESSION_CTRL_S *ctrl, IP_SESSION_S *session)
{
    OB_NOTIFY2(&ctrl->ob_list, session, IP_SESSION_EVENT_BEFORE_DEL);
}

static void ipsession_NotifyAfterDel(IP_SESSION_CTRL_S *ctrl, IP_SESSION_S *session)
{
    OB_NOTIFY2(&ctrl->ob_list, session, IP_SESSION_EVENT_AFTER_DEL);
}

static void ipsession_timeout(HANDLE timer, USER_HANDLE_S *pstUserHandle)
{
    IP_SESSION_S *session;
    IP_SESSION_CTRL_S *ctrl = pstUserHandle->ahUserHandle[0];

    session = container_of(timer, IP_SESSION_S, vclock_timer);

    IPSession_Del(ctrl, &session->key);
}

static inline int ipsession_KeyInit(IP_TUP_KEY_S *key, UCHAR family, UCHAR protocol,
        UINT client_ip, UINT server_ip, USHORT client_port, USHORT server_port)
{
    int client_index = MIN(client_ip, server_ip) == client_ip ? 0 : 1;
    int server_index = client_index == 0 ? 1 : 0;

    key->family = family;
    key->protocol = protocol;
    key->ip[client_index].ip4 = client_ip;
    key->ip[server_index].ip4 = server_ip;
    key->port[client_index] = client_port;
    key->port[server_index] = server_port;

    return client_index;
}

/* 返回Client ip所占的index */
int IPSession_KeyInit(IP_TUP_KEY_S *key, UCHAR family, UCHAR protocol,
        UINT client_ip, UINT server_ip, USHORT client_port, USHORT server_port)
{
    memset(key, 0, sizeof(IP_TUP_KEY_S));

    return ipsession_KeyInit(key, family, protocol,
            client_ip, server_ip, client_port, server_port);
}

void IPSession_SessInit(IP_SESSION_S *session, UCHAR family, UCHAR protocol,
        UINT client_ip, UINT server_ip, USHORT client_port, USHORT server_port)
{
    memset(session, 0, sizeof(IP_SESSION_S));
    session->client_index = ipsession_KeyInit(&session->key, family, protocol,
            client_ip, server_ip, client_port, server_port);
}

int IPSession_Add(IP_SESSION_CTRL_S *ctrl, IP_SESSION_S *session)
{
    int index;
    USER_HANDLE_S user_handle;

    ipsession_NotifyBeforeAdd(ctrl, session);

    index = Box_Add(&ctrl->ip_tup_box, &session->key);
    if (index < 0) {
        return index;
    }

    session->key_hash = Box_GetKeyHash(&ctrl->ip_tup_box, index);

    user_handle.ahUserHandle[0] = ctrl;

    VCLOCK_AddTimer(&ctrl->vclock, &session->vclock_timer,
            ctrl->timeout_tick, ctrl->timeout_tick,
            0, ipsession_timeout, &user_handle);

    ipsession_NotifyAfterAdd(ctrl, session);

    return index;
}

int IPSession_Find(IP_SESSION_CTRL_S *ctrl, IP_TUP_KEY_S *key)
{
    return Box_Find(&ctrl->ip_tup_box, key);
}

IP_SESSION_S * IPSession_FindSession(IP_SESSION_CTRL_S *ctrl, IP_TUP_KEY_S *key)
{
    return Box_FindData(&ctrl->ip_tup_box, key);
}

IP_SESSION_S * IPSession_DelByIndex(IP_SESSION_CTRL_S *ctrl, int index)
{
    IP_SESSION_S *session;

    session = Box_GetData(&ctrl->ip_tup_box, index);
    if (! session) {
        return NULL;
    }

    ipsession_NotifyBeforeDel(ctrl, session);
    Box_DelByIndex(&ctrl->ip_tup_box, index);
    VCLOCK_DelTimer(&ctrl->vclock, &session->vclock_timer);
    ipsession_NotifyAfterDel(ctrl, session);

    return session;
}

IP_SESSION_S * IPSession_Del(IP_SESSION_CTRL_S *ctrl, IP_TUP_KEY_S *key)
{
    int index;

    index = Box_Find(&ctrl->ip_tup_box, key);
    if (index < 0) {
        return NULL;
    }

    return IPSession_DelByIndex(ctrl, index);
}

IP_SESSION_S * IPSession_GetSession(IP_SESSION_CTRL_S *ctrl, int index)
{
    return Box_GetData(&ctrl->ip_tup_box, index);
}

/* index=-1表示从头开始, return -1表示结束 */
int IPSession_GetNext(IP_SESSION_CTRL_S *ctrl, int index)
{
    return Box_GetNext(&ctrl->ip_tup_box, index);
}

int IPSession_Count(IP_SESSION_CTRL_S *ctrl)
{
    return Box_Count(&ctrl->ip_tup_box);
}

void IPSession_TimeStep(IP_SESSION_CTRL_S *ctrl)
{
    VCLOCK_Step(&ctrl->vclock);
}

void IPSession_Refresh(IP_SESSION_CTRL_S *ctrl, IP_SESSION_S *session)
{
    VCLOCK_Refresh(&ctrl->vclock, &session->vclock_timer);
}

