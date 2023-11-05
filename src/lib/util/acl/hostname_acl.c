/*================================================================
*   Created by LiXingang: 2018.11.15
*   Description: hostname acl
*      基于字典树,执行最长匹配策略
*
================================================================*/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/acl_string.h"
#include "utl/hostname_acl.h"
#include "utl/file_utl.h"
#include "utl/trie_utl.h"

static void hostnameacl_ProcessLine(HOSTNAME_ACL_S *hostname_acl, char *line)
{
    ACL_STR_S acl_str;
    int action;

    line = TXT_Strim(line);

    if ((*line == '\0') || (*line == '#')) {
        return;
    }

    if (0 != ACLSTR_Simple_Parse(line, &acl_str)) {
        return;
    }

    action = HOSTNAME_ACL_DENY;
    if (acl_str.action[0] == 'b') {
        action = HOSTNAME_ACL_BYPASS;
    } else if (acl_str.action[0] == 'p') {
        action = HOSTNAME_ACL_PERMIT;
    }

    
    Trie_Insert(hostname_acl->trie, (UCHAR *)acl_str.pattern, strlen(acl_str.pattern), 
            TRIE_NODE_FLAG_WILDCARD, UINT_HANDLE(action));

    return;
}

int HostnameACL_Init(HOSTNAME_ACL_S *hostname_acl, char *config_file)
{
    FILE *fp;
    char buf[256];

    hostname_acl->trie = Trie_Create(TRIE_TYPE_4BITS);

    fp = FILE_Open(config_file, FALSE, "rb");
    if (NULL == fp) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    while(NULL != fgets(buf, sizeof(buf), fp)) {
        hostnameacl_ProcessLine(hostname_acl, buf);
    }

    fclose(fp);

    return 0;
}

void HostnameACL_Fini(IN HOSTNAME_ACL_S *hostname_acl)
{
    
    Trie_Destroy(hostname_acl->trie, NULL);
}

int HostnameACL_Match(HOSTNAME_ACL_S *hostname_acl, char *hostname)
{
    TRIE_COMMON_S *ret;

    ret = Trie_Match(hostname_acl->trie, (UCHAR *)hostname, strlen(hostname), TRIE_MATCH_MAXLEN);
    if (NULL == ret) {
        return HOSTNAME_ACL_UNDEF;
    }

    return HANDLE_UINT(ret->ud);
}

