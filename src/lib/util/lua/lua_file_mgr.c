/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/lua_file_mgr.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

static struct lua_State * luafm_init_lua(LUA_FILE_S *luaf)
{
    struct lua_State *L = luaL_newstate();
    int error;

    if (! L) {
        return NULL;
    }

    luaL_openlibs(L);

    error = luaL_dofile(L, luaf->filename);
    if (error) {
        lua_close(L);
        return NULL;
    }

    return L;
}

static inline void luafm_fini_lua(struct lua_State *L)
{
    lua_close(L);
}

int LuaFM_Init(LUA_FM_S * lfm)
{
    DLL_INIT(&lfm->list);
    return 0;
}

int luaFM_Add(LUA_FM_S *lfm, LUA_FILE_S *luaf)
{
    DLL_ADD(&lfm->list, &luaf->link_node);
    return 0;
}

void luaFM_Del(LUA_FM_S *lfm, LUA_FILE_S *luaf)
{
    DLL_DEL(&lfm->list, luaf);
}

LUA_FILE_S * LuaFM_Find(LUA_FM_S *lfm, char *file)
{
    LUA_FILE_S *luaf;

    DLL_SCAN(&lfm->list, luaf) {
        if (0 == strcmp(file, luaf->filename)) {
            return luaf;
        }
    }

    return NULL;
}

int LuaFM_MallocAndAdd(LUA_FM_S *lfm, char *file)
{
    LUA_FILE_S *luaf;

    luaf = MEM_ZMalloc(sizeof(LUA_FILE_S));
    if (!luaf) {
        RETURN(BS_NO_MEMORY);
    }

    luaf->filename = strdup(file);
    if (! luaf->filename) {
        MEM_Free(luaf);
        RETURN(BS_NO_MEMORY);
    }

    if (0 != luaFM_Add(lfm, luaf)) {
        MEM_Free(luaf->filename);
        MEM_Free(luaf);
        RETURN(BS_ERR);
    }

    return 0;
}

int LuaFM_DelAndFree(LUA_FM_S *lfm, char *file)
{
    LUA_FILE_S *luaf = LuaFM_Find(lfm, file);

    if (! luaf) {
        RETURN(BS_NOT_FOUND);
    }

    luaFM_Del(lfm, luaf);

    MEM_Free(luaf->filename);
    MEM_Free(luaf);

    return 0;
}

LUA_FILE_S * LuaFM_Next(LUA_FM_S *lfm, LUA_FILE_S *curr/*NULL表示获取第一个*/)
{
    if (! curr) {
        return DLL_FIRST(&lfm->list);
    }

    return DLL_NEXT(&lfm->list, &curr->link_node);
}

static int luafm_call(LUA_FM_S *lfm, LUA_FILE_S *file, LUA_ENV_S *env)
{
    int i;
    struct lua_State *L = luafm_init_lua(file);

    if (! L) {
        return -1;
    }

    lua_getglobal(L, env->func);

    /* 创建一个新的table并压入栈中 */
    lua_newtable(L);

    for (i=0; i<env->param_count; i++) {
        switch (env->params[i].type) {
            case LUA_FM_PARAM_TYPE_INT:
                lua_pushinteger(L, (int)HANDLE_UINT(env->params[i].value));
                break;
            case LUA_FM_PARAM_TYPE_STRING:
                lua_pushstring(L, env->params[i].value);
                break;
        }
        lua_setfield(L, -2, env->params[i].name);
    }

    /* 调用函数 */
    lua_pcall(L, 1, 1, 0);

    /* 获取栈顶元素(结果) */
    lua_tonumber(L, -1);

    /* 清除堆栈, 清除计算结果 */
    lua_pop(L, 1);

    luafm_fini_lua(L);

    return 0;
}

int LuaFM_EnvAddParam(LUA_ENV_S *env, int type, char *name, void *value)
{
    if (env->param_count >= LUA_FM_MAX_PARAM) {
        RETURN(BS_OUT_OF_RANGE);
    }

    env->params[env->param_count].type = type;
    env->params[env->param_count].name = name;
    env->params[env->param_count].value = value;
    env->param_count ++;

    return 0;
}

int LuaFM_Call(LUA_FM_S *lfm, LUA_ENV_S *env)
{
    LUA_FILE_S *file = NULL;

    while ((file = LuaFM_Next(lfm, file))) {
        luafm_call(lfm, file, env);
    }

    return 0;
}

