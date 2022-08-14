/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/subcmd_utl.h"
#include "utl/cmd_agent.h"

static int cmd_agent_cmd_show(CMD_AGENT_S *cmd_agent, int argc, char **argv, void *ud)
{
    char buf[256];
    CMD_AGENT_OB_S *ob;

    DLL_SCAN(&cmd_agent->ob_list, ob) {
        if (ob->enabled) {
            scnprintf(buf, sizeof(buf), " %-16s Enabled\r\n", ob->ob_name);
        } else {
            scnprintf(buf, sizeof(buf), " %-16s \r\n", ob->ob_name);
        }
        cmd_agent->print_func(buf, ud);
    }

    return 0;
}

static CMD_AGENT_OB_S * cmd_agent_find(CMD_AGENT_S *cmd_agent, char *ob_name)
{
    CMD_AGENT_OB_S *ob;

    DLL_SCAN(&cmd_agent->ob_list, ob) {
        if (0 == strcmp(ob->ob_name, ob_name)) {
            return ob;
        }
    }

    return NULL;
}

static int cmd_agent_ob_action(CMD_AGENT_S *cmd_agent, int argc, char **argv, void *ud)
{
    CMD_AGENT_OB_S *ob;

    if (argc < 2) {
        cmd_agent->print_func("Need OB name\r\n", ud);
        return -1;
    }

    ob = cmd_agent_find(cmd_agent, argv[1]);
    if (! ob) {
        cmd_agent->print_func("Can't find the OB.\r\n", ud);
        return -1;
    }

    return ob->action_func(argv[0], argc-1, argv+1, ud);
}

static int cmd_agent_cmd_enable(CMD_AGENT_S *cmd_agent, int argc, char **argv, void *ud)
{
    int ret;
    CMD_AGENT_OB_S *ob;

    ret = cmd_agent_ob_action(cmd_agent, argc, argv, ud);
    if (ret == 0) {
        ob = cmd_agent_find(cmd_agent, argv[1]);
        ob->enabled = 1;
    }

    return ret;
}

static int cmd_agent_cmd_disable(CMD_AGENT_S *cmd_agent, int argc, char **argv, void *ud)
{
    int ret;
    CMD_AGENT_OB_S *ob;

    ret = cmd_agent_ob_action(cmd_agent, argc, argv, ud);
    if (ret == 0) {
        ob = cmd_agent_find(cmd_agent, argv[1]);
        ob->enabled = 0;
    }

    return ret;
}

static int cmd_agent_add_cmd(CMD_AGENT_S *cmd_agent, char *cmd, char *help, void *func)
{
    int i;
    SUB_CMD_NODE_S *node;

    for (i=0; i<CMD_AGENT_MAX_SUB_CMD; i++) {
        node = &cmd_agent->sub_cmds[i];
        if (node->subcmd) {
            continue;
        }

        node->subcmd = cmd;
        node->help = help;
        node->func = func;
        return 0;
    }

    RETURN(BS_FULL);
}

static int cmd_agent_default_print(char *buf, void *ud)
{
    printf("%s", buf);
    return 0;
}

int CmdAgent_Init(CMD_AGENT_S *cmd_agent)
{
    memset(cmd_agent, 0, sizeof(CMD_AGENT_S));
    DLL_INIT(&cmd_agent->ob_list);
    cmd_agent->print_func = cmd_agent_default_print;

    /* cmd agent的内置管理命令 */
    cmd_agent_add_cmd(cmd_agent, "show", "Show OB list", cmd_agent_cmd_show);
    cmd_agent_add_cmd(cmd_agent, "enable", "Enable OB", cmd_agent_cmd_enable);
    cmd_agent_add_cmd(cmd_agent, "disable", "DisableOB", cmd_agent_cmd_disable);

    return 0;
}

void CmdAgent_SetPrintFunc(CMD_AGENT_S *cmd_agent, PF_CMD_AGENT_PRINT print_func)
{
    cmd_agent->print_func = print_func;
}

/* action/help 不能是临时内存 */
int CmdAgent_RegCmd(CMD_AGENT_S *cmd_agent, char *action, char *help)
{
    return cmd_agent_add_cmd(cmd_agent, action, help, cmd_agent_ob_action);
}

/* action/help 不能是临时内存 */
int CmdAgent_RegCmdExt(CMD_AGENT_S *cmd_agent, char *action, char *help, PF_CMD_AGENT_SUBCMD func)
{
    return cmd_agent_add_cmd(cmd_agent, action, help, func);
}

/* ob不能是临时内存 */
void CmdAgent_RegOB(CMD_AGENT_S *cmd_agent, CMD_AGENT_OB_S *ob)
{
    if (DLL_IN_LIST(&ob->link_node)) {
        return;
    }

    DLL_ADD(&cmd_agent->ob_list, &ob->link_node);
}

/* 注册多个ob, 每个ob不能是临时内存 */
void CmdAgent_RegOBs(CMD_AGENT_S *cmd_agent, CMD_AGENT_OB_S *obs/*多个ob,以{0}结束*/)
{
    CMD_AGENT_OB_S *ob;

    for (ob=obs; ob->ob_name!=NULL; ob++) {
        CmdAgent_RegOB(cmd_agent, ob);
    }
}

void CmdAgent_UnRegOB(CMD_AGENT_S *cmd_agent, CMD_AGENT_OB_S *ob)
{
    if (DLL_IN_LIST(&ob->link_node)) {
        DLL_DEL(&cmd_agent->ob_list, &ob->link_node);
    }
}

int CmdAgent_Cmd(CMD_AGENT_S *cmd_agent, int argc, char **argv, void *ud)
{
    return SUBCMD_Do(cmd_agent->sub_cmds, argc, argv);
}

CMD_AGENT_OB_S * CmdAgent_GetNext(CMD_AGENT_S *cmd_agent, CMD_AGENT_OB_S *curr/* NULL获取第一个 */)
{
    if (! curr) {
        return DLL_FIRST(&cmd_agent->ob_list);
    }

    return DLL_NEXT(&cmd_agent->ob_list, curr);
}

int CmdAgent_AllDoAction(CMD_AGENT_S *cmd_agent, char *action, void *ud)
{
    CMD_AGENT_OB_S *ob = NULL;
    char buf[256];

    while((ob = CmdAgent_GetNext(cmd_agent, ob))) {
        if (0 != ob->action_func(action, 0, 0, ud)) {
            scnprintf(buf, sizeof(buf), "ob %s %s failed\r\n", ob->ob_name, action);
            cmd_agent->print_func(buf, ud);
        }
    }

    return 0;
}

