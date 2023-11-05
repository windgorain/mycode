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


#define SSLVPN_JSPARSER_WORD_MAXLEN 15     

#define SSLVPN_JSPARSER_MAX_WORD_COUNT 8   

#define SSLVPN_JSPARSER_FLAG_ATTR_VALUE_END         0x1  
#define SSLVPN_JSPARSER_FLAG_ATTR_VALUE_COND_EXPR   0x2  
#define SSLVPN_JSPARSER_FLAG_NONE_SAVE 0x4               

#define SSLVPN_JSPARSER_OLD_STATE_MAXNUM 8


#define SSLVPN_JSPARSER_KEYWORD_BEFORE_REGEXPR_MAXLEN 7




typedef enum tagSSLVPN_JSPARSER_LEX
{
    SSLVPN_JSPARSER_LEX_DATA,                  
    SSLVPN_JSPARSER_LEX_WORD,                  
    SSLVPN_JSPARSER_LEX_WORD_DOT,              
    SSLVPN_JSPARSER_LEX_WORD_LWS,              
    SSLVPN_JSPARSER_LEX_FUNC,                  
    SSLVPN_JSPARSER_LEX_BEFORE_ATTR_VALUE,     
    SSLVPN_JSPARSER_LEX_ATTR_VALUE,            
    SSLVPN_JSPARSER_LEX_COMMON_SLASH,          
    SSLVPN_JSPARSER_LEX_REGEXPR,               
    SSLVPN_JSPARSER_LEX_SINGLE_COMMENT,        
    SSLVPN_JSPARSER_LEX_MULTI_COMMENT,         
    SSLVPN_JSPARSER_LEX_MULTI_COMMENT_END,     
    SSLVPN_JSPARSER_LEX_DOUBLE_QUOTE,          
    SSLVPN_JSPARSER_LEX_SINGLE_QUOTE,          
    SSLVPN_JSPARSER_LEX_ESCAPE,                
    SSLVPN_JSPARSER_LEX_ATTR_VALUE_NEWLINE,    
    SSLVPN_JSPARSER_LEX_REGEXPR_BRACKET,       

    SSLVPN_JSPARSER_LEX_MAX
}SSLVPN_JSPARSER_LEX_E;


typedef enum
{
    SSLVPN_JSPARSER_PARAM_URL = 0,       
    SSLVPN_JSPARSER_PARAM_JSCODE,        
    SSLVPN_JSPARSER_PARAM_HTMLCODE,      
    SSLVPN_JSPARSER_PARAM_WRITE,         
    SSLVPN_JSPARSER_PARAM_EVAL,          
    SSLVPN_JSPARSER_PARAM_AJAX,          
    SSLVPN_JSPARSER_PARAM_ATTRNAME,      
    SSLVPN_JSPARSER_PARAM_ATTRVALUE,     
    SSLVPN_JSPARSER_PARAM_MAX
}SSLVPN_JSPARSER_FUNC_PARAM_TYPE_E;


typedef enum
{
    SSLVPN_JSPARSER_VALUE_HTMLCODE = 0, 
    SSLVPN_JSPARSER_VALUE_URL,          
    SSLVPN_JSPARSER_VALUE_COOKIE,       

    SSLVPN_JSPARSER_VALUE_MAX
}SSLVPN_JSPARSER_ATTR_VALUE_TYPE_E;

typedef struct
{
    SSLVPN_JSPARSER_FUNC_PARAM_TYPE_E enParamType; 
    UINT uiParamIndex;
}SSLVPN_JSPARSER_FUNC_PARAM_S;

typedef struct
{
    SSLVPN_JSPARSER_FUNC_PARAM_S *pstParamInfo;    
    UINT uiParamInfoCnt;                           
    CHAR *pcPattern;                               
    UINT uiWordCnt;                                
}SSLVPN_JSPARSER_FUNC_PATTERN_S;


typedef struct
{
    SSLVPN_JSPARSER_ATTR_VALUE_TYPE_E enType;
    CHAR *pcPattern;
}SSLVPN_JSPARSER_ATTR_PATTERN_S;


