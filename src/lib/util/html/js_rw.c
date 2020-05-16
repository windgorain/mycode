/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-10-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/html_parser.h"
#include "utl/dfa_utl.h"
#include "utl/action_utl.h"
#include "utl/txt_utl.h"
#include "utl/bit_opt.h"
#include "utl/que_utl.h"
#include "utl/stack_utl.h"
#include "utl/js_rw.h"


#define SSLVPN_JSPARSER_WORD_MAXLEN 15     /* js关键字的最大长度 */

#define SSLVPN_JSPARSER_MAX_WORD_COUNT 8   /* js关键字组成的数组的最大个数 */

#define SSLVPN_JSPARSER_FLAG_ATTR_VALUE_END         0x1  /* 属性值结束标志 */
#define SSLVPN_JSPARSER_FLAG_ATTR_VALUE_COND_EXPR   0x2  /* 解析到了属性值中的条件表达式中的问号 */
#define SSLVPN_JSPARSER_FLAG_NONE_SAVE 0x4               /* 表示不需要缓存当前字符 */

#define SSLVPN_JSPARSER_OLD_STATE_MAXNUM 8

/* 正则表达式前能够使用的关键字的最大长度 */
#define SSLVPN_JSPARSER_KEYWORD_BEFORE_REGEXPR_MAXLEN 7



/* 解析js文件时出现的状态定义 */
typedef enum tagSSLVPN_JSPARSER_LEX
{
    SSLVPN_JSPARSER_LEX_DATA,                  /* 0 处于解析普通数据状态 */
    SSLVPN_JSPARSER_LEX_WORD,                  /* 1 处于解析关键词(字母、数字、下划线组成)状态 */
    SSLVPN_JSPARSER_LEX_WORD_DOT,              /* 2 处于解析关键字状态时，遇到了字符'.'时将要进入的状态 */
    SSLVPN_JSPARSER_LEX_WORD_LWS,              /* 3 处于解析关键词后紧随的空白字符状态 */
    SSLVPN_JSPARSER_LEX_FUNC,                  /* 4 处于解析函数状态 */
    SSLVPN_JSPARSER_LEX_BEFORE_ATTR_VALUE,     /* 5 处于解析关键字状态时遇到字符'='需要进入到状态 */
    SSLVPN_JSPARSER_LEX_ATTR_VALUE,            /* 6 处于解析属性值状态 */
    SSLVPN_JSPARSER_LEX_COMMON_SLASH,          /* 7 处于解析字符'/' 状态 */
    SSLVPN_JSPARSER_LEX_REGEXPR,               /* 8 表示解析正则表达式 */
    SSLVPN_JSPARSER_LEX_SINGLE_COMMENT,        /* 9 处于解析单行注释状态 */
    SSLVPN_JSPARSER_LEX_MULTI_COMMENT,         /* 10 处于解析多行注释状态 */
    SSLVPN_JSPARSER_LEX_MULTI_COMMENT_END,     /* 11 处于多行注释状态时遇到字符'*'需要进入的状态 */
    SSLVPN_JSPARSER_LEX_DOUBLE_QUOTE,          /* 12 处于解析属性值中的双引号状态 */
    SSLVPN_JSPARSER_LEX_SINGLE_QUOTE,          /* 13 处于解析属性值中的单引号状态 */
    SSLVPN_JSPARSER_LEX_ESCAPE,                /* 14 */
    SSLVPN_JSPARSER_LEX_ATTR_VALUE_NEWLINE,    /* 15 */
    SSLVPN_JSPARSER_LEX_REGEXPR_BRACKET,       /* 16 处于正则表达式中的中括号状态 */

    SSLVPN_JSPARSER_LEX_MAX
}SSLVPN_JSPARSER_LEX_E;

/* js函数参数类型 */
typedef enum
{
    SSLVPN_JSPARSER_PARAM_URL = 0,       /* js函数参数为一个url */
    SSLVPN_JSPARSER_PARAM_JSCODE,        /* js函数参数为一段js代码 */
    SSLVPN_JSPARSER_PARAM_HTMLCODE,      /* js函数参数为一段html代码 */
    SSLVPN_JSPARSER_PARAM_WRITE,         /* js的write/writeln函数参数 */
    SSLVPN_JSPARSER_PARAM_EVAL,          /* js全局函数eval的参数 */
    SSLVPN_JSPARSER_PARAM_AJAX,          /* jquery的Ajax函数的参数 */
    SSLVPN_JSPARSER_PARAM_ATTRNAME,      /* attr和setAttribute方法的第一个参数 */
    SSLVPN_JSPARSER_PARAM_ATTRVALUE,     /* attr和setAttribute方法的第二个参数 */
    SSLVPN_JSPARSER_PARAM_MAX
}SSLVPN_JSPARSER_FUNC_PARAM_TYPE_E;

/* js属性值类型 */
typedef enum
{
    SSLVPN_JSPARSER_VALUE_HTMLCODE = 0, /* js属性值为一段html代码 */
    SSLVPN_JSPARSER_VALUE_URL,          /* js属性值为url */
    SSLVPN_JSPARSER_VALUE_COOKIE,       /* js属性值为cookie */

    SSLVPN_JSPARSER_VALUE_MAX
}SSLVPN_JSPARSER_ATTR_VALUE_TYPE_E;

typedef struct
{
    SSLVPN_JSPARSER_FUNC_PARAM_TYPE_E enParamType; /* 参数的类型 */
    UINT uiParamIndex;
}SSLVPN_JSPARSER_FUNC_PARAM_S;

typedef struct
{
    SSLVPN_JSPARSER_FUNC_PARAM_S *pstParamInfo;    /* 字符串对应的js函数参数信息 */
    UINT uiParamInfoCnt;                           /* 字符串对应的js函数参数信息个数 */
    CHAR *pcPattern;                               /* 需要改写的js函数应该包含的最小字符串 */
    UINT uiWordCnt;                                /* pcPattern中包含关键词的个数 */
}SSLVPN_JSPARSER_FUNC_PATTERN_S;

/* 定义需要改写的js属性使用的结构体 */
typedef struct
{
    SSLVPN_JSPARSER_ATTR_VALUE_TYPE_E enType;
    CHAR *pcPattern;
}SSLVPN_JSPARSER_ATTR_PATTERN_S;

/* JavaScript的关键字结构 */
typedef struct tagSSLVPN_JSPARSER_WORD
{
    CHAR acWord[SSLVPN_JSPARSER_WORD_MAXLEN + 1];
    UINT uiWordLen;
    BOOL_T bIsMember;
}SSLVPN_JSPARSER_WORD_S;

/* 保存连续多个Word的数组结构 */
typedef struct tagSSLVPN_JSPARSER_WORD_ARRAY
{
    /* 关键字组成的数组  
     * eg: document.write
     * astWords[0] --> document
     * astWords[1] --> write
     */
    SSLVPN_JSPARSER_WORD_S astWords[SSLVPN_JSPARSER_MAX_WORD_COUNT]; 
    UINT uiWordCount;
}SSLVPN_JSPARSER_WORD_ARRAY_S;

/* 需要改写的js函数信息结构 */
typedef struct tagSSLVPN_JSPARSER_FUNC_INFO
{
    SSLVPN_JSPARSER_FUNC_PARAM_S *pstCurParam;       /* 当前需要改写的函数参数信息*/
    SSLVPN_JSPARSER_FUNC_PARAM_S *pstRemanentParam;  /* 剩余的需要改写的参数信息 */
    UINT uiRemanentParamCnt;                         /* 剩余的需要改写的参数的个数 */
    UINT uiCurParamIndex;                            /* 当前改写参数索引 */
    UINT uiParamCounter;                             /* 解析函数每一个参数时使用的计数器 */
}SSLVPN_JSPARSER_FUNC_INFO_S;

typedef struct tagSSLVPN_JSPARSER_STATE
{
    SSLVPN_JSPARSER_LEX_E enState; /* 解析器所处状态值 */
    UINT uiInnerCounter;
    UINT uiOuterCounter;
    SSLVPN_JSPARSER_FUNC_INFO_S stFuncInfo;
}SSLVPN_JSPARSER_STATE_NODE_S;

/* 原有状态结构 */
typedef struct
{
    UINT auiState[SSLVPN_JSPARSER_OLD_STATE_MAXNUM];
    UINT uiStateNum;
}SSLVPN_JSPARSER_OLD_STATE_S;

/* 改写JavaScript文件时使用到的结构体 */
typedef struct tagSSLVPN_JSPARSER
{
    DFA_HANDLE hDfa;
    CHAR *pcOutputPtr;        /* 指向原始内存中,输出指针 */
    CHAR *pcCurrent;
    UINT uiJsDataLen;
    UINT uiFlags;
    SSLVPN_JSPARSER_WORD_ARRAY_S stWordArray;
    JS_RW_OUTPUT_PF pfOutput;
    SSLVPN_JSPARSER_FUNC_INFO_S stFuncInfo;
    VOID *pUserContext;
    SSLVPN_JSPARSER_OLD_STATE_S stOldState;
    BYTE_QUE_HANDLE hCharQue;
    HANDLE hStack; /* 记录解析状态信息的栈 */
    UINT uiInnerCounter;
    UINT uiOuterCounter;
    UINT uiCondExprFlagTimes;    /* 条件表达式('?')标记出现的次数 */
}SSLVPN_JSPARSER_S;

/***************************************** 局部函数声明:Begin ***************************************************/
STATIC VOID _jsparser_RewriteFunc(IN VOID *pActionInfo);
STATIC VOID _jsparser_RewriteAttrValue(IN VOID *pActionInfo);
STATIC VOID _jsparser_IncCounterInAttrValue(IN VOID *pActionInfo);
STATIC VOID _jsparser_IncCounterInFunc(IN VOID *pActionInfo);
STATIC VOID _jsparser_DecCounterInAttrValue(IN VOID *pActionInfo);
STATIC VOID _jsparser_DecCounterInFunc(IN VOID *pActionInfo);
STATIC VOID _jsparser_RecordState(IN VOID *pActionInfo);
STATIC VOID _jsparser_RecoverState(IN VOID *pActionInfo);
STATIC VOID _jsparser_EndAttrValue(IN VOID *pActionInfo);
STATIC VOID _jsparser_EndFunc(VOID *pActionInfo);
STATIC VOID _jsparser_InitWordArray(IN VOID *pActionInfo);
STATIC VOID _jsparser_SaveWord(IN VOID *pActionInfo);
STATIC VOID _jsparser_OutputDataIncludeCurPos(IN VOID *pActionInfo);
STATIC VOID _jsparser_ClearWordArray(IN VOID *pActionInfo);
STATIC VOID _jsparser_IncreaseWordCount(IN VOID *pActionInfo);
STATIC VOID _jsparser_OutputDataBeforeCurPos(IN VOID *pActionInfo);
STATIC VOID _jsparser_CheckArrayWordCount(IN VOID *pActionInfo);
STATIC VOID _jsparser_CheckLws(IN VOID *pActionInfo);
STATIC VOID _jsparser_CheckFuncInfo(IN VOID *pActionInfo);
STATIC VOID _jsparser_InitOldState(IN VOID *pActionInfo);
STATIC VOID _jsparser_SetCondExprFlag(IN VOID *pActionInfo);
STATIC VOID _jsparser_ClearCondExprFlag(IN VOID *pActionInfo);
STATIC VOID _jsparser_SaveCurCharToQueue(IN VOID *pActionInfo);
STATIC VOID _jsparser_CheckPrevChar(IN VOID *pActionInfo);
STATIC VOID _jsparser_IncOuterCounter(IN VOID *pActionInfo);
STATIC VOID _jsparser_DecOuterCounter(IN VOID *pActionInfo);
STATIC VOID _jsparser_CheckOuterCounter(IN VOID *pActionInfo);
STATIC VOID _jsparser_CheckAttrValue(IN VOID *pActionInfo);
STATIC VOID _jsparser_CheckNewLine(IN VOID *pActionInfo);
STATIC VOID _jsparser_SetNoneSaveFlag(IN VOID *pActionInfo);
STATIC VOID _jsparser_ClearNoneSaveFlag(IN VOID *pActionInfo);

/***************************************** 局部函数声明:End ***************************************************/

