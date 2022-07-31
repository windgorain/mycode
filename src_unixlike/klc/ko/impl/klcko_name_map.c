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
	if (!f.file)
        return NULL;

	return f.file->private_data;
}

static void * _klcko_name_map_get(char *name)
{
    KLCTOOL_MAP_S *name_map;

    name_map = (void*)KUtlHash_Find(&g_klcko_namemap_hash, name);
    if (! name_map) {
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
    int ret;
    struct fd f;

    if (fd < 0) {
        return KO_ERR_BAD_PARAM;
    }

    f = fdget(fd);
	kfmap->map = _klcko_name_map_get_by_fd(f);
    if (! kfmap->map) {
        return KO_ERR_FAIL;
    }

    ret = _klcko_name_map_add(kfmap);

	fdput(f);

    return ret;
}

static int _klcko_name_map_del(char *name)
{
    void *map = _klcko_name_map_get(name);
    if (! map) {
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
    int ret = 0;
    void *data = msg->data;

    switch (cmd) {
        case KLC_NL_MAP_ADD:
            ret = _klcko_name_map_nl_add(data);
            break;
        case KLC_NL_MAP_DEL:
            ret = _klcko_name_map_nl_del(data);
            break;
        case KLC_NL_MAP_GET:
            ret = KUtlHash_NlGet(&g_klcko_namemap_hash, msg);
            break;
        default:
            return KO_ERR_BAD_PARAM;
            break;
    }

    return ret;
}

static void _klcko_name_map_knode_event(int event, void *knode)
{
    KLCTOOL_MAP_S *node = knode;

    if (event == KUTL_KNODE_EV_BEFORE_DEL) {
        KlcKoComp_MapDec(node->map);
    }
}

int KlcKoNameMap_BpfAdd(char *name, void *map)
{
    int ret;
    int len;
    KLCTOOL_MAP_S name_map = {0};

    if ((!name) || (name[0] == '\0')) {
        return KO_ERR_BAD_PARAM;
    }

    name_map.hdr.size = sizeof(KLCTOOL_MAP_S);
    name_map.fd = -1;
    name_map.map = map;

    len = strlcpy(name_map.hdr.name, name, sizeof(name_map.hdr.name));
    if (len >= sizeof(name_map.hdr.name)) {
        return KO_ERR_BAD_PARAM;
    }

    KlcKoCfg_Lock();
    ret = _klcko_name_map_add(&name_map);
    KlcKoCfg_Unlock();

    return ret;
}

void * KlcKoNameMap_BpfGet(char *name)
{
    KLCTOOL_MAP_S *name_map;

    name_map = (void*)KUtlHash_Find(&g_klcko_namemap_hash, name);
    if (! name_map) {
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

