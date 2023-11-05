/*================================================================
*   Created by LiXingang
*   Description: 
================================================================*/
#include "bs.h"

#include "utl/cff_utl.h"
#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/plug_utl.h"
#include "utl/plug_mgr.h"

typedef struct {
    CFF_HANDLE hCff;
    char *tag;
    PLUG_MGR_NODE_S *plug_mgr_node;
}PLUG_MGR_ENV_S;

static PLUG_MGR_NODE_S *plugmgr_AllocNode(char *plug_name, char *filename)
{
    PLUG_MGR_NODE_S *node;

    node = MEM_ZMalloc(sizeof(PLUG_MGR_NODE_S));
    if (! node) {
        return NULL;
    }

    node->plug_name = TXT_Strdup(plug_name);
    if (! node->plug_name) {
        MEM_Free(node);
        return NULL;
    }

    node->filename = TXT_Strdup(filename);
    if (! node->filename) {
        MEM_Free(node->plug_name);
        MEM_Free(node);
        return NULL;
    }

    return node;
}

static void plugmgr_FreeNode(PLUG_MGR_NODE_S *node)
{
    if (node->filename) {
        MEM_Free(node->filename);
    }

    if (node->plug_name) {
        MEM_Free(node->plug_name);
    }

    MEM_Free(node);
}

static PLUG_MGR_NODE_S * plugmgr_LoadFile(PLUG_MGR_S *plug_mgr,
        char *plug_name, char *filename, char *conf_path, char *save_path)
{
    PLUG_HDL hPlug;
    CHAR *pszErroInfo = NULL;
    PLUG_MGR_NODE_S *node = plugmgr_AllocNode(plug_name, filename);
    UINT_FUNC_3 pfFunc;

    if (! node) {
        return NULL;
    }

    hPlug = (PLUG_HDL)PLUG_LOAD(filename);
    if (hPlug == NULL) {
#ifdef IN_UNIXLIKE
        pszErroInfo = dlerror();
        printf(" Failed to load %s, error info: %s \r\n",
            filename, pszErroInfo == NULL ? "null" : pszErroInfo);
#endif

#ifdef IN_WINDOWS
        printf(" Failed to load %s, error info:", pszFile);
        LastErr_Print();
#endif
        plugmgr_FreeNode(node);
        return NULL;
    }

    pfFunc = PLUG_GET_FUNC_BY_NAME(hPlug, "PlugMain");
    if (pfFunc) {
        int eRet = pfFunc(filename, conf_path, save_path);
        if (eRet != BS_OK) {
            printf(" Failed to init.\r\n");
            PLUG_FREE(hPlug);
            plugmgr_FreeNode(node);
            return NULL;
        }
    }

    node->hPlug = hPlug;
    DLL_ADD(&plug_mgr->plug_list, node);
 
    return node;
}

static void plugmgr_Unload(PLUG_MGR_S *plug_mgr, PLUG_MGR_NODE_S *node)
{
    DLL_DEL(&plug_mgr->plug_list, node);
    PLUG_FREE(node->hPlug);
    plugmgr_FreeNode(node);
}

PLUG_MGR_NODE_S * PlugMgr_Find(PLUG_MGR_S *mgr, char *plug_name)
{
    PLUG_MGR_NODE_S *node;

    DLL_SCAN(&mgr->plug_list, node) {
        if (strcmp(plug_name, node->plug_name) == 0) {
            return node;
        }
    }

    return node;
}

PLUG_MGR_NODE_S * PlugMgr_FindByFilename(PLUG_MGR_S *mgr, char *filename)
{
    PLUG_MGR_NODE_S *node;

    DLL_SCAN(&mgr->plug_list, node) {
        if (strcmp(filename, node->filename) == 0) {
            return node;
        }
    }

    return node;
}

int PlugMgr_Init(PLUG_MGR_S *plug_mgr)
{
    memset(plug_mgr, 0, sizeof(PLUG_MGR_S));
    DLL_INIT(&plug_mgr->plug_list);
    return 0;
}