static ACTION_S g_astJsRwActions[] = 
{
    ACTION_LINE("_jsparser_RewriteFunc", _jsparser_RewriteFunc),
    ACTION_LINE("_jsparser_RewriteAttrValue", _jsparser_RewriteAttrValue),
    ACTION_LINE("_jsparser_IncCounterInAttrValue", _jsparser_IncCounterInAttrValue),
    ACTION_LINE("_jsparser_IncCounterInFunc", _jsparser_IncCounterInFunc),
    ACTION_LINE("_jsparser_DecCounterInAttrValue", _jsparser_DecCounterInAttrValue),
    ACTION_LINE("_jsparser_DecCounterInFunc", _jsparser_DecCounterInFunc),
    ACTION_LINE("_jsparser_RecordState", _jsparser_RecordState),
    ACTION_LINE("_jsparser_RecoverState", _jsparser_RecoverState),
    ACTION_LINE("_jsparser_EndAttrValue", _jsparser_EndAttrValue),
    ACTION_LINE("_jsparser_EndFunc", _jsparser_EndFunc),
    ACTION_LINE("_jsparser_InitWordArray", _jsparser_InitWordArray),
    ACTION_LINE("_jsparser_SaveWord", _jsparser_SaveWord),
    ACTION_LINE("_jsparser_OutputDataIncludeCurPos", _jsparser_OutputDataIncludeCurPos),
    ACTION_LINE("_jsparser_ClearWordArray", _jsparser_ClearWordArray),
    ACTION_LINE("_jsparser_IncreaseWordCount", _jsparser_IncreaseWordCount),
    ACTION_LINE("_jsparser_OutputDataBeforeCurPos", _jsparser_OutputDataBeforeCurPos),
    ACTION_LINE("_jsparser_CheckArrayWordCount", _jsparser_CheckArrayWordCount),
    ACTION_LINE("_jsparser_CheckLws", _jsparser_CheckLws),
    ACTION_LINE("_jsparser_CheckFuncInfo", _jsparser_CheckFuncInfo),
    ACTION_LINE("_jsparser_InitOldState", _jsparser_InitOldState),
    ACTION_LINE("_jsparser_SetCondExprFlag", _jsparser_SetCondExprFlag),
    ACTION_LINE("_jsparser_ClearCondExprFlag", _jsparser_ClearCondExprFlag),
    ACTION_LINE("_jsparser_SaveCurCharToQueue", _jsparser_SaveCurCharToQueue),
    ACTION_LINE("_jsparser_CheckPrevChar", _jsparser_CheckPrevChar),
    ACTION_LINE("_jsparser_IncOuterCounter", _jsparser_IncOuterCounter),
    ACTION_LINE("_jsparser_DecOuterCounter", _jsparser_DecOuterCounter),
    ACTION_LINE("_jsparser_CheckOuterCounter", _jsparser_CheckOuterCounter),
    ACTION_LINE("_jsparser_CheckAttrValue", _jsparser_CheckAttrValue),
    ACTION_LINE("_jsparser_CheckNewLine", _jsparser_CheckNewLine),
    ACTION_LINE("_jsparser_SetNoneSaveFlag", _jsparser_SetNoneSaveFlag),
    ACTION_LINE("_jsparser_ClearNoneSaveFlag", _jsparser_ClearNoneSaveFlag),
    ACTION_END
};


/* 当前状态机处于 SSLVPN_JSPARSER_LEX_DATA 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateData[] =
{
    {DFA_CODE_WORD, SSLVPN_JSPARSER_LEX_WORD, "_jsparser_InitWordArray,_jsparser_SaveWord"},
    {DFA_CODE_CHAR('/'), SSLVPN_JSPARSER_LEX_COMMON_SLASH, "_jsparser_InitOldState,_jsparser_RecordState"},
    {DFA_CODE_CHAR('.'), SSLVPN_JSPARSER_LEX_WORD_DOT, "_jsparser_ClearWordArray"},
    {DFA_CODE_CHAR('"'), SSLVPN_JSPARSER_LEX_DOUBLE_QUOTE, "_jsparser_InitOldState,_jsparser_RecordState,"},
    {DFA_CODE_CHAR('\''), SSLVPN_JSPARSER_LEX_SINGLE_QUOTE, "_jsparser_InitOldState,_jsparser_RecordState"},
    {DFA_CODE_CHAR('\\'), SSLVPN_JSPARSER_LEX_ESCAPE, "_jsparser_InitOldState,_jsparser_RecordState"},
    {DFA_CODE_CHAR('('), DFA_STATE_SELF, "_jsparser_IncOuterCounter"},
    {DFA_CODE_CHAR('{'), DFA_STATE_SELF, "_jsparser_IncOuterCounter"},
    {DFA_CODE_CHAR(')'), DFA_STATE_SELF, "_jsparser_CheckOuterCounter"},
    {DFA_CODE_CHAR('}'), DFA_STATE_SELF, "_jsparser_DecOuterCounter"},
    {DFA_CODE_CHAR(','), DFA_STATE_SELF, "_jsparser_CheckOuterCounter"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_END, DFA_STATE_SELF, NULL},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_WORD 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateWord[] =
{
    {DFA_CODE_WORD, DFA_STATE_SELF, "_jsparser_SaveWord"},
    {DFA_CODE_LWS, SSLVPN_JSPARSER_LEX_WORD_LWS, NULL},
    {DFA_CODE_CHAR('.'), SSLVPN_JSPARSER_LEX_WORD_DOT, "_jsparser_CheckArrayWordCount"},
    {DFA_CODE_CHAR('('), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_RewriteFunc,_jsparser_ClearWordArray"},     
    {DFA_CODE_CHAR('{'), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_IncOuterCounter"},
    {DFA_CODE_CHAR(')'), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_CheckOuterCounter"},
    {DFA_CODE_CHAR('}'), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_DecOuterCounter"},
    {DFA_CODE_CHAR(','), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_CheckOuterCounter"},
    {DFA_CODE_CHAR('='), SSLVPN_JSPARSER_LEX_BEFORE_ATTR_VALUE, NULL},
    {DFA_CODE_CHAR('/'), SSLVPN_JSPARSER_LEX_COMMON_SLASH, "_jsparser_InitOldState,_jsparser_ClearWordArray"},
    {DFA_CODE_CHAR('"'), SSLVPN_JSPARSER_LEX_DOUBLE_QUOTE, "_jsparser_InitOldState,_jsparser_ClearWordArray"},
    {DFA_CODE_CHAR('\''), SSLVPN_JSPARSER_LEX_SINGLE_QUOTE, "_jsparser_InitOldState,_jsparser_ClearWordArray"},
    {DFA_CODE_CHAR('\\'), SSLVPN_JSPARSER_LEX_ESCAPE, "_jsparser_InitOldState,_jsparser_ClearWordArray"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_END, SSLVPN_JSPARSER_LEX_DATA, NULL},
    {DFA_CODE_OTHER, SSLVPN_JSPARSER_LEX_DATA, "_jsparser_ClearWordArray"},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_WORD_DOT 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateWordDot[] =
{
    {DFA_CODE_WORD, SSLVPN_JSPARSER_LEX_WORD, "_jsparser_IncreaseWordCount,_jsparser_SaveWord"},
    {DFA_CODE_CHAR('/'), SSLVPN_JSPARSER_LEX_COMMON_SLASH, "_jsparser_InitOldState,_jsparser_ClearWordArray"},
    {DFA_CODE_CHAR('"'), SSLVPN_JSPARSER_LEX_DOUBLE_QUOTE, "_jsparser_InitOldState,_jsparser_ClearWordArray"},
    {DFA_CODE_CHAR('\''), SSLVPN_JSPARSER_LEX_SINGLE_QUOTE, "_jsparser_InitOldState,_jsparser_ClearWordArray"},
    {DFA_CODE_CHAR('\\'), SSLVPN_JSPARSER_LEX_ESCAPE, "_jsparser_InitOldState,_jsparser_ClearWordArray"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_END, SSLVPN_JSPARSER_LEX_DATA, NULL},
    {DFA_CODE_OTHER, SSLVPN_JSPARSER_LEX_DATA, "_jsparser_ClearWordArray"},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_WORD_LWS 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateWordLws[] =
{
    {DFA_CODE_LWS, DFA_STATE_SELF, NULL},
    {DFA_CODE_WORD, SSLVPN_JSPARSER_LEX_WORD, "_jsparser_InitWordArray,_jsparser_SaveWord"},
    {DFA_CODE_CHAR('('), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_RewriteFunc,_jsparser_ClearWordArray"},
    {DFA_CODE_CHAR('{'), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_IncOuterCounter"},                
    {DFA_CODE_CHAR(')'), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_CheckOuterCounter"},
    {DFA_CODE_CHAR('}'), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_DecOuterCounter"},        
    {DFA_CODE_CHAR(','), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_CheckOuterCounter"},
    {DFA_CODE_CHAR('='), SSLVPN_JSPARSER_LEX_BEFORE_ATTR_VALUE, NULL},
    {DFA_CODE_CHAR('/'), SSLVPN_JSPARSER_LEX_COMMON_SLASH, "_jsparser_InitOldState,_jsparser_ClearWordArray"},
    {DFA_CODE_CHAR('"'), SSLVPN_JSPARSER_LEX_DOUBLE_QUOTE, "_jsparser_InitOldState,_jsparser_ClearWordArray"},
    {DFA_CODE_CHAR('\''), SSLVPN_JSPARSER_LEX_SINGLE_QUOTE, "_jsparser_InitOldState,_jsparser_ClearWordArray"},
    {DFA_CODE_CHAR('\\'), SSLVPN_JSPARSER_LEX_ESCAPE, "_jsparser_InitOldState,_jsparser_ClearWordArray"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_END, SSLVPN_JSPARSER_LEX_DATA, NULL},
    {DFA_CODE_OTHER, SSLVPN_JSPARSER_LEX_DATA, "_jsparser_ClearWordArray"},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_FUNC 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateFunc[] =
{
    {DFA_CODE_CHAR('('), DFA_STATE_SELF, "_jsparser_IncCounterInFunc"},
    {DFA_CODE_CHAR('{'), DFA_STATE_SELF, "_jsparser_IncCounterInFunc"},
    {DFA_CODE_CHAR('['), DFA_STATE_SELF, "_jsparser_IncCounterInFunc"},
    {DFA_CODE_CHAR(')'), DFA_STATE_SELF, "_jsparser_DecCounterInFunc,_jsparser_EndFunc"},
    {DFA_CODE_CHAR('}'), DFA_STATE_SELF, "_jsparser_DecCounterInFunc,_jsparser_EndFunc"},
    {DFA_CODE_CHAR(']'), DFA_STATE_SELF, "_jsparser_DecCounterInFunc,_jsparser_EndFunc"},
    {DFA_CODE_CHAR('"'), SSLVPN_JSPARSER_LEX_DOUBLE_QUOTE, "_jsparser_InitOldState,_jsparser_RecordState"},
    {DFA_CODE_CHAR('\''), SSLVPN_JSPARSER_LEX_SINGLE_QUOTE, "_jsparser_InitOldState,_jsparser_RecordState"},
    {DFA_CODE_CHAR('\\'), SSLVPN_JSPARSER_LEX_ESCAPE, "_jsparser_InitOldState,_jsparser_RecordState"},
    {DFA_CODE_CHAR('/'), SSLVPN_JSPARSER_LEX_COMMON_SLASH, "_jsparser_InitOldState,_jsparser_RecordState"},
    {DFA_CODE_CHAR(','), DFA_STATE_SELF, "_jsparser_CheckFuncInfo"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_BEFORE_ATTR_VALUE 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateBeforeAttrValue[] =
{
    {DFA_CODE_CHAR('='), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_ClearWordArray"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, SSLVPN_JSPARSER_LEX_DATA, "_jsparser_OutputDataBeforeCurPos,_jsparser_InitOldState,_jsparser_RewriteAttrValue"},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_ATTR_VALUE 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateAttrValue[] =
{
    {DFA_CODE_CHAR('('), DFA_STATE_SELF, "_jsparser_IncCounterInAttrValue"},
    {DFA_CODE_CHAR(')'), DFA_STATE_SELF, "_jsparser_DecCounterInAttrValue,_jsparser_EndAttrValue"},
    {DFA_CODE_CHAR('{'), DFA_STATE_SELF, "_jsparser_IncCounterInAttrValue"},
    {DFA_CODE_CHAR('}'), DFA_STATE_SELF, "_jsparser_DecCounterInAttrValue,_jsparser_EndAttrValue"},
    {DFA_CODE_CHAR('['), DFA_STATE_SELF, "_jsparser_IncCounterInAttrValue"},
    {DFA_CODE_CHAR(']'), DFA_STATE_SELF, "_jsparser_DecCounterInAttrValue,_jsparser_EndAttrValue"},
    {DFA_CODE_CHAR('?'), DFA_STATE_SELF, "_jsparser_SetCondExprFlag"},
    {DFA_CODE_CHAR(';'), DFA_STATE_SELF, "_jsparser_EndAttrValue"},
    {DFA_CODE_CHAR(','), DFA_STATE_SELF, "_jsparser_EndAttrValue"},
    {DFA_CODE_CHAR(':'), DFA_STATE_SELF, "_jsparser_EndAttrValue,_jsparser_ClearCondExprFlag"},
    {DFA_CODE_CHAR('"'), SSLVPN_JSPARSER_LEX_DOUBLE_QUOTE, "_jsparser_InitOldState,_jsparser_RecordState"},
    {DFA_CODE_CHAR('\''), SSLVPN_JSPARSER_LEX_SINGLE_QUOTE, "_jsparser_InitOldState,_jsparser_RecordState"},
    {DFA_CODE_CHAR('\\'), SSLVPN_JSPARSER_LEX_ESCAPE, "_jsparser_InitOldState,_jsparser_RecordState"},
    {DFA_CODE_CHAR('/'), SSLVPN_JSPARSER_LEX_COMMON_SLASH, "_jsparser_InitOldState,_jsparser_RecordState"},
    {DFA_CODE_CHAR('\n'), DFA_STATE_SELF, "_jsparser_CheckNewLine"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_END, SSLVPN_JSPARSER_LEX_DATA, "_jsparser_EndAttrValue"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_COMMON_SLASH 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateCommonSlash[] =
{
    {DFA_CODE_CHAR('/'), SSLVPN_JSPARSER_LEX_SINGLE_COMMENT, "_jsparser_SetNoneSaveFlag"},
    {DFA_CODE_CHAR('*'), SSLVPN_JSPARSER_LEX_MULTI_COMMENT, "_jsparser_SetNoneSaveFlag"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_END, SSLVPN_JSPARSER_LEX_DATA, NULL},
    {DFA_CODE_OTHER, SSLVPN_JSPARSER_LEX_DATA, "_jsparser_CheckPrevChar"},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_REGEXPR 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateRegExpr[] =
{
    {DFA_CODE_CHAR('/'), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_RecoverState"},
    {DFA_CODE_CHAR('['), SSLVPN_JSPARSER_LEX_REGEXPR_BRACKET, NULL},
    {DFA_CODE_CHAR('\\'), SSLVPN_JSPARSER_LEX_ESCAPE, "_jsparser_RecordState"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_SINGLE_COMMENT 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateSingleComment[] =
{
    {DFA_CODE_LWS, DFA_STATE_SELF, "_jsparser_CheckLws"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL},
};


/* 当前状态机处于 SSLVPN_JSPARSER_LEX_MULTI_COMMENT 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateMultiComment[] =
{
    {DFA_CODE_CHAR('*'), SSLVPN_JSPARSER_LEX_MULTI_COMMENT_END, NULL},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_END, DFA_STATE_SELF, NULL},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_MULTI_COMMENT_END 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateMultiCommentEnd[] =
{
    {DFA_CODE_CHAR('/'), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_RecoverState,_jsparser_ClearNoneSaveFlag"},
    {DFA_CODE_CHAR('*'), DFA_STATE_SELF, NULL},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, SSLVPN_JSPARSER_LEX_MULTI_COMMENT, NULL},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_DOUBLE_QUOTE 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateDoubleQuote[] =
{
    {DFA_CODE_CHAR('"'), DFA_STATE_SELF, "_jsparser_RecoverState"},
    {DFA_CODE_CHAR('\\'), SSLVPN_JSPARSER_LEX_ESCAPE, "_jsparser_RecordState"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_SINGLE_QUOTE 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateSingleQuote[] =
{
    {DFA_CODE_CHAR('\''), DFA_STATE_SELF, "_jsparser_RecoverState"},
    {DFA_CODE_CHAR('\\'), SSLVPN_JSPARSER_LEX_ESCAPE, "_jsparser_RecordState"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_ESCAPE 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateEscape[] =
{
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, SSLVPN_JSPARSER_LEX_DATA, "_jsparser_RecoverState"},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_ATTR_VALUE_NEWLINE 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateAttrValueNewLine[] =
{
    {DFA_CODE_LWS, DFA_STATE_SELF, NULL},        
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, SSLVPN_JSPARSER_LEX_DATA, "_jsparser_CheckAttrValue"},
    {DFA_CODE_END, DFA_STATE_SELF, "_jsparser_EndAttrValue"},
};

/* 当前状态机处于 SSLVPN_JSPARSER_LEX_REGEXPR_BRACKET 状态时对应的状态迁移表 */
STATIC DFA_NODE_S g_astSslvpnJsParserStateRegExprBracket[] =
{
    {DFA_CODE_CHAR(']'), SSLVPN_JSPARSER_LEX_REGEXPR, NULL},
    {DFA_CODE_CHAR('\\'), SSLVPN_JSPARSER_LEX_ESCAPE, "_jsparser_RecordState"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL},
};


