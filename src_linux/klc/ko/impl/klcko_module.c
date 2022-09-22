/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"

#define KLCKO_MODULE_HASH_BUCKETS 1024
#define KLCKO_MODULE_HASH_MASK (KLCKO_MODULE_HASH_BUCKETS - 1)

static KUTL_LIST_S g_klcko_module_buckets[KLCKO_MODULE_HASH_BUCKETS] = {0};
static KUTL_HASH_S g_klcko_module_hash = {
    .bucket_num = KLCKO_MODULE_HASH_BUCKETS,
    .mask = KLCKO_MODULE_HASH_MASK,
    .buckets = g_klcko_module_buckets};

/* 根据module name中获取module指针 */
static void * klcko_module_get(char *module_name)
{
    return KUtlHash_Find(&g_klcko_module_hash, module_name);
}

static int klcko_module_add(KLC_MODULE_S *mod)
{
    return KUtlHash_Add(&g_klcko_module_hash, &mod->hdr);
}

static int klcko_module_del(KLC_MODULE_S *mod)
{
    char module_prefix[KUTL_KNODE_NAME_SIZE];

    if (snprintf(module_prefix, sizeof(module_prefix), "%s.", mod->hdr.name) < 0) {
        return KO_ERR_BAD_PARAM;
    }

    KlcKoEvent_DelModule(module_prefix);
    KlcKoNameMap_DelModule(module_prefix);
    KlcKoNameFunc_DelModule(module_prefix);
    KlcKoIDFunc_DelModule(module_prefix);

    KUtlHash_Del(&g_klcko_module_hash, mod->hdr.name);

    return 0;
}

static int klcko_module_do(int cmd, NETLINK_MSG_S *msg)
{
    int ret = 0;
    void *data = msg->data;

    switch (cmd) {
        case KLC_NL_MODULE_ADD:
            ret = klcko_module_add(data);
            break;
        case KLC_NL_MODULE_DEL:
            ret = klcko_module_del(data);
            break;
        case KLC_NL_MODULE_GET:
            ret = KUtlHash_NlGet(&g_klcko_module_hash, msg);
            break;
        default:
            return KO_ERR_BAD_PARAM;
            break;
    }

    return ret;
}

int KlcKoModule_NLMsg(int cmd, void *msg)
{
    return klcko_module_do(cmd, msg);
}

void * KlcKoModule_GetModuleData(char *module_name, int data_size)
{
    KLC_MODULE_S *mod = klcko_module_get(module_name);
    if ((! mod) || (mod->data_size < data_size)) {
        return NULL;
    }
    return mod->data;
}

int KlcKoModule_Init(void)
{
    KlcKoNl_Reg(KLC_NL_TYPE_MODULE, KlcKoModule_NLMsg);
    return KUtlHash_Init(&g_klcko_module_hash, NULL);
}

void KlcKoModule_Fini(void)
{
    KlcKoNl_Reg(KLC_NL_TYPE_MODULE, NULL);
    KUtlHash_DelAll(&g_klcko_module_hash);
}