static VOID plugmgr_Load(PLUG_MGR_S *mgr, char *plug_name,
        char *file, char *conf_path, char *save_path)
{
    printf("Load %s...", file);
    if (NULL != PlugMgr_Find(mgr, plug_name)) {
        printf(" This plugin has been loaded!\r\n");
        return;
    }

    if (NULL == plugmgr_LoadFile(mgr, plug_name, file, conf_path, save_path)) {
		return;
    }
    printf("OK\r\n");

    return;
}

static VOID plugmgr_LoadOne(PLUG_MGR_S *mgr, CFF_HANDLE hCff, char *plug_name)
{
    char *file = NULL;
    char *conf_path = "";
    char *save_path = NULL;
	char ext_file[256];
	char ext_conf_path[256];
	char ext_save_path[256];

    CFF_GetPropAsString(hCff, plug_name, "file", &file);
    CFF_GetPropAsString(hCff, plug_name, "conf_path", &conf_path);
    CFF_GetPropAsString(hCff, plug_name, "save_path", &save_path);
    if (file == NULL) {
        return;
    }
    if (save_path == NULL) {
        save_path = conf_path;
    }
	
	SYSINFO_ExpandWorkDir(ext_file, sizeof(ext_file), file);
	SYSINFO_ExpandWorkDir(ext_conf_path, sizeof(ext_conf_path), conf_path);
	SYSINFO_ExpandWorkDir(ext_save_path, sizeof(ext_save_path), save_path);

    plugmgr_Load(mgr, plug_name, ext_file, ext_conf_path, ext_save_path);
}


static VOID plugmgr_EachLoad(CFF_HANDLE hCff, char *tag, void *ud)
{
    USER_HANDLE_S *uh = ud;
    PLUG_MGR_S *mgr = uh->ahUserHandle[0];

    int load = 0;
    CFF_GetPropAsInt(hCff, tag, "load", &load);
    if (load == 0) {
        return;
    }

    plugmgr_LoadOne(mgr, hCff, tag);
}

static VOID plugmgr_NotifyStage(CFF_HANDLE hCff, char *tag, void *ud)
{
    USER_HANDLE_S *uh = ud;
    PLUG_MGR_S *mgr = uh->ahUserHandle[0];
    int stage = HANDLE_UINT(uh->ahUserHandle[1]);
    PLUG_HDL ulPlugId;
    PF_PLUG_STAGE pfFunc;
    PLUG_MGR_ENV_S env;

    PLUG_MGR_NODE_S *node = PlugMgr_Find(mgr, tag);
    if (node == NULL) {
        return;
    }

    ulPlugId = node->hPlug;
    if (ulPlugId == 0) {
        return;
    }

    pfFunc = PLUG_GET_FUNC_BY_NAME(ulPlugId, "Plug_Stage");
    if (pfFunc == NULL) {
        return;
    }

    memset(&env, 0, sizeof(env));
    env.hCff = hCff;
    env.tag = tag;
    env.plug_mgr_node = node;

    pfFunc(stage, &env);

    return;
}

static VOID plugmgr_RegCmd(PLUG_MGR_S *mgr, char *plug_name, char *filename,
        char *conf_path, char *save_path, char *cmdfile)
{
    PLUG_HDL plug;
    PLUG_MGR_NODE_S *node;

    node = PlugMgr_Find(mgr, plug_name);
    if (NULL == node) {
        return;
    }

	plug = node->hPlug;

    
    CHAR buf[FILE_MAX_PATH_LEN + 1];
    scnprintf(buf, sizeof(buf), "%s/%s", conf_path, cmdfile);
    CMD_CFG_RegCmd(buf, plug, save_path);
}


static VOID plugmgr_UnRegCmd(PLUG_MGR_S *mgr, char *plug_name, 
        char *conf_path, char *save_path, char *cmdfile)
{
    if (NULL == PlugMgr_Find(mgr, plug_name)) {
        return;
    }

    
    CHAR buf[FILE_MAX_PATH_LEN + 1];
    scnprintf(buf, sizeof(buf), "%s/%s", conf_path, cmdfile);
    CMD_CFG_UnRegCmd(buf, save_path);
}