STATIC DFA_TBL_LINE_S g_astSslvpnJsParserDFATbl[] =
{
    DFA_TBL_LINE(g_astSslvpnJsParserStateData),
    DFA_TBL_LINE(g_astSslvpnJsParserStateWord),
    DFA_TBL_LINE(g_astSslvpnJsParserStateWordDot),
    DFA_TBL_LINE(g_astSslvpnJsParserStateWordLws),
    DFA_TBL_LINE(g_astSslvpnJsParserStateFunc),
    DFA_TBL_LINE(g_astSslvpnJsParserStateBeforeAttrValue),
    DFA_TBL_LINE(g_astSslvpnJsParserStateAttrValue),
    DFA_TBL_LINE(g_astSslvpnJsParserStateCommonSlash),
    DFA_TBL_LINE(g_astSslvpnJsParserStateRegExpr),
    DFA_TBL_LINE(g_astSslvpnJsParserStateSingleComment),
    DFA_TBL_LINE(g_astSslvpnJsParserStateMultiComment),
    DFA_TBL_LINE(g_astSslvpnJsParserStateMultiCommentEnd),
    DFA_TBL_LINE(g_astSslvpnJsParserStateDoubleQuote),
    DFA_TBL_LINE(g_astSslvpnJsParserStateSingleQuote),
    DFA_TBL_LINE(g_astSslvpnJsParserStateEscape),
    DFA_TBL_LINE(g_astSslvpnJsParserStateAttrValueNewLine),
    DFA_TBL_LINE(g_astSslvpnJsParserStateRegExprBracket),
    DFA_TBL_END
};

/* write/writeln函数的参数信息 */
STATIC SSLVPN_JSPARSER_FUNC_PARAM_S g_astSslvpnJsParserWrite[] =
{
    {SSLVPN_JSPARSER_PARAM_WRITE, 0}
};

/* open函数的参数信息
   需要改写的open函数有以下几种:
   (1)HTML DOM Window 对象的open函数 open(URL,name,features,replace)
   (2)Ajax函数的open函数 open(方法,URL,bool值)

   处理方法是将前两个参数都当做URL来处理
*/
STATIC SSLVPN_JSPARSER_FUNC_PARAM_S g_astSslvpnJsParserOpen[] =
{
    {SSLVPN_JSPARSER_PARAM_URL, 0},
    {SSLVPN_JSPARSER_PARAM_URL, 1}
};

/*
   第一个参数为js代码形式的函数参数信息，此类函数有以下几种
   (1)setTimeOut
   (2)setInterval
*/
STATIC SSLVPN_JSPARSER_FUNC_PARAM_S g_astSslvpnJsParserFuncParamJsCode[] =
{
    {SSLVPN_JSPARSER_PARAM_JSCODE, 0}
};

/* eval函数的参数 */
STATIC SSLVPN_JSPARSER_FUNC_PARAM_S g_astSslvpnJsParserFuncEval[] =
{
    {SSLVPN_JSPARSER_PARAM_EVAL, 0}
};

/* 第一个参数为url形式的函数参数信息，此类函数有以下几种
   (1)location.assign、location.replace
   (2)jQuery的load函数
*/
STATIC SSLVPN_JSPARSER_FUNC_PARAM_S g_astSslvpnJsParserFuncParamUrl0[] =
{
    {SSLVPN_JSPARSER_PARAM_URL, 0}
};

/* jQuery的Ajax函数的参数信息 */
STATIC SSLVPN_JSPARSER_FUNC_PARAM_S g_astSslvpnJsParserAjax[] =
{
    {SSLVPN_JSPARSER_PARAM_AJAX, 0}
};

/* (1)Element的setAttribute方法的参数信息 
 * (2)jQuery的attr方法的参数信息
 */
STATIC SSLVPN_JSPARSER_FUNC_PARAM_S g_astSslvpnJsParserAttr[] =
{
    {SSLVPN_JSPARSER_PARAM_ATTRNAME, 0},
    {SSLVPN_JSPARSER_PARAM_ATTRVALUE, 1}
};


#define SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(astParamInfo, pcPattern, uiWordCnt) \
    {(astParamInfo), sizeof(astParamInfo)/sizeof(astParamInfo[0]), pcPattern, uiWordCnt}

/* 可能包含需要改写的URL的HTML DOM对象方法数组 */
STATIC SSLVPN_JSPARSER_FUNC_PATTERN_S g_astSslvpnJsParserFuncs[] =
{
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserWrite, "write", 1),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserWrite, "writeln", 1),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserFuncParamUrl0, "location.assign", 2),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserFuncParamUrl0, ".location.assign", 2),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserFuncParamUrl0, "location.replace", 2),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserFuncParamUrl0, ".location.replace", 2),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserFuncParamUrl0, ".load", 1),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserFuncParamUrl0, "showModalDialog", 1),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserFuncParamUrl0, "showModelessDialog", 1),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserOpen, ".open", 1),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserFuncParamJsCode, "setTimeout", 1),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserFuncParamJsCode, "setInterval", 1),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserFuncEval, "eval", 1),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserAjax, ".ajax", 1),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserAttr, ".setAttribute", 1),
    SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(g_astSslvpnJsParserAttr, ".attr", 1),
};

/* 改写js属性时需要使用的字符串数组 */
STATIC CHAR *g_apcSslvpnJsParserAttrRwStrings[SSLVPN_JSPARSER_VALUE_MAX] =
{
    " s_r_htmlcode(",
    " s_r_url(",
    " s_r_cookie(",
};

/* 可能包含需要改写的URL的HTML DOM对象属性数组 */
STATIC SSLVPN_JSPARSER_ATTR_PATTERN_S g_astSslvpnJsParserAttrs[] =
{
    {SSLVPN_JSPARSER_VALUE_HTMLCODE, ".innerHTML"},
    {SSLVPN_JSPARSER_VALUE_HTMLCODE, ".outerHTML"},
    {SSLVPN_JSPARSER_VALUE_URL, ".location"},
    {SSLVPN_JSPARSER_VALUE_URL, "location"},
    {SSLVPN_JSPARSER_VALUE_URL, ".href"},
    {SSLVPN_JSPARSER_VALUE_URL, ".action"},
    {SSLVPN_JSPARSER_VALUE_URL, ".src"},
    {SSLVPN_JSPARSER_VALUE_URL, ".backgroundImage"},
    {SSLVPN_JSPARSER_VALUE_URL, ".background"},
    {SSLVPN_JSPARSER_VALUE_COOKIE, ".cookie"},
};

/* 正则表达式前能够使用的关键字列表 */
STATIC LSTR_S g_astSslvpnJsParserKeyword[] = 
{
    {"in", 2},
    {"return", 6},
};

