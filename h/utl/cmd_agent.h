/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _CMD_AGENT_H
#define _CMD_AGENT_H
#include "utl/dll_utl.h"
#include "utl/subcmd_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*PF_CMD_AGENT_PRINT)(char *buf, void *ud);
typedef int (*PF_CMD_AGENT_ACTION)(char *action, int argc, char **argv, void *ud);

#define CMD_AGENT_OB_DEF(name,func) {{0}, (name), (func)}

typedef struct {
    DLL_NODE_S link_node;
    char *ob_name;
    PF_CMD_AGENT_ACTION action_func;
    unsigned int enabled:1;
}CMD_AGENT_OB_S;

#define CMD_AGENT_MAX_SUB_CMD 255
typedef struct {
    DLL_HEAD_S ob_list;
    PF_CMD_AGENT_PRINT print_func;
    SUB_CMD_NODE_S sub_cmds[CMD_AGENT_MAX_SUB_CMD + 1];
}CMD_AGENT_S;

typedef int (*PF_CMD_AGENT_SUBCMD)(CMD_AGENT_S *cmd_agent, int argc, char **argv, void *ud);

int CmdAgent_Init(CMD_AGENT_S *cmd_agent);
void CmdAgent_SetPrintFunc(CMD_AGENT_S *cmd_agent, PF_CMD_AGENT_PRINT print_func);
/* action/help 不能是临时内存 */
int CmdAgent_RegCmd(CMD_AGENT_S *cmd_agent, char *action, char *help);
/* action/help 不能是临时内存 */
int CmdAgent_RegCmdExt(CMD_AGENT_S *cmd_agent, char *action, char *help, PF_CMD_AGENT_SUBCMD func);
/* ob不能是临时内存 */
void CmdAgent_RegOB(CMD_AGENT_S *cmd_agent, CMD_AGENT_OB_S *ob);
/* 注册多个ob, 每个ob不能是临时内存 */
void CmdAgent_RegOBs(CMD_AGENT_S *cmd_agent, CMD_AGENT_OB_S *obs/*多个ob,以{0}结束*/);
void CmdAgent_UnRegOB(CMD_AGENT_S *cmd_agent, CMD_AGENT_OB_S *ob);
int CmdAgent_Cmd(CMD_AGENT_S *cmd_agent, int argc, char **argv, void *ud);
CMD_AGENT_OB_S * CmdAgent_GetNext(CMD_AGENT_S *cmd_agent, CMD_AGENT_OB_S *curr/* NULL获取第一个 */);

#ifdef __cplusplus
}
#endif
#endif //CMD_AGENT_H_
