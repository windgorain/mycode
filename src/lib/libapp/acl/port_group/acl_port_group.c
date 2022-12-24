/*****************************************************************************
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
*****************************************************************************/
#include "bs.h"
#include "utl/mutex_utl.h"
#include "utl/atomic_utl.h"
#include "utl/socket_utl.h"
#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/exec_utl.h"
#include "utl/bit_opt.h"
#include "comp/comp_acl.h"
#include "bs/rcu_engine.h"
#include "utl/ip_utl.h"
#include "utl/mem_cap.h"
#include "utl/ip_string.h"
#include "utl/file_utl.h"
#include "utl/args_utl.h"
#include "utl/address_pool_utl.h"
#include "utl/port_pool_utl.h"
#include "../h/acl_muc.h"
#include "../h/acl_app_func.h"
#include "../h/acl_port_group.h"


static inline PORT_POOL_HANDLE _acl_address_get_port_pool_instance(int muc_id)
{
    ACL_MUC_S *muc;

    muc = AclMuc_Get(muc_id);
    if (! muc) {
        return NULL;
    }
    return muc->port_pool_handle;
}

static PORT_POOL_HANDLE _acl_address_get_port_pool_instance_by_env(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    return _acl_address_get_port_pool_instance(muc_id);
}

static inline int _acl_address_set_port_instance_locked(ACL_MUC_S *muc, PORT_POOL_HANDLE handle)
{
    if (muc->port_pool_handle != NULL) {
        RETURN(BS_ALREADY_EXIST);
    }

    muc->port_pool_handle = handle;

    return 0;
}

static inline void _acl_address_create_port_pool_instance(ACL_MUC_S *muc)
{
    PORT_POOL_HANDLE handle = PortPool_Create(RcuEngine_GetMemcap());
    if (! handle){
        return;
    }

    SPLX_P();
    int ret = _acl_address_set_port_instance_locked(muc, handle);
    SPLX_V();

    if (ret != 0) {
        PortPool_Destroy(handle);
    }

    return;
}

static PORT_POOL_HANDLE _acl_address_get_create_port_pool_instance(int muc_id)
{
    ACL_MUC_S *muc;

    muc = AclMuc_Get(muc_id);
    if (NULL ==  muc) {
        return NULL;
    }

    if (NULL != muc->port_pool_handle) {
        return muc->port_pool_handle;
    }

    _acl_address_create_port_pool_instance(muc);

    return muc->port_pool_handle;
}

static PORT_POOL_HANDLE _acl_address_get_create_port_pool_instance_by_env(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    return _acl_address_get_create_port_pool_instance(muc_id);
}

static LIST_RULE_LIST_S * _acl_address_get_port_pool(IN PORT_POOL_HANDLE hanlde, IN CHAR *pcGroupName)
{
    LIST_RULE_LIST_S *pstList = NULL;

    pstList = PortPool_FindListByName(hanlde, pcGroupName);
    if (pstList) {
        return pstList;
    }
    
    int ret = PortPool_AddList(hanlde, pcGroupName);
    if (ret < 0) {
        pstList = PortPool_FindListByName(hanlde, pcGroupName);
    }

    return pstList;
}

static int _acl_address_enter_port_pool_view(IN PORT_POOL_HANDLE hanlde, IN CHAR *pcGroupName, IN VOID *pEnv)
{
    LIST_RULE_LIST_S *pstList = NULL;
    
    pstList = _acl_address_get_port_pool(hanlde, pcGroupName);
    if (! pstList) {
        RETURN(BS_NO_MEMORY);
    }

    CmdExp_SetCurrentModeValue(pEnv, pcGroupName);

    return BS_OK;
}

static void _acl_port_group_save_port_rule(LIST_RULE_LIST_S *list, RULE_NODE_S *rule, void *hFile)
{
    PORT_POOL_NODE_S * node = container_of(rule, PORT_POOL_NODE_S, rule_node);
    CMD_EXP_OutputCmd(hFile, "port %u-%u", node->start, node->end);
}