/* 根据js函数参数类型获取改写字符串 */
STATIC CHAR * _jsparser_GetRwStringByParamType(IN SSLVPN_JSPARSER_FUNC_PARAM_TYPE_E enParamType)
{
    CHAR *pcRwString = NULL;

    switch (enParamType)
    {
        case SSLVPN_JSPARSER_PARAM_URL:
        {
            pcRwString = "s_r_url(";
            break;
        }
        case SSLVPN_JSPARSER_PARAM_EVAL:
        case SSLVPN_JSPARSER_PARAM_JSCODE:
        {
            pcRwString = "s_r_jscode(";
            break;
        }
        case SSLVPN_JSPARSER_PARAM_HTMLCODE:
        {
            pcRwString = "s_r_htmlcode(";
            break;
        }
        case SSLVPN_JSPARSER_PARAM_WRITE:
        {
            pcRwString = "s_r_write(";
            break;
        }
        case SSLVPN_JSPARSER_PARAM_AJAX:
        {
            pcRwString = "s_r_ajax(";
            break;
        }
        case SSLVPN_JSPARSER_PARAM_ATTRNAME:
        {
            pcRwString = "s_r_attr(";
            break;
        }
        case SSLVPN_JSPARSER_PARAM_ATTRVALUE:
        {
            pcRwString = "s_r_attrvalue(";
            break;
        }
        default:
        {
            DBGASSERT(0);
            break;
        }
    }

    return pcRwString;
}


/* 初始化Word数组 */
STATIC VOID _jsparser_InitWordArray(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray = &pstJsParser->stWordArray;

    memset(pstWordArray, 0, sizeof(SSLVPN_JSPARSER_WORD_ARRAY_S));

    pstWordArray->uiWordCount = 1;

    return;
}

/* Word个数加1处理 */
STATIC VOID _jsparser_IncreaseWordCount(IN DFA_HANDLE hDfa)
{
    UINT uiIndex = 0;
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray = &pstJsParser->stWordArray;
    SSLVPN_JSPARSER_WORD_S *pstWord;

    /* 进入该函数，关键词的个数肯定不能大于SSLVPN_JSPARSER_ARRAY_MAX_WORD_COUNT */
    DBGASSERT(SSLVPN_JSPARSER_MAX_WORD_COUNT > pstWordArray->uiWordCount);

    pstWordArray->uiWordCount++;

    uiIndex = pstWordArray->uiWordCount - 1;
    pstWord = pstWordArray->astWords + uiIndex;
    pstWord->bIsMember = BOOL_TRUE;

    return;
}

/* 将当前字符保存至解析器结构下的word数组中 */
STATIC VOID _jsparser_SaveWord(IN DFA_HANDLE hDfa)
{
    UINT uiIndex = 0;
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray = &pstJsParser->stWordArray;
    SSLVPN_JSPARSER_WORD_S *pstWord = NULL;

    DBGASSERT(0 < pstWordArray->uiWordCount);

    uiIndex = pstWordArray->uiWordCount - 1;
    pstWord = pstWordArray->astWords + uiIndex;
    if (SSLVPN_JSPARSER_WORD_MAXLEN > pstWord->uiWordLen)
    {
        pstWord->acWord[pstWord->uiWordLen] = *pstJsParser->pcCurrent;
        pstWord->uiWordLen++;
    }
    else
    {
        /* 如果word长度超长，则视为普通数据，当前状态切换到 SSLVPN_JSPARSER_LEX_DATA */
        _jsparser_ClearWordArray(hDfa);
        DFA_SetState(pstJsParser->hDfa, SSLVPN_JSPARSER_LEX_DATA);
    }

    return;
}

/* 将包括当前位置的数据全部输出 */
STATIC VOID _jsparser_OutputDataIncludeCurPos(IN DFA_HANDLE hDfa)
{
    ULONG ulDataLen;
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    ulDataLen = (ULONG)(pstJsParser->pcCurrent + 1) - (ULONG)pstJsParser->pcOutputPtr;
    if (0 < ulDataLen && ulDataLen <= pstJsParser->uiJsDataLen)
    {
        pstJsParser->pfOutput(pstJsParser->pcOutputPtr, ulDataLen, pstJsParser->pUserContext);
        pstJsParser->pcOutputPtr += ulDataLen;
    }

    return;
}

/* 将当前位置之前的所有数据输出 */
STATIC VOID _jsparser_OutputDataBeforeCurPos(IN DFA_HANDLE hDfa)
{
    ULONG ulDataLen;
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    ulDataLen = (ULONG)pstJsParser->pcCurrent - (ULONG)pstJsParser->pcOutputPtr;
    if (0 < ulDataLen && ulDataLen <= pstJsParser->uiJsDataLen)
    {
        pstJsParser->pfOutput(pstJsParser->pcOutputPtr, ulDataLen, pstJsParser->pUserContext);
        pstJsParser->pcOutputPtr += ulDataLen;
    }

    return;
}

/* 输出改写字符串 */
STATIC VOID _jsparser_OutputRwString(IN CHAR *pcRwString, IN SSLVPN_JSPARSER_S *pstJsParser)
{
    pstJsParser->pfOutput(pcRwString, strlen(pcRwString), pstJsParser->pUserContext);

    return;
}

/* 清除Word数组中的数据 */
STATIC VOID _jsparser_ClearWordArray(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray = &pstJsParser->stWordArray;

    memset(pstWordArray, 0, sizeof(SSLVPN_JSPARSER_WORD_ARRAY_S));

    return;
}

/* 检查Word数组中Word个数的合法性 */
STATIC VOID _jsparser_CheckArrayWordCount(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray = &pstJsParser->stWordArray;

    if (SSLVPN_JSPARSER_MAX_WORD_COUNT <= pstWordArray->uiWordCount)
    {
        _jsparser_ClearWordArray(hDfa);
        DFA_SetState(pstJsParser->hDfa, SSLVPN_JSPARSER_LEX_DATA);
    }

    return;
}

/* 关闭属性值的处理 */
STATIC VOID _jsparser_EndAttrValue(IN DFA_HANDLE hDfa)
{
    UINT uiInputCode = 0;
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
	SSLVPN_JSPARSER_LEX_E enState = SSLVPN_JSPARSER_LEX_MAX;

    if (0 != pstJsParser->uiInnerCounter)
    {
        return;
    }
    
	uiInputCode = DFA_GetInputCode(hDfa);
    switch (uiInputCode)
    {
        case ';':
        case ',':
        {
            _jsparser_OutputDataBeforeCurPos(hDfa);

            pstJsParser->pfOutput(")", 1UL, pstJsParser->pUserContext);

            pstJsParser->pfOutput(pstJsParser->pcCurrent, 1UL, pstJsParser->pUserContext);
            pstJsParser->pcOutputPtr++;

            enState = SSLVPN_JSPARSER_LEX_DATA;

            break;
        }
        case ':':
        {
            /* 需要改写的JavaScript属性的值中包含条件表达式，则冒号(':')不能视为属性值的结束位置 
             * 
             *  location.href = aaa ? bbb : ccc; 
             *     ==> location.href = sslvpn_rewrite_url( aaa ? bbb : ccc);
             *  aaa = bbb ? location.href = ccc : ddd; 
             *     ==>aaa = bbb ? location.href = sslvpn_rewrite_url(ccc) : ddd;
             */
            if (!BIT_TEST(pstJsParser->uiFlags, SSLVPN_JSPARSER_FLAG_ATTR_VALUE_COND_EXPR))
            {
                _jsparser_OutputDataBeforeCurPos(hDfa);

                pstJsParser->pfOutput(")", 1UL, pstJsParser->pUserContext);

                pstJsParser->pfOutput(pstJsParser->pcCurrent, 1UL, pstJsParser->pUserContext);
                pstJsParser->pcOutputPtr++;

                enState = SSLVPN_JSPARSER_LEX_DATA;
            }
            else
            {
                _jsparser_ClearCondExprFlag(hDfa);
            }

            break;
        }
        case '}':
        {
            if (BIT_TEST(pstJsParser->uiFlags, SSLVPN_JSPARSER_FLAG_ATTR_VALUE_END))
            {
                _jsparser_OutputDataBeforeCurPos(hDfa);
                BIT_RESET(pstJsParser->uiFlags, SSLVPN_JSPARSER_FLAG_ATTR_VALUE_END);
            }
            else
            {
                _jsparser_OutputDataIncludeCurPos(hDfa);
            }

            pstJsParser->pfOutput(")", 1UL, pstJsParser->pUserContext);

            enState = SSLVPN_JSPARSER_LEX_DATA;

            break;
        }
        case ']':
        case ')':
        {
            /* 当前字符为']'或')',只有设置了属性值结束标记，才表示属性值结束，否则，需要继续进行属性值的解析 
             * var s=[this.cookie||(this.cookie=this.options.cookie.name||"ui-tabs-"+ ++t)];
             *                                                                            ^
             * var s=(this.cookie||(this.cookie=this.options.cookie.name||"ui-tabs-"+ ++t));
             *                                                                           ^
             */
            if (BIT_TEST(pstJsParser->uiFlags, SSLVPN_JSPARSER_FLAG_ATTR_VALUE_END))
            {
                _jsparser_OutputDataBeforeCurPos(hDfa);
                pstJsParser->pfOutput(")", 1UL, pstJsParser->pUserContext);
                
                BIT_RESET(pstJsParser->uiFlags, SSLVPN_JSPARSER_FLAG_ATTR_VALUE_END);
                
                enState = SSLVPN_JSPARSER_LEX_DATA;
            }
            
            break;
        }
        case DFA_CODE_END:
        {
            pstJsParser->pfOutput(")", 1UL, pstJsParser->pUserContext);

            break;
        }
        default:
        {
            break;
        }
    }

    if (SSLVPN_JSPARSER_LEX_MAX != enState)
    {
        DFA_SetState(pstJsParser->hDfa, enState);
    }

    return;
}

/*****************************************************************************
  Description: 将计数器进行加1操作
*****************************************************************************/
STATIC VOID _jsparser_IncCounterInAttrValue(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    pstJsParser->uiInnerCounter++;

    return;
}

/*****************************************************************************
  Description: 将计数器进行加1操作
*****************************************************************************/
STATIC VOID _jsparser_IncCounterInFunc(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_FUNC_INFO_S *pstFuncInfo = &pstJsParser->stFuncInfo;

    pstJsParser->uiInnerCounter++;
    pstFuncInfo->uiParamCounter++;

    return;
}

/*****************************************************************************
  Description: 解析器处于解析属性值状态时，将计数器进行减1操作
 *****************************************************************************/
STATIC VOID _jsparser_DecCounterInAttrValue(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    if (0 < pstJsParser->uiInnerCounter)
    {
        pstJsParser->uiInnerCounter--;
    }
    else
    {
        BIT_SET(pstJsParser->uiFlags, SSLVPN_JSPARSER_FLAG_ATTR_VALUE_END);

        if (']' != *pstJsParser->pcCurrent)
        {
            pstJsParser->uiOuterCounter--;
        }
    }

    return;
}

/*****************************************************************************
  Description: 解析器处于解析函数状态时，将计数器进行减1操作
*****************************************************************************/
STATIC VOID _jsparser_DecCounterInFunc(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_FUNC_INFO_S *pstFuncInfo = &pstJsParser->stFuncInfo;

    DBGASSERT(0 < pstJsParser->uiInnerCounter);

    pstJsParser->uiInnerCounter--;

    if (0 < pstFuncInfo->uiParamCounter)
    {
        pstFuncInfo->uiParamCounter--;
    }

    return;
}

/*****************************************************************************
  Description: 初始化原有状态信息
 *****************************************************************************/
STATIC VOID _jsparser_InitOldState(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa); 
    SSLVPN_JSPARSER_OLD_STATE_S *pstOldState = &pstJsParser->stOldState;

    DBGASSERT(0 == pstOldState->uiStateNum);
    pstOldState->uiStateNum = 0;

    return;
}

/*****************************************************************************
  Description: 保存先前状态
*****************************************************************************/
STATIC VOID _jsparser_SaveOldState
(
    IN SSLVPN_JSPARSER_LEX_E enOldState, 
    INOUT SSLVPN_JSPARSER_OLD_STATE_S *pstOldState
)
{
    /* 记录当前状态 */
    pstOldState->auiState[pstOldState->uiStateNum] = enOldState;
    pstOldState->uiStateNum++;

    return ;
}

