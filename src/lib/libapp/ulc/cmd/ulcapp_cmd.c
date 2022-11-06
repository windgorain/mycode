/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/exec_utl.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_loader.h"
#include "utl/umap_utl.h"
#include "../h/ulcapp_cfg_lock.h"

/* load file %STRING */
int ULCAPP_CMD_LoadFile(int argc, char **argv)
{
    int ret;
    MYBPF_LOADER_PARAM_S p = {0};

    p.filename = argv[2];

    ULCAPP_CfgLock();
    ret = MYBPF_Loader_Load(&p);
    ULCAPP_CfgUnlock();

    if (ret < 0) {
        EXEC_OutInfo("Can't load file \r\n");
    }

    return ret;
}

/* no load xdp-file %STRING */
int ULCAPP_CMD_UnloadFile(int argc, char **argv)
{
    ULCAPP_CfgLock();
    MYBPF_Loader_UnLoad(argv[3]);
    ULCAPP_CfgUnlock();
    return 0;
}

/* show map */
int ULCAPP_CMD_ShowMap(int argc, char **argv)
{
    ULCAPP_CfgLock();
    UMAP_ShowMap(EXEC_OutInfo);
    ULCAPP_CfgUnlock();
    return 0;
}

/* dump map %INT */
int ULCAPP_CMD_DumpMap(int argc, char **argv)
{
    int map_fd;

    map_fd = atoi(argv[2]);

    ULCAPP_CfgLock();
    UMAP_DumpMap(map_fd, EXEC_OutInfo);
    ULCAPP_CfgUnlock();

    return 0;
}

/* show prog */
int ULCAPP_CMD_ShowProg(int argc, char **argv)
{
    ULCAPP_CfgLock();
    MYBPF_PROG_ShowProg(EXEC_OutInfo);
    ULCAPP_CfgUnlock();
    return 0;
}

/* save */
PLUG_API BS_STATUS ULCAPP_CMD_Save(IN HANDLE hFile)
{
    void *iter = NULL;
    MYBPF_LOADER_PARAM_S *p;

    ULCAPP_CfgLock();
    while ((p = MYBPF_Loader_GetNext(&iter))) {
        if (p->sec_name) {
            CMD_EXP_OutputCmd(hFile, "load file %s sec %s", p->filename, p->sec_name);
        } else {
            CMD_EXP_OutputCmd(hFile, "load file %s", p->filename);
        }
    }
    ULCAPP_CfgUnlock();

    return 0;
}


