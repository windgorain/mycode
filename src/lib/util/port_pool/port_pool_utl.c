/*================================================================
*   Description: port pool
*
================================================================*/
#include "bs.h"
#include "bs/rcu_engine.h"
#include "utl/mutex_utl.h"
#include "utl/atomic_utl.h"
#include "utl/socket_utl.h"
#include "utl/num_list.h"
#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/ip_utl.h"
#include "utl/mem_cap.h"
#include "utl/ip_string.h"
#include "utl/file_utl.h"
#include "utl/port_pool_utl.h"
#include "utl/args_utl.h"

static void _port_pool_free_List(IN void *pstRcuNode)
{
    PORT_POOL_LIST_S *pstList = container_of(pstRcuNode, PORT_POOL_LIST_S, rcu_node);
    MEM_Free(pstList);
    return;
}

static int _port_pool_del_list(IN PORT_POOL_HANDLE pool, LIST_RULE_LIST_S *list)
{
    PORT_POOL_LIST_S *port_list;

    if (list->uiRefCount > 0) {
        RETURN(BS_REF_NOT_ZERO);
    }

    port_list = list->user_handle[0];

    ListRule_DelList(pool, list, NULL, NULL);

    void *memcap = ListRule_GetMemcap(pool);

    MemCap_Call(memcap, &port_list->rcu_node, _port_pool_free_List);

    return 0;
}

PORT_POOL_HANDLE PortPool_Create(void *memcap)
{
    return ListRule_Create(memcap);
}

void PortPool_Destroy(PORT_POOL_HANDLE pool)
{
    LIST_RULE_HEAD_S *head;

    while ((head = ListRule_GetNextList(pool, NULL))) {
        int ret = _port_pool_del_list(pool, head->pstListRule);
        if (ret < 0) {
            BS_DBGASSERT(ret == 0);
        }
    }

    ListRule_Destroy(pool, NULL, NULL);
}

LIST_RULE_LIST_S * PortPool_FindListByName(IN PORT_POOL_HANDLE pool, IN CHAR *pcListName)
{
    return ListRule_GetListByName(pool, pcListName);
}

int PortPool_AddList(PORT_POOL_HANDLE pool, IN char *list_name)
{
    LIST_RULE_LIST_S *list;
    PORT_POOL_LIST_S *node;
    UINT list_id;

    node = MEM_ZMalloc(sizeof(PORT_POOL_LIST_S));
    if (! node) {
        RETURN(BS_NO_MEMORY);
    }

    list_id = ListRule_AddList(pool, list_name, sizeof(void *));
    if (list_id == NAP_INVALID_ID) {
        MEM_Free(node);
        RETURN(BS_ERR);
    }

    list = ListRule_GetListByID(pool, list_id);
    BS_DBGASSERT(list);
    list->user_handle[0] = node;

    return BS_OK;
}

int PortPool_DelList(IN PORT_POOL_HANDLE pool, IN char *pcName)
{
    LIST_RULE_LIST_S *list = ListRule_GetListByName(pool, pcName);
    if (! list) {
        return 0;
    }
    return _port_pool_del_list(pool, list);
}

PORT_POOL_NODE_S * PortPool_FindPortNode(IN LIST_RULE_LIST_S *pstList, USHORT start, USHORT end)
{
    RULE_NODE_S *rule_node = NULL;
    PORT_POOL_NODE_S *node;

    while ((rule_node = RuleList_GetNextByNode(&pstList->stRuleList, rule_node))) {
        node = container_of(rule_node, PORT_POOL_NODE_S, rule_node);
        if ((node->start == start) && (node->end == end)) {
            return node;
        }
    }

    return NULL;
}

int PortPool_AddPortRange(IN LIST_RULE_LIST_S *pstList, USHORT start, USHORT end)
{
    PORT_POOL_NODE_S *pstNode = NULL;
    PORT_POOL_LIST_S *port_list;
    int i;

    BS_DBGASSERT(pstList);

    if (PortPool_FindPortNode(pstList, start, end)) {
        return 0;
    }

    pstNode = MEM_ZMalloc(sizeof(PORT_POOL_NODE_S));
    if (! pstNode){
        RETURN(BS_NO_MEMORY);
    }

    port_list = pstList->user_handle[0];
    for (i=start; i<=end; i++) {
        port_list->ports[i] ++;
    }

    pstNode->start = start;
    pstNode->end = end;

    RuleList_Add(&pstList->stRuleList, 0, &pstNode->rule_node);

    return BS_OK;
}

int PortPool_DeletePortRange(IN LIST_RULE_LIST_S *pstList, USHORT start, USHORT end)
{
    PORT_POOL_NODE_S *pstNode = NULL;
    PORT_POOL_LIST_S *pool_list;

    pstNode = PortPool_FindPortNode(pstList, start, end);
    if (! pstNode){
        return BS_OK;
    }

    RuleList_DelByNode(&pstList->stRuleList, &pstNode->rule_node);
    MEM_Free(pstNode);

    pool_list = pstList->user_handle[0];

    for (int i=start; i<=end; i++) {
        pool_list->ports[i] --;
    }

    return BS_OK;
}

BS_STATUS PortPool_ParsePortRange(IN CHAR *pcPortString, OUT UINT *puiStartPort, OUT UINT *puiEndPort)
{
    LSTR_S lstr;

    lstr.pcData = pcPortString;
    lstr.uiLen = strlen(pcPortString);
    int ret = NumList_ParseElement(&lstr, puiStartPort, puiEndPort);
    if (ret < 0) {
        return ret;
    }

    if ((*puiStartPort < 1) || (*puiEndPort >= POOL_PORT_MAX)) {
        RETURN(BS_BAD_PARA);
    }

    return BS_OK;
}

int PortPool_LoadCfgFromFile(IN LIST_RULE_LIST_S *pstList, IN CHAR *pcFileName)
{
    FILE *fp;
    BS_STATUS ret = BS_OK;
    CHAR line[64];
    UINT uiStartPort;
    UINT uiEndPort;

    if(! FILE_IsFileExist(pcFileName)){
        return BS_NO_SUCH;
    }

    fp = fopen(pcFileName, "rb");
    if (! fp) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    while (1) {
        int len = FILE_ReadLine(fp, line, sizeof(line), '\n');
        if (len <=0){
            break;
        }

        if (len >= sizeof(line)) {
            continue;
        }

        ret = PortPool_ParsePortRange(line, &uiStartPort, &uiEndPort);
        if (ret < 0) {
            continue;
        }

        ret = PortPool_AddPortRange(pstList, uiStartPort, uiEndPort);
        if (ret < 0) {
            break;
        }
    }

    fclose(fp);

    return ret;
}

BOOL_T PortPool_MatchPortPool(IN LIST_RULE_LIST_S *pstList, IN USHORT usPort)
{
    DBGASSERT(NULL != pstList);

    PORT_POOL_LIST_S *port_list = ListRule_GetUserHandle(pstList, 0);

    return (port_list->ports[usPort] > 0);
}