typedef struct tagSSLVPN_JSPARSER_WORD
{
    CHAR acWord[SSLVPN_JSPARSER_WORD_MAXLEN + 1];
    UINT uiWordLen;
    BOOL_T bIsMember;
}SSLVPN_JSPARSER_WORD_S;


typedef struct tagSSLVPN_JSPARSER_WORD_ARRAY
{
    
    SSLVPN_JSPARSER_WORD_S astWords[SSLVPN_JSPARSER_MAX_WORD_COUNT]; 
    UINT uiWordCount;
}SSLVPN_JSPARSER_WORD_ARRAY_S;


typedef struct tagSSLVPN_JSPARSER_FUNC_INFO
{
    SSLVPN_JSPARSER_FUNC_PARAM_S *pstCurParam;       
    SSLVPN_JSPARSER_FUNC_PARAM_S *pstRemanentParam;  
    UINT uiRemanentParamCnt;                         
    UINT uiCurParamIndex;                            
    UINT uiParamCounter;                             
}SSLVPN_JSPARSER_FUNC_INFO_S;

typedef struct tagSSLVPN_JSPARSER_STATE
{
    SSLVPN_JSPARSER_LEX_E enState; 
    UINT uiInnerCounter;
    UINT uiOuterCounter;
    SSLVPN_JSPARSER_FUNC_INFO_S stFuncInfo;
}SSLVPN_JSPARSER_STATE_NODE_S;


typedef struct
{
    UINT auiState[SSLVPN_JSPARSER_OLD_STATE_MAXNUM];
    UINT uiStateNum;
}SSLVPN_JSPARSER_OLD_STATE_S;


typedef struct tagSSLVPN_JSPARSER
{
    DFA_HANDLE hDfa;
    CHAR *pcOutputPtr;        
    CHAR *pcCurrent;
    UINT uiJsDataLen;
    UINT uiFlags;
    SSLVPN_JSPARSER_WORD_ARRAY_S stWordArray;
    JS_RW_OUTPUT_PF pfOutput;
    SSLVPN_JSPARSER_FUNC_INFO_S stFuncInfo;
    VOID *pUserContext;
    SSLVPN_JSPARSER_OLD_STATE_S stOldState;
    BYTE_QUE_HANDLE hCharQue;
    HANDLE hStack; 
    UINT uiInnerCounter;
    UINT uiOuterCounter;
    UINT uiCondExprFlagTimes;    
}SSLVPN_JSPARSER_S;


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


STATIC DFA_NODE_S g_astSslvpnJsParserStateBeforeAttrValue[] =
{
    {DFA_CODE_CHAR('='), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_ClearWordArray"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, SSLVPN_JSPARSER_LEX_DATA, "_jsparser_OutputDataBeforeCurPos,_jsparser_InitOldState,_jsparser_RewriteAttrValue"},
};


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


STATIC DFA_NODE_S g_astSslvpnJsParserStateCommonSlash[] =
{
    {DFA_CODE_CHAR('/'), SSLVPN_JSPARSER_LEX_SINGLE_COMMENT, "_jsparser_SetNoneSaveFlag"},
    {DFA_CODE_CHAR('*'), SSLVPN_JSPARSER_LEX_MULTI_COMMENT, "_jsparser_SetNoneSaveFlag"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_END, SSLVPN_JSPARSER_LEX_DATA, NULL},
    {DFA_CODE_OTHER, SSLVPN_JSPARSER_LEX_DATA, "_jsparser_CheckPrevChar"},
};


STATIC DFA_NODE_S g_astSslvpnJsParserStateRegExpr[] =
{
    {DFA_CODE_CHAR('/'), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_RecoverState"},
    {DFA_CODE_CHAR('['), SSLVPN_JSPARSER_LEX_REGEXPR_BRACKET, NULL},
    {DFA_CODE_CHAR('\\'), SSLVPN_JSPARSER_LEX_ESCAPE, "_jsparser_RecordState"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL},
};


STATIC DFA_NODE_S g_astSslvpnJsParserStateSingleComment[] =
{
    {DFA_CODE_LWS, DFA_STATE_SELF, "_jsparser_CheckLws"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL},
};



STATIC DFA_NODE_S g_astSslvpnJsParserStateMultiComment[] =
{
    {DFA_CODE_CHAR('*'), SSLVPN_JSPARSER_LEX_MULTI_COMMENT_END, NULL},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_END, DFA_STATE_SELF, NULL},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL},
};