static void _acl_port_group_save_port_list(LIST_RULE_LIST_S *list, void *hFile)
{
    if (CMD_EXP_OutputMode(hFile,"port-group %s", list->list_name) == 0) {
        ListRule_WalkRule(list, _acl_port_group_save_port_rule, hFile);
        CMD_EXP_OutputModeQuit(hFile);
    }
}

static void _acl_port_group_save(HANDLE hFile, int muc_id)
{
    PORT_POOL_HANDLE ctx = _acl_address_get_port_pool_instance(muc_id);
    if (ctx) {
        ListRule_WalkList(ctx, _acl_port_group_save_port_list, hFile);
    }
}

static int _acl_port_group_add_port_range(IN LIST_RULE_LIST_S *pstList, IN CHAR *pcPortString)
{
    UINT uiStartPort = 0;
    UINT uiEndPort = 0;

    int ret = PortPool_ParsePortRange(pcPortString,&uiStartPort,&uiEndPort);
    if (ret < 0) {
        return ret;
    }

    return PortPool_AddPortRange(pstList, uiStartPort, uiEndPort);
}

static int _acl_port_group_del_port_range(IN LIST_RULE_LIST_S *pstList, IN CHAR *pcPortString)
{
    UINT uiStartPort = 0;
    UINT uiEndPort = 0;

    int ret = PortPool_ParsePortRange(pcPortString,&uiStartPort,&uiEndPort);
    if (ret < 0) {
        return ret;
    }

    return PortPool_DeletePortRange(pstList, uiStartPort, uiEndPort);
}

int AclPortGroup_Init(void)
{
    return 0;
}

int AclPortGroup_Save(HANDLE hFile) 
{
    int muc_id = CmdExp_GetMucIDBySaveHandle(hFile);
    _acl_port_group_save(hFile, muc_id);
    return BS_OK;
}

void * AclPortGroup_FindByName(int muc_id, char *list_name)
{
    return PortPool_FindListByName(_acl_address_get_port_pool_instance(muc_id), list_name);
}

/***** cmds *****/

PLUG_API int AclPortGroup_Show(int argc, char **argv, void *pEnv)
{
    int muc_id = CmdExp_GetEnvMucID(pEnv);
    PORT_POOL_HANDLE ctx = _acl_address_get_port_pool_instance(muc_id);
    LIST_RULE_LIST_S *list = NULL;
    LIST_RULE_HEAD_S *head = NULL;

    if (! ctx){
        EXEC_OutInfo("Can't get port group instance \r\n");
        return BS_ERR;
    }

    while ((head = ListRule_GetNextList(ctx, head))) {
        list = head->pstListRule;
        EXEC_OutInfo("%s \r\n", list->list_name);
    }

	return BS_OK;
}

PLUG_API int AclPortGroup_EnterView(int argc, char **argv, void *pEnv)
{
    PORT_POOL_HANDLE handle = _acl_address_get_create_port_pool_instance_by_env(pEnv);
    if (! handle){
        RETURN(BS_NO_MEMORY);
    }

    int ret = _acl_address_enter_port_pool_view(handle, argv[1], pEnv);
    if (ret < 0) {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(ret));
    }

    return ret;
}

PLUG_API int AclPortGroup_Delete(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcName = ppcArgv[2];
    int muc_id = CmdExp_GetEnvMucID(pEnv);

    DBGASSERT(NULL != pcName);
    
    int ret = PortPool_DelList(_acl_address_get_port_pool_instance(muc_id), pcName);

    if (ret < 0) {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(ret));
    }

    return ret;
}

