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

static inline ADDRESS_POOL_HANDLE _acl_ip_group_get_instance(int muc_id)
{
    ACL_MUC_S *muc;

    muc = AclMuc_Get(muc_id);
    if (! muc) {
        return NULL;
    }

    return muc->address_pool_handle;
}

static ADDRESS_POOL_HANDLE _acl_ip_group_get_instance_by_env(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    return _acl_ip_group_get_instance(muc_id);
}

static inline int _acl_ip_group_set_instance_locked(ACL_MUC_S *muc, ADDRESS_POOL_HANDLE handle)
{
    if (muc->address_pool_handle) {
        RETURN(BS_ALREADY_EXIST);
    }

    muc->address_pool_handle = handle;

    return 0;
}

static inline void _acl_ip_group_create_instance(ACL_MUC_S *muc)
{
    ADDRESS_POOL_HANDLE handle = AddressPool_Create(RcuEngine_GetMemcap());
    if (! handle){
        return;
    }

    SPLX_P();
    int ret = _acl_ip_group_set_instance_locked(muc, handle);
    SPLX_V();

    if (ret < 0) {
        AddressPool_Destroy(handle);
    }

    return;
}

static ADDRESS_POOL_HANDLE _acl_ip_group_get_create_instance(int muc_id)
{
    ACL_MUC_S *muc;

    muc = AclMuc_Get(muc_id);
    if (! muc) {
        return NULL;
    }

    if (muc->address_pool_handle) {
        return muc->address_pool_handle;
    }

    _acl_ip_group_create_instance(muc);

    return muc->address_pool_handle;
}

static ADDRESS_POOL_HANDLE _acl_address_get_create_address_pool_instance_by_env(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    return _acl_ip_group_get_create_instance(muc_id);
}

static LIST_RULE_LIST_S * _acl_ip_group_get_by_name(ADDRESS_POOL_HANDLE hanlde, char *group_name)
{
    LIST_RULE_LIST_S *pstList = NULL;

    pstList = AddressPool_FindListByName(hanlde, group_name);
    if (pstList) {
        return pstList;
    }

    int ret = AddressPool_AddIpList(hanlde, group_name);
    if (ret == 0) {
        pstList = AddressPool_FindListByName(hanlde, group_name);
    }

    return pstList;
}

static int _acl_address_enter_ip_group(ADDRESS_POOL_HANDLE hanlde, char *group_name, void *pEnv)
{
    LIST_RULE_LIST_S *pstList = NULL;
    
    pstList = _acl_ip_group_get_by_name(hanlde, group_name);
    if (! pstList) {
        RETURN(BS_NO_MEMORY);
    }

    CmdExp_SetCurrentModeValue(pEnv, group_name);

    return BS_OK;
}

static void _acl_ip_group_save_ip_rule(LIST_RULE_LIST_S *list, RULE_NODE_S *rule, void *hFile)
{
    ADDRESS_POOL_NODE_S * node = container_of(rule, ADDRESS_POOL_NODE_S, rule_node);
    CMD_EXP_OutputCmd(hFile,"ip %s/%d", Socket_IpToName(node->ip), node->prefix);
}

static void _acl_ip_group_save_ip_list(LIST_RULE_LIST_S *list, void *hFile)
{
    if (CMD_EXP_OutputMode(hFile, "ip-group %s", list->list_name) == 0) {
        ListRule_WalkRule(list, _acl_ip_group_save_ip_rule, hFile);
        CMD_EXP_OutputModeQuit(hFile);
    }
}

static void _acl_ip_group_save(IN HANDLE hFile, IN int muc_id)
{
    ADDRESS_POOL_HANDLE address_pool = _acl_ip_group_get_instance(muc_id);
    if (address_pool) {
        ListRule_WalkList(address_pool, _acl_ip_group_save_ip_list, hFile);
    }
}

static int _acl_ip_group_add_ip(LIST_RULE_LIST_S *pstList, char *pcIp)
{
    IP_PREFIX_S ip_prefix;

    int ret = IPString_ParseIpPrefix(pcIp, &ip_prefix);
    if (ret < 0) {
        return ret;
    }
 
    return AddressPool_AddIp(pstList, ip_prefix.uiIP, ip_prefix.ucPrefix);
}

static int _acl_ip_group_del_ip(IN LIST_RULE_LIST_S *pstList, IN CHAR *pcIp)
{
    IP_PREFIX_S ip_prefix;

    int ret = IPString_ParseIpPrefix(pcIp, &ip_prefix);
    if (ret < 0) {
        return ret;
    }

    return AddressPool_DelIp(pstList, ip_prefix.uiIP, ip_prefix.ucPrefix);
}

int AclIPGroup_Init()
{
    return 0;
}

int AclIPGroup_Save(HANDLE hFile) 
{
    int muc_id = CmdExp_GetMucIDBySaveHandle(hFile);

    _acl_ip_group_save(hFile, muc_id);

    return BS_OK;
}

void * AclIPGroup_FindByName(int muc_id, char *list_name)
{
    return AddressPool_FindListByName(_acl_ip_group_get_instance(muc_id), list_name);
}



PLUG_API int AclIPGroup_Show(int argc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    int muc_id = CmdExp_GetEnvMucID(pEnv);
    ADDRESS_POOL_HANDLE ctx = _acl_ip_group_get_instance(muc_id);
    LIST_RULE_LIST_S *list = NULL;
    LIST_RULE_HEAD_S *head = NULL;

    if (! ctx){
        EXEC_OutInfo("Can't get ip group instance \r\n");
        return BS_ERR;
    }

    while ((head = ListRule_GetNextList(ctx, head))) {
        list = head->pstListRule;
        EXEC_OutInfo("%s \r\n", list->list_name);
    }

	return BS_OK;
}

