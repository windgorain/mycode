/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _BRIDGE_CORE_H
#define _BRIDGE_CORE_H
#include "utl/map_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

int BridgeCore_CreateBr(MAP_HANDLE map, char *name);
int BridgeCore_DelBr(MAP_HANDLE map, char *name);
BOOL_T BridgeCore_IsExist(MAP_HANDLE map, char *name);

#ifdef __cplusplus
}
#endif
#endif 