/*****************************************************************************
  Description: 恢复先前状态
*****************************************************************************/
STATIC VOID _jsparser_RestoreOldState(IN SSLVPN_JSPARSER_S *pstJsParser)
{
    SSLVPN_JSPARSER_OLD_STATE_S *pstOldState = &pstJsParser->stOldState;

    if (0 == pstOldState->uiStateNum)
    {
        DFA_SetState(pstJsParser->hDfa, SSLVPN_JSPARSER_LEX_DATA);
    }
    else
    {
        pstOldState->uiStateNum--;
        DFA_SetState(pstJsParser->hDfa, pstOldState->auiState[pstOldState->uiStateNum]);
    }

    return;
}

/*****************************************************************************
  Description: 记录当前状态
*****************************************************************************/
STATIC VOID _jsparser_RecordState(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa); 

    /* 记录当前状态 */
    _jsparser_SaveOldState(DFA_GetOldState(hDfa), &pstJsParser->stOldState);

    return;
}

/*****************************************************************************
    Func Name: _jsparser_RecoverState
 Date Created: 2015/1/8
       Author: wkf3789
  Description: 恢复原始记录状态
        Input: VOID *pActionInfo
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC VOID _jsparser_RecoverState(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa); 

    _jsparser_RestoreOldState(pstJsParser);

    return;
}

/*****************************************************************************
    Func Name: _jsparser_CmpArrayWithFuncPattern
 Date Created: 2015/1/8
       Author: wkf3789
  Description: 比较Word数组和预先定义的js函数模式
        Input: const SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray
               const SSLVPN_JSPARSER_ATTR_PATTERN_S *pstPattern
       Output: NONE
       Return: INT,  0 - 相同
                    -1 - 不相同
      Caution: NONE
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC INT _jsparser_CmpArrayWithFuncPattern
(
    IN const SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray,
    IN const SSLVPN_JSPARSER_FUNC_PATTERN_S *pstPattern
)
{
    INT iCmpRet = 0;
    UINT uiIndex = 0;
    CHAR *pcTmpPattern = pstPattern->pcPattern;
    const SSLVPN_JSPARSER_WORD_S *pstWord = NULL;

    if (pstWordArray->uiWordCount < pstPattern->uiWordCnt)
    {
        return -1;
    }

    /* 获取比较的起始位置 */
    uiIndex = pstWordArray->uiWordCount - pstPattern->uiWordCnt;
    if ('.' == *pcTmpPattern)
    {
        pstWord = pstWordArray->astWords + uiIndex;
        if (BOOL_TRUE != pstWord->bIsMember)
        {
            return -1;
        }

        pcTmpPattern++;
    }

    for (; uiIndex < pstWordArray->uiWordCount; uiIndex++)
    {
        pstWord = pstWordArray->astWords + uiIndex;

        if ((pstWord->uiWordLen > strlen(pcTmpPattern)) ||
            (0 != strncmp(pcTmpPattern, pstWord->acWord, (ULONG)pstWord->uiWordLen)))
        {
            iCmpRet = -1;
            break;
        }

        pcTmpPattern += pstWord->uiWordLen;

        if (uiIndex + 1 < pstWordArray->uiWordCount)
        {
            if ('.' != *pcTmpPattern)
            {
                iCmpRet = -1;
                break;
            }

            pcTmpPattern ++;
        }
        else
        {
            if ('\0' != *pcTmpPattern)
            {
                iCmpRet = -1;
                break;
            }
        }
    }

    return iCmpRet;
}

/*****************************************************************************
    Func Name: _jsparser_CmpArrayWithAttrPattern
 Date Created: 2015/1/8
       Author: wkf3789
  Description: 比较Word数组和预先定义的js属性模式
        Input: const SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray
               const SSLVPN_JSPARSER_ATTR_PATTERN_S *pstPattern
       Output: NONE
       Return: INT,  0 - 相同
                    -1 - 不相同
      Caution: NONE
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC INT _jsparser_CmpArrayWithAttrPattern
(
    IN const SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray,
    IN const SSLVPN_JSPARSER_ATTR_PATTERN_S *pstPattern
)
{
    UINT uiLastIndex = 0;
    const SSLVPN_JSPARSER_WORD_S *pstWord = NULL;
    CHAR *pcTmpPattern = pstPattern->pcPattern;

    uiLastIndex = pstWordArray->uiWordCount - 1;
    pstWord = pstWordArray->astWords + uiLastIndex;
    if ('.' == *pcTmpPattern)
    {
        if (BOOL_TRUE != pstWord->bIsMember)
        {
            return -1;
        }

        pcTmpPattern ++;
    }

    if (strlen(pcTmpPattern) != pstWord->uiWordLen)
    {
        return -1;
    }

    if (0 != strcmp(pcTmpPattern, pstWord->acWord))
    {
        return -1;
    }

    return 0;
}

/*****************************************************************************
    Func Name: _jsparser_GetFuncPattern
 Date Created: 2015/1/8
       Author: wkf3789
  Description: 获取需要改写的函数模式
        Input: const SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC SSLVPN_JSPARSER_FUNC_PATTERN_S * _jsparser_GetFuncPattern(IN const SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray)
{
    INT iCmpResult = 0;
    UINT uiIndex;
    SSLVPN_JSPARSER_FUNC_PATTERN_S *pstPattern = NULL;

    if (0 == pstWordArray->uiWordCount)
    {
        return pstPattern;
    }

    for (uiIndex = 0; uiIndex < sizeof(g_astSslvpnJsParserFuncs)/sizeof(SSLVPN_JSPARSER_FUNC_PATTERN_S); uiIndex++)
    {
        iCmpResult = _jsparser_CmpArrayWithFuncPattern(pstWordArray, &g_astSslvpnJsParserFuncs[uiIndex]);
        if (0 == iCmpResult)
        {
            pstPattern = &g_astSslvpnJsParserFuncs[uiIndex];
            break;
        }
    }

    return pstPattern;
}

/*****************************************************************************
    Func Name: _jsparser_GetAttrPattern
 Date Created: 2015/1/8
       Author: wkf3789
  Description: 获取需要改写的属性模式
        Input: const SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC SSLVPN_JSPARSER_ATTR_PATTERN_S * _jsparser_GetAttrPattern(IN const SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray)
{
    INT iCmpResult = -1;
    UINT uiIndex;
    SSLVPN_JSPARSER_ATTR_PATTERN_S *pstPattern = NULL;

    if (0 == pstWordArray->uiWordCount)
    {
        return pstPattern;
    }

    for (uiIndex = 0; uiIndex < sizeof(g_astSslvpnJsParserAttrs)/sizeof(SSLVPN_JSPARSER_ATTR_PATTERN_S); uiIndex++)
    {
        iCmpResult = _jsparser_CmpArrayWithAttrPattern(pstWordArray, &g_astSslvpnJsParserAttrs[uiIndex]);
        if (0 == iCmpResult)
        {
            pstPattern = &g_astSslvpnJsParserAttrs[uiIndex];
            break;
        }
    }

    return pstPattern;
}

/*****************************************************************************
    Func Name: _jsparser_CheckLws
 Date Created: 2015/1/6
       Author: wkf3789
  Description: 检查空白字符
        Input: VOID *pActionInfo
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC VOID _jsparser_CheckLws(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa); 

    /* 如果空白字符为换行(有些场景只有'\r'也能实现换行)，则表明单行注释结束 */
    if ('\n' == *pstJsParser->pcCurrent || '\r' == *pstJsParser->pcCurrent)
    {
        _jsparser_RecoverState(hDfa);
        _jsparser_ClearNoneSaveFlag(hDfa);
    }

    return;
}

/*****************************************************************************
    Func Name: _jsparser_SetFuncParamInfo
 Date Created: 2015/1/26
       Author: wkf3789
  Description: 设置js函数的需要改写的参数信息
        Input: SSLVPN_JSPARSER_FUNC_PARAM_S *pstFuncParam - 需要改写的参数信息首地址
               UINT uiFuncParamCnt                        - 需要改写的参数的个数
               UINT uiCurParamIndex                       - 当前解析到的参数的索引
       Output: SSLVPN_JSPARSER_FUNC_INFO_S *pstFuncInfo   - 需要改写的函数信息首地址
       Return: none
      Caution: none
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC VOID _jsparser_SetFuncParamInfo
(
    IN SSLVPN_JSPARSER_FUNC_PARAM_S *pstFuncParam,
    IN UINT uiFuncParamCnt,
    IN UINT uiCurParamIndex,
    OUT SSLVPN_JSPARSER_FUNC_INFO_S *pstFuncInfo
)
{
    UINT uiRemanentParamCnt = 0;
    SSLVPN_JSPARSER_FUNC_PARAM_S *pstCurParam = NULL;

    if (0 < uiFuncParamCnt)
    {
        if (uiCurParamIndex == pstFuncParam[0].uiParamIndex)
        {
            pstCurParam = pstFuncParam;
            uiRemanentParamCnt = uiFuncParamCnt - 1;
        }
        else
        {
            uiRemanentParamCnt = uiFuncParamCnt;
        }
    }

    pstFuncInfo->pstCurParam = pstCurParam;
    pstFuncInfo->uiRemanentParamCnt = uiRemanentParamCnt;

    if (0 < uiRemanentParamCnt)
    {
        if (NULL != pstCurParam)
        {
            pstFuncInfo->pstRemanentParam = pstFuncParam + 1;
        }
        else
        {
            pstFuncInfo->pstRemanentParam = pstFuncParam;
        }
    }
    else
    {
        pstFuncInfo->pstRemanentParam = NULL;
    }

    return;
}

/*****************************************************************************
    Func Name: _jsparser_CheckFuncInfo
 Date Created: 2015/1/8
       Author: wkf3789
  Description: 检查函数信息
        Input: VOID *pActionInfo
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC VOID _jsparser_CheckFuncInfo(IN DFA_HANDLE hDfa)
{
    CHAR *pcRwString = NULL;
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa); 
    SSLVPN_JSPARSER_FUNC_INFO_S *pstFuncInfo = &pstJsParser->stFuncInfo;

    /* 如果函数参数信息计数器不为0，说明当前扫描到的逗号','不是该参数的结束位置 */
    if (0 != pstFuncInfo->uiParamCounter)
    {
        return;
    }

    /* 函数当前参数信息不为空，说明已经被改写，需要在参数结束位置输出右括号 */
    if (NULL != pstFuncInfo->pstCurParam)
    {
        if (SSLVPN_JSPARSER_PARAM_WRITE != pstFuncInfo->pstCurParam->enParamType &&
            SSLVPN_JSPARSER_PARAM_AJAX != pstFuncInfo->pstCurParam->enParamType)
        {
            _jsparser_OutputDataBeforeCurPos(hDfa);

            pstJsParser->pfOutput("),", 2UL, pstJsParser->pUserContext);

            pstJsParser->pcOutputPtr = pstJsParser->pcCurrent + 1;
        }
        else
        {
            return;
        }
    }

    /* 进入下一个函数参数的解析 */
    pstFuncInfo->uiCurParamIndex++;
    _jsparser_SetFuncParamInfo(pstFuncInfo->pstRemanentParam, pstFuncInfo->uiRemanentParamCnt,
                               pstFuncInfo->uiCurParamIndex, pstFuncInfo);

    if (NULL != pstFuncInfo->pstCurParam)
    {
        _jsparser_OutputDataIncludeCurPos(hDfa);
        pcRwString = _jsparser_GetRwStringByParamType(pstFuncInfo->pstCurParam->enParamType);
        _jsparser_OutputRwString(pcRwString, pstJsParser);
    }

    return;
}

