/*================================================================
*   Created by LiXingang: 2018.11.15
*   Description: 处理ACL字符串, 如: permit xxxx
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/acl_string.h"


int ACLSTR_Simple_Parse(char *aclstring, ACL_STR_S *node)
{
    char *split;
    char *pattern;

    memset(node, 0, sizeof(ACL_STR_S));

    if ((aclstring[0] == 'p') || (aclstring[0] == 'P')) {
        node->action = "permit";
    } else if ((aclstring[0] == 'd') || (aclstring[0] == 'D')) {
        node->action = "deny";
    } else if ((aclstring[0] == 'b') || (aclstring[0] == 'B')) {
        node->action = "bypass";
    }

    split = TXT_MStrnchr(aclstring, strlen(aclstring), " \t");
    if (NULL == split) {
        return -1;
    }

    pattern = split + 1;

    pattern = TXT_Strim(pattern);
    if (pattern[0] == 0) {
        return -1;
    }

    node->pattern = pattern;

    return 0;
}