static VOID plugmgr_EachRegCmd(CFF_HANDLE hCff, char *tag, void *ud)
{
    USER_HANDLE_S *uh = ud;
    PLUG_MGR_S *mgr = uh->ahUserHandle[0];
    char *cmdfile = uh->ahUserHandle[1];
    char *file = NULL;
    char *conf_path = NULL;
    char *save_path = NULL;
	char ext_file[256];
	char ext_conf_path[256];
	char ext_save_path[256];

    CFF_GetPropAsString(hCff, tag, "file", &file);
    CFF_GetPropAsString(hCff, tag, "conf_path", &conf_path);
    CFF_GetPropAsString(hCff, tag, "save_path", &save_path);
    if ((file == NULL) || (conf_path == NULL)){
        return;
    }
    if (save_path == NULL) {
        save_path = conf_path;
    }

	SYSINFO_ExpandWorkDir(ext_file, sizeof(ext_file), file);
	SYSINFO_ExpandWorkDir(ext_conf_path, sizeof(ext_conf_path), conf_path);
	SYSINFO_ExpandWorkDir(ext_save_path, sizeof(ext_save_path), save_path);
    
	plugmgr_RegCmd(mgr, tag, ext_file, ext_conf_path, ext_save_path, cmdfile);
}

static VOID plugmgr_EachUnRegCmd(CFF_HANDLE hCff, char *tag, void *ud)
{
    USER_HANDLE_S *uh = ud;
    PLUG_MGR_S *mgr = uh->ahUserHandle[0];
    char *cmdfile = uh->ahUserHandle[1];
    char *conf_path = NULL;
    char *save_path = NULL;
	char ext_cmdfile[256];
	char ext_conf_path[256];
	char ext_save_path[256];

    CFF_GetPropAsString(hCff, tag, "conf_path", &conf_path);
    CFF_GetPropAsString(hCff, tag, "save_path", &save_path);
    if (conf_path == NULL) {
        return;
    }
    if (save_path == NULL) {
        save_path = conf_path;
    }
	SYSINFO_ExpandWorkDir(ext_cmdfile, sizeof(ext_cmdfile), cmdfile);
	SYSINFO_ExpandWorkDir(ext_conf_path, sizeof(ext_conf_path), conf_path);
	SYSINFO_ExpandWorkDir(ext_save_path, sizeof(ext_save_path), save_path);

	plugmgr_UnRegCmd(mgr, tag, ext_conf_path, ext_save_path, ext_cmdfile);
}

static VOID plugmgr_RunCmd(PLUG_MGR_S *mgr, char *plug_name, char *file,
        char *conf_path, char *save_path, char *cfgfile)
{
    PLUG_MGR_NODE_S *node;
    
	node = PlugMgr_Find(mgr, plug_name);
    if (NULL == node) {
		return;
    }

    CHAR buf[FILE_MAX_PATH_LEN + 1];
    scnprintf(buf, sizeof(buf), "%s/%s", save_path, cfgfile);
    if (! FILE_IsFileExist(buf)) {
        scnprintf(buf, sizeof(buf), "%s/%s", conf_path, cfgfile);
        FILE_ChangeExternName(buf, "dft");
        if (! FILE_IsFileExist(buf)) {
            return;
        }
    }

    CMD_MNG_CmdRestoreByFile(0, buf);

    return;
}


static VOID plugmgr_EachLoadCfg(CFF_HANDLE hCff, char *tag, void *ud)
{
    USER_HANDLE_S *uh = ud;
    PLUG_MGR_S *mgr = uh->ahUserHandle[0];
    char *cfgfile = uh->ahUserHandle[1];
    char *file = NULL;
    char *conf_path = NULL;
    char *save_path = NULL;
	char ext_file[256];
	char ext_conf_path[256];
	char ext_save_path[256];

    CFF_GetPropAsString(hCff, tag, "file", &file);
    CFF_GetPropAsString(hCff, tag, "conf_path", &conf_path);
    CFF_GetPropAsString(hCff, tag, "save_path", &save_path);
    if ((file == NULL) || (conf_path == NULL)){
        return;
    }
    if (save_path == NULL) {
        save_path = conf_path;
    }

	SYSINFO_ExpandWorkDir(ext_file, sizeof(ext_file), file);
	SYSINFO_ExpandWorkDir(ext_conf_path, sizeof(ext_conf_path), conf_path);
	SYSINFO_ExpandWorkDir(ext_save_path, sizeof(ext_save_path), save_path);

    plugmgr_RunCmd(mgr, tag, ext_file, ext_conf_path, ext_save_path, cfgfile);
}

