/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-5-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/sysrun_utl.h"
#include "utl/args_utl.h"
#include "utl/exec_utl.h"
#include "utl/sprintf_utl.h"
#include "utl/passwd_utl.h"
#include "utl/cff_utl.h"
#include "utl/ic_utl.h"
#include "utl/rbuf.h"
#include "utl/mutex_utl.h"
#include "utl/ascii_utl.h"
#include "utl/signal_utl.h"
#include "utl/file_utl.h"
#include "utl/bit_opt.h"
#include "utl/stack_utl.h"
#include "utl/exchar_utl.h"
#include "utl/cmd_exp.h"

#define _DEF_CMD_EXP_MAX_VIEW_NAME_LEN           23
#define _DEF_CMD_EXP_MAX_MODE_NAME_LEN           23

/* 选项相关 */
#define DEF_CMD_OPT_STR_PREFIX    "__OPT_"   /* 选项前缀 */
#define DEF_CMD_OPT_FLAG_STR_NOSAVE   'S'    /* 不Save */
#define DEF_CMD_OPT_FLAG_STR_NOSHOW   'H'    /* 不Show */
#define DEF_CMD_OPT_STR_END           '_'    /* 选项结束标记 */

/* ---define--- */
#define _DEF_CMD_EXP_MAX_MODE_VALUE_LEN 255

#define _DEF_CMD_EXP_MAX_CMD_ELEMENT_IN_ONE_CMD  10
#define _DEF_CMD_EXP_MAX_CMD_ELEMENT_LEN         23

/* 检测命令元素是否可执行 */
#define _DEF_CMD_EXP_IS_CMD_RUNABLE(_pstNode) \
    (((_pstNode)->pfFunc != NULL) || (CMD_EXP_IS_VIEW((_pstNode)->uiType)))

/* ---struct--- */
typedef struct cmdTree
{
    UINT uiType;  /* 命令类型 */
    UINT uiProperty; /* 属性 */
    CHAR acCmd[_DEF_CMD_EXP_MAX_CMD_ELEMENT_LEN + 1];  /* 命令元素 */
    CHAR szViewName[_DEF_CMD_EXP_MAX_VIEW_NAME_LEN + 1]; /* View名字 */
    CHAR *pcHelp;  /* 帮助信息 */
    PF_CMD_EXP_RUN pfFunc;  /*当匹配这条命令时，所执行的函数*/
    HANDLE hParam;

    struct cmdTree *pstSubCmdTree;  /* 指向下级命令的第一个节点 */
    struct cmdTree *pstNextCmdTree;
    struct cmdTree *pstParent;
}_CMD_EXP_TREE_S;



typedef enum
{
    _CMD_EXP_EXACTMACTH = 1,
    _CMD_EXP_PREMACTH = 2,
    _CMD_EXP_NOTMACTH = 3
}_CMD_EXP_MATCH_E;

typedef struct
{
    CHAR *pcCmd;
    CHAR *pcHelp;
    CHAR *pcNext;
}_CMD_EXP_ELEMENT_S;

typedef struct
{
    DLL_NODE_S   stDllNode;
    PF_CMD_EXP_SAVE pfFunc;
    CHAR szCfgFileName[FILE_MAX_PATH_LEN + 1];
}_CMD_EXP_SVAE_NODE_S;

typedef struct
{
    DLL_NODE_S stLinkNode;  /* 兄弟链表节点 */

    _CMD_EXP_TREE_S *pstView;

    DLL_HEAD_S stSaveFuncList; /* save命令响应接口 */

    /* 子View列表 */
    DLL_HEAD_S stSubViewList;
}_CMD_EXP_VIEW_TBL_NODE_S;

typedef struct
{
    _CMD_EXP_VIEW_TBL_NODE_S *pstViewTbl;
    CHAR szModeName[_DEF_CMD_EXP_MAX_MODE_NAME_LEN + 1];  /* 模式名 */
    CHAR szModeValue[_DEF_CMD_EXP_MAX_MODE_VALUE_LEN + 1];
    CHAR szCmd[DEF_CMD_EXP_MAX_CMD_LEN + 1];
}_CMD_EXP_NEW_MODE_NODE_S;

typedef struct {
    _CMD_EXP_VIEW_TBL_NODE_S stRoot;
    DLL_HEAD_S stNoDbgFuncList;
    MUTEX_S lock; /* 命令行锁 */
    UINT flag;
}CMD_EXP_S;

typedef struct
{
    CMD_EXP_S *cmdexp;
    _CMD_EXP_NEW_MODE_NODE_S *pstCurrentMode;
    CHAR acInputs[DEF_CMD_EXP_MAX_CMD_LEN + 1];
    UINT ulInputsLen;
    UINT uiInputsPos; /* 当前的光标距离字符串尾部的距离 */
    UINT ulHistoryIndex;
    UINT alt_mode:1; /* 是否交互模式 */
    HANDLE hCmdRArray;
    BOOL_T bIsRChangeToN;
    HANDLE hExcharHandle;
    HANDLE hModeStack;
}_CMD_EXP_RUNNER_S;

/* 命令执行的上下文环境 */
typedef struct
{
    _CMD_EXP_RUNNER_S *pstRunner; /* 所属的命令实例 */
    _CMD_EXP_TREE_S *pstCmdNode;    /* 触发动作的命令节点 */
}_CMD_EXP_ENV_S;

typedef struct
{
    HANDLE hStack;
    _CMD_EXP_ENV_S *pstEnv;
    FILE *fp;
    UCHAR bIsNewFile:1;
    UCHAR bHasCfg:1;      /* 是否有配置 */
    UCHAR bIsMatched:1;   /* 是否匹配了命令执行所在的模式或其子模式下 */
    UCHAR bIsShowAll:1;   /* 是否显示所有,如果为0的话,_OPT_S_的配置不显示 */
    UCHAR ucPrefixSpaceNum;   /* 前缀空格个数 */
    UCHAR ucMatchedModeIndex; /* 匹配到了hStack中的第几个Mode */
    UCHAR ucModeDeepth;       /* 当前show/save到了多深的Mode */
}_CMD_EXP_SHOW_SAVE_NODE_S;

typedef _CMD_EXP_MATCH_E (*PF_CMD_EXP_PATTERN_PARSE)(IN CHAR *pszCmdElement,
        IN CHAR *pszInputCmd);

typedef struct
{
    CHAR *pcPattern;
    PF_CMD_EXP_PATTERN_PARSE pfModeParse;
}_CMD_EXP_PATTERN_PARSE_S;


static _CMD_EXP_MATCH_E cmdexp_ParsePatternString(IN CHAR *pszCmdElement,
        IN CHAR *pszInputCmd);
static _CMD_EXP_MATCH_E cmdexp_ParsePatternInt(IN CHAR *pszCmdElement,
        IN CHAR *pszInputCmd);
static _CMD_EXP_MATCH_E cmdexp_ParsePatternIP(IN CHAR *pszCmdElement,
        IN CHAR *pszInputCmd);
static int cmdexp_QuitCmd(IN UINT ulArgc,
        IN CHAR **pArgv, IN VOID *pEnv);

static _CMD_EXP_PATTERN_PARSE_S g_astCmdExpParse[] =
{
    {"%STRING", cmdexp_ParsePatternString},
    {"%INT", cmdexp_ParsePatternInt},
    {"%IP", cmdexp_ParsePatternIP},
    {0}
};

/* 用户视图 */
static _CMD_EXP_TREE_S g_stCmdExpUserView = {
    DEF_CMD_EXP_TYPE_VIEW,
    0, DEF_CMD_EXP_VIEW_USER,
    DEF_CMD_EXP_VIEW_USER, "",
    NULL, NULL, NULL, NULL};


static _CMD_EXP_VIEW_TBL_NODE_S * cmdexp_FindViewFromViewTree
(
    IN _CMD_EXP_VIEW_TBL_NODE_S *pstViewTreeNode,
    IN CHAR *pcViewName
)
{
    _CMD_EXP_VIEW_TBL_NODE_S *pstNode;
    _CMD_EXP_VIEW_TBL_NODE_S *pstNodeFound;
    
    if (strcmp(pstViewTreeNode->pstView->szViewName, pcViewName) == 0)
    {
        return pstViewTreeNode;
    }

    DLL_SCAN(&(pstViewTreeNode->stSubViewList), pstNode)
    {
        pstNodeFound = cmdexp_FindViewFromViewTree(pstNode, pcViewName);
        if (NULL != pstNodeFound)
        {
            return pstNodeFound;
        }
    }

    return NULL;
}

static _CMD_EXP_VIEW_TBL_NODE_S *cmdexp_FindView(CMD_EXP_S *cmdexp,
        CHAR *pcViewName)
{
    return cmdexp_FindViewFromViewTree(&cmdexp->stRoot, pcViewName);
}

/* 进入模式 */
static int cmdexp_EnterMode
(
    IN _CMD_EXP_RUNNER_S *pstRunner,
    IN _CMD_EXP_TREE_S * pstCmdNode,
    IN UINT uiArgc,
    IN CHAR **ppcArgv
)
{
    _CMD_EXP_NEW_MODE_NODE_S *pstModeNode;
    _CMD_EXP_VIEW_TBL_NODE_S *pstViewTbl;
    CHAR *pcModeValue = ppcArgv[uiArgc - 1];
    UINT i;

    pstViewTbl = cmdexp_FindView(pstRunner->cmdexp, pstCmdNode->szViewName);
    if (NULL == pstViewTbl) {
        return BS_ERR;
    }

    pstModeNode = MEM_ZMalloc(sizeof(_CMD_EXP_NEW_MODE_NODE_S));
    if (NULL == pstModeNode) {
        return BS_NO_MEMORY;
    }

    pstModeNode->pstViewTbl = pstViewTbl;

    if (CMD_EXP_IS_TEMPLET(pstCmdNode->uiProperty)) {
        BS_Snprintf(pstModeNode->szModeName, sizeof(pstModeNode->szModeName),
                "%s-%s", pstCmdNode->szViewName, pcModeValue);
        TXT_Strlcpy(pstModeNode->szModeValue, pcModeValue,
                sizeof(pstModeNode->szModeValue));
    } else {
        TXT_Strlcpy(pstModeNode->szModeName, pstCmdNode->szViewName,
                sizeof(pstModeNode->szModeName));
    }

    for (i=0; i<uiArgc; i++) {
        if (i != 0) {
            TXT_Strlcat(pstModeNode->szCmd, " ", sizeof(pstModeNode->szCmd));
        }
        TXT_Strlcat(pstModeNode->szCmd, ppcArgv[i], sizeof(pstModeNode->szCmd));
    }

    if (pstRunner->pstCurrentMode != NULL) {
        if (BS_OK != HSTACK_Push(pstRunner->hModeStack,
                    pstRunner->pstCurrentMode)) {
            MEM_Free(pstModeNode);
            return BS_ERR;
        }
    }

    pstRunner->pstCurrentMode = pstModeNode;

    return BS_OK;
}

/* 退出模式 */
static int cmdexp_QuitMode(IN _CMD_EXP_RUNNER_S *pstRunner)
{
    _CMD_EXP_NEW_MODE_NODE_S *pstModeTmp;

    if (BS_OK != HSTACK_Pop(pstRunner->hModeStack, (HANDLE*)&pstModeTmp)) {
        return BS_STOP;
    }

    MEM_Free(pstRunner->pstCurrentMode);
    pstRunner->pstCurrentMode = pstModeTmp; /* 返回到上一级mode */

    return BS_OK;
}

static int cmdexp_QuitCmd(IN UINT ulArgc, IN CHAR **pArgv,
        IN VOID *pEnv)
{
    _CMD_EXP_ENV_S *pstEnv = pEnv;

    return cmdexp_QuitMode(pstEnv->pstRunner);
}

