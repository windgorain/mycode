/*================================================================
*   Created by LiXingang
*   Description: 插件管理
*
================================================================*/
#ifndef _PLUG_MGR_H
#define _PLUG_MGR_H
#include "utl/cff_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    PLUG_STAGE_PLUG_LOAD = 1,      /* 加载插件 */
    PLUG_STAGE_PLUG_LOADED,        /* 加载插件后 */
    PLUG_STAGE_CMD_REG0,           /* 首期命令注册 */
    PLUG_STAGE_CMD_REGED0,         /* 首期命令注册完成 */
    PLUG_STAGE_CMD_REG,            /* 命令注册 */
    PLUG_STAGE_CMD_REGED,          /* 命令注册完成 */
    PLUG_STAGE_CFG0_LOAD,          /* 首期配置加载 */
    PLUG_STAGE_CFG0_LOADED,        /* 首期配置加载完成 */
    PLUG_STAGE_CFG_LOAD,           /* 配置加载 */
    PLUG_STAGE_CFG_LOADED,         /* 配置加载完成 */
    PLUG_STAGE_RUNNING,            /* 运行状态 */
    PLUG_STAGE_STOP,               /* 停止 */
    PLUG_STAGE_CMD_UNREG,          /* 取消命令行注册 */
    PLUG_STAGE_PLUG_UNLOAD         /* 卸载插件 */
}PLUG_STAGE_E;

typedef int (*PF_PLUG_STAGE)(int stage, void *env);

/* 所有Plug 需要在文件中使用这个宏 */
#define PLUG_MAIN \
    PLUG_API int PlugMain (char *file, char *conf_path, char *save_path)  \
    {   \
        LOCAL_INFO_SetHost(file);    \
        LOCAL_INFO_SetConfPath(conf_path);    \
        LOCAL_INFO_SetSavePath(save_path);    \
        return 0; \
    }


typedef struct {
    DLL_NODE_S link_node; /* 必须为第一个成员 */
    char *plug_name;
    char *filename;
    PLUG_ID hPlug;
}PLUG_MGR_NODE_S;

typedef struct {
    DLL_HEAD_S plug_list;
    int stage;
}PLUG_MGR_S;

int PlugMgr_Init(PLUG_MGR_S *plug_mgr);
int PlugMgr_LoadByCfgFile(PLUG_MGR_S *plug_mgr, char *cfg_file);
int PlugMgr_LoadManual(PLUG_MGR_S *mgr, char *ini_file, char *tag);
int PlugMgr_Unload(PLUG_MGR_S *plug_mgr, char *ini_file, char *tag);
PLUG_MGR_NODE_S * PlugMgr_Find(PLUG_MGR_S *mgr, char *plug_name);
PLUG_MGR_NODE_S * PlugMgr_Next(PLUG_MGR_S *mgr, PLUG_MGR_NODE_S *curr/* NULL表示获取第一个 */);
int PlugMgr_GetStage(PLUG_MGR_S *mgr);
char * PlugMgr_GetSavePathByEnv(void *env);

#ifdef __cplusplus
}
#endif
#endif //PLUG_MGR_H_
