/*================================================================
*   Description: domain group
*
================================================================*/
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
#include "utl/domain_group_utl.h"
#include "../h/acl_muc.h"
#include "../h/acl_app_func.h"

static inline int _acl_domain_group_set_instance_locked(ACL_MUC_S *muc, DOMAIN_GROUP_HANDLE handle)
{
    if (muc->domain_group_handle != NULL) {
        RETURN(BS_ALREADY_EXIST);
    }

    muc->domain_group_handle = handle;

    return 0;
}

static inline void _acl_domain_group_create_instance(ACL_MUC_S *muc)
{
    int ret;

    DOMAIN_GROUP_HANDLE handle = DomainGroup_Create(RcuEngine_GetMemcap());
    if (! handle){
        return;
    }

    SPLX_P();
    ret = _acl_domain_group_set_instance_locked(muc, handle);
    SPLX_V();

    if (ret != 0) {
        DomainGroup_Destroy(handle);
    }

    return;
}

static DOMAIN_GROUP_HANDLE _acl_domain_group_get_create_instance(int muc_id)
{
    ACL_MUC_S *muc;

    muc = AclMuc_Get(muc_id);
    if (NULL ==  muc) {
        return NULL;
    }

    if (NULL != muc->domain_group_handle) {
        return muc->domain_group_handle;
    }

    _acl_domain_group_create_instance(muc);

    return muc->domain_group_handle;
}

static DOMAIN_GROUP_HANDLE _acl_domain_group_get_create_instance_by_env(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    return _acl_domain_group_get_create_instance(muc_id);
}

static inline DOMAIN_GROUP_HANDLE _acl_domain_group_get_Instance(int muc_id)
{
    ACL_MUC_S *muc;

    muc = AclMuc_Get(muc_id);
    if (! muc) {
        return NULL;
    }
    return muc->domain_group_handle;
}

static DOMAIN_GROUP_HANDLE _acl_domain_group_get_instance_by_env(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    return _acl_domain_group_get_Instance(muc_id);
}

static BOOL_T _acl_domain_group_refered(DOMAIN_GROUP_HANDLE ctx)
{
    return ListRule_IsAnyListRefed(ctx);
}

static void _acl_domain_group_save_rule(LIST_RULE_LIST_S *list, RULE_NODE_S *rule, void *ud)
{
    DOMAIN_GROUP_NODE_S * node = container_of(rule, DOMAIN_GROUP_NODE_S, rule_node);
    CMD_EXP_OutputCmd(ud,"domain %s", node->szDomain);
}

static void _acl_domain_group_save_list(LIST_RULE_LIST_S *list, void *ud)
{
    if (CMD_EXP_OutputMode(ud,"domain-group %s", list->list_name) < 0) {
        return;
    }

    ListRule_WalkRule(list, _acl_domain_group_save_rule, ud);

    CMD_EXP_OutputModeQuit(ud);
}

void * AclDomainGroup_FindByName(IN INT muc_id, IN CHAR* pcName)
{
    return DomainGroup_FindListByName(_acl_domain_group_get_Instance(muc_id), pcName);
}

BS_STATUS AclDomainGroup_Save(IN HANDLE hFile) 
{
    INT muc_id = CmdExp_GetMucIDBySaveHandle(hFile);
    DOMAIN_GROUP_HANDLE domaingroup = _acl_domain_group_get_Instance(muc_id); 
    DOMAIN_GROUP_HANDLE pstCtx = domaingroup;

    if(! pstCtx) {
        return BS_OK;
    }

    ListRule_WalkList(pstCtx, _acl_domain_group_save_list, hFile);

    return BS_OK;
}

PLUG_API BS_STATUS AclDomainGroup_ShowName(int argc, char **argv, void *pEnv)
{
    DOMAIN_GROUP_HANDLE pstCtx = _acl_domain_group_get_create_instance_by_env(pEnv);
    LIST_RULE_LIST_S *list = NULL;
    LIST_RULE_HEAD_S *head = NULL;
    
    if (! pstCtx){
        RETURN(BS_NO_MEMORY);
    }

    while ((head = ListRule_GetNextList(pstCtx, head))) {
        list = head->pstListRule;
        EXEC_OutInfo("%s \r\n", list->list_name);
    }

	return BS_OK;
}


int AclDomainGroup_Init(void)
{
    return 0;
}

PLUG_API BS_STATUS AclDomainGroup_EnterView(int argc, char **argv, void *pEnv)
{
    DOMAIN_GROUP_HANDLE domaingroup = _acl_domain_group_get_create_instance_by_env(pEnv);
    
    if (NULL == domaingroup){
            RETURN(BS_NO_MEMORY);
    }

    if (! DomainGroup_FindListByName(domaingroup, argv[1])) {
        int ret = DomainGroup_AddList(domaingroup, argv[1]);
        if(ret < 0) {
            EXEC_OutInfo("%s.\r\n", ErrInfo_Get(BS_NO_MEMORY));
            return BS_NO_MEMORY;
        }
    }

    CmdExp_SetCurrentModeValue(pEnv, argv[1]);

    return 0;
}