VOID CmdExp_ExitApp(UINT argc, CHAR **argv)
{
    UCHAR ucChar;

    EXEC_OutString(" Exit the program? y/n:");
    EXEC_Flush();

    do {
        ucChar = EXEC_GetChar();
    } while ((ucChar != 'Y') && (ucChar != 'y')
            && (ucChar != 'N') && (ucChar != 'n'));

    if ((ucChar == 'Y') || (ucChar == 'y'))
    {
        SYSRUN_Exit(0);
    }

    return;
}

static _CMD_EXP_VIEW_TBL_NODE_S * cmdexp_AddViewToViewTbl
(
    IN _CMD_EXP_VIEW_TBL_NODE_S *pstFatherView,
    IN _CMD_EXP_TREE_S *pstView
)
{
    _CMD_EXP_VIEW_TBL_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(_CMD_EXP_VIEW_TBL_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }

    DLL_INIT(&pstNode->stSubViewList);
    DLL_INIT(&pstNode->stSaveFuncList);

    pstNode->pstView = pstView;

    DLL_ADD(&(pstFatherView->stSubViewList), pstNode);

    return pstNode;
}

/* 初始化命令行注册模块 */
int cmdexp_Init(CMD_EXP_S *cmdexp)
{
    CMD_EXP_REG_CMD_PARAM_S stCmdParam = {0};

    cmdexp->stRoot.pstView = &g_stCmdExpUserView;
    DLL_INIT(&cmdexp->stRoot.stSubViewList);
    DLL_INIT(&cmdexp->stRoot.stSaveFuncList);
    DLL_INIT(&cmdexp->stNoDbgFuncList);
    MUTEX_Init(&cmdexp->lock);

    stCmdParam.uiType = DEF_CMD_EXP_TYPE_CMD;
    stCmdParam.uiProperty = 0;
    stCmdParam.pcViews = DEF_CMD_EXP_VIEW_USER;
    stCmdParam.pcCmd = "quit(Quit current view)";
    stCmdParam.pcViewName = NULL;
    stCmdParam.pfFunc = cmdexp_QuitCmd;
    CmdExp_RegCmd(cmdexp, &stCmdParam);

    stCmdParam.pcCmd = "show(Show information) this [all]";
    stCmdParam.pfFunc = CmdExp_CmdShow;
    CmdExp_RegCmd(cmdexp, &stCmdParam);

    return BS_OK;
}

CMD_EXP_HDL CmdExp_Create()
{
    CMD_EXP_S *cmdexp = MEM_ZMalloc(sizeof(CMD_EXP_S));
    if (! cmdexp) {
        return NULL;
    }

    cmdexp_Init(cmdexp);

    return cmdexp;
}

void CmdExp_SetFlag(CMD_EXP_HDL hCmdExp, UINT flag)
{
    CMD_EXP_S *cmdexp = hCmdExp;
    cmdexp->flag = flag;
}

/* 得到变量命令元素的允许范围 */
static int cmdexp_GetCmdElementRange(CHAR *pszCmdElemet,
        OUT INT64 *min, OUT INT64 *max)
{
    CHAR *pcSplit = NULL, *pcSplitTmp = NULL;
    CHAR szNum[64] = "";
    INT64 lMin = 0, lMax= 0;
    INT i;

    pcSplit = strchr(pszCmdElemet, '<');
    if (pcSplit == NULL)
    {
        BS_WARNNING(("Bad cmd element!"));
        RETURN(BS_ERR);
    }

    pcSplit++;

    pcSplitTmp = strchr(pcSplit + 1, '-');
    if (pcSplitTmp == NULL)
    {
        BS_WARNNING(("Bad cmd element!"));
        RETURN(BS_ERR);
    }

    for (i=0; i<pcSplitTmp - pcSplit; i++)
    {
        szNum[i] = pcSplit[i];
    }
    szNum[i] = '\0';

    if (szNum[0] == '\0')
    {
        BS_WARNNING(("Bad cmd element!"));
        RETURN(BS_ERR);        
    }

    TXT_Atoll(szNum, &lMin);

    pcSplit = pcSplitTmp + 1;
    pcSplitTmp = strchr(pcSplit, '>');
    if (pcSplitTmp == NULL)
    {
        BS_WARNNING(("Bad cmd element!"));
        RETURN(BS_ERR);
    }

    for (i=0; i<pcSplitTmp - pcSplit; i++)
    {
        szNum[i] = pcSplit[i];
    }
    szNum[i] = '\0';

    if (szNum[0] == '\0')
    {
        BS_WARNNING(("Bad cmd element!"));
        RETURN(BS_ERR);        
    }

    TXT_Atoll(szNum, &lMax);

    *min = lMin;
    *max = lMax;

    return BS_OK;    
}

static _CMD_EXP_MATCH_E cmdexp_ParsePatternString(IN CHAR *pszCmdElement,
        IN CHAR *pszInputCmd)
{
    INT64 min, max;
    int len = strlen(pszInputCmd);

    if (cmdexp_GetCmdElementRange(pszCmdElement, &min, &max) != BS_OK)
    {
        return _CMD_EXP_NOTMACTH;
    }

    if ((len < min) || (len > max))
    {
        return _CMD_EXP_NOTMACTH;
    }

    return _CMD_EXP_PREMACTH;
}

static _CMD_EXP_MATCH_E cmdexp_ParsePatternInt(IN CHAR *pszCmdElement,
        IN CHAR *pszInputCmd)
{
    INT64 var = 0;
    INT64 min, max;

    if (cmdexp_GetCmdElementRange(pszCmdElement, &min, &max) != BS_OK)
    {
        return _CMD_EXP_NOTMACTH;
    }

    if (BS_OK != TXT_Atoll(pszInputCmd, &var))
    {
        return _CMD_EXP_NOTMACTH;
    }
    
    if ((var < min) || (var > max))
    {
        return _CMD_EXP_NOTMACTH;
    }
    
    return _CMD_EXP_PREMACTH;
}

static _CMD_EXP_MATCH_E cmdexp_ParsePatternIP(IN CHAR *pszCmdElement,
        IN CHAR *pszInputCmd)
{
    CHAR *pcElement;
    UINT uiTimes = 0;
    UINT uiNum;

    TXT_SCAN_ELEMENT_BEGIN(pszInputCmd, '.', pcElement)
    {
        uiTimes ++;
        if (uiTimes > 4)
        {
            TXT_SCAN_ELEMENT_STOP();
            return _CMD_EXP_NOTMACTH;
        }

        if (BS_OK != TXT_AtouiWithCheck(pcElement, &uiNum))
        {
            TXT_SCAN_ELEMENT_STOP();
            return _CMD_EXP_NOTMACTH;
        }

        if (uiNum > 255)
        {
            TXT_SCAN_ELEMENT_STOP();
            return _CMD_EXP_NOTMACTH;
        }
    }TXT_SCAN_ELEMENT_END();

    if (uiTimes != 4)
    {
        return _CMD_EXP_NOTMACTH;
    }

    return _CMD_EXP_PREMACTH;
}

/* 获取Pattern长度, 比如%INT<1-2>的获取结果就是%INT的长度4 */
static ULONG cmdexp_GetPatternLen(IN CHAR *pcCmdEle)
{
    CHAR *pcSplit;

    pcSplit = strchr(pcCmdEle, '<');
    if (NULL == pcSplit)
    {
        return strlen(pcCmdEle);
    }

    return pcSplit - pcCmdEle;
}

static _CMD_EXP_MATCH_E cmdexp_CmdElementCmp(IN CHAR *pszCmdElement,
        IN CHAR *pszInputCmd)
{
    if (pszCmdElement[0] != '%') {
        if (strcmp(pszCmdElement, pszInputCmd) == 0) {
            return _CMD_EXP_EXACTMACTH;
        }

        if (strncmp(pszCmdElement, pszInputCmd, strlen(pszInputCmd)) == 0) {
            return _CMD_EXP_PREMACTH;
        }

        return _CMD_EXP_NOTMACTH;
    } else {      /* 变量命令元素 */
        _CMD_EXP_PATTERN_PARSE_S *node;
        for (node=g_astCmdExpParse; node->pcPattern != NULL; node++) {
            if (strncmp(node->pcPattern, pszCmdElement,
                        cmdexp_GetPatternLen(pszCmdElement)) == 0) {
                return node->pfModeParse(pszCmdElement, pszInputCmd);
            }
        }
    }

    return _CMD_EXP_NOTMACTH;
}

static _CMD_EXP_TREE_S * cmdexp_FindCmdElementExact(_CMD_EXP_TREE_S *pstRoot,
        IN CHAR *pcCmdElement /*命令元素*/)
{
    _CMD_EXP_TREE_S *pstNode;

    pstNode = pstRoot->pstSubCmdTree;

    while (pstNode) {
        if (strcmp(pstNode->acCmd, pcCmdElement) == 0) {
            return pstNode;
        }

        pstNode = pstNode->pstNextCmdTree;
    }

    return NULL;
}

/* 
  如果只找到一个可见命令,返回它.
  如果找到多个可见命令, 提示并返回NULL
  如果没有找到可见命令,并且只有一个隐藏命令, 返回它
  如果没有找到可见命令,并且有多个隐藏命令,返回NULL且不给提示
*/
static _CMD_EXP_TREE_S * cmdexp_FindCmdElement(_CMD_EXP_TREE_S *pstRoot,
        IN CHAR *pcCmd /*命令元素*/)
{
    _CMD_EXP_TREE_S *pstNode;
    _CMD_EXP_TREE_S *pstNodeFoundVisable = NULL;
    _CMD_EXP_TREE_S *pstNodeFoundHide = NULL;
    _CMD_EXP_MATCH_E eMatchResult;
    UINT uiHideCmdCount = 0;
    UINT uiVisableCmdCount = 0;

    pstNode = pstRoot->pstSubCmdTree;

    /* 优先找可见命令, 隐藏命令优先级低 */
    while (pstNode) {
        eMatchResult = cmdexp_CmdElementCmp(pstNode->acCmd, pcCmd);

        if (eMatchResult == _CMD_EXP_EXACTMACTH) {/* 精确匹配 */
            return pstNode;
        }

        if (eMatchResult == _CMD_EXP_PREMACTH) {
            if (BIT_ISSET(pstNode->uiProperty, DEF_CMD_EXP_PROPERTY_HIDE)) {
                uiHideCmdCount ++;
                pstNodeFoundHide = pstNode;
            } else {
                uiVisableCmdCount ++;
                pstNodeFoundVisable = pstNode;
            }
        }

        pstNode = pstNode->pstNextCmdTree;
    }

    if (uiVisableCmdCount > 1) {
        EXEC_OutInfo(" There is more than one elements match the cmd element:%s.\r\n", pcCmd);
        EXEC_Flush();
        return NULL;
    }

    if (uiVisableCmdCount == 1) {
        return pstNodeFoundVisable;
    }
    else if (uiHideCmdCount == 1)
    {
        return pstNodeFoundHide;
    }

    return NULL;
}

static _CMD_EXP_TREE_S * cmdexp_FindCmd
(
    IN _CMD_EXP_TREE_S *pstRoot,
    IN UINT uiArgc,
    INOUT CHAR **ppArgv
)
{
    _CMD_EXP_TREE_S *pstNode = NULL;

    pstNode = cmdexp_FindCmdElement(pstRoot, ppArgv[0]);
 
    if (pstNode == NULL)
    {
        return NULL;
    }

    if (CMD_EXP_IS_VIEW(pstNode->uiType))
    {
        if (uiArgc != 1)
        {
            return NULL;
        }
    }

    if (pstNode->acCmd[0] != '%')  /* 如果是静态元素,为了补全命令形式,指向节点中的完整命令 */
    {
        ppArgv[0] = pstNode->acCmd;
    }

    if (uiArgc > 1)
    {
        return cmdexp_FindCmd(pstNode, uiArgc - 1, ppArgv + 1);
    }

    return pstNode;
}