PLUG_API int AclIPGroup_EnterView(int argc, char **ppcArgv, void *pEnv)
{
    ADDRESS_POOL_HANDLE handle = _acl_address_get_create_address_pool_instance_by_env(pEnv);
    if (NULL == handle){
        RETURN(BS_NO_MEMORY);
    }

    int ret = _acl_address_enter_ip_group(handle, ppcArgv[1], pEnv);
    if (ret < 0) {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(ret));
    }

    return ret;
}

PLUG_API int AclIPGroup_Delete(int argc, char **argv, void *pEnv)
{
    char *name = argv[2];
    int muc_id = CmdExp_GetEnvMucID(pEnv);

    int ret = AddressPool_DelIpList(_acl_ip_group_get_instance(muc_id), name);

    if (ret < 0) {
        EXEC_OutInfo("%s. \r\n", ErrInfo_Get(ret));
    }

    return ret;
}

PLUG_API int AclIPGroup_Clear(int argc, char **argv, IN VOID *pEnv)
{
    int muc_id = CmdExp_GetEnvMucID(pEnv);
    ACL_MUC_S *muc = AclMuc_Get(muc_id);

    if (! muc){
        return 0;
    }

    
    if (ListRule_IsAnyListRefed(muc->address_pool_handle)) {
        EXEC_OutString("group has been refered, can not be cleared. \r\n");
        RETURN(BS_REF_NOT_ZERO);
    }

    ADDRESS_POOL_HANDLE addressHandle = AddressPool_Create(RcuEngine_GetMemcap());
    if (NULL == addressHandle){
        RETURN(BS_NO_MEMORY);
    }

    AddressPool_Destroy(muc->address_pool_handle);

    muc->address_pool_handle = addressHandle;

    RETURN(BS_OK);
}

PLUG_API int AclIPGroup_SetIp(int argc, char **argv, void *pEnv)
{
    char *pcIp = NULL;
    int ret;
    LIST_RULE_LIST_S *pstList = NULL;
    LIST_RULE_HANDLE hdl = _acl_ip_group_get_instance_by_env(pEnv);

    char *pcName = CmdExp_GetCurrentModeValue(pEnv);
    DBGASSERT(NULL != pcName);

    pstList = AddressPool_FindListByName(hdl, pcName);
    if (! pstList){
        EXEC_OutString("Addresses is not exist.\r\n");
        RETURN(BS_NO_SUCH);
    }

    if ('n' != argv[0][0]){
        pcIp = argv[1];
        ret = _acl_ip_group_add_ip(pstList, pcIp);
    } else{
        pcIp = argv[2];
        ret = _acl_ip_group_del_ip(pstList, pcIp);
    }

    if (ret < 0) {
        EXEC_OutInfo("%s. \r\n", ErrInfo_Get(ret));
    }

    return ret;
}

PLUG_API int AclIPGroup_ShowRefCount(int argc, char **argv, void *pEnv)
{
    LIST_RULE_LIST_S *pstList = NULL;
    LIST_RULE_HANDLE hdl = _acl_ip_group_get_instance_by_env(pEnv);

    CHAR *pcName = CmdExp_GetCurrentModeValue(pEnv);
    DBGASSERT(NULL != pcName);

    pstList = AddressPool_FindListByName(hdl, pcName);
    if (! pstList){
        EXEC_OutString("Addresses is not exist.\r\n");
        RETURN(BS_NO_SUCH);
    }

    EXEC_OutInfo("refered-count: %d \r\n", pstList->uiRefCount);
    
    return BS_OK;
}


PLUG_API int AclIPGroup_TestMatchIp(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    char *pcIp = NULL;
    UINT uiIp = 0;
    int ret;
    LIST_RULE_LIST_S *pstList = NULL;
    ADDRESS_POOL_LIST_S *list;
    UINT64 uiNextHop = 0;
    LIST_RULE_HANDLE hdl = _acl_ip_group_get_instance_by_env(pEnv);

    char *pcName = CmdExp_GetCurrentModeValue(pEnv);
    DBGASSERT(NULL != pcName);

    pcIp = ppcArgv[1];

    if (! Socket_IsIPv4(pcIp)) {
        EXEC_OutString("Ip address is wrong.\r\n");
        RETURN(BS_NOT_SUPPORT);
    }

    pstList = AddressPool_FindListByName(hdl,pcName);
    if (NULL == pstList){
        EXEC_OutString("Addresses is not exist.\r\n");
        RETURN(BS_NO_SUCH);
    }

    list = ListRule_GetUserHandle(pstList, 0);

    uiIp = Socket_Ipsz2IpHost(pcIp);
    ret = LPM_Lookup(&list->stLpm, uiIp, &uiNextHop);
    if (ret < 0){
        EXEC_OutString("Ip is in this ip pool. \r\n");
    } else{
        EXEC_OutString("Ip is not found in this ip pool.\r\n");
    }

    return ret;
}

PLUG_API int AclIPGroup_LoadFromFile(int argc, char **argv, void *pEnv)
{
    LIST_RULE_LIST_S *pstList = NULL;
    CHAR *pcFileName = argv[1];
    int ret;
    CHAR *pcName = CmdExp_GetCurrentModeValue(pEnv);
    LIST_RULE_HANDLE hdl = _acl_ip_group_get_instance_by_env(pEnv);

    DBGASSERT(NULL != pcName);

    pstList = AddressPool_FindListByName(hdl,pcName);
    if (! pstList){
        EXEC_OutString("Addresses file is not exist.\r\n");
        RETURN(BS_NO_SUCH);
    }

    ret = AddressPool_LoadCfgFromFile(pstList, pcFileName);
    if (ret < 0){
        EXEC_OutString("Load ip config from file error.\r\n");
    }

    return ret;
}


