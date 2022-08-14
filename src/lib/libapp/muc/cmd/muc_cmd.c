/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/exec_utl.h"
#include "utl/txt_utl.h"
#include "comp/comp_muc.h"

#include "../h/muc_core.h"

static MUC_S * muc_cmd_get_muc(void *env)
{
    CHAR *pcModeValue = CMD_EXP_GetCurrentModeValue(env);
    UINT index = TXT_Str2Ui(pcModeValue);
    return MucCore_Get(index);
}

/* muc %INT */
PLUG_API BS_STATUS MucCmd_CreateInstance(int argc, char **argv, void *env)
{
    return MucCore_Create(TXT_Str2Ui(argv[1]));
}

/* no muc %INT */
PLUG_API BS_STATUS MucCmd_DestroyInstance(int argc, char **argv, void *env)
{
    MucCore_Destroy(TXT_Str2Ui(argv[2]));
    return BS_OK;
}

/* description %STRING */
PLUG_API int MucCmd_SetDescription(int argc, char **argv, void *env)
{
    MUC_S *muc = muc_cmd_get_muc(env);
    BS_DBGASSERT(muc != NULL);
    strlcpy(muc->description, argv[1], MUC_DESC_LEN + 1);

    return 0;
}

/* start */
PLUG_API int MucCmd_Start(int argc, char **argv, void *env)
{
    MUC_S *muc = muc_cmd_get_muc(env);
    BS_DBGASSERT(muc != NULL);
    MucCore_Start(muc);

    return 0;
}

/* stop */
PLUG_API int MucCmd_Stop(int argc, char **argv, void *env)
{
    MUC_S *muc = muc_cmd_get_muc(env);
    BS_DBGASSERT(muc != NULL);
    MucCore_Stop(muc);

    return 0;
}

/* enter */
PLUG_API int MucCmd_Enter(int argc, char **argv, void *env)
{
    MUC_S *muc = muc_cmd_get_muc(env);
    BS_DBGASSERT(muc != NULL);
    return MucCore_EnterCmd(muc, env);
}

/* switchto muc %INT<1-4095> */
PLUG_API int MucCmd_SwitchTo(int argc, char **argv, void *env)
{
    int muc_id = TXT_Str2Ui(argv[2]);

    MUC_S *muc = MucCore_Get(muc_id);
    if (NULL == muc) {
        EXEC_OutString("Can't find the muc \r\n");
        return -1;
    }

    return MucCore_EnterCmd(muc, env);
}

/* show muc */
PLUG_API int MucCmd_ShowMuc(int argc, char **argv, void *env)
{
    int i;
    MUC_S *muc;

    EXEC_OutString(" ID   Start Description \r\n");

    for (i=1; i<MUC_MAX; i++) {
        muc = MucCore_Get(i);
        if (! muc->used) {
            continue;
        }

        EXEC_OutInfo(" %-4d %-5d %s \r\n", i, muc->start, muc->description);
    }

    return 0;
}

static void muc_cmd_save_muc(HANDLE hFile, MUC_S *muc)
{
    if (muc->description[0]) {
        CMD_EXP_OutputCmd(hFile, "description %s", muc->description);
    }
    if (muc->start) {
        CMD_EXP_OutputCmd(hFile, "start");
    }
}

PLUG_API BS_STATUS MUC_Save(HANDLE hFile)
{
    int i;
    MUC_S *muc;

    for (i=1; i<MUC_MAX; i++) {
        muc = MucCore_Get(i);
        if (! muc->used) {
            continue;
        }

        if (0 == CMD_EXP_OutputMode(hFile, "muc %d", i)) {
            muc_cmd_save_muc(hFile, muc);
            CMD_EXP_OutputModeQuit(hFile);
        }
    }

    return BS_OK;
}

void MucCmd_Init()
{
}

