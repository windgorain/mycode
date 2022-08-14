/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/bridge_utl.h"
#include "../h/bridge_core.h"

static inline int _bridge_set_instance_locked(BRIDGE_MUC_S *bridge_muc, MAP_HANDLE map)
{
    if (bridge_muc->bridge_ctrl) {
        RETURN(BS_ALREADY_EXIST);
    }
    bridge_muc->bridge_ctrl = map;
    return 0;
}

static inline void _bridge_create_instance(BRIDGE_MUC_S *bridge_muc)
{
    int ret;

    MAP_HANDLE map = MAP_HashCreate(NULL);
    if (! map) {
        return;
    }

    SPLX_P();
    ret = _bridge_set_instance_locked(bridge_muc, map);
    SPLX_V();

    if (ret != 0) {
        MAP_Destroy(map, NULL, NULL);
    }

    return;
}

static void * _bridge_get_create_instance(int muc_id)
{
    BRIDGE_MUC_S *bridge_muc;

    bridge_muc = BridgeMuc_Get(muc_id);
    if (! bridge_muc) {
        return NULL;
    }

    if (bridge_muc->bridge_ctrl) {
        return bridge_muc->bridge_ctrl;
    }

    _bridge_create_instance(bridge_muc);

    return bridge_muc->bridge_ctrl;
}

static inline void * _bridge_get_instance(int muc_id)
{
    BRIDGE_MUC_S *bridge_muc;

    bridge_muc = BridgeMuc_Get(muc_id);
    if (! bridge_muc) {
        return NULL;
    }

    return bridge_muc->bridge_ctrl;
}

static void * _bridge_get_create_instance_by_env(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    return _bridge_get_create_instance(muc_id);
}

static void * _bridge_get_instance_by_env(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    return _bridge_get_instance(muc_id);
}

/* bridge %STRING  */
PLUG_API int BridgeCmd_EnterBr(int argc, char **argv, void *env)
{
    int ret;
    void *bridge_ctrl = _bridge_get_create_instance_by_env(env);

    if (! bridge_ctrl) {
        EXEC_OutInfo("Can't create instance \r\n");
        RETURN(BS_ERR);
    }

    if (BridgeCore_IsExist(bridge_ctrl, argv[1])) {
        return 0;
    }

    ret = BridgeCore_CreateBr(bridge_ctrl, argv[1]);
    if (0 != ret) {
        EXEC_OutInfo("%s \r\n", ErrInfo_Get(ret));
        return ret;
    }

    return BS_OK;
}

/* no bridge %STRING  */
PLUG_API int BridgeCmd_NoBr(int argc, char **argv, void *env)
{
    int ret;
    void *bridge_ctrl = _bridge_get_instance_by_env(env);
    if (! bridge_ctrl) {
        return 0;
    }

    return BridgeCore_DelBr(bridge_ctrl, argv[2]);
}


