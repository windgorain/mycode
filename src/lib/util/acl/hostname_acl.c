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

    DnsNameTrie_Insert(&hostname_acl->trie, acl_str.pattern, UINT_HANDLE(action));

    return;
}

int HostnameACL_Init(HOSTNAME_ACL_S *hostname_acl, char *config_file)
{
    FILE *fp;
    char buf[256];

    DnsNameTrie_Init(&hostname_acl->trie, 0);

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
    DnsNameTrie_Fini(&hostname_acl->trie, NULL);
}

int HostnameACL_Match(HOSTNAME_ACL_S *hostname_acl, char *hostname)
{
    void *ret;

    ret = DnsNameTrie_Match(&hostname_acl->trie, hostname, strlen(hostname));
    if (NULL == ret) {
        return HOSTNAME_ACL_UNDEF;
    }

    return HANDLE_UINT(ret);
}

