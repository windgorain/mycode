/*================================================================
*   Created by LiXingang
*   Description: length data
*
================================================================*/
#include "bs.h"
#include "utl/dfa_utl.h"

typedef struct {
    char *ptr; /* 当前执行位置的指针 */
    char **args;
    int max_count;
    int current_count;
}_ARGS_CTRL_S;

typedef enum {
    ARGS_STATE_LWS = 0,   /* 遇到了空白字符 */
    ARGS_STATE_CHAR,  /* 遇到了非空白非引号 */
    ARGS_STATE_DQUOT, /* 遇到了双引号 */
    ARGS_STATE_SQUOT, /* 遇到了单引号 */
    ARGS_STATE_TRANS, /* 遇到了转义符'\' */
}ARGS_STATE_E;

static void args_arg_start(DFA_HANDLE hDfa);
static void args_arg_end(DFA_HANDLE hDfa);
static void args_quot_start(DFA_HANDLE hDfa);

static ACTION_S g_args_actions[] =
{
    ACTION_LINE("arg_start", args_arg_start),
    ACTION_LINE("arg_end", args_arg_end),
    ACTION_LINE("quot_start", args_quot_start),
    ACTION_END
};

static DFA_NODE_S g_args_state_lws[] = 
{
    {DFA_CODE_LWS, DFA_STATE_SELF, NULL},
    {'\"', ARGS_STATE_DQUOT, "quot_start"},
    {'\'', ARGS_STATE_SQUOT, "quot_start"},
    {DFA_CODE_END, DFA_STATE_SELF, NULL},
    {DFA_CODE_OTHER, ARGS_STATE_CHAR, "arg_start"},
};

static DFA_NODE_S g_args_state_char[] = 
{
    {DFA_CODE_LWS, ARGS_STATE_LWS, "arg_end"},
    {DFA_CODE_END, DFA_STATE_SELF, "arg_end"},
};

static DFA_NODE_S g_args_state_dquot[] = 
{
    {'\\', ARGS_STATE_TRANS, NULL},
    {'\"', ARGS_STATE_LWS, "arg_end"},
};

static DFA_NODE_S g_args_state_squot[] = 
{
    {'\\', ARGS_STATE_TRANS, NULL},
    {'\'', ARGS_STATE_LWS, "arg_end"},
};

static DFA_NODE_S g_args_state_trans[] = 
{
    {DFA_CODE_OTHER, DFA_STATE_BACK, NULL},
};

static DFA_TBL_LINE_S g_args_dfa[] =
{
    DFA_TBL_LINE(g_args_state_lws),
    DFA_TBL_LINE(g_args_state_char),
    DFA_TBL_LINE(g_args_state_dquot),
    DFA_TBL_LINE(g_args_state_squot),
    DFA_TBL_LINE(g_args_state_trans),
    DFA_TBL_END
};

static void args_arg_start(DFA_HANDLE hDfa)
{
    _ARGS_CTRL_S *ctrl = DFA_GetUserData(hDfa);

    if (ctrl->current_count >= ctrl->max_count) {
        return;
    }

    ctrl->args[ctrl->current_count] = ctrl->ptr;
    ctrl->current_count ++;
}

static void args_arg_end(DFA_HANDLE hDfa)
{
    _ARGS_CTRL_S *ctrl = DFA_GetUserData(hDfa);
    *ctrl->ptr = '\0';
}

static void args_quot_start(DFA_HANDLE hDfa)
{
    _ARGS_CTRL_S *ctrl = DFA_GetUserData(hDfa);

    if (ctrl->current_count >= ctrl->max_count) {
        return;
    }

    ctrl->args[ctrl->current_count] = ctrl->ptr + 1;
    ctrl->current_count ++;
}

static void args_parser_init()
{
    static BOOL_T bInit = FALSE;

    if (bInit == FALSE) {
        bInit = TRUE;
        DFA_Compile(g_args_dfa, g_args_actions); 
    }
}

/* 按照命令行分割argc/argv的方式,将数据分隔成多段, 返回分割成了多少个段 */
int ARGS_Split
(
    char *string,
    char **args,   /* 指向数组 */
    int max_count/* 数组中元素个数 */
)
{
    char *c;
    DFA_S dfa;
    _ARGS_CTRL_S ctrl = {0};

    args_parser_init();
    DFA_Init(&dfa, g_args_dfa, g_args_actions, ARGS_STATE_LWS); /* 初始字符串,和前面有空白字符的字符串等价 */

    ctrl.args = args;
    ctrl.max_count = max_count;
    DFA_SetUserData(&dfa, &ctrl);

    for (c=string; *c!='\0'; c++) {
        ctrl.ptr = c;
        DFA_InputChar(&dfa, *c);
    }

    ctrl.ptr = c;
    DFA_Input(&dfa, DFA_CODE_END);

    return ctrl.current_count;
}