PLUG_API int AclPortGroup_Clear(int argc, char **argv, void *pEnv)
{
    int muc_id = CmdExp_GetEnvMucID(pEnv);
    ACL_MUC_S *muc = AclMuc_Get(muc_id);
    if (NULL == muc){
        RETURN(BS_OK);
    }

    // 如果有被引用，则不能被清除
    if (ListRule_IsAnyListRefed(muc->port_pool_handle)){
        EXEC_OutString("port pool has been refered, can not be cleared.\r\n");
        RETURN(BS_REF_NOT_ZERO);
    }

    PORT_POOL_HANDLE portHandle = PortPool_Create(RcuEngine_GetMemcap());
    if (! portHandle){
        RETURN(BS_NO_MEMORY);
    }

    PortPool_Destroy(muc->port_pool_handle);
    muc->port_pool_handle = portHandle;

    return 0;
}

PLUG_API int AclPortGroup_SetPortRange(int argc, char **argv, void *pEnv)
{
    int ret;
    LIST_RULE_LIST_S *pstList = NULL;
    CHAR *pcPortString;

    CHAR *pcName = CmdExp_GetCurrentModeValue(pEnv);

    DBGASSERT(NULL != pcName);

    pstList = PortPool_FindListByName(_acl_address_get_port_pool_instance_by_env(pEnv),pcName);
    if (! pstList){
        EXEC_OutString("Addresses is not exist.\r\n");
        RETURN(BS_NO_SUCH);
    }

    if ('n' != argv[0][0]) {
        pcPortString = argv[1];
        ret = _acl_port_group_add_port_range(pstList, pcPortString);
    } else{
        pcPortString = argv[2];
        ret = _acl_port_group_del_port_range(pstList, pcPortString);
    }

    if (ret < 0) {
        EXEC_OutInfo("%s. \r\n", ErrInfo_Get(ret));
    }

    return ret;
}

PLUG_API int AclPortGroup_TestMatchPort(int argc, char **argv, void *pEnv)
{
    LIST_RULE_LIST_S *pstList = NULL;
    PORT_POOL_LIST_S *port_list;
    UINT uiPort = 0;

    CHAR *pcName = CmdExp_GetCurrentModeValue(pEnv);
    DBGASSERT(NULL != pcName);

    uiPort = TXT_Str2Ui(argv[1]);

    pstList = PortPool_FindListByName(_acl_address_get_port_pool_instance_by_env(pEnv), pcName);
    if (! pstList){
        EXEC_OutString("Port group is not exist.\r\n");
        RETURN(BS_NO_SUCH);
    }

    port_list = ListRule_GetUserHandle(pstList, 0);

    if (port_list->ports[uiPort] > 0){
        EXEC_OutString("Port is in this group. \r\n");
    } else{
        EXEC_OutString("Port is not in this group. \r\n");
    }

    return 0;
}

PLUG_API int AclPortGroup_ShowRefCount(int argc, char **argv, void *pEnv)
{
    LIST_RULE_LIST_S *pstList;
    LIST_RULE_HANDLE hdl = _acl_address_get_port_pool_instance_by_env(pEnv);

    CHAR *pcName = CmdExp_GetCurrentModeValue(pEnv);
    DBGASSERT(NULL != pcName);

    pstList = ListRule_GetListByName(hdl, pcName);
    if (! pstList){
        EXEC_OutString("Addresses is not exist.\r\n");
        RETURN(BS_NO_SUCH);
    }

    EXEC_OutInfo("refered-count: %d \r\n", pstList->uiRefCount);
    
    return BS_OK;
}

PLUG_API int AclAddresses_LoadPortCfgFromFile(int argc, char **argv, void *pEnv)
{
    LIST_RULE_LIST_S *pstList = NULL;
    CHAR *pcFileName = argv[1];
    CHAR *pcName = CmdExp_GetCurrentModeValue(pEnv);
    DBGASSERT(NULL != pcName);

    pstList = PortPool_FindListByName(_acl_address_get_port_pool_instance_by_env(pEnv), pcName);
    if (! pstList){
        EXEC_OutString("Port file is not exist.\r\n");
        RETURN(BS_NO_SUCH);
    }

    int ret = PortPool_LoadCfgFromFile(pstList, pcFileName);
    if (ret < 0) {
        EXEC_OutString("Load port config from file error.\r\n");
    }

    return ret;
}

