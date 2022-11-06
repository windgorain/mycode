/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/map_utl.h"
#include "utl/txt_utl.h"
#include "kfapp_func.h"

static MAP_HANDLE g_kfapp_rule_map;

int kfappkv_add_rule(char *key, int key_len, char *value)
{
    int ret;
    char *v = TXT_Strdup(value);

    if (! v) {
        BS_DBGASSERT(0);
        RETURN(BS_NO_MEMORY);
    }

    ret =  MAP_Add(g_kfapp_rule_map, key, key_len, v, MAP_FLAG_DUP_KEY);
    if (0 != ret) {
        BS_DBGASSERT(0);
        MEM_Free(v);
        return ret;
    }

    return 0;
}

int kfappkv_load_file(char *file)
{
    FILE *fp;
    char line[2048];
    char *split;

    fp = fopen(file, "rb");
    if (!fp) {
        BS_DBGASSERT(0);
        RETURN(BS_CAN_NOT_OPEN);
    }

    while ((fgets(line, sizeof(line), fp)) != NULL) {
        split = strchr(line, ':');
        if (! split) {
            continue;
        }

        kfappkv_add_rule(line, split-line-1, split+1);
    }

    fclose(fp);

    return 0;
}

int KFAPP_KV_Init()
{
    g_kfapp_rule_map = MAP_HashCreate(NULL);
    if (! g_kfapp_rule_map) {
        RETURN(BS_NO_MEMORY);
    }

    kfappkv_load_file("plug/kfapp/kfapp_kvcmd.txt");

    return 0;
}

char * KFAPP_KV_Get(char *key)
{
    return MAP_Get(g_kfapp_rule_map, key, strlen(key));
}