/*****************************************************************************
    Func Name: _jsparser_SaveStateInfo
 Date Created: 2016/2/2 
       Author: wuqingchun 11459
  Description: 保存状态信息
        Input: IN SSLVPN_JSPARSER_LEX_E enState,
               INOUT SSLVPN_JSPARSER_S *pstJsParser               
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History                                                      
  DATE        NAME             DESCRIPTION                                  
  --------------------------------------------------------------------------
                                                                            
*****************************************************************************/
STATIC VOID _jsparser_SaveStateInfo
(
    IN SSLVPN_JSPARSER_LEX_E enState,
    INOUT SSLVPN_JSPARSER_S *pstJsParser
)
{
    SSLVPN_JSPARSER_STATE_NODE_S *pstState = NULL;

    pstState = MEM_ZMalloc(sizeof(SSLVPN_JSPARSER_STATE_NODE_S));
    if (likely(NULL != pstState))
    {
        pstState->enState = enState;
        pstState->stFuncInfo = pstJsParser->stFuncInfo;
        pstState->uiInnerCounter = pstJsParser->uiInnerCounter;
        pstState->uiOuterCounter = pstJsParser->uiOuterCounter;
        
        pstJsParser->uiInnerCounter = 0;
        pstJsParser->uiOuterCounter = 0;
        
        HSTACK_Push(pstJsParser->hStack, pstState);
    }

    return;
}

/*****************************************************************************
    Func Name: _jsparser_RestoreStateInfo
 Date Created: 2016/2/2 
       Author: wuqingchun 11459
  Description: 进行状态信息的恢复
        Input: SSLVPN_JSPARSER_S *pstJsParser  
       Output: SSLVPN_JSPARSER_S *pstJsParser
       Return: BOOL_TRUE  - 表示有状态信息需要恢复
               BOOL_FALSE - 表示没有状态信息需要恢复
      Caution: 
------------------------------------------------------------------------------
  Modification History                                                      
  DATE        NAME             DESCRIPTION                                  
  --------------------------------------------------------------------------
                                                                            
*****************************************************************************/
STATIC BOOL_T _jsparser_RestoreStateInfo(INOUT SSLVPN_JSPARSER_S *pstJsParser)
{
    SSLVPN_JSPARSER_STATE_NODE_S *pstState = NULL;
    
    if (BS_OK != HSTACK_Pop(pstJsParser->hStack, (HANDLE*)&pstState))
    {
        return BOOL_FALSE;
    }

    /* 恢复之前保存的数据 */
    pstJsParser->stFuncInfo = pstState->stFuncInfo;
    pstJsParser->uiInnerCounter = pstState->uiInnerCounter;
    pstJsParser->uiOuterCounter = pstState->uiOuterCounter;
    
    DFA_SetState(pstJsParser->hDfa, pstState->enState);

    MEM_Free(pstState);

    return BOOL_TRUE;
}

/*****************************************************************************
    Func Name: _jsparser_GetFirstParamRwString
 Date Created: 2015/1/26
       Author: wkf3789
  Description: 获取第一个参数的改写字符串
        Input: IN const SSLVPN_JSPARSER_FUNC_PARAM_S *pstParamInfo
       Output:
       Return: CHAR *
      Caution:
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC CHAR *_jsparser_GetFirstParamRwString(IN const SSLVPN_JSPARSER_FUNC_PARAM_S *pstParamInfo)
{
    CHAR * pcRwString = NULL;

    if (0 == pstParamInfo[0].uiParamIndex)
    {
        pcRwString = _jsparser_GetRwStringByParamType(pstParamInfo[0].enParamType);
    }

    return pcRwString;
}

/*****************************************************************************
    Func Name: _jsparser_RewriteFunc
 Date Created: 2015/1/8
       Author: wkf3789
  Description: 改写javascript文件中的函数
        Input: VOID *pActionInfo
       Output: NONE
       Return: NONE
      Caution: 函数的改写采用以下方式:
               根据js函数参数的格式(普通数据/url/js代码等)，在指定的参数前添加
               相应的自定义的js函数处理该参数。
               (1)普通数据
               改写之前:xxx.write(arg1, arg2, ..., argn)
               改写之后:xxx.write(sslvpn_rewrite_write(arg1, arg2, ..., argn))
               (2)url
               改写之前:window.open(URL,name,features,replace)
               改写之后:window.open(sslvpn_rewrite_url(URL), name,features,replace)
               (3)js代码
               改写之前:setTimeOut(jscode, ms)
               改写之后:setTimeOut(sslvpn_rewrite_jscode(jscode) ms)
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC VOID _jsparser_RewriteFunc(IN DFA_HANDLE hDfa)
{
    CHAR *pcRwString = NULL;
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa); 
    SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray = &pstJsParser->stWordArray;
    SSLVPN_JSPARSER_FUNC_PATTERN_S *pstPattern;
    SSLVPN_JSPARSER_FUNC_PARAM_TYPE_E enParamType = SSLVPN_JSPARSER_PARAM_MAX;
    SSLVPN_JSPARSER_LEX_E enState = SSLVPN_JSPARSER_LEX_MAX;

    pstPattern = _jsparser_GetFuncPattern(pstWordArray);
    if (NULL != pstPattern)
    {
        pcRwString = _jsparser_GetFirstParamRwString(pstPattern->pstParamInfo);
        if (NULL != pcRwString)
        {
            _jsparser_OutputDataIncludeCurPos(hDfa);
            _jsparser_OutputRwString(pcRwString, pstJsParser);
            enParamType = pstPattern->pstParamInfo[0].enParamType;
        }

        memset(&pstJsParser->stFuncInfo, 0, sizeof(SSLVPN_JSPARSER_FUNC_INFO_S));
        _jsparser_SetFuncParamInfo(pstPattern->pstParamInfo, pstPattern->uiParamInfoCnt,
                                   0, &pstJsParser->stFuncInfo);

        pstJsParser->uiInnerCounter = 0;
        pstJsParser->uiInnerCounter++;

        if (SSLVPN_JSPARSER_PARAM_JSCODE == enParamType)
        {
            /* 如果参数类型是一段JavaScript代码，状态机恢复到初始状态，
             * 并记录当前解析到的信息，然后进入内部JavaScript代码的解析 
             */
            _jsparser_SaveStateInfo(SSLVPN_JSPARSER_LEX_FUNC, pstJsParser);
            enState = SSLVPN_JSPARSER_LEX_DATA;
        }
        else
        {
            enState = SSLVPN_JSPARSER_LEX_FUNC;
        }
        
        DFA_SetState(pstJsParser->hDfa, enState);
    }
    else
    {
        pstJsParser->uiOuterCounter++;
    }

    return;
}

/*****************************************************************************
    Func Name: _jsparser_EndFunc
 Date Created: 2015/1/8
       Author: wkf3789
  Description: 结束js函数处理
        Input: VOID *pActionInfo
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC VOID _jsparser_EndFunc(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_FUNC_INFO_S *pstFuncInfo = &pstJsParser->stFuncInfo;

    if (0 == pstJsParser->uiInnerCounter)
    {
        _jsparser_OutputDataIncludeCurPos(hDfa);

        if (NULL != pstFuncInfo->pstCurParam)
        {
            pstJsParser->pfOutput(")", 1UL, pstJsParser->pUserContext);
        }

        DFA_SetState(pstJsParser->hDfa, SSLVPN_JSPARSER_LEX_DATA);
    }

    return;
}

/*****************************************************************************
    Func Name: _jsparser_ModifyStateInRewriteValue
 Date Created: 2015/2/6
       Author: wkf3789
  Description: 修改解析器状态信息
        Input: SSLVPN_JSPARSER_ACTION_INFO_S *pstActionInfo
       Output: none
       Return: none
      Caution: 仅在需要修改js属性值时调用
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC VOID _jsparser_ModifyStateInRewriteValue(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_OLD_STATE_S *pstOldState = &pstJsParser->stOldState;
    SSLVPN_JSPARSER_LEX_E enNewState = SSLVPN_JSPARSER_LEX_MAX;
    SSLVPN_JSPARSER_LEX_E enOldState = SSLVPN_JSPARSER_LEX_MAX;

    switch (*pstJsParser->pcCurrent)
    {
        case '"':
        {
            enOldState = SSLVPN_JSPARSER_LEX_ATTR_VALUE;
            enNewState = SSLVPN_JSPARSER_LEX_DOUBLE_QUOTE;
            break;
        }
        case '\'':
        {
            enOldState = SSLVPN_JSPARSER_LEX_ATTR_VALUE;
            enNewState = SSLVPN_JSPARSER_LEX_SINGLE_QUOTE;
            break;
        }
        case '(':
        case '{':
        case '[':
        {
            _jsparser_IncCounterInAttrValue(hDfa);
            break;
        }
        case '/':
        {
            enOldState = SSLVPN_JSPARSER_LEX_ATTR_VALUE;
            enNewState = SSLVPN_JSPARSER_LEX_COMMON_SLASH;
            break;
        }
        default:
        {
            break;
        }
    }

    if (SSLVPN_JSPARSER_LEX_MAX != enNewState)
    {
        DFA_SetState(pstJsParser->hDfa, enNewState);
        _jsparser_SaveOldState(enOldState, pstOldState);
    }

    return;
}

/*****************************************************************************
    Func Name: _jsparser_ModifyState
 Date Created: 2015/2/6
       Author: wkf3789
  Description: 根据当前字符修改解析器相关状态信息
        Input: UINT uiInputCode
               SSLVPN_JSPARSER_S *pstJsParser
       Output: none
       Return: none
      Caution: 仅在不需要修改js属性值时调用
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC VOID _jsparser_ModifyState(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_OLD_STATE_S *pstOldState = &pstJsParser->stOldState;
    SSLVPN_JSPARSER_LEX_E enNewState = SSLVPN_JSPARSER_LEX_MAX;
    SSLVPN_JSPARSER_LEX_E enOldState = SSLVPN_JSPARSER_LEX_MAX;

    switch (*pstJsParser->pcCurrent)
    {
        case '"':
        {
            enOldState = SSLVPN_JSPARSER_LEX_DATA;
            enNewState = SSLVPN_JSPARSER_LEX_DOUBLE_QUOTE;
            break;
        }
        case '\'':
        {
            enOldState = SSLVPN_JSPARSER_LEX_DATA;
            enNewState = SSLVPN_JSPARSER_LEX_SINGLE_QUOTE;
            break;
        }
        case '\\':
        {
            enOldState = SSLVPN_JSPARSER_LEX_DATA;
            enNewState = SSLVPN_JSPARSER_LEX_ESCAPE;
            break;
        }
        case '/':
        {
            enOldState = SSLVPN_JSPARSER_LEX_DATA;
            enNewState = SSLVPN_JSPARSER_LEX_COMMON_SLASH;
            break;
        }
        default:
        {
            if (BOOL_TRUE == DFA_IsWord((UCHAR)*pstJsParser->pcCurrent))
            {
                _jsparser_InitWordArray(hDfa);
                _jsparser_SaveWord(hDfa);
                
                enNewState = SSLVPN_JSPARSER_LEX_WORD;
            }
            
            break;
        }
    }

    if (SSLVPN_JSPARSER_LEX_MAX != enOldState)
    {
        _jsparser_SaveOldState(enOldState, pstOldState);
        _jsparser_ClearWordArray(hDfa);
    }
    
    if (SSLVPN_JSPARSER_LEX_MAX != enNewState)
    {
        DFA_SetState(pstJsParser->hDfa, enNewState);
    }

    return;
}

/*****************************************************************************
    Func Name: _jsparser_RewriteAttrValue
 Date Created: 2015/1/8
       Author: wkf3789
  Description: 改写js对象属性值
        Input: VOID *pActionInfo
       Output: NONE
       Return: NONE
      Caution: js对象属性值改写方法:
               改写之前的形式:obj.attr = value
               改写之后的形式:obj.attr = xxxx(value)
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC VOID _jsparser_RewriteAttrValue(IN DFA_HANDLE hDfa)
{
    CHAR *pcRwString  = NULL;
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray = &pstJsParser->stWordArray;
    SSLVPN_JSPARSER_ATTR_PATTERN_S *pstPattern = NULL;

    pstPattern = _jsparser_GetAttrPattern(pstWordArray);
    if (NULL != pstPattern)
    {
        pcRwString = g_apcSslvpnJsParserAttrRwStrings[pstPattern->enType];
        _jsparser_OutputRwString(pcRwString, pstJsParser);

        /* 匹配到了，需要将下一状态切换为SSLVPN_JSPARSER_LEX_ATTR_VALUE */
        DFA_SetState(pstJsParser->hDfa, SSLVPN_JSPARSER_LEX_ATTR_VALUE);

        pstJsParser->uiInnerCounter = 0;

        _jsparser_ModifyStateInRewriteValue(hDfa);
        _jsparser_ClearWordArray(hDfa);
        ByteQue_Clear(pstJsParser->hCharQue);
        ByteQue_Push(pstJsParser->hCharQue, *pstJsParser->pcCurrent);
    }
    else
    {
        _jsparser_ModifyState(hDfa);
    }

    return;
}

