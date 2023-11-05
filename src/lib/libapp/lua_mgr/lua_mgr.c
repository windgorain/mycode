/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-18
* Description: Lua Manger
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/file_utl.h"
#include "utl/lua_file_mgr.h"

#define LUA_MGR_MAX 32
#define LUA_MGR_NAME_SIZE 32

typedef struct {
    UINT valid:1;
    UINT disable:1;
    LUA_FM_S luafm;
    char name[LUA_MGR_NAME_SIZE];
}LUA_GROUP_S;

static LUA_GROUP_S g_lua_groups[LUA_MGR_MAX];

static inline LUA_GROUP_S * luamgr_get_group_by_str(char *str)
{
    UINT index;
    TXT_Atoui(str, &index);
    return &g_lua_groups[index];
}

static inline LUA_GROUP_S * luamgr_get_group(void *env)
{
    return luamgr_get_group_by_str(CMD_EXP_GetCurrentModeValue(env));
}

static void luamgr_load_file(LUA_GROUP_S *group, char *filename, int is_load)
{
    if (is_load) {
        LuaFM_MallocAndAdd(&group->luafm, filename);
    } else {
        LuaFM_DelAndFree(&group->luafm, filename);
    }
}

static int luamgr_load_unload(LUA_GROUP_S *group, char *filename, int is_load)
{
    if (is_load) {
        if (NULL != LuaFM_Find(&group->luafm, filename)) {
            EXEC_OutString("This file has been loaded\r\n");
            return 0;
        }

        if (! FILE_IsFileExist(filename)) {
            EXEC_OutString("Can't find the lua file\r\n");
            return -1;
        }
    }

    luamgr_load_file(group, filename, is_load);

    return 0;
}

static void luamgr_save_group(HANDLE hFile, LUA_GROUP_S *group)
{
    LUA_FILE_S *luafile = NULL;

    if (group->disable) {
        CMD_EXP_OutputCmd(hFile, "disable");
    }

    if (group->name[0]) {
        CMD_EXP_OutputCmd(hFile, "name %s", group->name);
    }

    while((luafile = LuaFM_Next(&group->luafm, luafile))) {
        CMD_EXP_OutputCmd(hFile, "load %s", luafile->filename);
    }
}


PLUG_API BS_STATUS LuaMgr_EnterGroup(int argc, char **argv, void *env)
{
    LUA_GROUP_S *group = luamgr_get_group_by_str(argv[1]);
    group->valid = 1;
    return 0;
}


PLUG_API BS_STATUS LuaMgr_NoGroup(int argc, char **argv, void *env)
{
    LUA_GROUP_S *group = luamgr_get_group_by_str(argv[1]);
    group->valid = 0;
    return 0;
}


PLUG_API BS_STATUS LuaMgr_SetName(int argc, char **argv, void *env)
{
    LUA_GROUP_S *group = luamgr_get_group(env);
    strlcpy(group->name, argv[1], LUA_MGR_NAME_SIZE);
    return 0;
}


PLUG_API BS_STATUS LuaMgr_Disable(int argc, char **argv, void *env)
{
    LUA_GROUP_S *group = luamgr_get_group(env);
    group->disable = 1;
    return 0;
}


PLUG_API BS_STATUS LuaMgr_Enable(int argc, char **argv, void *env)
{
    LUA_GROUP_S *group = luamgr_get_group(env);
    group->disable = 0;
    return 0;
}


PLUG_API int LuaMgr_LoadFile(int argc, char **argv, void *env)
{
    LUA_GROUP_S *group = luamgr_get_group(env);
    return luamgr_load_unload(group, argv[1], 1);
}


PLUG_API int LuaMgr_UnLoadFile(int argc, char **argv, void *env)
{
    LUA_GROUP_S *group = luamgr_get_group(env);
    return luamgr_load_unload(group, argv[2], 0);
}

PLUG_API BS_STATUS LuaMgr_Save(HANDLE hFile)
{
    int i;
    LUA_GROUP_S *group;

    for (i=0; i<LUA_MGR_MAX; i++) {
        group = &g_lua_groups[i];
        if (group->valid == 0) {
            continue;
        }

        if (0 == CMD_EXP_OutputMode(hFile, "group %d", i)) {
            luamgr_save_group(hFile, group);
            CMD_EXP_OutputModeQuit(hFile);
        }
    }

    return 0;
}

int LuaMgr_Init()
{
    int i;

    for (i=0; i<LUA_MGR_MAX; i++) {
        LuaFM_Init(&g_lua_groups[i].luafm);
    }

    return 0;
}

PLUG_API int LuaMgr_Call(int group_index, LUA_ENV_S *env)
{
    if (group_index >= LUA_MGR_MAX) {
        RETURN(BS_OUT_OF_RANGE);
    }

    LUA_GROUP_S *group = &g_lua_groups[group_index];

    if ((! group->valid) || (group->disable)) {
        return 0;
    }

    return LuaFM_Call(&group->luafm, env);
}