STATIC DFA_NODE_S g_astSslvpnJsParserStateMultiCommentEnd[] =
{
    {DFA_CODE_CHAR('/'), SSLVPN_JSPARSER_LEX_DATA, "_jsparser_RecoverState,_jsparser_ClearNoneSaveFlag"},
    {DFA_CODE_CHAR('*'), DFA_STATE_SELF, NULL},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, SSLVPN_JSPARSER_LEX_MULTI_COMMENT, NULL},
};


STATIC DFA_NODE_S g_astSslvpnJsParserStateDoubleQuote[] =
{
    {DFA_CODE_CHAR('"'), DFA_STATE_SELF, "_jsparser_RecoverState"},
    {DFA_CODE_CHAR('\\'), SSLVPN_JSPARSER_LEX_ESCAPE, "_jsparser_RecordState"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL},
};


STATIC DFA_NODE_S g_astSslvpnJsParserStateSingleQuote[] =
{
    {DFA_CODE_CHAR('\''), DFA_STATE_SELF, "_jsparser_RecoverState"},
    {DFA_CODE_CHAR('\\'), SSLVPN_JSPARSER_LEX_ESCAPE, "_jsparser_RecordState"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL},
};


STATIC DFA_NODE_S g_astSslvpnJsParserStateEscape[] =
{
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, SSLVPN_JSPARSER_LEX_DATA, "_jsparser_RecoverState"},
};


STATIC DFA_NODE_S g_astSslvpnJsParserStateAttrValueNewLine[] =
{
    {DFA_CODE_LWS, DFA_STATE_SELF, NULL},        
    {DFA_CODE_EDGE, DFA_STATE_SELF, "_jsparser_OutputDataIncludeCurPos"},
    {DFA_CODE_OTHER, SSLVPN_JSPARSER_LEX_DATA, "_jsparser_CheckAttrValue"},
    {DFA_CODE_END, DFA_STATE_SELF, "_jsparser_EndAttrValue"},
};


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


STATIC SSLVPN_JSPARSER_FUNC_PARAM_S g_astSslvpnJsParserWrite[] =
{
    {SSLVPN_JSPARSER_PARAM_WRITE, 0}
};


STATIC SSLVPN_JSPARSER_FUNC_PARAM_S g_astSslvpnJsParserOpen[] =
{
    {SSLVPN_JSPARSER_PARAM_URL, 0},
    {SSLVPN_JSPARSER_PARAM_URL, 1}
};


STATIC SSLVPN_JSPARSER_FUNC_PARAM_S g_astSslvpnJsParserFuncParamJsCode[] =
{
    {SSLVPN_JSPARSER_PARAM_JSCODE, 0}
};


STATIC SSLVPN_JSPARSER_FUNC_PARAM_S g_astSslvpnJsParserFuncEval[] =
{
    {SSLVPN_JSPARSER_PARAM_EVAL, 0}
};


STATIC SSLVPN_JSPARSER_FUNC_PARAM_S g_astSslvpnJsParserFuncParamUrl0[] =
{
    {SSLVPN_JSPARSER_PARAM_URL, 0}
};


STATIC SSLVPN_JSPARSER_FUNC_PARAM_S g_astSslvpnJsParserAjax[] =
{
    {SSLVPN_JSPARSER_PARAM_AJAX, 0}
};


STATIC SSLVPN_JSPARSER_FUNC_PARAM_S g_astSslvpnJsParserAttr[] =
{
    {SSLVPN_JSPARSER_PARAM_ATTRNAME, 0},
    {SSLVPN_JSPARSER_PARAM_ATTRVALUE, 1}
};


#define SSLVPN_JSPARSER_FUNC_PATTERN_ITEM(astParamInfo, pcPattern, uiWordCnt) \
    {(astParamInfo), sizeof(astParamInfo)/sizeof(astParamInfo[0]), pcPattern, uiWordCnt}


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


