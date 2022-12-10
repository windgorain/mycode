/******************************************************************************
* Copyright (C), LiXingang
* Description: 
* History:     
******************************************************************************/

#include <string.h>
#include "utl/acl_utl.h"


char *g_acl_type_names[] = {
#define _(a,b) b,
    ACL_TYPE_DEF
#undef _
    "unknown"
};

char * ACL_GetStrByType(int type)
{
    if (type >= ACL_TYPE_MAX) {
        return "unkonwn";
    }

    return g_acl_type_names[type];
}

int ACL_GetTypeByStr(char *type_str)
{
    int i;

    for (i=0; i<ACL_TYPE_MAX; i++) {
        if (0 == strcmp(g_acl_type_names[i], type_str)) {
            return i;
        }
    }

    return -1;
}

