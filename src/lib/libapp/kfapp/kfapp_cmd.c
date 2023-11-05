/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/exec_utl.h"
#include "comp/comp_kfapp.h"
#include "kfapp_func.h"


PLUG_API int KFAPP_CmdDo(int argc, char **argv, void *env)
{
    KFAPP_PARAM_S stKfappParam;
    int ret;

    if (BS_OK != KFAPP_ParamInit(&stKfappParam)) {
        EXEC_OutString("init param failed \r\n");
        return BS_NO_MEMORY;
    }

    ret = KFAPP_RunString(argv[1], &stKfappParam);
    if (ret < 0) {
        EXEC_OutString("kfapp run failed \r\n");
        return ret;
    }

    char * out = KFAPP_BuildParamString(&stKfappParam);

    if (out) {
        EXEC_OutInfo("%s \r\n", out);
    }

    KFAPP_ParamFini(&stKfappParam);

    return 0;
}
