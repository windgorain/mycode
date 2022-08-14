/*================================================================
*   Created by LiXingang
*   Description: 执行命令,并将命令结果和输出存在CMD_BUF_S中
*
================================================================*/
#include "bs.h"
#include "utl/string_utl.h"
#include "utl/exec_utl.h"
#include "utl/cmd_buf.h"

typedef struct {
    HANDLE exec;
    HANDLE cmd_runner;
    HSTRING hbuf;
    int ret;
}CMD_BUF_S;

static void cmd_buf_send(HANDLE hExec, CHAR *msg)
{
    CMD_BUF_S *cmd_buf = EXEC_GetUD(hExec, 0);
    STRING_CatFromBuf(cmd_buf->hbuf, msg);
    return;
}

CMD_BUF_HDL CmdBuf_Create()
{
    CMD_BUF_S *cmd_buf = MEM_ZMalloc(sizeof(CMD_BUF_S));

    cmd_buf->cmd_runner = CMD_EXP_CreateRunner(CMD_EXP_RUNNER_TYPE_NONE);
    if (! cmd_buf->cmd_runner) {
        CmdBuf_Destroy(cmd_buf);
        return NULL;
    }

    cmd_buf->exec = EXEC_Create(cmd_buf_send, NULL);
    if (! cmd_buf->exec) {
        CmdBuf_Destroy(cmd_buf);
        return NULL;
    }

    cmd_buf->hbuf = STRING_Create();
    if (! cmd_buf->hbuf) {
        CmdBuf_Destroy(cmd_buf);
        return NULL;
    }

    EXEC_SetUD(cmd_buf->exec, 0, cmd_buf);

    return cmd_buf;
}

void CmdBuf_Destroy(CMD_BUF_HDL hdl)
{
    CMD_BUF_S *cmd_buf = hdl;

    if (! cmd_buf) {
        return;
    }
    if (cmd_buf->hbuf) {
        STRING_Delete(cmd_buf->hbuf);
    }
    if (cmd_buf->exec) {
        EXEC_Delete(cmd_buf->exec);
    }
    if (cmd_buf->cmd_runner) {
        CMD_EXP_DestroyRunner(cmd_buf->cmd_runner);
    }
    MEM_Free(cmd_buf);
}

int CmdBuf_RunCmd(CMD_BUF_HDL hdl, char *cmd)
{
    CMD_BUF_S *cmd_buf = hdl;
    HANDLE old;

    old = EXEC_GetExec();

    STRING_Clear(cmd_buf->hbuf);
    EXEC_Attach(cmd_buf->exec);
    CmdExp_ResetRunner(cmd_buf->cmd_runner);
    cmd_buf->ret = CmdExp_RunLine(cmd_buf->cmd_runner, cmd);
    EXEC_Detach(cmd_buf->exec);
    EXEC_Attach(old);

    return cmd_buf->ret;
}

int CmdBuf_Ret(CMD_BUF_HDL hdl)
{
    CMD_BUF_S *cmd_buf = hdl;
    return cmd_buf->ret;
}

char * CmdBuf_Buf(CMD_BUF_HDL hdl)
{
    CMD_BUF_S *cmd_buf = hdl;
    return STRING_GetBuf(cmd_buf->hbuf);
}

