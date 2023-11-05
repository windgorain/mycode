/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-5-24
* Description: 
* History:     
******************************************************************************/


#define RETCODE_FILE_NUM RETCODE_FILE_NUM_CMD_CFG

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/file_utl.h"
#include "utl/cmd_lst.h"

#include "cmd_func.h"



#define _DEF_NAME_AND_ELEMENT(element)  {#element, (PF_CMD_EXP_RUN)element}
#define _DEF_CMD_CFG_FILE_PATH  "base.lst"



typedef struct
{
    CHAR            *pcFuncName;
    PF_CMD_EXP_RUN pfFunc;
}_CMD_CFG_NODE_S;

typedef struct {
    DLL_NODE_S link_node;
    _CMD_CFG_NODE_S cmd_cfg;
}_CMD_CFG_LINK_NODE_S;

static DLL_HEAD_S g_cmd_cfg_list = DLL_HEAD_INIT_VALUE(&g_cmd_cfg_list);


static _CMD_CFG_NODE_S g_stCmdCfg[] = 
{
    _DEF_NAME_AND_ELEMENT(THREAD_Display),
    _DEF_NAME_AND_ELEMENT(SSHOW_ShowAll),
    _DEF_NAME_AND_ELEMENT(SSHOW_ShowTcp),
    _DEF_NAME_AND_ELEMENT(SSHOW_ShowUdp),
    _DEF_NAME_AND_ELEMENT(MemDebug_Check),
    _DEF_NAME_AND_ELEMENT(MemDebug_ShowSizeOfMem),
    _DEF_NAME_AND_ELEMENT(MemDebug_ShowLineConflict),
    _DEF_NAME_AND_ELEMENT(MEM_ShowStat),
    _DEF_NAME_AND_ELEMENT(MEM_ShowSizeOfMemStat),
    _DEF_NAME_AND_ELEMENT(SPLX_Display),
    _DEF_NAME_AND_ELEMENT(CMD_EXP_CmdShow),
    _DEF_NAME_AND_ELEMENT(CMD_EXP_CmdNoDebugAll),
    _DEF_NAME_AND_ELEMENT(CMD_EXP_CmdSave),
    _DEF_NAME_AND_ELEMENT(EXEC_TM),
    _DEF_NAME_AND_ELEMENT(EXEC_NoTM),
    _DEF_NAME_AND_ELEMENT(SYSINFO_Show),
    _DEF_NAME_AND_ELEMENT(PLUGCT_LoadPlug),
    _DEF_NAME_AND_ELEMENT(PLUGCT_ShowPlug),
    _DEF_NAME_AND_ELEMENT(ProcessKey_ShowStatus),
    {NULL, NULL}
};



static void cmd_cfg_RegLocalCfg()
{
    _CMD_CFG_NODE_S *node;

    node = g_stCmdCfg;

    while (node->pcFuncName != NULL) {
        CMD_CFG_RegFunc(node->pcFuncName, node->pfFunc);
        node ++;
    }
}

static PF_CMD_EXP_RUN cmd_cfg_GetFunc(IN PLUG_HDL ulPlugId, IN CHAR *pszFuncName)
{
    _CMD_CFG_LINK_NODE_S *node;
    PF_CMD_EXP_RUN pfFunc = NULL;

    if (0 != ulPlugId) {
        pfFunc = (PF_CMD_EXP_RUN)PLUG_GET_FUNC_BY_NAME(ulPlugId, pszFuncName);
    }

    if (NULL == pfFunc) { 
        DLL_SCAN(&g_cmd_cfg_list, node) {
            if (strcmp(node->cmd_cfg.pcFuncName, pszFuncName) == 0) {
                pfFunc = node->cmd_cfg.pfFunc;
                break;
            }
        }
    }

    return pfFunc;
}

static int cmd_cfg_RegSave(char *save_path, CMD_EXP_REG_CMD_PARAM_S *param)
{
    return CMD_EXP_RegSave(save_path, param);
}

static int cmd_cfg_RegEnter(CMD_EXP_REG_CMD_PARAM_S *param)
{
    return CMD_EXP_RegEnter(param);
}

