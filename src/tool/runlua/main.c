/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/subcmd_utl.h"
#include "utl/getopt2_utl.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

static int runlua_file(int argc, char **argv);
static int runlua_string(int argc, char **argv);
static int runlua_run(int argc, char **argv);

static SUB_CMD_NODE_S g_subcmds[] = 
{
    {"file", runlua_file, "run lua file"},
    {"string", runlua_string, "run lua string"},
    {"run", runlua_run, "run lua with stdin"},
    {NULL, NULL}
};

static void runlua_opt_help(GETOPT2_NODE_S *opt)
{
    char buf[512];
    printf("%s", GETOPT2_BuildHelpinfo(opt, buf, sizeof(buf)));
    return;
}

static void runlua_run_file(char *file)
{
    int error;
    struct lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    error = luaL_dofile(L, file);
    if (error) {
        fprintf(stderr, "%s\r\n", lua_tostring(L, -1));
        lua_pop(L, 1);/* pop error message from the stack */
    }
    lua_close(L);
}

static void runlua_run_string(char *str)
{
    struct lua_State *L = luaL_newstate();
    int error;

    luaL_openlibs(L);
    error = luaL_dostring(L, str);
    if (error) {
        fprintf(stderr, "%s\r\n", lua_tostring(L, -1));
        lua_pop(L, 1);/* pop error message from the stack */
    }
    lua_close(L);
}

static int runlua_run(int argc, char **argv)
{
    char buff[256];
    int error;
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    while (fgets(buff, sizeof(buff), stdin) != NULL) {
        error = luaL_dostring(L, buff);
        if (error) {
            fprintf(stderr, "%s", lua_tostring(L, -1));
            lua_pop(L, 1);/* pop error message from the stack */
        }
    }

    lua_close(L);
    return 0;
}

static int runlua_string(int argc, char **argv)
{
    static char *str=NULL;
    static GETOPT2_NODE_S opt[] = {
        {'o', 'h', "help", 0, NULL, NULL, 0},
        {'P', 0, "lua-string", 's', &str, NULL, 0},
        {0} };

    if (0 != GETOPT2_Parse(argc, argv, opt)) {
        runlua_opt_help(opt);
        return -1;
    }

    if (GETOPT2_IsOptSetted(opt, 'h', NULL)) {
        runlua_opt_help(opt);
        return 0;
    }

    runlua_run_string(str);

    return 0;
}

static int runlua_file(int argc, char **argv)
{
    static char *filename=NULL;
    static GETOPT2_NODE_S opt[] = {
        {'o', 'h', "help", 0, NULL, NULL, 0},
        {'p', 0, "lua-file", 's', &filename, NULL, 0},
        {0} };

    if (argc < 2) {
        runlua_opt_help(opt);
        return -1;
    }

    if (BS_OK != GETOPT2_Parse(argc, argv, opt)) {
        runlua_opt_help(opt);
        return -1;
    }

    if (GETOPT2_IsOptSetted(opt, 'h', NULL)) {
        runlua_opt_help(opt);
        return 0;
    }

    if (filename == NULL) {
        runlua_opt_help(opt);
        return -1;
    }

    runlua_run_file(filename);

    return 0;
}

int main(int argc, char **argv)
{
    return SUBCMD_Do(g_subcmds, argc, argv);
}


