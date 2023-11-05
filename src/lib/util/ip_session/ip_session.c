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
#include "utl/ip_utl.h"
#include "utl/ip_session.h"

static UINT g_ipsession_dft_timeout_steps[IP_SESSION_STATE_MAX] = {
    5,  
    5,  
    10, 
    10, 
    300,
    5,  
    10, 
    10, 
    10, 
    10, 
    5,  
    10, 
    5,  
};

#ifdef IN_DEBUG
static void ipsession_Test()
{
    int i;
    for (i=1; i<IP_SESSION_STATE_MAX; i++) {
        BS_DBGASSERT(g_ipsession_dft_timeout_steps[i] != 0);
    }
}

CONSTRUCTOR(test) {
    ipsession_Test();
}
#endif

int IPSession_Init(IP_SESSION_CTRL_S *ctrl, CUCKOO_HASH_NODE_S *hash_table,
        UINT bucket_num, UINT bucket_depth)
{
    int ret;
    int i;

    memset(ctrl, 0, sizeof(IP_SESSION_CTRL_S));

    ret = IPTupBox_Init(&ctrl->ip_tup_box, hash_table, bucket_num,
            bucket_depth);
    if (ret != BS_OK) {
        BS_WARNNING(("Can't init ip session"));
        return ret;
    }

    for (i=0; i<IP_SESSION_STATE_MAX; i++) {
        ctrl->timeout_tick[i] = g_ipsession_dft_timeout_steps[i];
    }

    VCLOCK_InitInstance(&ctrl->vclock, FALSE);

    OB_INIT(&ctrl->ob_list);

    return BS_OK;
}

void IPSession_Fini(IP_SESSION_CTRL_S *ctrl)
{
    Box_Fini(&ctrl->ip_tup_box, NULL, NULL);
}

void IPSession_SetTimeoutSteps(IP_SESSION_CTRL_S *ctrl, int state, UINT tick)
{
    ctrl->timeout_tick[state] = tick;
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
    if (ctrl->find_cache) {
        MEM_Free(ctrl->find_cache);
    }
    MEM_Free(ctrl);
}

int IPSession_CacheEnable(IP_SESSION_CTRL_S *ctrl)
{
    if (ctrl->find_cache) {
        return 0;
    }

    ctrl->find_cache = MEM_ZMalloc(sizeof(IP_SESSION_CACHE_S));
    if (! ctrl->find_cache) {
        RETURN(BS_NO_MEMORY);
    }

    return 0;
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
    OB_NOTIFY(&ctrl->ob_list, PF_IPSESSION_OB, session, IP_SESSION_EVENT_BEFORE_ADD);
}

static void ipsession_NotifyAfterAdd(IP_SESSION_CTRL_S *ctrl, IP_SESSION_S *session)
{
    OB_NOTIFY(&ctrl->ob_list, PF_IPSESSION_OB, session, IP_SESSION_EVENT_AFTER_ADD);
}

static void ipsession_NotifyBeforeDel(IP_SESSION_CTRL_S *ctrl, IP_SESSION_S *session)
{
    OB_NOTIFY(&ctrl->ob_list, PF_IPSESSION_OB, session, IP_SESSION_EVENT_BEFORE_DEL);
}

static void ipsession_NotifyAfterDel(IP_SESSION_CTRL_S *ctrl, IP_SESSION_S *session)
{
    OB_NOTIFY(&ctrl->ob_list, PF_IPSESSION_OB, session, IP_SESSION_EVENT_AFTER_DEL);
}

static void ipsession_timeout(HANDLE timer, USER_HANDLE_S *pstUserHandle)
{
    IP_SESSION_S *session;
    IP_SESSION_CTRL_S *ctrl = pstUserHandle->ahUserHandle[0];

    session = container_of(timer, IP_SESSION_S, vclock_timer);

    IPSession_Del(ctrl, &session->key);
}

static inline int ipsession_cacal_client_index(UCHAR family, UINT *client_ip, UINT *server_ip)
{
    int index = 0;

    if (family == AF_INET) {
        index = (MIN(client_ip[0], server_ip[0]) == client_ip[0]) ? 0 : 1;
    } else if (family == AF_INET6) {
        index = (IP6_ADDR_MIN(client_ip, server_ip) == client_ip) ? 0 : 1;
    } else {
        BS_DBGASSERT(0);
    }

    return index;
}

static inline int ipsession_KeyInit(IP_TUP_KEY_S *key, UCHAR family, UCHAR protocol,
        UINT *client_ip, UINT *server_ip, USHORT client_port, USHORT server_port, UINT tun_id)
{
    int client_index = ipsession_cacal_client_index(family, client_ip, server_ip);
    int server_index = client_index == 0 ? 1 : 0;

    key->family = family;
    key->protocol = protocol;
    if (family == AF_INET) {
        key->ip[client_index].ip4 = client_ip[0];
        key->ip[server_index].ip4 = server_ip[0];
    } else {
        IP6_ADDR_COPY(key->ip[client_index].ip6, client_ip);
        IP6_ADDR_COPY(key->ip[server_index].ip6, server_ip);
    }
    key->port[client_index] = client_port;
    key->port[server_index] = server_port;
    key->tun_id = tun_id;

    return client_index;
}


int IPSession_KeyInit(IP_TUP_KEY_S *key, UCHAR family, UCHAR protocol,
        UINT *client_ip, UINT *server_ip, USHORT client_port, USHORT server_port, UINT vnet_id)
{
    memset(key, 0, sizeof(IP_TUP_KEY_S));
    return ipsession_KeyInit(key, family, protocol,
            client_ip, server_ip, client_port, server_port, vnet_id);
}

void IPSession_SessInit(IP_SESSION_S *session, UCHAR family, UCHAR protocol,
        UINT *client_ip, UINT *server_ip, USHORT client_port, USHORT server_port, UINT vnet_id)
{
    memset(session, 0, sizeof(IP_SESSION_S));
    session->client_index = ipsession_KeyInit(&session->key, family, protocol,
            client_ip, server_ip, client_port, server_port, vnet_id);
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

    int state = session->state;
    VCLOCK_AddTimer(&ctrl->vclock, &session->vclock_timer,
            ctrl->timeout_tick[state], ctrl->timeout_tick[state],
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
    IP_SESSION_S *sess=NULL;
    USHORT index=0;

    if (ctrl->find_cache) {
        index = key->port[0] ^ key->port[1];
        sess = ctrl->find_cache->session[index];
        if (sess) {
            if (0 == memcmp(&sess->key, key, sizeof(IP_TUP_KEY_S))) {
                return sess;
            }
        }
    }

    sess = Box_FindData(&ctrl->ip_tup_box, key);
    if (ctrl->find_cache && sess) {
        ctrl->find_cache->session[index] = sess;
    }

    return sess;
}

IP_SESSION_S * IPSession_DelByIndex(IP_SESSION_CTRL_S *ctrl, int index)
{
    IP_SESSION_S *session;

    session = Box_GetData(&ctrl->ip_tup_box, index);
    if (! session) {
        return NULL;
    }

    ipsession_NotifyBeforeDel(ctrl, session);
    if (ctrl->find_cache) {
        USHORT index = session->key.port[0] ^ session->key.port[1];
        ctrl->find_cache->session[index] = NULL;
    }
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

void IPSession_SetState(IP_SESSION_CTRL_S *ctrl, IP_SESSION_S *session, int state)
{
    session->state = state;
    VCLOCK_RestartWithTick(&ctrl->vclock, &session->vclock_timer,
            ctrl->timeout_tick[state], ctrl->timeout_tick[state]);
}

