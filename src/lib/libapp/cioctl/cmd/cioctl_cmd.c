/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/exec_utl.h"
#include "utl/stream_server.h"
#include "app/pipe_app_lib.h"
#include "../h/cioctl_server.h"

int CIOCTL_CMD_Init()
{
    return 0;
}


PLUG_API BS_STATUS CIOCTL_CMD_NamePKey(IN UINT ulArgc, IN CHAR ** argv)
{
    char *name = ProcessKey_GetKey();

    if ((name == NULL) || (name[0] == '\0')) {
        EXEC_OutInfo(" Have not process key.\r\n");
        return BS_ERR;
    }

    if (CIOCTL_SERVER_SetNamePKey(name) < 0) {
        EXEC_OutInfo(" The server has enabled, please stop.\r\n");
        return BS_ERR;
    }

    return 0;
}


PLUG_API BS_STATUS CIOCTL_CMD_NameString(IN UINT ulArgc, IN CHAR ** argv)
{
    if (CIOCTL_SERVER_SetNameString(argv[2]) < 0) {
        EXEC_OutInfo(" The server has enabled, please stop.\r\n");
        return BS_ERR;
    }

    return 0;
}


PLUG_API BS_STATUS CIOCTL_CMD_CmdNoName(IN UINT ulArgc, IN CHAR ** argv)
{
    if (CIOCTL_SERVER_SetNameDefault() < 0) {
        EXEC_OutInfo(" The server has enabled, please stop.\r\n");
        return BS_ERR;
    }

    return 0;
}


PLUG_API int CIOCTL_CMD_PipeServerEnable(IN UINT ulArgc, IN CHAR ** argv)
{
    if (CIOCTL_SERVER_Enable() < 0) {
        EXEC_OutInfo(" servier enable failed \r\n");
        EXEC_OutErrCodeInfo();
        return BS_ERR;
    }

    return 0;
}

PLUG_API BS_STATUS CIOCTL_CMD_Save(HANDLE hFileHandle)
{
    int name_type = CIOCTL_SERVER_GetNameType();
    char *name = CIOCTL_SERVER_GetName();

    if (name_type == PIPE_APP_NAME_TYPE_STRING) {
        CMD_EXP_OutputCmd(hFileHandle, "name string %s", name);
    }

    if (name_type == PIPE_APP_NAME_TYPE_PKEY) {
        CMD_EXP_OutputCmd(hFileHandle, "name process-key");
    }

    if (CIOCTL_SERVER_IsEnabled()) {
        CMD_EXP_OutputCmd(hFileHandle, "pipe-server enable");
    }

    return BS_OK;
}

