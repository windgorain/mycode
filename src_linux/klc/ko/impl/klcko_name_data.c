/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"

#define KLCKO_NAME_DATA_HASH_BUCKETS 1024
#define KLCKO_NAME_DATA_HASH_MASK (KLCKO_NAME_DATA_HASH_BUCKETS - 1)

static KUTL_LIST_S g_klcko_nameobject_buckets[KLCKO_NAME_DATA_HASH_BUCKETS] = {0};
static KUTL_HASH_S g_klcko_nameobject_hash = {
    .bucket_num = KLCKO_NAME_DATA_HASH_BUCKETS,
    .mask = KLCKO_NAME_DATA_HASH_MASK,
    .buckets = g_klcko_nameobject_buckets};

/* 申请一个 */
void * KlcKoNameData_Alloc(char *name, unsigned int size)
{
    KUTL_KNODE_S *knode = NULL;
    int ret;

    if (unlikely((name == NULL) || (name[0] == '\0'))) {
        return NULL;
    }

    ret = KUtlHash_Alloc(&g_klcko_nameobject_hash, name, sizeof(KUTL_KNODE_S) + size, (void**)&knode);
    if (unlikely(ret < 0)) {
        return NULL;
    }

    return (void*)(knode + 1);
}

/* 如果存在则直接获取,否则申请一个 */
void * KlcKoNameData_FindOrAlloc(char *name, unsigned int size)
{
    KUTL_KNODE_S *knode = NULL;

    if (unlikely((name == NULL) || (name[0] == '\0'))) {
        return NULL;
    }

    KUtlHash_Alloc(&g_klcko_nameobject_hash, name, sizeof(KUTL_KNODE_S) + size, (void**)&knode);
    if (unlikely(knode == NULL)) {
        return NULL;
    }

    return (void*)(knode + 1);
}

int KlcKoNameData_FreeByName(char *name)
{
    KUtlHash_Del(&g_klcko_nameobject_hash, name);
    return 0;
}

int KlcKoNameData_Free(void *node)
{
    KUTL_KNODE_S *knode;

    if (unlikely(node == NULL)) {
        return 0;
    }

    knode = KlcKoNameData_GetKNodeByNode(node);

    KUtlHash_Del(&g_klcko_nameobject_hash, knode->name);

    return 0;
}

void * KlcKoNameData_Find(char *name)
{
    KUTL_KNODE_S *knode = KUtlHash_Find(&g_klcko_nameobject_hash, name);

    if (unlikely(knode == NULL)) {
        return NULL;
    }

    return (knode + 1);
}

KUTL_KNODE_S * KlcKoNameData_FindKNode(char *name)
{
    return KUtlHash_Find(&g_klcko_nameobject_hash, name);
}

KUTL_KNODE_S * KlcKoNameData_GetKNodeByNode(void *node)
{
    if (unlikely(node == NULL)) {
        return NULL;
    }
    return (void*)((char*)node - sizeof(KUTL_KNODE_S));
}

int KlcKoNameData_Init(void)
{
    return KUtlHash_Init(&g_klcko_nameobject_hash, NULL);
}

void KlcKoNameData_Fini(void)
{
    KUtlHash_DelAll(&g_klcko_nameobject_hash);
}