static int cmd_cfg_UnRegSave(char *save_path, char *pcView)
{
    return CMD_EXP_UnRegSave(save_path, pcView);
}

BS_STATUS CMD_CFG_RegFunc(char *func_name, PF_CMD_EXP_RUN func)
{
    _CMD_CFG_LINK_NODE_S *node = MEM_ZMalloc(sizeof(_CMD_CFG_LINK_NODE_S));

    if (node == NULL) {
        RETURN(BS_NO_MEMORY);
    }

    node->cmd_cfg.pcFuncName = func_name;
    node->cmd_cfg.pfFunc = func;

    DLL_ADD(&g_cmd_cfg_list, node);

    return BS_OK;
}

typedef struct {
    CMDLST_S cmdlst;
    char *save_path;
    PLUG_HDL plug;
}CMD_CFG_S;

static int cmd_cfg_RegLine(void *cmdlst, CMDLST_ELE_S *ele)
{
    CMD_CFG_S *cfg = container_of(cmdlst, CMD_CFG_S, cmdlst);
    PF_CMD_EXP_RUN func = NULL;

    if (strcmp(ele->func_name, "NULL") != 0) {
        func = cmd_cfg_GetFunc(cfg->plug, ele->func_name);
        if (NULL == func) {
            BS_WARNNING(("Can't get function %s", ele->func_name));
            RETURN(BS_ERR);
        }
    }

    CMD_EXP_REG_CMD_PARAM_S stCmdParam = {0};
    stCmdParam.uiType = ele->type;
    stCmdParam.uiProperty = ele->property;
    stCmdParam.level= ele->level;
    stCmdParam.pcViews = ele->view;
    stCmdParam.pcCmd = ele->cmd;
    stCmdParam.pcViewName = ele->view_name;
    stCmdParam.pfFunc = func;
    if (ele->param) {
        INT64 var;
        TXT_Atoll(ele->param, &var);
        stCmdParam.hParam = (HANDLE)(ULONG)var;
    }

    if (CMD_EXP_IS_SAVE(ele->type)) {
        return cmd_cfg_RegSave(cfg->save_path, &stCmdParam);
    } else if (ele->type == DEF_CMD_EXP_TYPE_ENTER) {
        return cmd_cfg_RegEnter(&stCmdParam);
    } else {
        return CMD_EXP_RegCmd(&stCmdParam);
    }
}

static int cmd_cfg_UnRegLine(void *cmdlst, CMDLST_ELE_S *ele)
{
    CMD_CFG_S *cfg = container_of(cmdlst, CMD_CFG_S, cmdlst);

    if (CMD_EXP_IS_SAVE(ele->type)) {
        return cmd_cfg_UnRegSave(cfg->save_path, ele->view);
    } else {
        CMD_EXP_REG_CMD_PARAM_S stCmdParam = {0};
        stCmdParam.uiType = ele->type;
        stCmdParam.pcViews = ele->view;
        stCmdParam.pcCmd = ele->cmd;
        return CMD_EXP_UnregCmd(&stCmdParam);
    }
}
 
BS_STATUS CMD_CFG_RegCmd(CHAR *pszFileName, PLUG_HDL plug, char *save_path)
{
    CMD_CFG_S cfg;

    cfg.cmdlst.line_func = cmd_cfg_RegLine;
    cfg.plug = plug;
    cfg.save_path = save_path;

    return CMDLST_ScanByFile(&cfg.cmdlst, pszFileName);
}

BS_STATUS CMD_CFG_UnRegCmd(CHAR *pszFileName, char *save_path)
{
    CMD_CFG_S cfg;

    cfg.cmdlst.line_func = cmd_cfg_UnRegLine;
    cfg.save_path = save_path;

    return CMDLST_ScanByFile(&cfg.cmdlst, pszFileName);
}


BS_STATUS CMD_CFG_Init()
{
    char buf1[128];
    char buf2[128];

    cmd_cfg_RegLocalCfg();

    return CMD_CFG_RegCmd(SYSINFO_ExpandConfPath(buf1, sizeof(buf1), _DEF_CMD_CFG_FILE_PATH), 0,
            SYSINFO_ExpandConfPath(buf2, sizeof(buf2), "save"));
}