STATIC CHAR *g_apcSslvpnJsParserAttrRwStrings[SSLVPN_JSPARSER_VALUE_MAX] =
{
    " s_r_htmlcode(",
    " s_r_url(",
    " s_r_cookie(",
};


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


STATIC LSTR_S g_astSslvpnJsParserKeyword[] = 
{
    {"in", 2},
    {"return", 6},
};


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



STATIC VOID _jsparser_InitWordArray(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray = &pstJsParser->stWordArray;

    memset(pstWordArray, 0, sizeof(SSLVPN_JSPARSER_WORD_ARRAY_S));

    pstWordArray->uiWordCount = 1;

    return;
}


STATIC VOID _jsparser_IncreaseWordCount(IN DFA_HANDLE hDfa)
{
    UINT uiIndex = 0;
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray = &pstJsParser->stWordArray;
    SSLVPN_JSPARSER_WORD_S *pstWord;

    
    DBGASSERT(SSLVPN_JSPARSER_MAX_WORD_COUNT > pstWordArray->uiWordCount);

    pstWordArray->uiWordCount++;

    uiIndex = pstWordArray->uiWordCount - 1;
    pstWord = pstWordArray->astWords + uiIndex;
    pstWord->bIsMember = BOOL_TRUE;

    return;
}


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
        
        _jsparser_ClearWordArray(hDfa);
        DFA_SetState(pstJsParser->hDfa, SSLVPN_JSPARSER_LEX_DATA);
    }

    return;
}


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


STATIC VOID _jsparser_OutputRwString(IN CHAR *pcRwString, IN SSLVPN_JSPARSER_S *pstJsParser)
{
    pstJsParser->pfOutput(pcRwString, strlen(pcRwString), pstJsParser->pUserContext);

    return;
}


STATIC VOID _jsparser_ClearWordArray(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_WORD_ARRAY_S *pstWordArray = &pstJsParser->stWordArray;

    memset(pstWordArray, 0, sizeof(SSLVPN_JSPARSER_WORD_ARRAY_S));

    return;
}


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


STATIC VOID _jsparser_IncCounterInAttrValue(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    pstJsParser->uiInnerCounter++;

    return;
}


STATIC VOID _jsparser_IncCounterInFunc(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_FUNC_INFO_S *pstFuncInfo = &pstJsParser->stFuncInfo;

    pstJsParser->uiInnerCounter++;
    pstFuncInfo->uiParamCounter++;

    return;
}


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


STATIC VOID _jsparser_InitOldState(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa); 
    SSLVPN_JSPARSER_OLD_STATE_S *pstOldState = &pstJsParser->stOldState;

    DBGASSERT(0 == pstOldState->uiStateNum);
    pstOldState->uiStateNum = 0;

    return;
}


STATIC VOID _jsparser_SaveOldState
(
    IN SSLVPN_JSPARSER_LEX_E enOldState, 
    INOUT SSLVPN_JSPARSER_OLD_STATE_S *pstOldState
)
{
    
    pstOldState->auiState[pstOldState->uiStateNum] = enOldState;
    pstOldState->uiStateNum++;

    return ;
}


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


STATIC VOID _jsparser_RecordState(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa); 

    
    _jsparser_SaveOldState(DFA_GetOldState(hDfa), &pstJsParser->stOldState);

    return;
}


STATIC VOID _jsparser_RecoverState(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa); 

    _jsparser_RestoreOldState(pstJsParser);

    return;
}


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


STATIC VOID _jsparser_CheckLws(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa); 

    
    if ('\n' == *pstJsParser->pcCurrent || '\r' == *pstJsParser->pcCurrent)
    {
        _jsparser_RecoverState(hDfa);
        _jsparser_ClearNoneSaveFlag(hDfa);
    }

    return;
}


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


STATIC VOID _jsparser_CheckFuncInfo(IN DFA_HANDLE hDfa)
{
    CHAR *pcRwString = NULL;
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa); 
    SSLVPN_JSPARSER_FUNC_INFO_S *pstFuncInfo = &pstJsParser->stFuncInfo;

    
    if (0 != pstFuncInfo->uiParamCounter)
    {
        return;
    }

    
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


