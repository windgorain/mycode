/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"

#define KLCKO_NAME_MAP_HASH_BUCKETS 1024
#define KLCKO_NAME_MAP_HASH_MASK (KLCKO_NAME_MAP_HASH_BUCKETS - 1)

static KUTL_LIST_S g_klcko_namemap_buckets[KLCKO_NAME_MAP_HASH_BUCKETS] = {0};
static KUTL_HASH_S g_klcko_namemap_hash = {
    .bucket_num = KLCKO_NAME_MAP_HASH_BUCKETS,
    .mask = KLCKO_NAME_MAP_HASH_MASK,
    .buckets = g_klcko_namemap_buckets,
};

static void * _klcko_name_map_get_by_fd(struct fd f)
{
	if (unlikely(f.file == NULL)) {
        return NULL;
    }

	return f.file->private_data;
}

static void * _klcko_name_map_get(char *name)
{
    KLCTOOL_MAP_S *name_map;

    name_map = (void*)KUtlHash_Find(&g_klcko_namemap_hash, name);
    if (unlikely(name_map == NULL)) {
        return NULL;
    }

    return name_map->map;
}

static int _klcko_name_map_add(KLCTOOL_MAP_S *kfmap)
{
    int ret;

    if (KlcKoComp_MapInc(kfmap->map) < 0) {
        return KO_ERR_FAIL;
    }

    ret = KUtlHash_Add(&g_klcko_namemap_hash, &kfmap->hdr);
    if (ret != 0) {
        KlcKoComp_MapDec(kfmap->map);
        return ret;
    }

    return 0;
}

static int _klcko_name_map_nl_add(KLCTOOL_MAP_S *kfmap)
{
    int fd = kfmap->fd;
    struct fd f;
    int ret;

    if (unlikely(fd < 0)) {
        return KO_ERR_BAD_PARAM;
    }

    f = fdget(fd);

	kfmap->map = _klcko_name_map_get_by_fd(f);

    if (unlikely(kfmap->map == NULL)) {
        return KO_ERR_FAIL;
    }

    ret = _klcko_name_map_add(kfmap);

	fdput(f);

    return ret;
}

static int _klcko_name_map_del(char *name)
{
    void *map = _klcko_name_map_get(name);

    if (unlikely(map == NULL)) {
        return 0;
    }

    synchronize_rcu();

    KUtlHash_Del(&g_klcko_namemap_hash, name);

    return 0;
}

static int _klcko_name_map_nl_del(KLCTOOL_MAP_S *kfmap)
{
    return _klcko_name_map_del(kfmap->hdr.name);
}

static int _klcko_name_map_do(int cmd, NETLINK_MSG_S *msg)
{
    void *data = msg->data;

    switch (cmd) {
        case KLC_NL_MAP_ADD:
            return _klcko_name_map_nl_add(data);
        case KLC_NL_MAP_DEL:
            return _klcko_name_map_nl_del(data);
        case KLC_NL_MAP_GET:
            return KUtlHash_NlGet(&g_klcko_namemap_hash, msg);
        default:
            return KO_ERR_BAD_PARAM;
    }
}

static void _klcko_name_map_knode_event(int event, void *knode)
{
    KLCTOOL_MAP_S *node = knode;

    if (event == KUTL_KNODE_EV_BEFORE_DEL) {
        KlcKoComp_MapDec(node->map);
    }
}

void * KlcKoNameMap_Get(char *name)
{
    KLCTOOL_MAP_S *name_map;

    name_map = (void*)KUtlHash_Find(&g_klcko_namemap_hash, name);
    if (unlikely(name_map == NULL)) {
        return NULL;
    }

    return name_map->map;
}

void KlcKoNameMap_DelModule(char *module_prefix)
{
    KUtlHash_DelModule(&g_klcko_namemap_hash, module_prefix);
}

int KlcKoNameMap_NLMsg(int cmd, void *msg)
{
    return _klcko_name_map_do(cmd, msg);
}

int KlcKoNameMap_Init(void)
{
    KlcKoNl_Reg(KLC_NL_TYPE_MAP, KlcKoNameMap_NLMsg);
    return KUtlHash_Init(&g_klcko_namemap_hash, _klcko_name_map_knode_event);
}

void KlcKoNameMap_Fini(void)
{
    KlcKoNl_Reg(KLC_NL_TYPE_MAP, NULL);
    KUtlHash_DelAll(&g_klcko_namemap_hash);
}