int PlugMgr_LoadByCff(PLUG_MGR_S *mgr, CFF_HANDLE hCff)
{
    USER_HANDLE_S ud;

    mgr->stage = PLUG_STAGE_PLUG_LOAD;
    ud.ahUserHandle[0] = mgr;
    CFF_WalkTag(hCff, plugmgr_EachLoad, &ud);

    mgr->stage = PLUG_STAGE_PLUG_LOAD;
    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_PLUG_LOAD);
    CFF_WalkTag(hCff, plugmgr_NotifyStage, &ud);

    mgr->stage = PLUG_STAGE_PLUG_LOADED;
    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_PLUG_LOADED);
    CFF_WalkTag(hCff, plugmgr_NotifyStage, &ud);

    mgr->stage = PLUG_STAGE_CMD_REG0;
    ud.ahUserHandle[1] = "cmd0.lst";
    CFF_WalkTag(hCff, plugmgr_EachRegCmd, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CMD_REG0);
    CFF_WalkTag(hCff, plugmgr_NotifyStage, &ud);

    mgr->stage = PLUG_STAGE_CMD_REGED0;
    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CMD_REGED0);
    CFF_WalkTag(hCff, plugmgr_NotifyStage, &ud);

    mgr->stage = PLUG_STAGE_CMD_REG;
    ud.ahUserHandle[1] = "cmd.lst";
    CFF_WalkTag(hCff, plugmgr_EachRegCmd, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CMD_REG);
    CFF_WalkTag(hCff, plugmgr_NotifyStage, &ud);

    mgr->stage = PLUG_STAGE_CMD_REGED;
    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CMD_REGED);
    CFF_WalkTag(hCff, plugmgr_NotifyStage, &ud);

    mgr->stage = PLUG_STAGE_CFG0_LOAD;
    ud.ahUserHandle[1] = "config0.cfg";
    CFF_WalkTag(hCff, plugmgr_EachLoadCfg, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CFG0_LOAD);
    CFF_WalkTag(hCff, plugmgr_NotifyStage, &ud);

    mgr->stage = PLUG_STAGE_CFG0_LOADED;
    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CFG0_LOADED);
    CFF_WalkTag(hCff, plugmgr_NotifyStage, &ud);

    mgr->stage = PLUG_STAGE_CFG_LOAD;
    ud.ahUserHandle[1] = "config.cfg";
    CFF_WalkTag(hCff, plugmgr_EachLoadCfg, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CFG_LOAD);
    CFF_WalkTag(hCff, plugmgr_NotifyStage, &ud);

    mgr->stage = PLUG_STAGE_CFG_LOADED;
    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CFG_LOADED);
    CFF_WalkTag(hCff, plugmgr_NotifyStage, &ud);

    mgr->stage = PLUG_STAGE_RUNNING;
    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_RUNNING);
    CFF_WalkTag(hCff, plugmgr_NotifyStage, &ud);

    return 0;
}