static VOID cmdexp_GetElement(IN CHAR *pcCmd, OUT _CMD_EXP_ELEMENT_S *pstEle)
{
    CHAR *pcTmp = pcCmd;
    BOOL_T bIsHelp = FALSE;

    memset(pstEle, 0, sizeof(_CMD_EXP_ELEMENT_S));
    pstEle->pcCmd = pcCmd;

    while (*pcTmp != '\0')
    {
        if (FALSE == bIsHelp)
        {
            if (*pcTmp == '(') /* 帮助信息开始 */
            {
                bIsHelp = TRUE;
                *pcTmp = '\0';
                pstEle->pcHelp = pcTmp + 1;
            }
            else if (*pcTmp == ' ') /* 获取第一个命令元素结束 */
            {
                *pcTmp = '\0';
                pstEle->pcNext = pcTmp + 1;
                break;
            }
        }
        else  /* 是帮助信息 */
        {
            if (*pcTmp == ')')   /* 帮助信息结束 */
            {
                *pcTmp = '\0';
                bIsHelp = FALSE;
            }
        }

        pcTmp ++;
    }

    return;
}

static VOID cmdexp_AddSubCmd(IN _CMD_EXP_TREE_S *pstRoot,
        IN _CMD_EXP_TREE_S *pstNodeToAdd)
{
    _CMD_EXP_TREE_S *pstNodeTmp;
    _CMD_EXP_TREE_S **ppstNodePrevPtr;

    pstNodeToAdd->pstParent = pstRoot;

    if (pstRoot->pstSubCmdTree == NULL) {
        pstRoot->pstSubCmdTree = pstNodeToAdd;
    } else {
        pstNodeTmp = pstRoot->pstSubCmdTree;
        ppstNodePrevPtr = &(pstRoot->pstSubCmdTree);

        while (pstNodeTmp != NULL) {
            if (strcmp(pstNodeTmp->acCmd, pstNodeToAdd->acCmd) >= 0) {
                pstNodeToAdd->pstNextCmdTree = pstNodeTmp;
                *ppstNodePrevPtr = pstNodeToAdd;
                return;
            }

            ppstNodePrevPtr = &(pstNodeTmp->pstNextCmdTree);
            pstNodeTmp = pstNodeTmp->pstNextCmdTree;
        }
        
        /* 找到了最后，直接添加在最后 */
        *ppstNodePrevPtr = pstNodeToAdd;
    }
}

static VOID cmdexp_DelSubCmd(IN _CMD_EXP_TREE_S *pstRoot,
        IN _CMD_EXP_TREE_S *pstNodeToDel)
{
    _CMD_EXP_TREE_S *pstNodeTmp;
    _CMD_EXP_TREE_S **ppstNodePrevPtr;

    pstNodeTmp = pstRoot->pstSubCmdTree;
    ppstNodePrevPtr = &(pstRoot->pstSubCmdTree);

    while (pstNodeTmp != NULL) {
        if (strcmp(pstNodeTmp->acCmd, pstNodeToDel->acCmd) == 0) {
            *ppstNodePrevPtr = pstNodeToDel->pstNextCmdTree;
            return;
        }

        ppstNodePrevPtr = &(pstNodeTmp->pstNextCmdTree);
        pstNodeTmp = pstNodeTmp->pstNextCmdTree;
    }
}

static int cmdexp_DelCmd(CMD_EXP_S *cmdexp, _CMD_EXP_TREE_S *pstRoot,
        CHAR *pcCmd)
{
    _CMD_EXP_TREE_S *pstNode;
    _CMD_EXP_ELEMENT_S stEle;
    UINT uiLen;

    cmdexp_GetElement(pcCmd, &stEle);

    uiLen = strlen(stEle.pcCmd);

    if ((uiLen > _DEF_CMD_EXP_MAX_CMD_ELEMENT_LEN) || (uiLen == 0))
    {
        BS_WARNNING(("The cmd element length is too long, cmd:%s", stEle.pcCmd));
        RETURN(BS_NOT_SUPPORT);
    }

    pstNode = cmdexp_FindCmdElementExact(pstRoot, stEle.pcCmd);
    if (pstNode == NULL) {
        return BS_OK;
    } 

    if (stEle.pcNext == NULL) { /*这是命令行的最后一个元素*/
        pstNode->pfFunc = NULL;
        pstNode->hParam = NULL;
        if (NULL == pstNode->pstSubCmdTree) {
            cmdexp_DelSubCmd(pstRoot, pstNode);
            MEM_Free(pstNode);
        }
    } else {
        cmdexp_DelCmd(cmdexp, pstNode, stEle.pcNext);
        if (! _DEF_CMD_EXP_IS_CMD_RUNABLE(pstNode)) {
            if (NULL == pstNode->pstSubCmdTree) {
                cmdexp_DelSubCmd(pstRoot, pstNode);
                MEM_Free(pstNode);
            }
        }
    }

    return BS_OK;
}

static int cmdexp_AddCmd
(
    IN CMD_EXP_S *cmdexp,
    IN UINT uiType,
    IN UINT uiProperty,
    IN _CMD_EXP_VIEW_TBL_NODE_S *pstViewTblNode,
    IN _CMD_EXP_TREE_S *pstRoot,
    IN CHAR *pcCmd,
    IN CHAR *pcViewName,
    IN PF_CMD_EXP_RUN pfFunc,
    IN HANDLE hParam
)
{
    _CMD_EXP_TREE_S *pstNode;
    _CMD_EXP_ELEMENT_S stEle;
    UINT uiLen;
    BOOL_T bIsExist = FALSE;
    _CMD_EXP_VIEW_TBL_NODE_S *pstViewTblNodeTmp;

    cmdexp_GetElement(pcCmd, &stEle);

    uiLen = strlen(stEle.pcCmd);

    if ((uiLen > _DEF_CMD_EXP_MAX_CMD_ELEMENT_LEN) || (uiLen == 0))
    {
        BS_WARNNING(("The cmd element length is too long, cmd:%s", stEle.pcCmd));
        RETURN(BS_NOT_SUPPORT);
    }

    pstNode = cmdexp_FindCmdElementExact(pstRoot, stEle.pcCmd);
    if (pstNode == NULL) {
        pstNode = MEM_Malloc(sizeof(_CMD_EXP_TREE_S));
        if (pstNode == NULL) {
            BS_WARNNING(("No memory!"));
            RETURN(BS_NULL_PARA);
        }

        Mem_Zero(pstNode, sizeof(_CMD_EXP_TREE_S));
        TXT_StrCpy(pstNode->acCmd, stEle.pcCmd);
        pstNode->pcHelp = TXT_Strdup(stEle.pcHelp);
        pstNode->uiProperty = uiProperty;
        pstNode->uiType = uiType;
        BIT_CLR(pstNode->uiType, DEF_CMD_EXP_TYPE_VIEW);  /* 只有最后一个元素才具有View属性, 中间元素不具有 */

        cmdexp_AddSubCmd(pstRoot, pstNode);
    } else {
        bIsExist = TRUE;
    }

    if (stEle.pcNext == NULL) { /*这是命令行的最后一个元素*/
        if (bIsExist == TRUE) {
            if ((_DEF_CMD_EXP_IS_CMD_RUNABLE(pstNode))
                || (BIT_ISSET(uiType, DEF_CMD_EXP_TYPE_VIEW))) {
                BS_WARNNING(("The cmd(%s) is exist!", pcCmd));
                RETURN(BS_ALREADY_EXIST);
            } else {
                if (BIT_ISSET(pstNode->uiProperty, DEF_CMD_EXP_PROPERTY_HIDE)) {
                    pstNode->uiProperty = uiProperty;
                } else {
                    pstNode->uiProperty = uiProperty;
                    pstNode->uiProperty &= ~((UINT)DEF_CMD_EXP_PROPERTY_HIDE);
                }
            }
        } else {
            pstNode->uiProperty = uiProperty;
            pstNode->uiType = uiType;
        }

        pstNode->pfFunc = pfFunc;
        pstNode->hParam = hParam;

        if (CMD_EXP_IS_VIEW(uiType)) {
            CHAR szQuit[] = "quit(Quit current view)";
            CHAR szShowThis[] = "show this";
            CHAR szShowThis2[] = "show this all";

            TXT_Strlcpy(pstNode->szViewName, pcViewName, sizeof(pstNode->szViewName));
            pstViewTblNodeTmp = cmdexp_AddViewToViewTbl(pstViewTblNode, pstNode);

            /* 对View注册quit命令 */
            cmdexp_AddCmd(cmdexp, DEF_CMD_EXP_TYPE_CMD, 0, pstViewTblNodeTmp,
                    pstNode, szQuit, NULL, cmdexp_QuitCmd, NULL);

            cmdexp_AddCmd(cmdexp, DEF_CMD_EXP_TYPE_CMD, 0, pstViewTblNodeTmp,
                    pstNode, szShowThis, NULL, CmdExp_CmdShow, NULL);

            cmdexp_AddCmd(cmdexp, DEF_CMD_EXP_TYPE_CMD, 0, pstViewTblNodeTmp,
                    pstNode, szShowThis2, NULL, CmdExp_CmdShow, NULL);
            
        }
    } else {
        /* 如果注册的是可见命令, 则将匹配的节点的隐藏属性去掉 */
        if (! BIT_ISSET(uiProperty, DEF_CMD_EXP_PROPERTY_HIDE)) {
            BIT_CLR(pstNode->uiProperty, DEF_CMD_EXP_PROPERTY_HIDE);
        }

        cmdexp_AddCmd(cmdexp, uiType, uiProperty, pstViewTblNode, pstNode,
                stEle.pcNext, pcViewName, pfFunc, hParam);
    }

    return BS_OK;
}

static char * cmdexp_ProcessSingleCmd(char *cmd, char *out, int out_size)
{
    unsigned ulLen;

    ulLen = strlen(cmd);
    if (ulLen == 0) {
        BS_WARNNING(("Cmd length is 0!"));
        return NULL;
    }
    if (ulLen > DEF_CMD_EXP_MAX_CMD_LEN) {
        BS_WARNNING(("Cmd length exceed!"));
        return NULL;
    }

    TXT_ReplaceSubStr(cmd, "  ", " ", out, out_size);

    ulLen = strlen(out);
    if (ulLen == 0) {
        BS_WARNNING(("Cmd length is 0!"));
        return NULL;
    }

    return TXT_Strim(out);
}

static int cmdexp_UnregSingleCmd
(
    IN CMD_EXP_S *cmdexp,
    IN CHAR *pcView,
    IN CHAR *pcCmd
)
{
    _CMD_EXP_VIEW_TBL_NODE_S *pstViewTblNode;
    CHAR            acCmd[DEF_CMD_EXP_MAX_CMD_LEN + 1];
    CHAR            *pcCmdTmp;
    
    if ((pcView == NULL) || (pcCmd == NULL)) {
        BS_WARNNING(("Null ptr!"));
        RETURN(BS_NULL_PARA);
    }

    pstViewTblNode = cmdexp_FindView(cmdexp, pcView);
    if (pstViewTblNode == NULL) {  /*没有找到视图*/
#ifdef IN_DEBUG
        BS_WARNNING(("The %s view does not exist!", pcView));
#endif
        RETURN(BS_NO_SUCH);
    }

    pcCmdTmp = cmdexp_ProcessSingleCmd(pcCmd, acCmd, sizeof(acCmd));
    if (! pcCmdTmp) {
        RETURN(BS_ERR);
    }

    return cmdexp_DelCmd(cmdexp, pstViewTblNode->pstView, pcCmdTmp);
}

