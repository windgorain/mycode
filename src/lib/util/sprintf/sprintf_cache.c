/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/sprintf_cache.h"

#define SPRINTF_CACHE_SIZE 1024

typedef struct {
    FormatCompile_S *fc;
    char *fmt;
}SPRINTF_CACHE_NODE_S;

typedef struct tagSPRINTF_CACHE {
    SPRINTF_CACHE_NODE_S nodes[SPRINTF_CACHE_SIZE];
}SPRINTF_CACHE_S;

SPRINTF_CACHE_HDL SprintfCache_Create()
{
    return MEM_ZMalloc(sizeof(SPRINTF_CACHE_S));
}

void SprintfCache_Destroy(SPRINTF_CACHE_HDL hdl)
{
    int i;

    for (i=0; i<SPRINTF_COMPILE_ELE_NUM; i++) {
        if (hdl->nodes[i].fc) {
            MEM_Free(hdl->nodes[i].fc);
        }
    }

    MEM_Free(hdl);
}

int SprintfCache_Sprintf(SPRINTF_CACHE_HDL hdl, char *buf, int size, char *fmt, va_list args)
{
    int index = HANDLE_UINT(fmt) % SPRINTF_CACHE_SIZE;
    SPRINTF_CACHE_NODE_S *node = &hdl->nodes[index];

    if (unlikely(node->fc == NULL)) {
        FormatCompile_S *fc = MEM_ZMalloc(sizeof(FormatCompile_S));
        if (0 != BS_FormatCompile(fc, fmt)) {
            MEM_Free(fc);
        } else {
            node->fc = fc;
            node->fmt = fmt;
        }
    }

    if (likely(node->fmt == fmt)) {
        return BS_VFormat(node->fc, buf, size, args);
    }

    return BS_Vsnprintf(buf, size, fmt, args);
}