PLUG_API BS_STATUS AclDomainGroup_Delete(int argc, char **argv, void *pEnv)
{
    CHAR *pcName = argv[2];
    DOMAIN_GROUP_HANDLE domaingroup = _acl_domain_group_get_create_instance_by_env(pEnv);
    
    if (NULL == domaingroup){
            RETURN(BS_NO_MEMORY);
    }

    DBGASSERT(NULL != pcName);

    int ret = DomainGroup_DelList(domaingroup, pcName);

    if (ret < 0) {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(ret));
    }

    return 0;
}

PLUG_API BS_STATUS AclDomainGroup_Clear(int argc, char **argv, void *pEnv)
{
    int muc_id = CmdExp_GetEnvMucID(pEnv);
    ACL_MUC_S *muc = AclMuc_Get(muc_id);
    if (NULL == muc){
        RETURN(BS_OK);
    }

    if (_acl_domain_group_refered(muc->domain_group_handle)){
        EXEC_OutString("domain group has been refered, can not be cleared \r\n");
        RETURN(BS_REF_NOT_ZERO);
    }

    DOMAIN_GROUP_HANDLE domaingroup = DomainGroup_Create(RcuEngine_GetMemcap());
    if (NULL == domaingroup){
        RETURN(BS_NO_MEMORY);
    }

    DomainGroup_Destroy(muc->domain_group_handle);
    muc->domain_group_handle = domaingroup;

    return 0;
}


PLUG_API BS_STATUS  AclDomainGroup_SetDomain(int argc, char **argv, void *pEnv)
{
    int ret;
    LIST_RULE_LIST_S *pstList = NULL;
    CHAR *pcName = NULL;
    DOMAIN_GROUP_HANDLE domaingroup = _acl_domain_group_get_create_instance_by_env(pEnv);
    
    if (NULL == domaingroup){
            RETURN(BS_NO_MEMORY);
    }

    pcName = CmdExp_GetCurrentModeValue(pEnv);
    DBGASSERT(NULL != pcName);
    pstList = DomainGroup_FindListByName(domaingroup,pcName);
    if (NULL == pstList){
        EXEC_OutString("Addresses is not exist.\r\n");
        RETURN(BS_NO_SUCH);
    }

    if ('n' != argv[0][0]){
        ret = DomainGroup_AddDomain(pstList, argv[1]);

    } else{
        ret = DomainGroup_DelDomain(pstList, argv[2]);
    }

    if (ret < 0) {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(ret));
    }

    return ret;
}

PLUG_API BS_STATUS AclDomainGroup_ShowReferedCount(int argc, char **argv, void *pEnv){
    LIST_RULE_LIST_S *pstList = NULL;
    CHAR* pcName = NULL;
    DOMAIN_GROUP_HANDLE domaingroup = _acl_domain_group_get_create_instance_by_env(pEnv);
    
    if (! domaingroup){
        RETURN(BS_NO_MEMORY);
    }

    pcName = CmdExp_GetCurrentModeValue(pEnv);
    DBGASSERT(NULL != pcName);

    pstList = DomainGroup_FindListByName(domaingroup,pcName);
    if (! pstList){
        EXEC_OutString("Addresses is not exist.\r\n");
        RETURN(BS_NO_SUCH);
    }
    EXEC_OutInfo("refered-count: %d \r\n", pstList->uiRefCount);
    
    return BS_OK;
}

PLUG_API BS_STATUS AclDomainGroup_LoadCfgFromFile(int argc, char **argv, void *pEnv)
{
    LIST_RULE_LIST_S *pstList = NULL;
    CHAR *pcFileName = argv[1];
    BS_STATUS ret = BS_OK;
    CHAR* pcName = NULL;
    DOMAIN_GROUP_HANDLE domaingroup = _acl_domain_group_get_create_instance_by_env(pEnv);
    
    if (NULL == domaingroup){
            RETURN(BS_NO_MEMORY);
    }
    
    pcName = CmdExp_GetCurrentModeValue(pEnv);
    DBGASSERT(NULL != pcName);

    pstList = DomainGroup_FindListByName(domaingroup,pcName);
    if (NULL == pstList){
        EXEC_OutString("Addresses file is not exist.\r\n");
        RETURN(BS_NO_SUCH);
    }
    ret = DomainGroup_LoadCfgFromFile(pstList, pcFileName);
    if (BS_OK != ret){
        EXEC_OutString("Load ip config from file error.\r\n");
    }

    return ret;
}

PLUG_API BS_STATUS AclDomainGroup_TestMatch(int argc, char **argv, void *pEnv)
{
    LIST_RULE_LIST_S *pstList = NULL;

    CHAR *pcName = CmdExp_GetCurrentModeValue(pEnv);
    DBGASSERT(NULL != pcName);

    pstList = DomainGroup_FindListByName(_acl_domain_group_get_instance_by_env(pEnv), pcName);
    if (NULL == pstList){
        EXEC_OutString("Domain group is not exist.\r\n");
        RETURN(BS_NO_SUCH);
    }
	
	if(NULL != DomainGroup_FindNode(pstList, argv[1])) {
		EXEC_OutString("Domain is in this group.\r\n");
	}else {
		EXEC_OutString("Domain is not in this group.\r\n");
        RETURN(BS_NOT_FOUND);
	}

    return 0;
}