/*注册命令，命令字以空格隔开*/
static int cmdexp_RegSingleCmd
(
    IN CMD_EXP_S *cmdexp,
    IN UINT uiType,
    IN UINT uiProperty,
    IN CHAR *pcView,
    IN CHAR *pcCmd,
    IN CHAR *pcViewName,
    IN PF_CMD_EXP_RUN pfFunc,
    IN HANDLE hParam
)
{
    _CMD_EXP_VIEW_TBL_NODE_S *pstViewTblNode;
    CHAR            acCmd[DEF_CMD_EXP_MAX_CMD_LEN + 1];
    CHAR            *pcCmdTmp;
    
    if ((pcView == NULL) || (pcCmd == NULL))
    {
        BS_WARNNING(("Null ptr!"));
        RETURN(BS_NULL_PARA);
    }

    pstViewTblNode = cmdexp_FindView(cmdexp, pcView);
    if (pstViewTblNode == NULL) {  /*没有找到视图*/
#ifdef IN_DEBUG
        BS_WARNNING(("The %s view does not exist!", pcView));
#endif
        RETURN(BS_NO_SUCH);
    }

    pcCmdTmp = cmdexp_ProcessSingleCmd(pcCmd, acCmd, sizeof(acCmd));
    if (! pcCmdTmp) {
        RETURN(BS_ERR);
    }

    return cmdexp_AddCmd(cmdexp, uiType, uiProperty, pstViewTblNode,
            pstViewTblNode->pstView, pcCmdTmp, pcViewName, pfFunc, hParam);
}

static VOID cmdexp_OutPutByFile(IN FILE *fp, IN CHAR *pcString)
{
    if (fp) {
        fputs(pcString, fp);
    } else {
        EXEC_OutString(pcString);
    }
}


/* 判断是否在show */
BOOL_T CmdExp_IsShowing(IN HANDLE hFileHandle)
{
    _CMD_EXP_SHOW_SAVE_NODE_S *pstSSNode = hFileHandle;

    if (pstSSNode->fp == NULL) {
        return TRUE;
    }

    return FALSE;
}

/* 判断是否在save */
BOOL_T CmdExp_IsSaving(IN HANDLE hFileHandle)
{
    _CMD_EXP_SHOW_SAVE_NODE_S *pstSSNode = hFileHandle;

    if (pstSSNode->fp == NULL) {
        return FALSE;
    }

    return TRUE;
}

static int cmdexp_save_OutputString(_CMD_EXP_SHOW_SAVE_NODE_S *pstSSNode,
        CHAR *pcString)
{
    UINT i;

    for (i=0; i<pstSSNode->ucPrefixSpaceNum; i++) {
        cmdexp_OutPutByFile(pstSSNode->fp, " ");
    }

    cmdexp_OutPutByFile(pstSSNode->fp, pcString);
    cmdexp_OutPutByFile(pstSSNode->fp, "\r\n");

    return BS_OK;
}

/* 获取将要匹配的Mode */
static _CMD_EXP_NEW_MODE_NODE_S *cmdexp_save_GetToMatchMode(IN _CMD_EXP_SHOW_SAVE_NODE_S *pstSSNode)
{
    UINT uiIndex = pstSSNode->ucMatchedModeIndex + 1; /* 加1是因为要忽略ModeStack中的user-view */

    if (uiIndex < HSTACK_GetCount(pstSSNode->pstEnv->pstRunner->hModeStack))
    {
        return HSTACK_GetValueByIndex(pstSSNode->pstEnv->pstRunner->hModeStack, uiIndex);
    }

    if (uiIndex == HSTACK_GetCount(pstSSNode->pstEnv->pstRunner->hModeStack))
    {
        return pstSSNode->pstEnv->pstRunner->pstCurrentMode;
    }

    return NULL;
}

static VOID cmdexp_Prepare(IN _CMD_EXP_SHOW_SAVE_NODE_S *pstSSNode)
{
    UINT uiStackCount;
    UINT i;

    if (pstSSNode->bHasCfg == FALSE) {
        pstSSNode->bHasCfg = TRUE;

        uiStackCount = HSTACK_GetCount(pstSSNode->hStack);

        for (i=0; i<uiStackCount; i++) {
            CmdExp_OutputMode(pstSSNode, "%s",
                    HSTACK_GetValueByIndex(pstSSNode->hStack, i));
        }
    }
}

int CmdExp_OutputMode(IN HANDLE hFileHandle, IN CHAR *fmt, ...)
{
    va_list args;
    CHAR acMsg[DEF_CMD_EXP_MAX_CMD_LEN + 1];
    _CMD_EXP_SHOW_SAVE_NODE_S *pstSSNode = hFileHandle;
    _CMD_EXP_NEW_MODE_NODE_S *pstMode;

    cmdexp_Prepare(pstSSNode);

    va_start(args, fmt);
    vsnprintf(acMsg, sizeof(acMsg), fmt, args);
    va_end(args);

    pstSSNode->ucModeDeepth ++;

    if (pstSSNode->bIsMatched == TRUE) {
        cmdexp_save_OutputString(hFileHandle, acMsg);
        pstSSNode->ucPrefixSpaceNum ++;
        return BS_OK;
    }

    if (pstSSNode->ucModeDeepth - 1 > pstSSNode->ucMatchedModeIndex) {
        return BS_OK;
    }

    pstMode = cmdexp_save_GetToMatchMode(pstSSNode);

    if (NULL == pstMode) {
        BS_DBGASSERT(0);
        return BS_ERR;
    }

    if (strcmp(pstMode->szCmd, acMsg) == 0) {
        pstSSNode->ucMatchedModeIndex ++;
        if (pstSSNode->ucMatchedModeIndex
                == HSTACK_GetCount(pstSSNode->pstEnv->pstRunner->hModeStack)) {
            pstSSNode->bIsMatched = TRUE;
            pstSSNode->ucPrefixSpaceNum ++;
        }
    }

    return BS_OK;
}

int CmdExp_OutputModeQuit(IN HANDLE hFileHandle)
{
    _CMD_EXP_SHOW_SAVE_NODE_S *pstSSNode = hFileHandle;
    BOOL_T bNeedDec = FALSE;

    BS_DBGASSERT(pstSSNode->ucModeDeepth > 0);

    if (pstSSNode->bIsMatched == TRUE) {
        bNeedDec = TRUE;
    }

    if (pstSSNode->ucModeDeepth == pstSSNode->ucMatchedModeIndex) {
        pstSSNode->ucMatchedModeIndex --;
        pstSSNode->bIsMatched = FALSE;
    }

    pstSSNode->ucModeDeepth --;

    if (pstSSNode->bIsMatched == TRUE) {
        cmdexp_save_OutputString(hFileHandle, "quit");
    }

    if (bNeedDec) {
        pstSSNode->ucPrefixSpaceNum --;
    }

    return BS_OK;
}

int CmdExp_OutputCmd(IN HANDLE hFileHandle, IN CHAR *fmt, ...)
{
    va_list args;
    _CMD_EXP_SHOW_SAVE_NODE_S *pstSSNode = hFileHandle;
    CHAR acMsg[DEF_CMD_EXP_MAX_CMD_LEN + 1];

    cmdexp_Prepare(pstSSNode);

    if (pstSSNode->bIsMatched != TRUE)
    {
        return BS_OK;
    }

    va_start(args, fmt);
    BS_Vsnprintf(acMsg, sizeof(acMsg), fmt, args);
    va_end(args);

    return cmdexp_save_OutputString(hFileHandle, acMsg);
}

static int cmdexp_Save(IN _CMD_EXP_SHOW_SAVE_NODE_S *pstSSNode, IN _CMD_EXP_VIEW_TBL_NODE_S *pstViewTbl)
{
    FILE *fq = NULL;
    UINT uiStackCount = HSTACK_GetCount(pstSSNode->hStack);
    UINT i;
    _CMD_EXP_SVAE_NODE_S *pstSaveNode;

    if (uiStackCount == 0)
    {
        return BS_ERR;
    }

    DLL_SCAN(&pstViewTbl->stSaveFuncList, pstSaveNode)
    {
        fq = FILE_Open(pstSaveNode->szCfgFileName, TRUE, "ab");
        if (fq == NULL)
        {
            continue;
        }

        pstSSNode->bIsNewFile = TRUE;
        pstSSNode->bHasCfg = FALSE;
        pstSSNode->fp = fq;

        pstSaveNode->pfFunc(pstSSNode);

        if (pstSSNode->bHasCfg == TRUE)
        {
            for (i=0; i<uiStackCount; i++)
            {
                CmdExp_OutputModeQuit(pstSSNode);
            }
        }

        fclose(fq);
    }

    return BS_OK;
}

static int cmdexp_CmdShow(IN _CMD_EXP_SHOW_SAVE_NODE_S *pstSSNode, IN _CMD_EXP_VIEW_TBL_NODE_S *pstViewTbl)
{
    UINT uiStackCount = HSTACK_GetCount(pstSSNode->hStack);
    ULONG i;
    _CMD_EXP_SVAE_NODE_S *pstSaveNode;

    pstSSNode->bHasCfg = FALSE;
    pstSSNode->bIsNewFile = FALSE;
    pstSSNode->fp = 0;

    DLL_SCAN(&pstViewTbl->stSaveFuncList, pstSaveNode)
    {
        pstSaveNode->pfFunc(pstSSNode);
    }

    if (pstSSNode->bHasCfg == TRUE)
    {
        for (i=0; i<uiStackCount; i++)
        {
            CmdExp_OutputModeQuit(pstSSNode);
        }
    }

    return BS_OK;
}

static VOID cmdexp_SaveOrShow
(
    IN _CMD_EXP_VIEW_TBL_NODE_S *pstViewTbl,
    IN BOOL_T bIsShow,
    IN _CMD_EXP_SHOW_SAVE_NODE_S *pstSSNode
)
{
    _CMD_EXP_VIEW_TBL_NODE_S *pstNode;

    if (bIsShow)
    {
        cmdexp_CmdShow(pstSSNode, pstViewTbl);
    }
    else
    {
        cmdexp_Save(pstSSNode, pstViewTbl);
    }

    DLL_SCAN(&pstViewTbl->stSubViewList, pstNode)
    {
        HSTACK_Push(pstSSNode->hStack, pstNode->pstView->szViewName);
        cmdexp_SaveOrShow(pstNode, bIsShow, pstSSNode);
        HSTACK_Pop(pstSSNode->hStack, NULL);
    }
}

static VOID cmdexp_DeleteAllCfgFile(IN _CMD_EXP_VIEW_TBL_NODE_S *pstViewTreeNode)
{
    _CMD_EXP_VIEW_TBL_NODE_S *pstNode;
    _CMD_EXP_SVAE_NODE_S *pstSaveNode;
    FILE *fp = NULL;

    DLL_SCAN(&pstViewTreeNode->stSaveFuncList, pstSaveNode)
    {
        fp = FOPEN(pstSaveNode->szCfgFileName, "wb+");
        if (fp != NULL)
        {
            fclose(fp);
        }
    }
    
    DLL_SCAN(&(pstViewTreeNode->stSubViewList), pstNode)
    {
        cmdexp_DeleteAllCfgFile(pstNode);
    }
}