/*****************************************************************************
    Func Name: _jsparser_SetCondExprFlag
 Date Created: 2015/7/10
       Author: wuqingchun 11459
  Description: 设置解析到了条件表达式标记
        Input: IN VOID *pActionInfo
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC VOID _jsparser_SetCondExprFlag(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    if (0 == pstJsParser->uiCondExprFlagTimes)
    {
        BIT_SET(pstJsParser->uiFlags, SSLVPN_JSPARSER_FLAG_ATTR_VALUE_COND_EXPR);
    }

    pstJsParser->uiCondExprFlagTimes++;

    return;
}

/*****************************************************************************
    Func Name: _jsparser_ClearCondExprFlag
 Date Created: 2015/7/10
       Author: wuqingchun 11459
  Description: 清除条件表达式标记
        Input: IN VOID *pActionInfo
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
STATIC VOID _jsparser_ClearCondExprFlag(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    if (0 < pstJsParser->uiCondExprFlagTimes)
    {
        pstJsParser->uiCondExprFlagTimes--;
    }

    if (0 == pstJsParser->uiCondExprFlagTimes)
    {
        BIT_RESET(pstJsParser->uiFlags, SSLVPN_JSPARSER_FLAG_ATTR_VALUE_COND_EXPR);
    }
    
    return;
}

/*****************************************************************************
  Description: 将当前扫描到的字符保存到队列中
*****************************************************************************/
STATIC VOID _jsparser_SaveCurCharToQueue(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    
    if (!BIT_TEST(pstJsParser->uiFlags, SSLVPN_JSPARSER_FLAG_NONE_SAVE))
    {
        ByteQue_Push(pstJsParser->hCharQue, *pstJsParser->pcCurrent);
    }
    
    return;
}

/*****************************************************************************
  Description: 解析字符'/'代表的含义
        Input: UCHAR ucPrevData               - 字符'/'之前的非空白字符
 *****************************************************************************/
STATIC VOID _jsparser_ParseSlash(IN UCHAR ucPrevData, INOUT SSLVPN_JSPARSER_S *pstJsParser)
{
    SSLVPN_JSPARSER_OLD_STATE_S *pstOldState = &pstJsParser->stOldState;
    SSLVPN_JSPARSER_LEX_E enNewState = SSLVPN_JSPARSER_LEX_MAX;

    if ((']' != ucPrevData) && (')' != ucPrevData))
    {
        /* 如果字符'/'前面的字符']'或')'，并且后面的第一个字符不是'*'或'/'，
         * 则表示为正则表达式的开始位置，需要将解析器的状态修改为SSLVPN_JSPARSER_LEX_REGEXPR
         */
        enNewState = SSLVPN_JSPARSER_LEX_REGEXPR;
        switch (*pstJsParser->pcCurrent)
        {
            case '\\':
            {
                _jsparser_SaveOldState(SSLVPN_JSPARSER_LEX_REGEXPR, pstOldState);
                enNewState = SSLVPN_JSPARSER_LEX_ESCAPE;
                break;
            }
            case '[':
            {
                enNewState = SSLVPN_JSPARSER_LEX_REGEXPR_BRACKET;
                break;
            }
            default:
            {
                break;
            }
        }
            
        DFA_SetState(pstJsParser->hDfa, enNewState);
    }
    else
    {
        switch (*pstJsParser->pcCurrent)
        {
            case '(':
            case '{':
            {
                pstJsParser->uiOuterCounter++;
                break;
            }
            default:
            {
                break;
            }
        }

        _jsparser_RestoreOldState(pstJsParser);
    }

    return;
}

/*****************************************************************************
    Func Name: _jsparser_IsValidKeyword
 Date Created: 2016/2/24 
       Author: wuqingchun 11459
  Description: 判断关键字是否为正则表达式前的合法关键字
        Input: IN const CHAR *pcKeyword  
       Output: NONE
       Return: BOOL_TRUE  - 是
               BOOL_FALSE - 否
      Caution: NONE
------------------------------------------------------------------------------
  Modification History                                                      
  DATE        NAME             DESCRIPTION                                  
  --------------------------------------------------------------------------
                                                                            
*****************************************************************************/
STATIC BOOL_T _jsparser_IsValidKeyword(IN const CHAR *pcKeyword)
{
    BOOL_T bIsKeyword = BOOL_FALSE;
    ULONG ulIndex = 0;
    ULONG ulLwsPos = 0;
    ULONG ulKeywordLen = strlen(pcKeyword);
    
    for (ulIndex = 0; ulIndex < sizeof(g_astSslvpnJsParserKeyword)/sizeof(g_astSslvpnJsParserKeyword[0]); ulIndex++)
    {
        ulLwsPos = ulKeywordLen - g_astSslvpnJsParserKeyword[ulIndex].uiLen;
        if (BOOL_TRUE == DFA_IsLws((UCHAR)pcKeyword[ulLwsPos - 1]) && 
            0 == strcmp(g_astSslvpnJsParserKeyword[ulIndex].pcData, pcKeyword + ulLwsPos))
        {
            bIsKeyword = BOOL_TRUE;
            break;
        }
    }

    return bIsKeyword;
}

/*****************************************************************************
    Func Name: _jsparser_CheckWordBeforeSlash
 Date Created: 2016/2/24 
       Author: wuqingchun 11459
  Description: 检查斜杠前的WORD类型的字符
        Input: IN UINT uiIndex                       
               INOUT SSLVPN_JSPARSER_S *pstJsParser  
       Output: INOUT SSLVPN_JSPARSER_S *pstJsParse
       Return: 
      Caution: 如果字符斜杠('/')前面连续的WORD类型的字符为"return"、"in"等
               JavaScript的关键字，则斜杠表示正则表达式的开始，并非除法运算符。
               例如
               (1) return /^(([1-9]\d|\d)\.){3}(25[0-5]|1\d\d|[1-9]\d|\d)$/.test(sValue);
               (2) 'a' in /[a-z]?/.exec(sValue);
------------------------------------------------------------------------------
  Modification History                                                      
  DATE        NAME             DESCRIPTION                                  
  --------------------------------------------------------------------------
                                                                            
*****************************************************************************/
VOID _jsparser_CheckWordBeforeSlash(IN UINT uiIndex, INOUT SSLVPN_JSPARSER_S *pstJsParser)
{
    CHAR szKeyWord[SSLVPN_JSPARSER_KEYWORD_BEFORE_REGEXPR_MAXLEN + 1];
    UINT uiReadLen;
    SSLVPN_JSPARSER_OLD_STATE_S *pstOldState = &pstJsParser->stOldState;
    SSLVPN_JSPARSER_LEX_E enNewState = SSLVPN_JSPARSER_LEX_MAX;
    
    uiReadLen = ByteQue_Peek(pstJsParser->hCharQue, uiIndex, (UCHAR*)szKeyWord, SSLVPN_JSPARSER_KEYWORD_BEFORE_REGEXPR_MAXLEN);
    if (uiReadLen > 0)
    {
        szKeyWord[uiReadLen] = '\0';

        if (BOOL_TRUE == _jsparser_IsValidKeyword(szKeyWord))
        {
            enNewState = SSLVPN_JSPARSER_LEX_REGEXPR;
            switch (*pstJsParser->pcCurrent)
            {
                case '\\':
                {
                    _jsparser_SaveOldState(SSLVPN_JSPARSER_LEX_REGEXPR, pstOldState);
                    enNewState = SSLVPN_JSPARSER_LEX_ESCAPE;
                    break;
                }
                case '[':
                {
                    enNewState = SSLVPN_JSPARSER_LEX_REGEXPR_BRACKET;
                    break;
                }
                default:
                {
                    break;
                }
            }
            
            DFA_SetState(pstJsParser->hDfa, enNewState);
        }
    }

    if (SSLVPN_JSPARSER_LEX_MAX == enNewState)
    {
        switch (*pstJsParser->pcCurrent)
        {
            case '(':
            case '{':
            {
                pstJsParser->uiOuterCounter++;
                break;
            }
            default:
            {
                break;
            }
        }

        _jsparser_RestoreOldState(pstJsParser);
    }

    return;
}

/*****************************************************************************
  Description: 检查当前字符的之前的字符
*****************************************************************************/
STATIC VOID _jsparser_CheckPrevChar(IN DFA_HANDLE hDfa)
{
    UCHAR ucData = 0;
    UINT uiLoop = 0;
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    UINT uiCount;

    uiCount = ByteQue_Count(pstJsParser->hCharQue);
    
    if (uiCount <= 1)
    {
        return;
    }
    
    for (uiLoop = uiCount - 1; uiLoop > 0; uiLoop--)    /* 跳过'/'检查'/'之前的字符 */
    {
        ucData = ByteQue_PeekOne(pstJsParser->hCharQue, uiLoop-1);
        if (BOOL_TRUE == DFA_IsWord(ucData))
        {
            _jsparser_CheckWordBeforeSlash(uiLoop-1, pstJsParser);
            break;
        }
        else if (BOOL_TRUE != DFA_IsLws(ucData))
        {
            _jsparser_ParseSlash(ucData, pstJsParser);
            break;
        }
    }

    return;
}

/*****************************************************************************
    Func Name: _jsparser_IncOuterCounter
 Date Created: 2016/2/3 
       Author: wuqingchun 11459
  Description: Outer计数器加一
        Input: IN VOID *pActionInfo  
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History                                                      
  DATE        NAME             DESCRIPTION                                  
  --------------------------------------------------------------------------
                                                                            
*****************************************************************************/
STATIC VOID _jsparser_IncOuterCounter(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    pstJsParser->uiOuterCounter++;

    return;
}

/*****************************************************************************
    Func Name: _jsparser_DecOuterCounter
 Date Created: 2016/2/3 
       Author: wuqingchun 11459
  Description: Outer计数器减一
        Input: IN VOID *pActionInfo  
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History                                                      
  DATE        NAME             DESCRIPTION                                  
  --------------------------------------------------------------------------
                                                                            
*****************************************************************************/
STATIC VOID _jsparser_DecOuterCounter(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    if (0 < pstJsParser->uiOuterCounter)
    {
        pstJsParser->uiOuterCounter--;
    }
    
    return;
}

/*****************************************************************************
    Func Name: _jsparser_CheckOuterCounter
 Date Created: 2016/2/3 
       Author: wuqingchun 11459
  Description: 检查Outer计数器的值
        Input: IN VOID *pActionInfo  
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History                                                      
  DATE        NAME             DESCRIPTION                                  
  --------------------------------------------------------------------------
                                                                            
*****************************************************************************/
STATIC VOID _jsparser_CheckOuterCounter(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    BOOL_T bRestore;

    if (0 == pstJsParser->uiOuterCounter)
    {
        bRestore = _jsparser_RestoreStateInfo(pstJsParser);
        if (BOOL_TRUE == bRestore)
        {
            if (')' == *pstJsParser->pcCurrent)
            {
                _jsparser_DecCounterInFunc(hDfa);
                
                /* 表示解析最后一个函数参数(参数为一段嵌套的JavaScript代码)结束 */
                _jsparser_EndFunc(hDfa);
            }
            else if (',' == *pstJsParser->pcCurrent)
            {
                /* 表示解析非最后一个函数参数(参数为一段嵌套的JavaScript代码)结束 */
                _jsparser_CheckFuncInfo(hDfa);
            }
        }
    }
    else
    {
        if (')' == *pstJsParser->pcCurrent)
        {
            pstJsParser->uiOuterCounter--;
        }
    }

    return;
}