int PlugMgr_LoadManual(PLUG_MGR_S *mgr, char *ini_file, char *tag)
{
    USER_HANDLE_S ud;

    if (PlugMgr_Find(mgr, tag)) {
        return 0;
    }

    CFF_HANDLE hCff = CFF_INI_Open(ini_file, 0);
    if (hCff == NULL) {
        RETURN(BS_ERR);
    }

    plugmgr_LoadOne(mgr, hCff, tag);

    ud.ahUserHandle[0] = mgr;
    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_PLUG_LOAD);
    plugmgr_NotifyStage(hCff, tag, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_PLUG_LOADED);
    plugmgr_NotifyStage(hCff, tag, &ud);

    ud.ahUserHandle[1] = "cmd0.lst";
    plugmgr_EachRegCmd(hCff, tag, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CMD_REG0);
    plugmgr_NotifyStage(hCff, tag, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CMD_REGED0);
    plugmgr_NotifyStage(hCff, tag, &ud);

    ud.ahUserHandle[1] = "cmd.lst";
    plugmgr_EachRegCmd(hCff, tag, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CMD_REG);
    plugmgr_NotifyStage(hCff, tag, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CMD_REGED);
    plugmgr_NotifyStage(hCff, tag, &ud);

    ud.ahUserHandle[1] = "config0.cfg";
    plugmgr_EachLoadCfg(hCff, tag, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CFG0_LOAD);
    plugmgr_NotifyStage(hCff, tag, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CFG0_LOADED);
    plugmgr_NotifyStage(hCff, tag, &ud);

    ud.ahUserHandle[1] = "config.cfg";
    plugmgr_EachLoadCfg(hCff, tag, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CFG_LOAD);
    plugmgr_NotifyStage(hCff, tag, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CFG_LOADED);
    plugmgr_NotifyStage(hCff, tag, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_RUNNING);
    plugmgr_NotifyStage(hCff, tag, &ud);

    CFF_Close(hCff);

    return 0;
}

BS_STATUS PlugMgr_LoadByCfgFile(PLUG_MGR_S *mgr, char *cfg_file)
{
    CFF_HANDLE hCff = CFF_INI_Open(cfg_file, 0);
    if (hCff == NULL) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    PlugMgr_LoadByCff(mgr, hCff);

    CFF_Close(hCff);

    return BS_OK;
}

int PlugMgr_Unload(PLUG_MGR_S *mgr, char *ini_file, char *plug_name)
{
    USER_HANDLE_S ud;

    PLUG_MGR_NODE_S *node = PlugMgr_Find(mgr, plug_name);
    if (! node) {
        RETURN(BS_NOT_FOUND);
    }

    CFF_HANDLE hCff = CFF_INI_Open(ini_file, 0);
    if (hCff == NULL) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    int hot_remove = 0;
    CFF_GetPropAsInt(hCff, plug_name, "hot_remove", &hot_remove);
    if (! hot_remove) {
        EXEC_OutString("Not support hot remove \r\n");
        RETURN(BS_NOT_SUPPORT);
    }

    ud.ahUserHandle[0] = mgr;
    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_STOP);
    plugmgr_NotifyStage(hCff, plug_name, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_CMD_UNREG);
    plugmgr_NotifyStage(hCff, plug_name, &ud);

    ud.ahUserHandle[1] = "cmd.lst";
    plugmgr_EachUnRegCmd(hCff, plug_name, &ud);

    ud.ahUserHandle[1] = "cmd0.lst";
    plugmgr_EachUnRegCmd(hCff, plug_name, &ud);

    ud.ahUserHandle[1] = UINT_HANDLE(PLUG_STAGE_PLUG_UNLOAD);
    plugmgr_NotifyStage(hCff, plug_name, &ud);
    plugmgr_Unload(mgr, node);

    CFF_Close(hCff);

    return 0;
}

PLUG_MGR_NODE_S * PlugMgr_Next(PLUG_MGR_S *mgr, PLUG_MGR_NODE_S *curr)
{
    if (! curr) {
        return DLL_FIRST(&mgr->plug_list);
    }

    return DLL_NEXT(&mgr->plug_list, curr);
}

int PlugMgr_GetStage(PLUG_MGR_S *mgr)
{
    return mgr->stage;
}

char * PlugMgr_GetSavePathByEnv(void *env)
{
    PLUG_MGR_ENV_S *plug_env  = env;
    CFF_HANDLE hCff = plug_env->hCff;
    char *save_path = NULL;

    CFF_GetPropAsString(hCff, plug_env->tag, "save_path", &save_path);

    if (save_path == NULL) {
        CFF_GetPropAsString(hCff, plug_env->tag, "conf_path", &save_path);
    }

    return save_path;
}