STATIC BOOL_T _jsparser_RestoreStateInfo(INOUT SSLVPN_JSPARSER_S *pstJsParser)
{
    SSLVPN_JSPARSER_STATE_NODE_S *pstState = NULL;
    
    if (BS_OK != HSTACK_Pop(pstJsParser->hStack, (HANDLE*)&pstState))
    {
        return BOOL_FALSE;
    }

    
    pstJsParser->stFuncInfo = pstState->stFuncInfo;
    pstJsParser->uiInnerCounter = pstState->uiInnerCounter;
    pstJsParser->uiOuterCounter = pstState->uiOuterCounter;
    
    DFA_SetState(pstJsParser->hDfa, pstState->enState);

    MEM_Free(pstState);

    return BOOL_TRUE;
}


STATIC CHAR *_jsparser_GetFirstParamRwString(IN const SSLVPN_JSPARSER_FUNC_PARAM_S *pstParamInfo)
{
    CHAR * pcRwString = NULL;

    if (0 == pstParamInfo[0].uiParamIndex)
    {
        pcRwString = _jsparser_GetRwStringByParamType(pstParamInfo[0].enParamType);
    }

    return pcRwString;
}


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


STATIC VOID _jsparser_SaveCurCharToQueue(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    
    if (!BIT_TEST(pstJsParser->uiFlags, SSLVPN_JSPARSER_FLAG_NONE_SAVE))
    {
        ByteQue_Push(pstJsParser->hCharQue, *pstJsParser->pcCurrent);
    }
    
    return;
}


STATIC VOID _jsparser_ParseSlash(IN UCHAR ucPrevData, INOUT SSLVPN_JSPARSER_S *pstJsParser)
{
    SSLVPN_JSPARSER_OLD_STATE_S *pstOldState = &pstJsParser->stOldState;
    SSLVPN_JSPARSER_LEX_E enNewState = SSLVPN_JSPARSER_LEX_MAX;

    if ((']' != ucPrevData) && (')' != ucPrevData))
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
    
    for (uiLoop = uiCount - 1; uiLoop > 0; uiLoop--)    
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


STATIC VOID _jsparser_IncOuterCounter(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    pstJsParser->uiOuterCounter++;

    return;
}


STATIC VOID _jsparser_DecOuterCounter(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    if (0 < pstJsParser->uiOuterCounter)
    {
        pstJsParser->uiOuterCounter--;
    }
    
    return;
}


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
                
                
                _jsparser_EndFunc(hDfa);
            }
            else if (',' == *pstJsParser->pcCurrent)
            {
                
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


STATIC VOID _jsparser_ParseNewLine(IN UCHAR ucPrevData, IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);
    SSLVPN_JSPARSER_LEX_E enNewState = SSLVPN_JSPARSER_LEX_MAX;
    SSLVPN_JSPARSER_LEX_E enOldState = SSLVPN_JSPARSER_LEX_MAX;
    UCHAR ucCurCode = (UCHAR)*pstJsParser->pcCurrent;
    SSLVPN_JSPARSER_OLD_STATE_S *pstOldState = &pstJsParser->stOldState;

    
    if ('+' == ucPrevData || '?' == ucPrevData || ':' == ucPrevData || '.' == ucPrevData)
    {
        
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


STATIC VOID _jsparser_SetNoneSaveFlag(IN DFA_HANDLE hDfa)
{
    SSLVPN_JSPARSER_S *pstJsParser = DFA_GetUserData(hDfa);

    BIT_SET(pstJsParser->uiFlags, SSLVPN_JSPARSER_FLAG_NONE_SAVE);

    ByteQue_PopTail(pstJsParser->hCharQue);

    return;
}


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


VOID JS_RW_End(IN JS_RW_HANDLE hJsParser)
{
    SSLVPN_JSPARSER_S *pstJsParser = hJsParser;

    DFA_End(pstJsParser->hDfa);

    return;
}


