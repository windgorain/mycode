/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/exec_utl.h"
#include "utl/err_code.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/umap_utl.h"
#include "utl/getopt2_utl.h"
#include "../h/ulcapp_runtime.h"
#include "../h/ulcapp_cfg_lock.h"

static int _ulcapp_cmd_help(GETOPT2_NODE_S *opts)
{
    char buf[512];
    EXEC_OutInfo("%s \r\n", GETOPT2_BuildHelpinfo(opts, buf, sizeof(buf)));
    return 0;
}


PLUG_API int ULCAPP_CMD_LoadFile(int argc, char **argv)
{
    char *instance = argv[1];
    char *filename = argv[3];

    int ret = ULCAPP_LoadFile(filename, instance);
    if (ret < 0) {
        ErrCode_Output(EXEC_OutInfo);
    }
    return ret;
}


PLUG_API int ULCAPP_CMD_ReplaceFile(int argc, char **argv)
{
    char *instance = argv[1];
    char *filename = argv[3];
    UINT keep_map = 0;
    GETOPT2_NODE_S opts[] = {
        {'o', 'k', "keep-map", GETOPT2_V_NONE, NULL, "keep map", 0},
        {0}
    };

    if (argc > 4) {
        if (BS_OK != GETOPT2_ParseFromArgv0(argc - 4, argv + 4, opts)) {
            _ulcapp_cmd_help(opts);
            return 0;
        }

        if (GETOPT2_IsOptSetted(opts, 'k', NULL)) {
            keep_map = 1;
        }
    }

    int ret = ULCAPP_ReplaceFile(filename, instance, keep_map);
    if (ret < 0) {
        ErrCode_Output(EXEC_OutInfo);
    }
    return ret;
}


PLUG_API int ULCAPP_CMD_UnloadInstance(int argc, char **argv)
{
    ULCAPP_UnloadInstance(argv[1]);
    return 0;
}


PLUG_API int ULCAPP_CMD_NoLoadInstance(int argc, char **argv)
{
    ULCAPP_UnloadInstance(argv[2]);
    return 0;
}


PLUG_API int ULCAPP_CMD_ShowMap(int argc, char **argv)
{
    ULCAPP_ShowMap();
    return 0;
}


PLUG_API int ULCAPP_CMD_DumpMap(int argc, char **argv)
{
    int map_fd;

    map_fd = atoi(argv[2]);

    ULCAPP_DumpMap(map_fd);

    return 0;
}


PLUG_API int ULCAPP_CMD_ShowProg(int argc, char **argv)
{
    ULCAPP_ShowProg();
    return 0;
}


PLUG_API int ULCAPP_CMD_TestCmd(int argc, char **argv)
{
    if (argc < 2) {
        return -1;
    }

    ULCAPP_Tcmd(argc-2, argv+2);
    return 0;
}


PLUG_API BS_STATUS ULCAPP_CMD_Save(IN HANDLE hFile)
{
    ULCAPP_RuntimeSave(hFile);
    return 0;
}