int CmdExp_CmdSave(UINT ulArgc, CHAR **pArgv, VOID *pEnv)
{
    _CMD_EXP_ENV_S *pstEnv = pEnv;
    CMD_EXP_S *cmdexp = pstEnv->pstRunner->cmdexp;
    HANDLE hStack;
    _CMD_EXP_SHOW_SAVE_NODE_S stSSNode = {0};

    hStack = HSTACK_Create(0);
    if (NULL == hStack) {
        return BS_NO_MEMORY;
    }

    stSSNode.hStack = hStack;
    stSSNode.pstEnv = pEnv;
    stSSNode.bIsMatched = TRUE;

    cmdexp_DeleteAllCfgFile(&cmdexp->stRoot);
    cmdexp_SaveOrShow(&cmdexp->stRoot, FALSE, &stSSNode);

    HSTACK_Destory(hStack);

    return BS_OK;
}

int CmdExp_CmdShow(UINT ulArgc, CHAR **pArgv, VOID *pEnv)
{
    HANDLE hStack;
    _CMD_EXP_ENV_S *pstEnv = pEnv;
    CMD_EXP_S *cmdexp = pstEnv->pstRunner->cmdexp;
    _CMD_EXP_SHOW_SAVE_NODE_S stSSNode = {0};

    hStack = HSTACK_Create(0);
    if (NULL == hStack) {
        return BS_NO_MEMORY;
    }

    stSSNode.hStack = hStack;
    stSSNode.pstEnv = pEnv;

    /* show this all */
    if ((ulArgc >= 3) && (pArgv[2][0] == 'a')) {
        stSSNode.bIsShowAll = 1;
    }

    if (pstEnv->pstRunner->pstCurrentMode->pstViewTbl == &cmdexp->stRoot) {
        stSSNode.bIsMatched = TRUE;
    }

    cmdexp_SaveOrShow(&cmdexp->stRoot, TRUE, &stSSNode);

    HSTACK_Destory(hStack);

    EXEC_Flush();

    return BS_OK;
}

int CmdExp_CmdNoDebugAll(UINT ulArgc, CHAR **pArgv, VOID *pEnv)
{
    _CMD_EXP_ENV_S *pstEnv = pEnv;
    CMD_EXP_S *cmdexp = pstEnv->pstRunner->cmdexp;
    CMD_EXP_NO_DBG_NODE_S *pstNode;

    DLL_SCAN(&cmdexp->stNoDbgFuncList, pstNode)
    {
        pstNode->pfNoDbgFunc();
    }

    return BS_OK;
}

VOID CmdExp_RegNoDbgFunc(CMD_EXP_HDL hCmdExp, CMD_EXP_NO_DBG_NODE_S *pstNode)
{
    CMD_EXP_S *cmdexp = hCmdExp;

    DLL_ADD(&cmdexp->stNoDbgFuncList, pstNode);
}

static int cmdexp_UnregCmd(CMD_EXP_S *cmdexp, CHAR *pcView, CHAR *pcCmd)
{
    CHAR *pcFindStart, *pcFindEnd;
    CHAR acCmdTmp[_DEF_CMD_EXP_MAX_CMD_ELEMENT_IN_ONE_CMD * (_DEF_CMD_EXP_MAX_CMD_ELEMENT_LEN + 1) + 1];

    TXT_Strlcpy(acCmdTmp, pcCmd, sizeof(acCmdTmp));

    /* 将命令行中的{} | []解析成多条命令行 */
 
    /* 查找 [] , 其中的命令元素是可选的 */
    if (BS_OK == TXT_FindBracket(acCmdTmp, strlen(pcCmd),
                "[]", &pcFindStart, &pcFindEnd)) {
        *pcFindStart = ' ';
        *pcFindEnd = ' ';
        cmdexp_UnregCmd(cmdexp, pcView, acCmdTmp);
 
        /* 处理不包含[]中元素的命令行 */
        TXT_StrCpy (acCmdTmp, pcCmd);
        while (pcFindStart <= pcFindEnd) {
            *pcFindStart = ' ';
            pcFindStart ++;
        }
        cmdexp_UnregCmd(cmdexp, pcView, acCmdTmp);

        return BS_OK;
    }

    /* 查找 {} ,  和'|' 配合使用, 表示其中的命令元素必选一个 */
    if (BS_OK == TXT_FindBracket(acCmdTmp, strlen(pcCmd),
                "{}", &pcFindStart, &pcFindEnd)) {
        CHAR *pcFindOr;
        CHAR *pcCopyStart;
        UINT ulCopyLen;

        pcCopyStart = pcCmd + ((pcFindStart - acCmdTmp) + 1);
        
        for (;;) {
            /* 将 命令行中{}中的元素全清成空白 */
            pcFindOr = pcFindStart;
            while (pcFindOr <= pcFindEnd) {
                *pcFindOr = ' ';
                pcFindOr ++;
            }

            pcFindOr = strchr (pcCopyStart, '|');
            if ((pcFindOr == NULL)
                    || ((pcFindOr - pcCmd) > (pcFindEnd - acCmdTmp))) {
                /* 处理{}中最后一个命令元素 */
                ulCopyLen = (UINT)((pcFindEnd - acCmdTmp) - (pcCopyStart - pcCmd));
                MEM_Copy(pcFindStart, pcCopyStart, ulCopyLen);
                pcFindStart[ulCopyLen] = ' ';
                cmdexp_UnregCmd(cmdexp, pcView, acCmdTmp);

                break;
            }

            ulCopyLen = (UINT)(pcFindOr - pcCopyStart);
            MEM_Copy(pcFindStart, pcCopyStart, ulCopyLen);
            pcFindStart[ulCopyLen] = ' ';
            
            cmdexp_UnregCmd(cmdexp, pcView, acCmdTmp);
            
            pcCopyStart = pcFindOr + 1;
            TXT_StrCpy(acCmdTmp, pcCmd);
        }

        return BS_OK;
    }

    /* 分解命令行完毕, 进行注册 */
    return cmdexp_UnregSingleCmd(cmdexp, pcView, acCmdTmp);
}

static int cmdexp_RegCmd
(
    IN CMD_EXP_S *cmdexp,
    IN UINT uiType,
    IN UINT uiProperty,
    IN CHAR *pcView,
    IN CHAR *pcCmd,
    IN CHAR *pcViewName, /* 当type参数指明注册为view时, 需要一个view名.  */
    IN PF_CMD_EXP_RUN pfFunc,
    IN HANDLE hParam
)
{
    CHAR *pcFindStart, *pcFindEnd;
    CHAR acCmdTmp[_DEF_CMD_EXP_MAX_CMD_ELEMENT_IN_ONE_CMD * (_DEF_CMD_EXP_MAX_CMD_ELEMENT_LEN + 1) + 1];

    TXT_Strlcpy(acCmdTmp, pcCmd, sizeof(acCmdTmp));

    /* 将命令行中的{} | []解析成多条命令行分别注册 */
    
    /* 查找 [] , 其中的命令元素是可选的 */
    if (BS_OK == TXT_FindBracket(acCmdTmp, strlen(pcCmd), "[]", &pcFindStart, &pcFindEnd))
    {
        *pcFindStart = ' ';
        *pcFindEnd = ' ';
        cmdexp_RegCmd(cmdexp, uiType, uiProperty, pcView, acCmdTmp, pcViewName, pfFunc, hParam);
        
        /* 注册不包含[]中元素的命令行 */
        TXT_StrCpy (acCmdTmp, pcCmd);
        while (pcFindStart <= pcFindEnd)
        {
            *pcFindStart = ' ';
            pcFindStart ++;
        }
        cmdexp_RegCmd(cmdexp, uiType, uiProperty, pcView, acCmdTmp, pcViewName, pfFunc, hParam);

        return BS_OK;
    }

    /* 查找 {} ,  和'|' 配合使用, 表示其中的命令元素必选一个 */
    if (BS_OK == TXT_FindBracket(acCmdTmp, strlen(pcCmd), "{}", &pcFindStart, &pcFindEnd))
    {
        CHAR *pcFindOr;
        CHAR *pcCopyStart;
        UINT ulCopyLen;

        pcCopyStart = pcCmd + ((pcFindStart - acCmdTmp) + 1);
        
        for (;;)
        {
            /* 将 命令行中{}中的元素全清成空白 */
            
            pcFindOr = pcFindStart;
            while (pcFindOr <= pcFindEnd)
            {
                *pcFindOr = ' ';
                pcFindOr ++;
            }

            pcFindOr = strchr (pcCopyStart, '|');
            if ((pcFindOr == NULL) || ((pcFindOr - pcCmd) > (pcFindEnd - acCmdTmp)))
            {
                /* 处理{}中最后一个命令元素 */
                ulCopyLen = (UINT)((pcFindEnd - acCmdTmp) - (pcCopyStart - pcCmd));
                MEM_Copy(pcFindStart, pcCopyStart, ulCopyLen);
                pcFindStart[ulCopyLen] = ' ';
                cmdexp_RegCmd(cmdexp, uiType, uiProperty, pcView, acCmdTmp, pcViewName, pfFunc, hParam);

                break;
            }

            ulCopyLen = (UINT)(pcFindOr - pcCopyStart);
            MEM_Copy(pcFindStart, pcCopyStart, ulCopyLen);
            pcFindStart[ulCopyLen] = ' ';
            
            cmdexp_RegCmd(cmdexp, uiType, uiProperty, pcView, acCmdTmp, pcViewName, pfFunc, hParam);
            
            pcCopyStart = pcFindOr + 1;
            TXT_StrCpy(acCmdTmp, pcCmd);
        }

        return BS_OK;
    }

    /* 分解命令行完毕, 进行注册 */
    return cmdexp_RegSingleCmd(cmdexp, uiType, uiProperty, pcView, acCmdTmp, pcViewName, pfFunc, hParam);
}

static int cmdexp_RegSave(CMD_EXP_S *cmdexp, CHAR *pcFileName,
        CHAR *pcViewName, PF_CMD_EXP_SAVE pfSaveFunc)
{
    _CMD_EXP_VIEW_TBL_NODE_S *pstView;
    _CMD_EXP_SVAE_NODE_S *pstCmdNode = NULL;

    if (pfSaveFunc == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        RETURN(BS_NULL_PARA);
    }

    pstView = cmdexp_FindView(cmdexp, pcViewName);
    if (NULL == pstView) {
        RETURN(BS_NO_SUCH);
    }

    pstCmdNode =  MEM_ZMalloc(sizeof(_CMD_EXP_SVAE_NODE_S));
    if (NULL == pstCmdNode) {
        RETURN(BS_NO_MEMORY);
    }

    pstCmdNode->pfFunc = pfSaveFunc;
    if (pcFileName != NULL) {
        strlcpy(pstCmdNode->szCfgFileName,
                pcFileName, sizeof(pstCmdNode->szCfgFileName));
    } else {
        TXT_Strlcpy(pstCmdNode->szCfgFileName,
                "config.cfg", sizeof(pstCmdNode->szCfgFileName));
    }

    DLL_ADD(&pstView->stSaveFuncList, pstCmdNode);

    return BS_OK;
}

static int cmdexp_UnRegSave(CMD_EXP_S *cmdexp, char *filename, CHAR *pcViewName)
{
    _CMD_EXP_VIEW_TBL_NODE_S *pstView;
    _CMD_EXP_SVAE_NODE_S *pstCmdNode = NULL;

    pstView = cmdexp_FindView(cmdexp, pcViewName);
    if (NULL == pstView) {
        RETURN(BS_NO_SUCH);
    }

    DLL_SCAN(&pstView->stSaveFuncList, pstCmdNode) {
        if (strcmp(pstCmdNode->szCfgFileName, filename)) {
            DLL_DEL(&pstView->stSaveFuncList, pstCmdNode);
            MEM_Free(pstCmdNode);
            break;
        }
    }

    return BS_OK;
}