/*****************************************************************************
  Description: 解析换行符
*****************************************************************************/
STATIC VOID _jsparser_ParseNewLine(IN UCHAR ucPrevData, IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_LEX_E enNewState = SSLVPN_JSPARSER_LEX_MAX;
    SSLVPN_JSPARSER_LEX_E enOldState = SSLVPN_JSPARSER_LEX_MAX;
    UCHAR ucCurCode = (UCHAR)*pstJsParser->pcCurrent;
    SSLVPN_JSPARSER_OLD_STATE_S *pstOldState = &pstJsParser->stOldState;

    /* 之前的字符为属性值连接符 */
    if ('+' == ucPrevData || '?' == ucPrevData || ':' == ucPrevData || '.' == ucPrevData)
    {
        /* 表示属性值还没有结束，需要继续进入解析属性值状态 */
        enNewState = SSLVPN_JSPARSER_LEX_ATTR_VALUE;
        
        switch (ucCurCode)
        {
            case '(':
            case '{':
            case '[':
            {
                pstJsParser->uiInnerCounter++;
                break;
            }
            case '"':
            {
                enOldState = SSLVPN_JSPARSER_LEX_ATTR_VALUE;
                enNewState = SSLVPN_JSPARSER_LEX_DOUBLE_QUOTE;
                break;
            }
            case '\'':
            {
                enOldState = SSLVPN_JSPARSER_LEX_ATTR_VALUE;
                enNewState = SSLVPN_JSPARSER_LEX_SINGLE_QUOTE;
                break;
            }
            case '/':
            {
                enOldState = SSLVPN_JSPARSER_LEX_ATTR_VALUE;
                enNewState = SSLVPN_JSPARSER_LEX_COMMON_SLASH;
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else if ('+' == ucCurCode || '?' == ucCurCode || ':' == ucCurCode || '.' == ucCurCode)
    {
        /* 当前的字符为属性值连接符 */

        /* 表示属性值还没有结束，需要继续进入解析属性值状态 */
        enNewState = SSLVPN_JSPARSER_LEX_ATTR_VALUE;
        
        if ('?' == ucCurCode)
        {
            _jsparser_SetCondExprFlag(hDfa);
        }
        else if (':' == ucCurCode)
        {
            _jsparser_ClearCondExprFlag(hDfa);
        }
    }
    else
    {
        /* 结束属性值的解析 */
        switch (ucCurCode)
        {
            case ')':
            case '}':
            {
                pstJsParser->uiOuterCounter--;
                enNewState = SSLVPN_JSPARSER_LEX_DATA;
                break;
            }
            case '"':
            {
                enOldState = SSLVPN_JSPARSER_LEX_DATA;
                enNewState = SSLVPN_JSPARSER_LEX_DOUBLE_QUOTE;
                break;
            }
            case '\'':
            {
                enOldState = SSLVPN_JSPARSER_LEX_DATA;
                enNewState = SSLVPN_JSPARSER_LEX_SINGLE_QUOTE;
                break;
            }
            case '/':
            {
                enOldState = SSLVPN_JSPARSER_LEX_DATA;
                enNewState = SSLVPN_JSPARSER_LEX_COMMON_SLASH;
                break;
            }
            default:
            {
                enNewState = SSLVPN_JSPARSER_LEX_DATA;
                break;
            }
        }
        
        _jsparser_OutputDataBeforeCurPos(hDfa);
        pstJsParser->pfOutput(");", 2UL, pstJsParser->pUserContext);
        if (BOOL_TRUE == DFA_IsWord(ucCurCode))
        {
            enNewState = SSLVPN_JSPARSER_LEX_WORD;
            _jsparser_InitWordArray(hDfa);
            _jsparser_SaveWord(hDfa);
        }
    }

    DFA_SetState(pstJsParser->hDfa, enNewState);

    if (SSLVPN_JSPARSER_LEX_MAX != enOldState)
    {
        _jsparser_InitOldState(hDfa);
        _jsparser_SaveOldState(enOldState, pstOldState);
    }

    return;
}

/*****************************************************************************
    Func Name: _jsparser_CheckAttrValue
 Date Created: 2016/2/4 
       Author: wuqingchun 11459
  Description: 检查属性值是否结束
        Input: IN VOID *pActionInfo  
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History                                                      
  DATE        NAME             DESCRIPTION                                  
  --------------------------------------------------------------------------
                                                                            
*****************************************************************************/
STATIC VOID _jsparser_CheckAttrValue(IN DFA_HANDLE hDfa)
{
    UCHAR ucData = 0;
    UINT uiLoop = 0;
    UINT uiCount;
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_LEX_E enNewState = SSLVPN_JSPARSER_LEX_MAX;
    SSLVPN_JSPARSER_LEX_E enOldState = SSLVPN_JSPARSER_LEX_MAX;
    SSLVPN_JSPARSER_OLD_STATE_S *pstOldState = &pstJsParser->stOldState;

    uiCount = ByteQue_Count(pstJsParser->hCharQue);
    for (uiLoop = uiCount; uiLoop > 0; uiLoop--)
    {
        ucData = ByteQue_PeekOne(pstJsParser->hCharQue, uiLoop-1);
        if (BOOL_TRUE != DFA_IsLws(ucData))
        {
            _jsparser_ParseNewLine(ucData, hDfa);
            break;
        }
    }

    /* 如果缓存的属性值均为空白字符，则表示等号之后，立即出现了换行，需要继续解析属性值 
     * document.cookie =
     *    escape(cookieName) + '=' + escape(cookieValue)
     *    + (expires ? '; expires=' + expires.toGMTString() : '')
     *    + (path ? '; path=' + path : '')
     *    + (domain ? '; domain=' + domain : '')
     *    + (secure ? '; secure' : '');
     */
    if (uiLoop == 0)
    {
        enNewState = SSLVPN_JSPARSER_LEX_ATTR_VALUE;
        switch (*pstJsParser->pcCurrent)
        {
            case '(':
            case '{':
            case '[':
            {
                pstJsParser->uiInnerCounter++;
                break;
            }
            case '"':
            {
                enOldState = SSLVPN_JSPARSER_LEX_ATTR_VALUE;
                enNewState = SSLVPN_JSPARSER_LEX_DOUBLE_QUOTE;
                break;
            }
            case '\'':
            {
                enOldState = SSLVPN_JSPARSER_LEX_ATTR_VALUE;
                enNewState = SSLVPN_JSPARSER_LEX_SINGLE_QUOTE;
                break;
            }
            case '/':
            {
                enOldState = SSLVPN_JSPARSER_LEX_ATTR_VALUE;
                enNewState = SSLVPN_JSPARSER_LEX_COMMON_SLASH;
                break;
            }
            default:
            {
                break;
            }
        }
        
        DFA_SetState(pstJsParser->hDfa, enNewState);

        if (SSLVPN_JSPARSER_LEX_MAX != enOldState)
        {
            _jsparser_InitOldState(hDfa);
            _jsparser_SaveOldState(enOldState, pstOldState);
        }
    }
    
    return;
}

STATIC VOID _jsparser_CheckNewLine(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    if (0 == pstJsParser->uiInnerCounter)
    {
        DFA_SetState(pstJsParser->hDfa, SSLVPN_JSPARSER_LEX_ATTR_VALUE_NEWLINE);
    }

    return;
}

/*****************************************************************************
    Func Name: _jsparser_SetNoneSaveFlag
 Date Created: 2016/2/22 
       Author: wuqingchun 11459
  Description: 设置不缓存标记
        Input: IN VOID *pActionInfo  
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History                                                      
  DATE        NAME             DESCRIPTION                                  
  --------------------------------------------------------------------------
                                                                            
*****************************************************************************/
STATIC VOID _jsparser_SetNoneSaveFlag(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    BIT_SET(pstJsParser->uiFlags, SSLVPN_JSPARSER_FLAG_NONE_SAVE);

    ByteQue_PopTail(pstJsParser->hCharQue);

    return;
}

/*****************************************************************************
    Func Name: _jsparser_ClearNoneSaveFlag
 Date Created: 2016/2/22 
       Author: wuqingchun 11459
  Description: 清除不缓存标记
        Input: VOID *pActionInfo  
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History                                                      
  DATE        NAME             DESCRIPTION                                  
  --------------------------------------------------------------------------
                                                                            
*****************************************************************************/
STATIC VOID _jsparser_ClearNoneSaveFlag(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    BIT_RESET(pstJsParser->uiFlags, SSLVPN_JSPARSER_FLAG_NONE_SAVE);

    return;
}

static VOID _js_rw_Init()
{
    static BOOL_T bInit = FALSE;

    if (bInit == FALSE) {
        bInit = TRUE;
        DFA_Compile(g_astSslvpnJsParserDFATbl, g_astJsRwActions);
    }
}

/*****************************************************************************
  Description: js解析器创建函数
*****************************************************************************/
JS_RW_HANDLE JS_RW_Create(IN JS_RW_OUTPUT_PF pfOutput, IN VOID *pUserContext)
{
    SSLVPN_JSPARSER_S *pstJsParser;

    _js_rw_Init();

    pstJsParser = MEM_ZMalloc(sizeof(SSLVPN_JSPARSER_S));
    if (NULL == pstJsParser)
    {
        return NULL;
    }

    pstJsParser->hDfa = DFA_Create(g_astSslvpnJsParserDFATbl, g_astJsRwActions, SSLVPN_JSPARSER_LEX_DATA);

    if (NULL == pstJsParser->hDfa)
    {
        JS_RW_Destroy(pstJsParser);
        return NULL;
    }

    DFA_SetUserData(pstJsParser->hDfa, pstJsParser);

    pstJsParser->pfOutput = pfOutput;
    pstJsParser->pUserContext = pUserContext;
    pstJsParser->hCharQue = ByteQue_Create(128, QUE_FLAG_OVERWRITE);
    if (NULL == pstJsParser->hCharQue)
    {
        JS_RW_Destroy(pstJsParser);
        return NULL;
    }

    pstJsParser->hStack = HSTACK_Create(1024);
    if (NULL == pstJsParser->hStack)
    {
        JS_RW_Destroy(pstJsParser);
        return NULL;
    }

    return pstJsParser;
}

/*****************************************************************************
    Func Name: SSLVPN_jsparser_Destroy
 Date Created: 2015/1/8
       Author: wkf3789
  Description: js解析器销毁函数
        Input: JS_RW_HANDLE hJsParser - 解析器句柄
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
VOID JS_RW_Destroy(IN JS_RW_HANDLE hJsParser)
{
    SSLVPN_JSPARSER_S *pstJsParser = hJsParser;

    if (NULL != pstJsParser->hDfa)
    {
        DFA_Destory(pstJsParser->hDfa);
    }

    if (NULL != pstJsParser->hCharQue)
    {
        ByteQue_Destory(pstJsParser->hCharQue);
    }

    if (NULL != pstJsParser->hStack)
    {
        HSTACK_Destory(pstJsParser->hStack);
    }

    MEM_Free(pstJsParser);

    return;
}

/*****************************************************************************
    Func Name: SSLVPN_jsparser_Run
 Date Created: 2015/1/8
       Author: wkf3789
  Description: js数据处理函数
        Input: JS_RW_HANDLE hJsParser  - 解析器句柄
               CHAR * pcJsData                   - js数据
               UINT uiJsDataLen                  - 数据长度
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
VOID JS_RW_Run(IN JS_RW_HANDLE hJsParser, IN CHAR *pcJsData, IN UINT uiJsDataLen)
{
    SSLVPN_JSPARSER_S *pstJsParser = hJsParser;
    CHAR *pcEnd;

    pcEnd = pcJsData + uiJsDataLen;

    pstJsParser->pcCurrent = pcJsData;
    pstJsParser->pcOutputPtr = pcJsData;
    pstJsParser->uiJsDataLen = uiJsDataLen;

    while (pstJsParser->pcCurrent < pcEnd)
    {
        DFA_InputChar(pstJsParser->hDfa, *pstJsParser->pcCurrent);
        _jsparser_SaveCurCharToQueue(pstJsParser->hDfa);
        pstJsParser->pcCurrent++;
    }

    pstJsParser->pcCurrent--;

    DFA_Edge(pstJsParser->hDfa);

    return;
}

/*****************************************************************************
    Func Name: SSLVPN_jsparser_End
 Date Created: 2015/1/8
       Author: wkf3789
  Description: 结束js数据处理函数
        Input: JS_RW_HANDLE hJsParser
       Output: NONE
       Return: NONE
      Caution: NONE
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
  --------------------------------------------------------------------------

*****************************************************************************/
VOID JS_RW_End(IN JS_RW_HANDLE hJsParser)
{
    SSLVPN_JSPARSER_S *pstJsParser = hJsParser;

    DFA_End(pstJsParser->hDfa);

    return;
}


