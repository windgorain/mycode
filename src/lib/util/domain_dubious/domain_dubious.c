/*================================================================
*   Created by LiXingang
*   Description: 可疑域名检测
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/domain_dubious.h"
#include "utl/trie_utl.h"
#include "utl/dnsname_trie.h"

int DomainDubious_Init(DOMAIN_DUBIOUS_S *ctrl)
{
    ctrl->domain_name_trie = Trie_Create(TRIE_TYPE_4BITS);
    if (! ctrl->domain_name_trie) {
        RETURN(BS_NO_MEMORY);
    }

    return 0;
}

/* 添加一条域名,比如: www.163.com, *.baidu.com */
/* type: DOMAIN_DUBIOUS_ADD_WHITE  or DOMAIN_DUBIOUS_ADD_BLACK */
int DomainDubious_Add(DOMAIN_DUBIOUS_S *ctrl, char *domain_name, int domain_name_len, int type)
{
    return DnsNameTrie_Insert(ctrl->domain_name_trie, domain_name, domain_name_len, UINT_HANDLE(type));
}

int DomainDubious_LoadFile(DOMAIN_DUBIOUS_S *ctrl, char *file, int type)
{
    FILE *fp;
    char buf[1024];
    char *line;

    fp = fopen(file, "rb");
    if (!fp) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    while (NULL != fgets(buf, sizeof(buf), fp)) {
        line = TXT_Strim(buf);
        DomainDubious_Add(ctrl, line, strlen(line), type);
    }

    fclose(fp);

    return 0;
}

/* return: DOMAIN_DUBIOUS_RESULT_XXX */
int DomainDubious_Check(DOMAIN_DUBIOUS_S *ctrl, char *domain_name, int domain_name_len)
{
    void *ud;

    ud = DnsNameTrie_Match(ctrl->domain_name_trie, domain_name, domain_name_len, TRIE_MATCH_HOSTNAME);
    if (ud) {
        if (ud == UINT_HANDLE(DOMAIN_DUBIOUS_ADD_WHITE)) {
            return DOMAIN_DUBIOUS_RESULT_OK;
        } 
        return DOMAIN_DUBIOUS_RESULT_BLACK;
    }

    return DOMAIN_DUBIOUS_RESULT_OK;
}
