/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/bridge_utl.h"
#include "utl/map_utl.h"
#include "../h/bridge_muc.h"

#define BRIDGE_NAME_SIZE 32

typedef struct {
    char br_name[BRIDGE_NAME_SIZE];
}BRIDGE_SERVICE_S;

int BridgeCore_CreateBr(MAP_HANDLE map, char *name)
{
    BRIDGE_SERVICE_S *svr;

    if (MAP_Get(map, name, strlen(name))) {
        RETURN(BS_ALREADY_EXIST);
    }

    svr = MEM_ZMalloc(sizeof(BRIDGE_SERVICE_S));
    if (! svr) {
        RETURN(BS_NO_MEMORY);
    }

    strlcpy(svr->br_name, name, BRIDGE_NAME_SIZE);

    int ret = MAP_Add(map, svr->br_name, strlen(name), svr, 0);
    if (0 != ret) {
        MEM_Free(svr);
        return ret;
    }

    return 0;
}

int BridgeCore_DelBr(MAP_HANDLE map, char *name)
{
    BRIDGE_SERVICE_S svr = MAP_Del(map, name, strlen(name));
    if (svr) {
        MEM_Free(svr);
    }

    return 0;
}

BOOL_T BridgeCore_IsExist(MAP_HANDLE map, char *name)
{
    if (MAP_Get(map, name, strlen(name))) {
        return TRUE;
    }
    return FALSE;
}


