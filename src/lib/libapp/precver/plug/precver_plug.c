/*================================================================
*   Created by LiXingang
*   Description: 插件管理 
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/socket_utl.h"
#include "utl/plug_mgr.h"
#include "utl/exec_utl.h"
#include "utl/subcmd_utl.h"

#include "../h/precver_conf.h"

static PLUG_MGR_S g_precver_plug_mgr;
static char g_precver_plug_path[256];

static void precver_plug_show_one(PLUG_MGR_NODE_S *node)
{
    EXEC_OutInfo("%-16s %s\r\n", node->plug_name, node->filename);
}

PLUG_API int PRecverPlug_CmdShow(UINT argc, char **argv, void *env)
{
    PLUG_MGR_NODE_S *node = NULL;

    EXEC_OutInfo("plug_name        filename\r\n");
    EXEC_OutInfo("-------------------------------------------------\r\n");

    while ((node= PlugMgr_Next(&g_precver_plug_mgr, node))) {
        precver_plug_show_one(node);
    }

    return 0;
}

int PRecverPlug_Init()
{
    SYSINFO_ExpandConfPath(g_precver_plug_path, sizeof(g_precver_plug_path), "plug/precver/precver_plug.ini");
    return 0;
}

int PRecverPlug_LoadPlug(char *plug_name)
{
    return PlugMgr_LoadManual(&g_precver_plug_mgr, g_precver_plug_path, plug_name);
}

PLUG_HDL PRecverPlug_GetPlug(char *plug_name)
{
    PLUG_MGR_NODE_S *node = PlugMgr_Find(&g_precver_plug_mgr, plug_name);
    if (! node) {
        return NULL;
    }

    return node->hPlug;
}

BOOL_T PRecverPlug_CfgIsExist(char *plug_name)
{
    CFF_HANDLE hCff;

    hCff = CFF_INI_Open(g_precver_plug_path, 0);
    if (! hCff) {
        return FALSE;
    }

    BOOL_T bIsExist = CFF_IsTagExist(hCff, plug_name);

    CFF_Close(hCff);

    return bIsExist;
}

char * PRecverPlug_CfgGetNext(char *curr/* NULL表示获取第一个 */)
{
    CFF_HANDLE hCff;
    static char tmp[128];

    hCff = CFF_INI_Open(g_precver_plug_path, CFF_FLAG_SORT);
    if (! hCff) {
        return NULL;
    }

    char * name = CFF_GetNextTag(hCff, curr);
    if (name) {
        strlcpy(tmp, name, sizeof(tmp));
    }

    CFF_Close(hCff);

    if (! name) {
        return NULL;
    }

    return tmp;
}

CONSTRUCTOR(init) {
    PlugMgr_Init(&g_precver_plug_mgr);
}