int CmdExp_RegSave(CMD_EXP_HDL hCmdExp, CHAR *pcFileName,
        CHAR *pcViews, PF_CMD_EXP_SAVE pfSaveFunc)
{
    CMD_EXP_S *cmdexp = hCmdExp;
    CHAR *pcView;

    /* 向不同view注册命令行 */
    TXT_SCAN_ELEMENT_BEGIN(pcViews,'|',pcView) {
        cmdexp_RegSave(cmdexp, pcFileName, pcView, pfSaveFunc);
    }TXT_SCAN_ELEMENT_END();

    return BS_OK;
}

int CmdExp_UnRegSave(CMD_EXP_HDL hCmdExp, CHAR *filename, CHAR *pcViews)
{
    CMD_EXP_S *cmdexp = hCmdExp;
    CHAR *pcView;

    TXT_SCAN_ELEMENT_BEGIN(pcViews,'|',pcView) {
        cmdexp_UnRegSave(cmdexp, filename, pcView);
    }TXT_SCAN_ELEMENT_END();

    return BS_OK;
}

int CmdExp_UnregCmd(CMD_EXP_HDL hCmdExp, CMD_EXP_REG_CMD_PARAM_S *pstParam)
{
    CMD_EXP_S *cmdexp = hCmdExp;
    CHAR *pcView;

    /* 向不同view去注册命令行 */
    TXT_SCAN_ELEMENT_BEGIN(pstParam->pcViews,'|',pcView) {
        cmdexp_UnregCmd(cmdexp, pcView, pstParam->pcCmd);
    }TXT_SCAN_ELEMENT_END();

    return BS_OK;
}

int CmdExp_RegCmd(CMD_EXP_HDL hCmdExp, CMD_EXP_REG_CMD_PARAM_S *pstParam)
{
    CMD_EXP_S *cmdexp = hCmdExp;
    CHAR *pcView;

    if (CMD_EXP_IS_VIEW(pstParam->uiType)) {
        if (NULL == pstParam->pcViewName) {
            RETURN(BS_NULL_PARA);
        }
        
        if (strlen(pstParam->pcViewName) > _DEF_CMD_EXP_MAX_VIEW_NAME_LEN) {
            RETURN(BS_BAD_PARA);
        }

        if (cmdexp_FindView(cmdexp, pstParam->pcViewName) != NULL) {
            RETURN(BS_ALREADY_EXIST);
        }
    }

    /* 向不同view注册命令行 */
    TXT_SCAN_ELEMENT_BEGIN(pstParam->pcViews,'|',pcView) {
        cmdexp_RegCmd(cmdexp, pstParam->uiType, pstParam->uiProperty, pcView,
            pstParam->pcCmd, pstParam->pcViewName, pstParam->pfFunc, pstParam->hParam);
    }TXT_SCAN_ELEMENT_END();

    return BS_OK;
}

int CmdExp_RegCmdSimple(CMD_EXP_HDL hCmdExp, char *view, char *cmd,
        PF_CMD_EXP_RUN func, void *ud)
{
    CMD_EXP_REG_CMD_PARAM_S stCmdParam = {0};

    stCmdParam.uiType = DEF_CMD_EXP_TYPE_CMD;
    stCmdParam.pcViews = view;
    stCmdParam.pcCmd = cmd;
    stCmdParam.pfFunc = func;
    stCmdParam.hParam = ud;

    return CmdExp_RegCmd(hCmdExp, &stCmdParam);
}

int CmdExp_UnregCmdSimple(CMD_EXP_HDL hCmdExp, char *view, char *cmd)
{
    CMD_EXP_REG_CMD_PARAM_S stCmdParam = {0};

    stCmdParam.uiType = DEF_CMD_EXP_TYPE_CMD;
    stCmdParam.pcViews = view;
    stCmdParam.pcCmd = cmd;

    return CmdExp_UnregCmd(hCmdExp, &stCmdParam);
}


/* 创建一个用于运行命令的实例 */
CMD_EXP_RUNNER CmdExp_CreateRunner(CMD_EXP_HDL hCmdExp)
{
    CMD_EXP_S *cmdexp = hCmdExp;
    _CMD_EXP_RUNNER_S *pstRunner;
    CHAR *ppArgv[1];

    pstRunner = MEM_ZMalloc(sizeof(_CMD_EXP_RUNNER_S));
    if (pstRunner == NULL) {
        BS_WARNNING(("No Memory!"));
        return NULL;
    }

    pstRunner->hExcharHandle = EXCHAR_Create();
    if (NULL == pstRunner->hExcharHandle) {
        CmdExp_DestroyRunner(pstRunner);
        return NULL;
    }

    pstRunner->hModeStack = HSTACK_Create(0);
    if (NULL == pstRunner->hModeStack) {
        EXCHAR_Delete(pstRunner->hExcharHandle);
        CmdExp_DestroyRunner(pstRunner);
        return NULL;
    }

    pstRunner->hCmdRArray = RArray_Create(100, DEF_CMD_EXP_MAX_CMD_LEN+1);
    if (pstRunner->hCmdRArray == NULL) {
        CmdExp_DestroyRunner(pstRunner);
        return NULL;
    }

    pstRunner->cmdexp = hCmdExp;

    ppArgv[0] = cmdexp->stRoot.pstView->szViewName;
    cmdexp_EnterMode(pstRunner, cmdexp->stRoot.pstView, 1, ppArgv);

    return pstRunner;
}

void CmdExp_AltEnable(CMD_EXP_RUNNER hRunner, int enable)
{
    _CMD_EXP_RUNNER_S *pstRunner = hRunner;

    pstRunner->alt_mode = 0;
    if (enable) {
        pstRunner->alt_mode = 1;
    }
}

static void cmdexp_OutputPrefix(_CMD_EXP_RUNNER_S *pstRunner)
{
    if (pstRunner->alt_mode) {
        EXEC_OutInfo("%s>", pstRunner->pstCurrentMode->szModeName);
    }
}

/* 开始cmd exp实例 */
void CmdExp_RunnerStart(CMD_EXP_RUNNER hRunner)
{
    _CMD_EXP_RUNNER_S *pstRunner = hRunner;
    if (NULL != pstRunner) {
        cmdexp_OutputPrefix(pstRunner);
        EXEC_Flush();
    }
}

/* 恢复一个运行实例至初始状态 */
void CmdExp_ResetRunner(CMD_EXP_RUNNER hRunner)
{
    _CMD_EXP_RUNNER_S *pstRunner = hRunner;

    if (pstRunner == NULL) {
        BS_WARNNING(("Null ptr!"));
        return;
    }

    if (NULL != pstRunner->hCmdRArray) {
        RArray_Reset(pstRunner->hCmdRArray);
    }

    if (NULL != pstRunner->hExcharHandle) {
        EXCHAR_Reset(pstRunner->hExcharHandle);
    }

    if (NULL != pstRunner->hModeStack) {
        int eRet;
        do {
            eRet = cmdexp_QuitMode(pstRunner);
        }while (eRet == BS_OK);

        HSTACK_Reset(pstRunner->hModeStack);
    }

    pstRunner->acInputs[0] = '\0';
    pstRunner->ulInputsLen = 0;
    pstRunner->uiInputsPos = 0;
    pstRunner->ulHistoryIndex = 0;
}

/* 删除一个用于运行命令的实例 */
int CmdExp_DestroyRunner(CMD_EXP_RUNNER hRunner)
{
    _CMD_EXP_RUNNER_S *pstRunner = hRunner;
    int eRet;
    
    if (pstRunner == NULL) {
        BS_WARNNING(("Null ptr!"));
        RETURN(BS_NULL_PARA);
    }

    if (NULL != pstRunner->hCmdRArray) {
        RArray_Delete(pstRunner->hCmdRArray);
    }

    if (NULL != pstRunner->hExcharHandle) {
        EXCHAR_Delete(pstRunner->hExcharHandle);
    }

    if (NULL != pstRunner->hModeStack) {
        do {
            eRet = cmdexp_QuitMode(pstRunner);
        }while (eRet == BS_OK);

        HSTACK_Destory(pstRunner->hModeStack);
    }

    MEM_Free(pstRunner);
    
    return BS_OK;    
}

static int cmdexp_RunCmdHelp(_CMD_EXP_RUNNER_S *pstRunner)
{
    CHAR *pcSplit,  *pcHelpCmd;
    CHAR acCmdTmp[DEF_CMD_EXP_MAX_CMD_LEN + 1];
    _CMD_EXP_TREE_S *pstNode, *pstNodeNext;
    UINT           ulLen;
    _CMD_EXP_TREE_S *pstFindView = NULL;

    TXT_StrCpy(acCmdTmp, pstRunner->acInputs);
    pcSplit = acCmdTmp;
    pcHelpCmd  = acCmdTmp;

    pstNode = pstRunner->pstCurrentMode->pstViewTbl->pstView;

    do {
        pcSplit = strchr(pcSplit, ' ');
        if (pcSplit != NULL)
        {
            *pcSplit = '\0';
            
            pstNode = cmdexp_FindCmdElement(pstNode, pcHelpCmd);
            if (pstNode == NULL)
            {
                RETURN(BS_NO_SUCH);
            }

            if (CMD_EXP_IS_VIEW(pstNode->uiType))
            {
                pstFindView = pstNode;
            }

            pcSplit++;
            pcHelpCmd = pcSplit;
        }
    }while (pcSplit);

    if (pstFindView != NULL)  /* 找到了View 或者View 下的命令 */
    {
        if (pstNode != pstFindView) /* 找到了View 下的命令 */
        {
            RETURN(BS_NO_SUCH);
        }
        else
        {
            if (! BIT_ISSET(pstNode->uiProperty, DEF_CMD_EXP_PROPERTY_HIDE_CR))
            {
                EXEC_OutString(" <CR>\r\n");
            }
            return BS_OK;
        }
    }

    ulLen = strlen(pcHelpCmd);
    pstNodeNext = pstNode->pstSubCmdTree;
    
    while (pstNodeNext)
    {
        if ((pstNodeNext->uiProperty & DEF_CMD_EXP_PROPERTY_HIDE) == 0)
        {
            if ((ulLen == 0 )
                    || (strncmp(pstNodeNext->acCmd, pcHelpCmd, ulLen) == 0))
            {
                if ((pstRunner->alt_mode)
                        || (pstNodeNext->pfFunc != cmdexp_QuitCmd)) {
                    EXEC_OutInfo(" %-20s %s\r\n",
                            pstNodeNext->acCmd,
                            pstNodeNext->pcHelp == NULL ? "": pstNodeNext->pcHelp);
                }
            }
        }
 
        pstNodeNext = pstNodeNext->pstNextCmdTree;
    }

    if (! CMD_EXP_IS_VIEW(pstNode->uiType))
    {
        if ((pstNode->pfFunc != NULL)
            && (! BIT_ISSET(pstNode->uiProperty, DEF_CMD_EXP_PROPERTY_HIDE_CR)))
        {
            EXEC_OutString(" <CR>\r\n");
        }
    }

    return BS_OK;
}

