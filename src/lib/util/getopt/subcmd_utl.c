/*================================================================
*   Created by LiXingang: 2018.11.09
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/subcmd_utl.h"

SUB_CMD_NODE_S * SUBCMD_Search(SUB_CMD_NODE_S *root, char *subcmd)
{
    SUB_CMD_NODE_S *tmp = root;

    if (! subcmd) {
        return NULL;
    }

    while(tmp->subcmd != NULL) {
        if (strcmp(subcmd, tmp->subcmd) == 0) {
            return tmp;
        }
        tmp ++;
    }

    return NULL;
}

char * SUBCMD_BuildHelpinfo(SUB_CMD_NODE_S *root, OUT char *buf, int buf_size)
{
    SUB_CMD_NODE_S *node;
    int len = 0;

    buf[0] = '\0';

    len += snprintf(buf, buf_size-len, "Commands:\r\n");

    for (node=root; node->subcmd!=NULL; node++) {
        len += snprintf(buf+len, buf_size-len, " ");
        len += snprintf(buf+len, buf_size-len, "%-16s ", node->subcmd);
        len += snprintf(buf+len, buf_size-len, "%s\r\n", node->help == NULL ? "": node->help);
    }

    return buf;
}
