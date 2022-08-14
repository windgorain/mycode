/*================================================================
*   Created by LiXingang：2018.11.07
*   Description：域名字典树, 用于查找/匹配 *.baidu.com之类的域名
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/mem_utl.h"
#include "utl/trie_utl.h"
#include "utl/dnsname_trie.h"

static void * dnsnametrie_dft_set_ud(void *old_ud, void *uh)
{
    return uh;
}

int DnsNameTrie_InsertExt(TRIE_HANDLE trie_ctrl, char *hostname, int len, PF_TRIE_UD_SET ud_set, void *uh)
{
    char invert_hostname[512];
    UINT flag = 0;

    if (hostname[0] == '\0') {
        RETURN(BS_BAD_PARA);
    }

    if (len >= sizeof(invert_hostname)) {
        RETURN(BS_OUT_OF_RANGE);
    }

    MEM_Invert(hostname, len, invert_hostname);
    invert_hostname[len] = '\0';

    if (hostname[0] == '*') {
        flag |= TRIE_NODE_FLAG_WILDCARD;
        len --;
    }

    if (NULL == Trie_InsertEx(trie_ctrl, (UCHAR*)invert_hostname, len, flag, ud_set, uh)) {
        RETURN(BS_ERR);
    }

    return 0;
}

int DnsNameTrie_Insert(TRIE_HANDLE trie_ctrl, char *hostname, int len, void *ud)
{
    return DnsNameTrie_InsertExt(trie_ctrl, hostname, len ,dnsnametrie_dft_set_ud, ud);
}

TRIE_COMMON_S * DnsNameTrie_MatchNode(TRIE_HANDLE trie, char *hostname, UINT len, int match_type)
{
    char invert_hostname[512];

    if (hostname[0] == '\0') {
        return NULL;
    }

    len = strlen(hostname);
    if (len >= sizeof(invert_hostname)) {
        return NULL;
    }

    MEM_Invert(hostname, len, invert_hostname);
    invert_hostname[len] = '\0';

    return Trie_Match(trie, (UCHAR*)invert_hostname, len, match_type);
}

void * DnsNameTrie_Match(TRIE_HANDLE trie, char *hostname, UINT len, int match_type)
{
    TRIE_COMMON_S *node;

    node = DnsNameTrie_MatchNode(trie, hostname, len, match_type);
    if (node == NULL) {
        return NULL;
    }

    return node->ud;
}