static int cmdexp_RunCmdExpand(_CMD_EXP_RUNNER_S *pstRunner)
{
    CHAR *pcSplit,  *pcHelpCmd;
    CHAR acCmdTmp[DEF_CMD_EXP_MAX_CMD_LEN + 1];
    _CMD_EXP_TREE_S *pstNode, *pstNodeTmp;
    UINT ulLen;
    UINT uiSamePrefixLen = 0;  
    UINT uiSamePrefixLenTmp;
    UINT uiFindCount = 0;

    TXT_StrCpy(acCmdTmp, pstRunner->acInputs);
    pcSplit = acCmdTmp;
    pcHelpCmd  = acCmdTmp;

    pstNode = pstRunner->pstCurrentMode->pstViewTbl->pstView;

    do {
        pcSplit = strchr(pcSplit, ' ');
        if (pcSplit != NULL)
        {
            *pcSplit = '\0';
            
            pstNode = cmdexp_FindCmdElement(pstNode, pcHelpCmd);
            if (pstNode == NULL)
            {
                RETURN(BS_NO_SUCH);
            }

            if (CMD_EXP_IS_VIEW(pstNode->uiType))
            {
                RETURN(BS_NO_SUCH);
            }
            
            pcSplit++;
            pcHelpCmd = pcSplit;
        }
    }while (pcSplit);

    ulLen = strlen(pcHelpCmd);
    pstNode = pstNode->pstSubCmdTree;
    pstNodeTmp = NULL;

    while (pstNode)
    {
        if (BIT_ISSET(pstNode->uiProperty, DEF_CMD_EXP_PROPERTY_HIDE) == 0)
        {
            if ((ulLen == 0 ) || (strncmp(pstNode->acCmd, pcHelpCmd, ulLen) == 0))
            {
                 /* 是变量命令元素，不能自动补全*/
                if (pstNode->acCmd[0] == '%')
                {
                    return cmdexp_RunCmdHelp(pstRunner);
                }

                /* 多个匹配项，计算最大能补全多少字符  */
                if (pstNodeTmp != NULL)
                {
                    uiSamePrefixLenTmp = (UINT)TXT_GetSamePrefixLen(pstNodeTmp->acCmd, pstNode->acCmd);
                    if (uiSamePrefixLenTmp < uiSamePrefixLen)
                    {
                        uiSamePrefixLen = uiSamePrefixLenTmp;
                    }
                }
                else
                {
                    uiSamePrefixLen = strlen(pstNode->acCmd);
                }

                pstNodeTmp = pstNode;
                uiFindCount ++;
            }
        }

        pstNode = pstNode->pstNextCmdTree;
    }

    /*  找到了匹配项, 自动补全 */
    if (pstNodeTmp != NULL)
    {
        if (pstRunner->ulInputsLen + strlen(pstNodeTmp->acCmd) - ulLen < DEF_CMD_EXP_MAX_CMD_LEN)
        {
            TXT_Strlcat(pstRunner->acInputs, pstNodeTmp->acCmd + ulLen, sizeof(pstRunner->acInputs));
            pstRunner->ulInputsLen += (uiSamePrefixLen - ulLen);
            pstRunner->acInputs[pstRunner->ulInputsLen] = '\0';

            if (uiFindCount == 1)
            {
                pstRunner->acInputs[pstRunner->ulInputsLen] = ' ';
                pstRunner->acInputs[pstRunner->ulInputsLen + 1] = '\0';
                pstRunner->ulInputsLen++;
            }
        }
    }

    if (uiFindCount > 1)
    {
        cmdexp_RunCmdHelp(pstRunner);
    }

    return BS_OK;
}

static UINT cmdexp_ParseCmd(IN CHAR *pcCmd, OUT CHAR *ppArgv[_DEF_CMD_EXP_MAX_CMD_ELEMENT_IN_ONE_CMD])
{
    Mem_Zero(ppArgv, sizeof(CHAR*)*_DEF_CMD_EXP_MAX_CMD_ELEMENT_IN_ONE_CMD);

    return ARGS_Split(pcCmd, ppArgv, _DEF_CMD_EXP_MAX_CMD_ELEMENT_IN_ONE_CMD);
}

static int cmdexp_Call(IN _CMD_EXP_TREE_S *pstNode, IN UINT uiArgc, IN CHAR **ppArgv, IN _CMD_EXP_ENV_S *pstEnv)
{
    return pstNode->pfFunc(uiArgc, ppArgv, pstEnv);
}

static int cmdexp_RunCmd(_CMD_EXP_RUNNER_S *pstRunner,
        _CMD_EXP_NEW_MODE_NODE_S *pstMode, CHAR *pcCmd)
{
    _CMD_EXP_TREE_S *pstNode;
    int enRet;
    _CMD_EXP_ENV_S stEnv = {0};
    UINT uiArgc;
    CHAR *ppArgv[_DEF_CMD_EXP_MAX_CMD_ELEMENT_IN_ONE_CMD];

    uiArgc = cmdexp_ParseCmd(pcCmd, ppArgv);
    if (uiArgc == 0) {
        EXEC_OutInfo(" command not found.\r\n");
        EXEC_Flush();
        RETURN(BS_NO_SUCH);
    }

    pstNode = cmdexp_FindCmd(pstMode->pstViewTbl->pstView, uiArgc, ppArgv);
    if (pstNode == NULL) {
        EXEC_OutInfo(" command not found. \r\n");
        EXEC_Flush();
        RETURN(BS_NO_SUCH);
    }

    stEnv.pstRunner = pstRunner;
    stEnv.pstCmdNode = pstNode;

    if (CMD_EXP_IS_VIEW(pstNode->uiType)) {
        if (pstNode->pfFunc != NULL) {
            enRet = cmdexp_Call(pstNode, uiArgc, ppArgv, &stEnv);
            if (BS_OK != enRet) {
                return enRet;
            }
        }

        cmdexp_EnterMode(pstRunner, pstNode, uiArgc, ppArgv);

        return BS_OK;
    }

    if (pstNode->pfFunc != NULL) {
        return cmdexp_Call(pstNode, uiArgc, ppArgv, &stEnv);
    } else {
        EXEC_OutInfo(" No action for this cmd.\r\n");
        EXEC_Flush();
    }

    return BS_OK;
}

static int cmdexp_RunOneChar(_CMD_EXP_RUNNER_S *pstRunner, CHAR cCmdChar)
{
    int eRet = BS_OK;
    CHAR *pszLastedCmd = NULL;
    UINT i;

    BS_DBGASSERT (pstRunner != NULL);

    if (cCmdChar == '\n') {   /*输入了一条完整的命令*/
        if (pstRunner->alt_mode) {
            EXEC_OutString("\r\n");
            EXEC_Flush();
        }
        TXT_ReplaceSubStr(pstRunner->acInputs, "  ", " ",
                pstRunner->acInputs, sizeof(pstRunner->acInputs));

        /* 去掉命令行最后的空格 */
        if ((pstRunner->ulInputsLen > 0)
                && (pstRunner->acInputs[pstRunner->ulInputsLen - 1] == ' ')) {

            pstRunner->ulInputsLen--;
            pstRunner->acInputs[pstRunner->ulInputsLen] = '\0';
        }

        if (pstRunner->acInputs[0] != '\0') {
            if ((BS_OK != RArray_ReadReversedIndex(pstRunner->hCmdRArray,
                            0, (UCHAR**)&pszLastedCmd, NULL))
                    || ((strcmp(pstRunner->acInputs, pszLastedCmd) != 0))) {

                RArray_WriteForce(pstRunner->hCmdRArray,
                        (UCHAR*)pstRunner->acInputs,
                        strlen(pstRunner->acInputs)+1);
            }
            
            eRet = cmdexp_RunCmd(pstRunner, pstRunner->pstCurrentMode,
                    pstRunner->acInputs);

            if (RETCODE(eRet) == BS_STOP) {
                pstRunner->acInputs[0] = '\0';
                pstRunner->ulInputsLen = 0;
                EXEC_Flush();
                return eRet;
            }
        }

        pstRunner->acInputs[0] = '\0';
        pstRunner->ulInputsLen = 0;
        pstRunner->uiInputsPos = 0;

        cmdexp_OutputPrefix(pstRunner);
        EXEC_Flush();
    } else if (cCmdChar == '\b') {
        if (pstRunner->ulInputsLen - pstRunner->uiInputsPos > 0) {
            TXT_RemoveChar(pstRunner->acInputs,
                    pstRunner->ulInputsLen - pstRunner->uiInputsPos - 1);
            pstRunner->ulInputsLen--;
            EXEC_OutString("\b");
            EXEC_OutString(pstRunner->acInputs
                    + pstRunner->ulInputsLen - pstRunner->uiInputsPos);
            EXEC_OutString(" ");
            for (i=0; i<=pstRunner->uiInputsPos; i++) {
                EXEC_OutString("\b");
            }
            EXEC_Flush();
        }
    } else if (cCmdChar == '?') {
        if (pstRunner->alt_mode) {
            EXEC_OutString("\r\n");
        }
        TXT_ReplaceSubStr(pstRunner->acInputs, "  ", " ", pstRunner->acInputs,
                sizeof(pstRunner->acInputs));
        pstRunner->ulInputsLen = strlen(pstRunner->acInputs);
        cmdexp_RunCmdHelp(pstRunner);
        pstRunner->uiInputsPos = 0;
        if (pstRunner->alt_mode) {
            EXEC_OutInfo("%s>%s", pstRunner->pstCurrentMode->szModeName,
                    pstRunner->acInputs);
        }
        EXEC_Flush();
    } else if (cCmdChar == '\t') {
        EXEC_OutString("\r\n");
        TXT_ReplaceSubStr(pstRunner->acInputs, "  ", " ",
                pstRunner->acInputs, sizeof(pstRunner->acInputs));
        pstRunner->ulInputsLen = strlen(pstRunner->acInputs);
        cmdexp_RunCmdExpand(pstRunner);
        pstRunner->uiInputsPos = 0;
        if (pstRunner->alt_mode) {
            EXEC_OutInfo("%s>%s",
                    pstRunner->pstCurrentMode->szModeName, pstRunner->acInputs);
        }
        EXEC_Flush();
    } else if ((cCmdChar != ' ') || (pstRunner->ulInputsLen != 0)) {
        if (pstRunner->ulInputsLen >= DEF_CMD_EXP_MAX_CMD_LEN) {
            EXEC_OutString(" The input is too long.\r\n");
			EXEC_Flush();
            pstRunner->ulInputsLen = 0;
            pstRunner->acInputs[0] = '\0';
            pstRunner->uiInputsPos = 0;
            RETURN(BS_NOT_SUPPORT);
        }

        if (TRUE == TXT_InsertChar(pstRunner->acInputs,
                    pstRunner->ulInputsLen - pstRunner->uiInputsPos,
                    cCmdChar)) {
            pstRunner->ulInputsLen++;
            if (pstRunner->alt_mode) {
                EXEC_OutString(pstRunner->acInputs
                        + pstRunner->ulInputsLen - pstRunner->uiInputsPos - 1);
            }
            for (i=0; i<pstRunner->uiInputsPos; i++) {
                EXEC_OutString("\b");
            }
            EXEC_Flush();
        }
    }

    return BS_OK;
}

static int cmdexp_RunHistroy(_CMD_EXP_RUNNER_S *pstRunner, EXCHAR enCmdType)
{
    UINT ulLen;
    CHAR *pszLastedCmd = NULL;

    if ((enCmdType != EXCHAR_EXTEND_UP)
        && (enCmdType != EXCHAR_EXTEND_DOWN))
    {
        RETURN(BS_ERR);
    }

    pstRunner->uiInputsPos = 0;

    /* 用户的显示区回退,重新显示新的字符串 */
    for (ulLen = 0; ulLen < pstRunner->ulInputsLen; ulLen++)
    {
        EXEC_OutInfo("\b \b");
    }

    if (enCmdType == EXCHAR_EXTEND_UP)       /* 向上箭头 */
    {
        if (RArray_ReadReversedIndex(pstRunner->hCmdRArray,
                    pstRunner->ulHistoryIndex,
                    (UCHAR**)&pszLastedCmd, &ulLen) == BS_OK)
        {
            TXT_StrCpy (pstRunner->acInputs, pszLastedCmd);
            pstRunner->ulInputsLen = ulLen - 1;
            pstRunner->ulHistoryIndex++;
        }
    }
    else if (enCmdType == EXCHAR_EXTEND_DOWN)
    {
        if ((pstRunner->ulHistoryIndex == 0)
                || (pstRunner->ulHistoryIndex == 1)) {
            pstRunner->acInputs[0] = '\0';
            pstRunner->ulInputsLen = 0;
            if (pstRunner->ulHistoryIndex == 1) {
                pstRunner->ulHistoryIndex = 0;
            }
        } else if (RArray_ReadReversedIndex(pstRunner->hCmdRArray,
                    pstRunner->ulHistoryIndex - 2,
                    (UCHAR**)&pszLastedCmd, &ulLen) != BS_OK) {
            pstRunner->acInputs[0] = '\0';
            pstRunner->ulInputsLen = 0;
        } else {
            TXT_StrCpy(pstRunner->acInputs, pszLastedCmd);
            pstRunner->ulInputsLen = ulLen - 1;
            pstRunner->ulHistoryIndex--;
        }
    }

    EXEC_OutInfo("%s", pstRunner->acInputs);
    EXEC_Flush();
    
    return BS_OK;
}

