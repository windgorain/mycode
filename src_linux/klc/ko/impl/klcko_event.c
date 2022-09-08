/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"

static KUTL_LIST_S g_klcko_events[KLC_EVENT_ID_MAX] = {0};
static KUTL_ARRAYLIST_S g_klcko_arraylist = {.size=KLC_EVENT_ID_MAX, .lists = g_klcko_events};


static int klcko_evob_nlget(NETLINK_MSG_S *msg)
{
    return KUtlArrayList_NlGet(&g_klcko_arraylist, msg);
}

static int _klcko_evob_get(INOUT KLC_EVENT_OB_S *ob)
{
    KLC_EVENT_OB_S * found;

    if (unlikely(NULL == ob)) {
        KO_PrintString("Param Null");
        return KO_ERR_BAD_PARAM;
    }

    rcu_read_lock();

    found = KUtlArrayList_Get(&g_klcko_arraylist, ob->event, ob->hdr.name);
    if (NULL != found) {
        memcpy(ob, found, sizeof(KLC_EVENT_OB_S));
    }

    rcu_read_unlock();

    return found ? 0 : KO_ERR_NO_SUCH;
}

static int _klcko_evob_del(KLC_EVENT_OB_S *ob)
{
    if (unlikely(ob == NULL))  {
        KO_PrintString("Param Null");
        return KO_ERR_BAD_PARAM;
    }

    return KUtlArrayList_Del(&g_klcko_arraylist, ob->event, ob->hdr.name);
}

static void _klcko_evob_del_all(void)
{
    KUtlArrayList_DelAll(&g_klcko_arraylist);
}

static int _klcko_evob_add(KLC_EVENT_OB_S *ob)
{
    if (unlikely((ob == NULL) || (ob->func[0] == '\0'))) {
        return KO_ERR_BAD_PARAM;
    }

    return KUtlArrayList_Add(&g_klcko_arraylist, ob->event, (void*)ob);
}

static int _klcko_evob_nl_do(int cmd, NETLINK_MSG_S *msg)
{
    int ret;
    void *data = msg->data;

    switch (cmd) {
        case KLC_NL_EVOB_ADD:
            ret = _klcko_evob_add(data);
            break;
        case KLC_NL_EVOB_DEL:
            ret = _klcko_evob_del(data);
            break;
        case KLC_NL_EVOB_GET:
            ret = klcko_evob_nlget(msg);
            break;
        default:
            ret = KO_ERR_BAD_PARAM;
            break;
    }

    return ret;
}

static int _klcko_evob_ctl(int cmd, void *data, unsigned int data_len)
{
    int ret;

    switch (cmd) {
        case KLC_EVENT_CMD_GET_OB:
            ret = _klcko_evob_get(data);
            break;
        case KLC_EVENT_CMD_NF_START:
            ret = KlcKoNf_Start((long)data);
            break;
        case KLC_EVENT_CMD_NF_STOP:
            ret = KlcKoNf_Stop((long)data);
            break;
        default:
            ret = KO_ERR_FAIL;
            break;
    }

    return ret;
}

static void _klcko_event_publish(KLC_EVENT_S *ev, u64 p1, u64 p2)
{
    KLC_EVENT_CTX_S ctx = {0};
    KLC_EVENT_OB_S *ob = NULL;

    ctx.event = ev;

    rcu_read_lock();

    while ((ob = (void*)KUtlList_GetNext(&g_klcko_events[ev->event], (void*)ob))) {
        if (ev->sub_event == ob->sub_event) {
            ctx.evob = ob;
            if (KLC_EVOB_RET_BREAK == KlcKo_NameLoadRun(ob->func, (long)&ctx, p1, p2)) {
                break;
            }
        }
    }

    rcu_read_unlock();
}

/* 发布系统事件 */
static int _klcko_event_system_publish(KLC_EVENT_S *ev, u64 p1, u64 p2)
{
    if (unlikely((ev == NULL) || (ev->event >= KLC_EVENT_USER_BASE))) {
        KO_PrintString("System event error");
        return -1;
    }

    _klcko_event_publish(ev, p1, p2);

    return 0;
}

static int _klcko_event_nlmsg(int cmd, void *msg)
{
    return _klcko_evob_nl_do(cmd, msg);
}

void KlcKoEvent_DelModule(char *module_prefix)
{
    KUtlArrayList_DelModule(&g_klcko_arraylist, module_prefix);
}

/* 用户事件发布. 系统事件不是这个函数 */
u64 KlcKoEvent_Publish(KLC_EVENT_S *ev, u64 p1, u64 p2)
{
    if (unlikely((ev == NULL) 
            || (ev->event < KLC_EVENT_USER_BASE)
            || (ev->event >= KLC_EVENT_ID_MAX))) {
        KO_PrintString("User event error");
        return KLC_RET_ERR;
    }

    _klcko_event_publish(ev, p1, p2);

    return 0;
}

u64 KlcKoEvent_Ctl(int cmd, void *data, unsigned int data_len)
{
    return _klcko_evob_ctl(cmd, data, data_len);
}

int KlcKoEvent_Init(void)
{
    KUtlArrayList_Init(&g_klcko_arraylist, NULL);
    KlcKo_SetSystemEventPublish(_klcko_event_system_publish);
    KlcKoNl_Reg(KLC_NL_TYPE_EVENT, _klcko_event_nlmsg);

    return 0;
}

void KlcKoEvent_Fini(void)
{
    KlcKo_SetSystemEventPublish(NULL);
    KlcKoNl_Reg(KLC_NL_TYPE_EVENT, NULL);
    _klcko_evob_del_all();
}

