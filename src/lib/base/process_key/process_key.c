/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/process_index.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"

static char g_process_key[256] = ""; 
static int g_process_index = -1;  

PLUG_API void ProcessKey_SetKey(char *key)
{
    strlcpy(g_process_key, key, sizeof(g_process_key));
}

PLUG_API char * ProcessKey_GetKey()
{
    return g_process_key;
}

PLUG_API void ProcessKey_SetIndex(int index)
{
    g_process_index = index;
}

PLUG_API int ProcessKey_GetIndex()
{
    return g_process_index;
}

PLUG_API int ProcessKey_ShowStatus()
{
    EXEC_OutInfo(" process key: %s\r\n", g_process_key);
    EXEC_OutInfo(" process index: %d\r\n", g_process_index);

    return BS_OK;
}

CONSTRUCTOR(init) {
    g_process_index = ProcessIndex_Get("process.index");
    char buf[32];
    sprintf(buf, "%d", g_process_index);
    ProcessKey_SetKey(buf);
}


