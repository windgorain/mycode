/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/file_utl.h"
#include "utl/exec_utl.h"
#include "utl/txt_utl.h"
#include "comp/comp_muc.h"

#include "../h/muc_core.h"

static MUC_S g_mucs[MUC_MAX];
static THREAD_LOCAL int g_muc_thread_muc_id = -1;
static DLL_HEAD_S g_muc_ob_list = DLL_HEAD_INIT_VALUE(&g_muc_ob_list);
static char g_muc_save_path[FILE_MAX_PATH_LEN + 1];

static int muc_core_event_notify(int muc_id, UINT event)
{
    MUC_EVENT_OB_S *ob;
    int ret = 0;

    DLL_SCAN(&g_muc_ob_list, ob) {
        ret |= ob->pfEventFunc(ob, muc_id, event);
    }

    return ret;
}

int MucCore_Init(void *env)
{
    char *save_path = PlugMgr_GetSavePathByEnv(env);

    if (save_path) {
        strlcpy(g_muc_save_path, save_path, sizeof(g_muc_save_path));
    }

    return 0;
}

static void muc_core_delete_save_dir(int muc_id)
{
    char conf_file[FILE_MAX_PATH_LEN + 1];

    if (snprintf(conf_file, sizeof(conf_file), "%s/muc_%d", g_muc_save_path, muc_id) < 0) {
        BS_DBGASSERT(0);
        return;
    }

    if (FILE_IsDirExist(conf_file)) {
        FILE_DelDir(conf_file);
    }
}

int MucCore_Create(int muc_id)
{
    BS_DBGASSERT(muc_id >= 0);

    if (muc_id >= MUC_MAX) {
        RETURN(BS_OUT_OF_RANGE);
    }

    MUC_S *muc = &g_mucs[muc_id];

    if (muc->used) {
        return 0;
    }

    if (PLUGCT_GetLoadStage() >= PLUG_STAGE_RUNNING) { 
        muc_core_delete_save_dir(muc_id);
    }

    memset(muc, 0, sizeof(MUC_S));

    muc->used = 1;
    muc->id = muc_id;

    muc_core_event_notify(muc_id, MUC_EVENT_CREATE);

    return 0;
}

int MucCore_Destroy(int muc_id)
{
    BS_DBGASSERT(muc_id >= 0);

    if (muc_id >= MUC_MAX) {
        RETURN(BS_OUT_OF_RANGE);
    }

    MUC_S *muc = &g_mucs[muc_id];

    if (muc->used == 0) {
        return 0;
    }

    muc_core_event_notify(muc_id, MUC_EVENT_DESTROY);

    muc->start = 0;
    muc->used = 0;

    muc_core_delete_save_dir(muc_id);

    return 0;
}

MUC_S * MucCore_Get(int muc_id)
{
    BS_DBGASSERT(muc_id >= 0);

    if (muc_id >= MUC_MAX) {
        return NULL;
    }

    return &g_mucs[muc_id];
}

int MucCore_Start(MUC_S *muc)
{
    muc->start = 1;
    char conf_file[FILE_MAX_PATH_LEN + 1];

    if (muc->restored) {
        return 0;
    }

    muc->restored = 1;
    if (snprintf(conf_file, sizeof(conf_file), "%s/muc_%d/config.cfg", g_muc_save_path, muc->id) < 0) {
        BS_DBGASSERT(0);
        RETURN(BS_ERR);
    }

    CMD_MNG_CmdRestoreByFile(muc->id, conf_file);

    return 0;
}

int MucCore_Stop(MUC_S *muc)
{
    muc->start = 0;
    return 0;
}

int MucCore_EnterCmd(MUC_S *muc, void *env)
{
    if (muc->used == 0) {
        EXEC_OutString("Have no such muc \r\n");
        RETURN(BS_NOT_INIT);
    }

    if (muc->start == 0) {
        EXEC_OutString("Muc has not started \r\n");
        RETURN(BS_NOT_INIT);
    }

    void * runner = CmdExp_GetEnvRunner(env);
    UINT runner_type = CmdExp_GetRunnerType(runner);

    HANDLE hCmdRunner = CMD_EXP_CreateRunner(runner_type);
    char info[32];

    if (NULL == hCmdRunner) {
        RETURN(BS_NO_MEMORY);
    }


    sprintf(info, "muc-%d", muc->id);

    CmdExp_SetViewPrefix(hCmdRunner, info);
    CmdExp_AltEnable(hCmdRunner, 1);
    CmdExp_SetRunnerLevel(hCmdRunner, CMD_EXP_LEVEL_MUC);
    CmdExp_SetRunnerMucID(hCmdRunner, muc->id);
    CmdExp_SetRunnerDir(hCmdRunner, g_muc_save_path);
    CmdExp_SetSubRunner(runner, hCmdRunner);

    return 0;
}

PLUG_API void * MUC_GetUD(int muc_id, int index)
{
    BS_DBGASSERT(muc_id >= 0);
    BS_DBGASSERT(index >= 0);

    if (muc_id >= MUC_MAX) {
        BS_DBGASSERT(0);
        return NULL;
    }

    if (index >= MUC_UD_MAX) {
        BS_DBGASSERT(0);
        return NULL;
    }

    return g_mucs[muc_id].user_data[index];
}

PLUG_API int MUC_SetUD(int muc_id, int index, void *user_data)
{
    BS_DBGASSERT(muc_id >= 0);
    BS_DBGASSERT(index >= 0);

    if (muc_id >= MUC_MAX) {
        BS_DBGASSERT(0);
        RETURN(BS_OUT_OF_RANGE);
    }

    if (index >= MUC_UD_MAX) {
        BS_DBGASSERT(0);
        RETURN(BS_OUT_OF_RANGE);
    }

    g_mucs[muc_id].user_data[index] = user_data;

    return 0;
}

PLUG_API void MUC_SetSelfMucID(int muc_id)
{
    g_muc_thread_muc_id = muc_id;
}

PLUG_API int MUC_GetSelfMucID()
{
    return g_muc_thread_muc_id;
}

PLUG_API void MUC_RegEventOB(MUC_EVENT_OB_S *ob)
{
    DLL_ADD(&g_muc_ob_list, &ob->link_node);
}

PLUG_API int MUC_GetNext(int curr)
{
    int i;
    for (i=curr + 1; i<MUC_MAX; i++) {
        if (g_mucs[i].used) {
            return i;
        }
    }

    return -1;
}


