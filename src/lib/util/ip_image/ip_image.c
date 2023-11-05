/*================================================================
* Author：LiXingang. Data: 2019.07.25
* Description：基于IP的画像
*
================================================================*/
#include "bs.h"
#include "utl/box_utl.h"
#include "utl/txt_utl.h"
#include "utl/ip_image.h"
#include "utl/bloomfilter_utl.h"
#include "utl/dns_utl.h"
#include "utl/idkey_utl.h"
#include "utl/map_utl.h"

typedef struct {
    int max_property;
    IDKEY_HDL id_tbl;
    MAP_HANDLE ip_map;
}IPIMG_S;

static UINT64 ipimg_find_add_id(IPIMG_S *ipimg, void *value, int value_len)
{
    INT64 id;

    id = IDKEY_GetIDByKey(ipimg->id_tbl, value, value_len);
    if (id >= 0) {
        return id;
    }

    return IDKEY_Add(ipimg->id_tbl, value, value_len, NULL, IDKEY_NODE_FLAG_DUP_KEY);
}

static IPIMG_IP_ID_S * ipimg_add_ip(IPIMG_S *ipimg, UINT ip)
{
    IPIMG_IP_ID_S *node = MEM_ZMalloc(sizeof(IPIMG_IP_ID_S) + ipimg->max_property * sizeof(IPIMG_ID_S));
    if (! node) {
        return NULL;
    }

    node->ip = ip;

    if (0 != MAP_Add(ipimg->ip_map, &node->ip, sizeof(node->ip), node, 0)) {
        MEM_Free(node);
        return NULL;
    }

    return node;
}

static int ipimg_add_ip_id(IPIMG_S *ipimg, UINT ip, int property, UINT64 id)
{
    IPIMG_IP_ID_S *node;
    IPIMG_ID_S *ip_id;
    int i;

    node = MAP_Get(ipimg->ip_map, &ip, sizeof(ip));
    if (! node) {
        node = ipimg_add_ip(ipimg, ip);
        if (! node) {
            RETURN(BS_NO_MEMORY);
        }
    }

    ip_id = &node->propertys[property];

    for (i = 0; i < IPIMG_MAX_ID_PER_IP; i++){
        if (ip_id->node[i].id == id) {
            ip_id->node[i].count ++;
            return 0;
        }
    }

    if (ip_id->count >= IPIMG_MAX_ID_PER_IP) {
        RETURN(BS_FULL);
    }

    ip_id->node[ip_id->count].id = id;
    ip_id->node[ip_id->count].count = 1;
    ip_id->count ++;

    return 0;
}

static void ipimg_free_ip(void *data, void *ud)
{
    MEM_Free(data);
}


IPIMG_HANDLE IPIMG_Create(int max_property)
{
    IPIMG_S *ctrl = MEM_ZMalloc(sizeof(IPIMG_S));
    if (! ctrl) {
        return NULL;
    }

    ctrl->max_property = max_property;
    ctrl->id_tbl = IDKEY_WrapperIdmgrCreate(IDKEY_BaseCreate(0), IDMGR_BitmapCreate(1024*1024));
    ctrl->ip_map = MAP_HashCreate(0);

    if ((ctrl->id_tbl == NULL) || (ctrl->ip_map == NULL)) {
        IPIMG_Destroy(ctrl);
        return NULL;
    }

    return ctrl;
}

void IPIMG_Reset(IPIMG_HANDLE hIpImg)
{
    IPIMG_S *ipimg = hIpImg;

    IDKEY_Reset(ipimg->id_tbl, NULL, NULL);
    MAP_Reset(ipimg->ip_map, ipimg_free_ip, NULL);
}

void IPIMG_Destroy(IPIMG_HANDLE hIpImg)
{
    IPIMG_S *ipimg = hIpImg;

    if (ipimg->id_tbl) {
        IDKEY_Destroy(ipimg->id_tbl, NULL, NULL);
    }
    if (ipimg->ip_map) {
        MAP_Destroy(ipimg->ip_map, ipimg_free_ip, NULL);
    }

    MEM_Free(ipimg);
}


void IPIMG_SetIDCapacity(IPIMG_HANDLE hIpImg, UINT64 capacity)
{
    IPIMG_S *ipimg = hIpImg;
    IDKEY_SetCapacity(ipimg->id_tbl, capacity);
}


void IPIMG_SetIPCapacity(IPIMG_HANDLE hIpImg, UINT capacity)
{
    IPIMG_S *ipimg = hIpImg;
    MAP_SetCapacity(ipimg->ip_map, capacity);
}

int IPIMG_Add(IPIMG_HANDLE ipimg_handle, UINT ip, int property, void *value, int value_len)
{
    IPIMG_S *ipimg = ipimg_handle;
    INT64 id;

    if (! value) {
        BS_DBGASSERT(0);
        RETURN(BS_NULL_PARA);
    }

    if (property >= ipimg->max_property) {
        RETURN(BS_BAD_PARA);
    }

    id = ipimg_find_add_id(ipimg, value, value_len);
    if (id < 0) {
        RETURN(BS_NO_MEMORY);
    }

    return ipimg_add_ip_id(ipimg, ip, property, id);
}

IDKEY_KL_S * IPIMG_GetKey(IPIMG_HANDLE ipimg_handle, UINT64 id)
{
    IPIMG_S *ipimg = ipimg_handle;

    return IDKEY_GetKeyByID(ipimg->id_tbl, id);
}

void IPIMG_Walk(IPIMG_HANDLE ipimg_handle, PF_IPIMG_WALK_IP walk_func, void *ud)
{
    IPIMG_S *ipimg = ipimg_handle;
    MAP_ELE_S *ele = NULL;
    IPIMG_IP_ID_S *node;

    while(NULL != (ele = MAP_GetNextEle(ipimg->ip_map, ele))) {
        node = ele->pData;
        walk_func(node, ud);
    }
}