HANDLE CmdExp_GetParam(VOID *pEnv)
{
    _CMD_EXP_ENV_S *pstEnv = pEnv;

    return pstEnv->pstCmdNode->hParam;
}

/* 获取当前的模式值, 例如"acl xyz"中的"xyz" */
CHAR * CmdExp_GetCurrentModeValue(IN VOID *pEnv)
{
    _CMD_EXP_ENV_S *pstEnv = pEnv;

    return pstEnv->pstRunner->pstCurrentMode->szModeValue;
}

/* 获取当前的模式名, 例如"acl xyz"中的"acl" */
CHAR * CmdExp_GetCurrentModeName(IN VOID *pEnv)
{
    _CMD_EXP_ENV_S *pstEnv = pEnv;

    return pstEnv->pstRunner->pstCurrentMode->szModeName;
}

static _CMD_EXP_NEW_MODE_NODE_S * cmdexp_GetHistroyMode(IN VOID *pEnv,
        IN UINT uiHistroyIndex)
{
    _CMD_EXP_ENV_S *pstEnv = pEnv;
    UINT uiIndex;
    UINT uiCount;

    if (uiHistroyIndex == 0) {
        return pstEnv->pstRunner->pstCurrentMode;
    }

    uiCount = HSTACK_GetCount(pstEnv->pstRunner->hModeStack);

    if (uiHistroyIndex > uiCount) {
        return NULL;
    }    

    uiIndex = uiCount - uiHistroyIndex;

    return HSTACK_GetValueByIndex(pstEnv->pstRunner->hModeStack, uiIndex);
}

/* 获取上级模式, 0表示当前模式, 1表示上一个模式, 2表示上上个模式, 以此类推 */
CHAR * CmdExp_GetUpModeValue(IN VOID *pEnv, IN UINT uiHistroyIndex)
{
    _CMD_EXP_NEW_MODE_NODE_S *pstModeTmp;

    pstModeTmp = cmdexp_GetHistroyMode(pEnv, uiHistroyIndex);
    if (NULL == pstModeTmp) {
        return NULL;
    }

    return pstModeTmp->szModeValue;
}

/* 获取上级模式, 0表示当前模式, 1表示上一个模式, 2表示上上个模式, 以此类推 */
CHAR * CmdExp_GetUpModeName(IN VOID *pEnv, IN UINT uiHistroyIndex)
{
    _CMD_EXP_NEW_MODE_NODE_S *pstModeTmp;

    pstModeTmp = cmdexp_GetHistroyMode(pEnv, uiHistroyIndex);
    if (NULL == pstModeTmp) {
        return NULL;
    }

    return pstModeTmp->szModeName;
}

static int cmdexp_MoveCursor(_CMD_EXP_RUNNER_S *pstRunner, EXCHAR enCmdType)
{
    UINT uiMoveLen;
    UINT i;

    if (enCmdType == EXCHAR_EXTEND_LEFT) {
        if (pstRunner->uiInputsPos >= pstRunner->ulInputsLen) {
            return BS_OK;
        }
        pstRunner->uiInputsPos ++;

        EXEC_OutInfo("\b");
        EXEC_Flush();
    } else if (enCmdType == EXCHAR_EXTEND_RIGHT) {
        if (pstRunner->uiInputsPos == 0) {
            return BS_OK;
        }

        int off = pstRunner->ulInputsLen - pstRunner->uiInputsPos;
        EXEC_OutInfo("%c", pstRunner->acInputs[off]);
        EXEC_Flush();

        pstRunner->uiInputsPos --;
    }
    else if (enCmdType == EXCHAR_EXTEND_HOME)
    {
        if (pstRunner->uiInputsPos >= pstRunner->ulInputsLen)
        {
            return BS_OK;
        }

        uiMoveLen = pstRunner->ulInputsLen - pstRunner->uiInputsPos;
        pstRunner->uiInputsPos = pstRunner->ulInputsLen;

        for (i=0; i<uiMoveLen; i++)
        {
            EXEC_OutInfo("\b");
        }
        EXEC_Flush();
    }
    else if (enCmdType == EXCHAR_EXTEND_END)
    {
        if (pstRunner->uiInputsPos == 0)
        {
            return BS_OK;
        }

        uiMoveLen = pstRunner->uiInputsPos;
        pstRunner->uiInputsPos = 0;
        for (i=0; i<uiMoveLen; i++)
        {
            EXEC_OutInfo("%c", pstRunner->acInputs[pstRunner->ulInputsLen - uiMoveLen + i]);
        }
        EXEC_Flush();
    }

    return BS_OK;
}

static int cmdexp_Run(CMD_EXP_RUNNER hRunner, UCHAR ucCmdChar)
{
    _CMD_EXP_RUNNER_S *pstRunner = hRunner;
    EXCHAR exchar;

    exchar = EXCHAR_Parse(pstRunner->hExcharHandle, ucCmdChar);
    if (exchar == EXCHAR_EXTEND_TEMP) {
        return BS_OK;
    }

    if ((exchar == EXCHAR_EXTEND_UP) || (exchar == EXCHAR_EXTEND_DOWN)) {
        cmdexp_RunHistroy(pstRunner, exchar);
        return BS_OK;
    }

    if ((exchar == EXCHAR_EXTEND_LEFT)
        || (exchar == EXCHAR_EXTEND_RIGHT)
        || (exchar == EXCHAR_EXTEND_HOME)
        || (exchar == EXCHAR_EXTEND_END)) {
        cmdexp_MoveCursor(pstRunner, exchar);
        return BS_OK;
    }

    if (exchar == EXCHAR_NORMAL_DELETE) {
        ucCmdChar = '\b';
    }

    if ((ucCmdChar == '&') || (ucCmdChar == '|') || (ucCmdChar == ';')) {
        ucCmdChar = '\n';
    }

    /* 将\r\n, \r都转换成\n */
    if ((ucCmdChar == '\n') && (pstRunner->bIsRChangeToN == TRUE)) {
        /* 如果上一个字符是\r，并且转换成了\n，则这个\n就不应该再被处理了 */
        pstRunner->bIsRChangeToN = FALSE;
        return BS_OK;
    } else if (ucCmdChar == '\r') {
        ucCmdChar = '\n';
        pstRunner->bIsRChangeToN = TRUE;
    } else {
        pstRunner->bIsRChangeToN = FALSE;
    }

    if (ucCmdChar == '\n') {
        pstRunner->ulHistoryIndex = 0;
    }

    return cmdexp_RunOneChar(pstRunner, (CHAR)ucCmdChar);
}

static void cmdexp_p(CMD_EXP_S *pstExp)
{
    if (pstExp->flag & CMD_EXP_FLAG_LOCK) {
        MUTEX_P(&pstExp->lock);
    }
}

static void cmdexp_v(CMD_EXP_S *pstExp)
{
    if (pstExp->flag & CMD_EXP_FLAG_LOCK) {
        MUTEX_V(&pstExp->lock);
    }
}

int CmdExp_Run(CMD_EXP_RUNNER hRunner, UCHAR ucCmdChar)
{
    _CMD_EXP_RUNNER_S *pstRunner = hRunner;
    int ret;

    cmdexp_p(pstRunner->cmdexp);
    ret = cmdexp_Run(hRunner, ucCmdChar);
    cmdexp_v(pstRunner->cmdexp);

    return ret;
}

static int cmdexp_RunLine(CMD_EXP_RUNNER hRunner, char *line)
{
    char *c;
    int ret = 0;

    if (line[0] == '\0') {
        return 0;
    }

    c = line;

    while (*c != 0) {
        ret = CmdExp_Run(hRunner, *c);
        c ++;
    }

    c--;
    if (*c != '?') {
        ret = CmdExp_Run(hRunner, '\n');
    }

    return ret;
}

int CmdExp_RunLine(CMD_EXP_RUNNER hRunner, char *line)
{
    _CMD_EXP_RUNNER_S *pstRunner = hRunner;
    int ret;

    cmdexp_p(pstRunner->cmdexp);
    ret = cmdexp_RunLine(hRunner, line);
    cmdexp_v(pstRunner->cmdexp);

    return ret;
}

static int cmdexp_RunString(CMD_EXP_RUNNER hRunner, char *string, int len)
{
    int i;

    for (i=0; i<len; i++) {
        if (BS_STOP == CmdExp_Run(hRunner, string[i])) {
            return BS_STOP;
        }
    }

    return 0;
}

int CmdExp_RunString(CMD_EXP_RUNNER hRunner, char *string, int len)
{
    _CMD_EXP_RUNNER_S *pstRunner = hRunner;
    int ret;

    cmdexp_p(pstRunner->cmdexp);
    ret = cmdexp_RunString(hRunner, string, len);
    cmdexp_v(pstRunner->cmdexp);

    return ret;
}

UINT CmdExp_GetOptFlag(IN CHAR *pcString)
{
    CHAR *pcTmp;
    UINT uiPrefixLen = sizeof(DEF_CMD_OPT_STR_PREFIX) - 1;
    UINT uiFlag = 0;

    if (strncmp(DEF_CMD_OPT_STR_PREFIX, pcString, uiPrefixLen) != 0) {
        return 0;
    }

    pcTmp = pcString + uiPrefixLen;

    while ((*pcTmp != '\0') && (*pcTmp != DEF_CMD_OPT_STR_END)) {
        if (*pcTmp == DEF_CMD_OPT_FLAG_STR_NOSAVE) {
            uiFlag |= DEF_CMD_OPT_FLAG_NOSAVE;
        } else if (*pcTmp == DEF_CMD_OPT_FLAG_STR_NOSHOW) {
            uiFlag |= DEF_CMD_OPT_FLAG_NOSHOW;
        }

        pcTmp ++;
    }

    return uiFlag;
}

/* 是否允许输出 */
BOOL_T CmdExp_IsOptPermitOutput(IN HANDLE hFile, IN CHAR *pcString)
{
    UINT uiFlag;
    _CMD_EXP_SHOW_SAVE_NODE_S *pstSSNode = hFile;

    uiFlag = CmdExp_GetOptFlag(pcString);

    if ((uiFlag & DEF_CMD_OPT_FLAG_NOSHOW)
            && (pstSSNode->bIsShowAll == 0)) {
        return FALSE;
    }

    if ((uiFlag & DEF_CMD_OPT_FLAG_NOSAVE)
            && (CmdExp_IsSaving(hFile))) {
        return FALSE;
    }

    return TRUE;
}

int CmdExp_DoCmd(CMD_EXP_HDL hCmdExp, char *cmd)
{
    CMD_EXP_RUNNER hRunner;

    hRunner = CmdExp_CreateRunner(hCmdExp);
    if (NULL == hRunner) {
        RETURN(BS_NO_MEMORY);
    }

    CmdExp_RunLine(hRunner, cmd);
    CmdExp_DestroyRunner(hRunner);

    return 0;
}

