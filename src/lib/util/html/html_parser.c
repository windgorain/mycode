/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-10-14
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/html_parser.h"
#include "utl/dfa_utl.h"
#include "utl/action_utl.h"
#include "utl/txt_utl.h"

#define HTML_PARSER_MAX_SAVE_BUF_LEN 63

typedef struct
{
    CHAR szBuf[HTML_PARSER_MAX_SAVE_BUF_LEN + 1];
    UINT uiLen;
}HTML_PARSER_SAVE_S;

#define HTML_PARSER_FLAG_NOT_HTML 0x1

typedef struct
{
    DFA_HANDLE hDfa;

    PF_HTML_OUTPUT_FUNC pfOutput;
    VOID *pUserContext;

    CHAR *pcHtmlData;   
    UINT uiHtmlDataLen; 
    CHAR *pcHtmlCurrent; 

    HTML_PARSER_SAVE_S stSaveBuf;

    LSTR_S stData;
    HTML_PARSER_TAG_S stTag;

    UINT uiFlag;
}HTML_PARSER_CTRL_S;

typedef enum
{
    HTML_LEX_INIT = 0,
    HTML_LEX_DATA,            
    HTML_LEX_TAG_OPEN,        
    HTML_LEX_END_TAG_OPEN,    
    HTML_LEX_SELF_CLOSING_START_TAG,
    HTML_LEX_TAG_NAME,
    HTML_LEX_BEFORE_ATTR_NAME,
    HTML_LEX_ATTR_NAME,
    HTML_LEX_AFTER_ATTR_NAME,
    HTML_LEX_BEFORE_ATTR_VALUE,
    HTML_LEX_ATTR_VALUE_DOUBLE_QUOTED,
    HTML_LEX_ATTR_VALUE_SINGLE_QUOTED,
    HTML_LEX_AFTER_ATTR_VALUE_QUOTED,
    HTML_LEX_ATTR_VALUE_UNQUOTED,
    HTML_LEX_BOGUS_COMMENT,
    HTML_LEX_MARKUP_DECLARATION,
    HTML_LEX_JS,
    HTML_LEX_JS_ENDTAG_START, 
    HTML_LEX_JS_ENDTAG_SLASH, 
    HTML_LEX_JS_ENDTAG_S, 
    HTML_LEX_JS_ENDTAG_C, 
    HTML_LEX_JS_ENDTAG_R, 
    HTML_LEX_JS_ENDTAG_I, 
    HTML_LEX_JS_ENDTAG_P, 
    HTML_LEX_JS_ENDTAG_T, 
    HTML_LEX_CSS,
    HTML_LEX_CSS_END_START, 
    HTML_LEX_CSS_END_SLASH, 
    HTML_LEX_CSS_END_S,     
    HTML_LEX_CSS_END_T,     
    HTML_LEX_CSS_END_Y,     
    HTML_LEX_CSS_END_L,     
    HTML_LEX_CSS_END_E,     

    HTML_LEX_MAX
}HTML_TP_STATE_E;

static VOID html_parser_NotHtml(IN DFA_HANDLE hDfa);
static VOID html_parser_OutputRecord(IN DFA_HANDLE hDfa);
static VOID html_parser_OutputJs(IN DFA_HANDLE hDfa);
static VOID html_parser_OutputCss(IN DFA_HANDLE hDfa);
static VOID html_parser_OutputTag(IN DFA_HANDLE hDfa);
static VOID html_parser_RecordData(IN DFA_HANDLE hDfa);
static VOID html_parser_Save(IN DFA_HANDLE hDfa);
static VOID html_parser_ClearSave(IN DFA_HANDLE hDfa);
static VOID html_parser_ClearRecord(IN DFA_HANDLE hDfa);
static VOID html_parser_OutputSave(IN DFA_HANDLE hDfa);
static VOID html_parser_OutputSaveAsJs(IN DFA_HANDLE hDfa);
static VOID html_parser_OutputSaveAsCss(IN DFA_HANDLE hDfa);
static VOID html_parser_ClearTagName(IN DFA_HANDLE hDfa);
static VOID html_parser_ClearTagAttrName(IN DFA_HANDLE hDfa);
static VOID html_parser_ClearTagAttrValue(IN DFA_HANDLE hDfa);
static VOID html_parser_TagName(IN DFA_HANDLE hDfa);
static VOID html_parser_TagAttrName(IN DFA_HANDLE hDfa);
static VOID html_parser_TagAttrValue(IN DFA_HANDLE hDfa);
static VOID html_parser_SelfClose(IN DFA_HANDLE hDfa);
static VOID html_parser_TagStart(IN DFA_HANDLE hDfa);
static VOID html_parser_TagEnd(IN DFA_HANDLE hDfa);
static VOID html_parser_TestSpecTag(IN DFA_HANDLE hDfa);
static VOID html_parser_EndTagSlash(IN DFA_HANDLE hDfa);
static VOID html_parser_JsEnd(IN DFA_HANDLE hDfa);
static VOID html_parser_CssEnd(IN DFA_HANDLE hDfa);



static ACTION_S g_astHtmlParserActions[] =
{
    ACTION_LINE("notHtml", html_parser_NotHtml),
    ACTION_LINE("outputRecord", html_parser_OutputRecord),
    ACTION_LINE("outputJs", html_parser_OutputJs),
    ACTION_LINE("outputCss", html_parser_OutputCss),
    ACTION_LINE("outputTag", html_parser_OutputTag),
    ACTION_LINE("record", html_parser_RecordData),
    ACTION_LINE("clearRecord", html_parser_ClearRecord),
    ACTION_LINE("save", html_parser_Save),
    ACTION_LINE("clearSave", html_parser_ClearSave),
    ACTION_LINE("outputSave", html_parser_OutputSave),
    ACTION_LINE("outputSaveAsJs", html_parser_OutputSaveAsJs),
    ACTION_LINE("outputSaveAsCss", html_parser_OutputSaveAsCss),
    ACTION_LINE("tagStart", html_parser_TagStart),
    ACTION_LINE("tagEnd", html_parser_TagEnd),
    ACTION_LINE("testSpecTag", html_parser_TestSpecTag),
    ACTION_LINE("endTagSlash", html_parser_EndTagSlash),
    ACTION_LINE("clearTagName", html_parser_ClearTagName),
    ACTION_LINE("clearTagAttrName", html_parser_ClearTagAttrName),
    ACTION_LINE("clearTagAttrValue", html_parser_ClearTagAttrValue),
    ACTION_LINE("tagName", html_parser_TagName),
    ACTION_LINE("tagAttrName", html_parser_TagAttrName),
    ACTION_LINE("tagAttrValue", html_parser_TagAttrValue),
    ACTION_LINE("selfClose", html_parser_SelfClose),
    ACTION_LINE("JsEnd", html_parser_JsEnd),
    ACTION_LINE("CssEnd", html_parser_CssEnd),
    ACTION_END
};

static DFA_NODE_S g_astHtmlParserStateInit[] = 
{
    {DFA_CODE_LWS, DFA_STATE_SELF, "record"},
    {'{', DFA_STATE_SELF, "notHtml,record,outputRecord"},    
    {'[', DFA_STATE_SELF, "notHtml,record,outputRecord"},    
    {'<', HTML_LEX_TAG_OPEN, "outputRecord,tagStart,clearTagName,clearSave,save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "outputRecord"},
    {DFA_CODE_END, HTML_LEX_DATA, "outputRecord"},
    {DFA_CODE_OTHER, HTML_LEX_DATA, "record"}
};

static DFA_NODE_S g_astHtmlParserStateData[] = 
{
    {'<', HTML_LEX_TAG_OPEN, "outputRecord,tagStart,clearTagName,clearSave,save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "outputRecord"},
    {DFA_CODE_END, HTML_LEX_DATA, "outputRecord"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, "record"}
};

static DFA_NODE_S g_astHtmlParserStateTagOpen[] = 
{
    {DFA_CODE_CHAR('!'), HTML_LEX_MARKUP_DECLARATION, "outputSave,record"},
    {DFA_CODE_CHAR('/'), HTML_LEX_END_TAG_OPEN, "save,endTagSlash"},
    {DFA_CODE_CHAR('?'), HTML_LEX_BOGUS_COMMENT, "outputSave,record"},
    {DFA_CODE_ALPHA, HTML_LEX_TAG_NAME, "save,tagName"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSave"},
    {DFA_CODE_OTHER, HTML_LEX_DATA, "outputSave,record"}
};

static DFA_NODE_S g_astHtmlParserStateEndTagOpen[] = 
{
    {DFA_CODE_CHAR('>'), HTML_LEX_DATA, "save,outputSave"},
    {DFA_CODE_ALPHA, HTML_LEX_TAG_NAME, "save,tagName"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSave"},
    {DFA_CODE_OTHER, HTML_LEX_BOGUS_COMMENT, "outputSave,record"},
};

static DFA_NODE_S g_astHtmlParserStateTagName[] =
{
    {DFA_CODE_ALPHA, DFA_STATE_SELF, "save,tagName"},
    {DFA_CODE_LWS, HTML_LEX_BEFORE_ATTR_NAME, NULL},
    {DFA_CODE_CHAR('/'), HTML_LEX_SELF_CLOSING_START_TAG, NULL},
    {DFA_CODE_CHAR('>'), HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_OTHER, HTML_LEX_DATA, "outputSave,record"}
};

static DFA_NODE_S g_astHtmlParserStateSelfClosingStartTag[] = 
{
    {DFA_CODE_CHAR('>'), HTML_LEX_DATA, "selfClose,tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "selfClose,tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_OTHER, HTML_LEX_BEFORE_ATTR_NAME, NULL},
};

static DFA_NODE_S g_astHtmlParserStateBeforeAttrName[] =
{
    {DFA_CODE_LWS, DFA_STATE_SELF, NULL},
    {DFA_CODE_CHAR('/'), HTML_LEX_SELF_CLOSING_START_TAG, NULL},
    {DFA_CODE_CHAR('>'), HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_OTHER, HTML_LEX_ATTR_NAME, "clearTagAttrName,tagAttrName"},
};

static DFA_NODE_S g_astHtmlParserStateAttrName[] =
{
    {DFA_CODE_LWS, HTML_LEX_AFTER_ATTR_NAME, NULL},
    {DFA_CODE_CHAR('/'), HTML_LEX_SELF_CLOSING_START_TAG, NULL},
    {DFA_CODE_CHAR('='), HTML_LEX_BEFORE_ATTR_VALUE, NULL},
    {DFA_CODE_CHAR('>'), HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, "tagAttrName"},
};

static DFA_NODE_S g_astHtmlParserStateAfterAttrName[] =
{
    {DFA_CODE_LWS, DFA_STATE_SELF, NULL},
    {DFA_CODE_CHAR('/'), HTML_LEX_SELF_CLOSING_START_TAG, NULL},
    {DFA_CODE_CHAR('='), HTML_LEX_BEFORE_ATTR_VALUE, NULL},
    {DFA_CODE_CHAR('>'), HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_OTHER, HTML_LEX_ATTR_NAME, "outputTag,clearTagAttrName,tagAttrName"},
};

static DFA_NODE_S g_astHtmlParserStateBeforeAttrValue[] =
{
    {DFA_CODE_LWS, DFA_STATE_SELF, NULL},
    {DFA_CODE_CHAR('"'), HTML_LEX_ATTR_VALUE_DOUBLE_QUOTED, "clearTagAttrValue,tagAttrValue"},
    {DFA_CODE_CHAR('\''), HTML_LEX_ATTR_VALUE_SINGLE_QUOTED, "clearTagAttrValue,tagAttrValue"},
    {DFA_CODE_CHAR('>'), HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_OTHER, HTML_LEX_ATTR_VALUE_UNQUOTED, "clearTagAttrValue,tagAttrValue"},
};

static DFA_NODE_S g_astHtmlParserStateAttrValueDobuldQuoted[] =
{
    {DFA_CODE_CHAR('"'), HTML_LEX_AFTER_ATTR_VALUE_QUOTED, "tagAttrValue,outputTag"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, "tagAttrValue"},
};

static DFA_NODE_S g_astHtmlParserStateAttrValueSingleQuoted[] =
{
    {DFA_CODE_CHAR('\''), HTML_LEX_AFTER_ATTR_VALUE_QUOTED, "tagAttrValue,outputTag"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, "tagAttrValue"}
};

static DFA_NODE_S g_astHtmlParserStateAfterAttrValueQuoted[] =
{
    {DFA_CODE_LWS, HTML_LEX_BEFORE_ATTR_NAME,  NULL},
    {DFA_CODE_CHAR('/'), HTML_LEX_SELF_CLOSING_START_TAG,  NULL},
    {DFA_CODE_CHAR('>'), HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_OTHER, HTML_LEX_BEFORE_ATTR_NAME,  NULL},
};

static DFA_NODE_S g_astHtmlParserStateAttrValueUnquoted[] =
{
    {DFA_CODE_LWS, HTML_LEX_BEFORE_ATTR_NAME, "outputTag"},
    {DFA_CODE_CHAR('>'), HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "tagEnd,outputTag,testSpecTag"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, "tagAttrValue"},
};

static DFA_NODE_S g_astHtmlParserStateMarkDeclaration[] =
{
    {DFA_CODE_CHAR('>'), HTML_LEX_DATA, "record,outputRecord"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "outputRecord"},
    {DFA_CODE_END, HTML_LEX_DATA, "outputRecord"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, "record"}
};

static DFA_NODE_S g_astHtmlParserStateBogusComment[] =
{
    {DFA_CODE_CHAR('>'), HTML_LEX_DATA, "record,outputRecord"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "outputRecord"},
    {DFA_CODE_END, HTML_LEX_DATA, "outputRecord"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, "record"}
};

static DFA_NODE_S g_astHtmlParserStateJs[] =
{
    {DFA_CODE_CHAR('<'), HTML_LEX_JS_ENDTAG_START, "outputJs,clearSave,save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "outputJs"},
    {DFA_CODE_END, HTML_LEX_DATA, "outputJs"},
    {DFA_CODE_OTHER, HTML_LEX_JS, "record"}
};

static DFA_NODE_S g_astHtmlParserStateJsEndStart[] =
{
    {DFA_CODE_CHAR('/'), HTML_LEX_JS_ENDTAG_SLASH, "save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsJs"},
    {DFA_CODE_OTHER, HTML_LEX_JS, "outputSaveAsJs,record"}
};

static DFA_NODE_S g_astHtmlParserStateJsEndSlash[] =
{
    {DFA_CODE_CHAR('s'), HTML_LEX_JS_ENDTAG_S, "save"},
    {DFA_CODE_CHAR('S'), HTML_LEX_JS_ENDTAG_S, "save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsJs"},
    {DFA_CODE_OTHER, HTML_LEX_JS, "outputSaveAsJs,record"}
};

static DFA_NODE_S g_astHtmlParserStateJsEnd_S[] =
{
    {DFA_CODE_CHAR('c'), HTML_LEX_JS_ENDTAG_C, "save"},
    {DFA_CODE_CHAR('C'), HTML_LEX_JS_ENDTAG_C, "save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsJs"},
    {DFA_CODE_OTHER, HTML_LEX_JS, "outputSaveAsJs,record"}
};

static DFA_NODE_S g_astHtmlParserStateJsEnd_C[] =
{
    {DFA_CODE_CHAR('r'), HTML_LEX_JS_ENDTAG_R, "save"},
    {DFA_CODE_CHAR('R'), HTML_LEX_JS_ENDTAG_R, "save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsJs"},
    {DFA_CODE_OTHER, HTML_LEX_JS, "outputSaveAsJs,record"}
};

static DFA_NODE_S g_astHtmlParserStateJsEnd_R[] =
{
    {DFA_CODE_CHAR('i'), HTML_LEX_JS_ENDTAG_I, "save"},
    {DFA_CODE_CHAR('I'), HTML_LEX_JS_ENDTAG_I, "save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsJs"},
    {DFA_CODE_OTHER, HTML_LEX_JS, "outputSaveAsJs,record"}
};

static DFA_NODE_S g_astHtmlParserStateJsEnd_I[] =
{
    {DFA_CODE_CHAR('p'), HTML_LEX_JS_ENDTAG_P, "save"},
    {DFA_CODE_CHAR('P'), HTML_LEX_JS_ENDTAG_P, "save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsJs"},
    {DFA_CODE_OTHER, HTML_LEX_JS, "outputSaveAsJs,record"}
};

static DFA_NODE_S g_astHtmlParserStateJsEnd_P[] =
{
    {DFA_CODE_CHAR('t'), HTML_LEX_JS_ENDTAG_T, "save"},
    {DFA_CODE_CHAR('T'), HTML_LEX_JS_ENDTAG_T, "save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsJs"},
    {DFA_CODE_OTHER, HTML_LEX_JS, "outputSaveAsJs,record"}
};

static DFA_NODE_S g_astHtmlParserStateJsEnd_T[] =
{
    {DFA_CODE_CHAR('>'), HTML_LEX_DATA, "save,JsEnd,outputTag"},
    {DFA_CODE_LWS, HTML_LEX_BEFORE_ATTR_NAME, "JsEnd"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsJs"},
    {DFA_CODE_OTHER, HTML_LEX_JS, "outputSaveAsJs,record"}
};

static DFA_NODE_S g_astHtmlParserStateCss[] =
{
    {DFA_CODE_CHAR('<'), HTML_LEX_CSS_END_START, "outputCss,clearSave,save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF,  "outputCss"},
    {DFA_CODE_END, HTML_LEX_DATA, "outputCss"},
    {DFA_CODE_OTHER, DFA_STATE_SELF, "record"}
};

STATIC DFA_NODE_S g_astHtmlParserStateCssEndStart[] =
{
    {DFA_CODE_CHAR('/'), HTML_LEX_CSS_END_SLASH, "save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsCss"},
    {DFA_CODE_OTHER, HTML_LEX_CSS, "outputSaveAsCss,record"}
};


STATIC DFA_NODE_S g_astHtmlParserStateCssEnd_Slash[] =
{
    {DFA_CODE_CHAR('s'), HTML_LEX_CSS_END_S, "save"},
    {DFA_CODE_CHAR('S'), HTML_LEX_CSS_END_S, "save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsCss"},
    {DFA_CODE_OTHER, HTML_LEX_CSS, "outputSaveAsCss,record"}
};

STATIC DFA_NODE_S g_astHtmlParserStateCssEnd_S[] =
{
    {DFA_CODE_CHAR('t'), HTML_LEX_CSS_END_T, "save"},
    {DFA_CODE_CHAR('T'), HTML_LEX_CSS_END_T, "save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsCss"},
    {DFA_CODE_OTHER, HTML_LEX_CSS, "outputSaveAsCss,record"}
};

STATIC DFA_NODE_S g_astHtmlParserStateCssEnd_T[] =
{
    {DFA_CODE_CHAR('y'), HTML_LEX_CSS_END_Y, "save"},
    {DFA_CODE_CHAR('Y'), HTML_LEX_CSS_END_Y, "save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsCss"},
    {DFA_CODE_OTHER, HTML_LEX_CSS, "outputSaveAsCss,record"}
};

STATIC DFA_NODE_S g_astHtmlParserStateCssEnd_Y[] =
{
    {DFA_CODE_CHAR('l'), HTML_LEX_CSS_END_L, "save"},
    {DFA_CODE_CHAR('L'), HTML_LEX_CSS_END_L, "save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsCss"},
    {DFA_CODE_OTHER, HTML_LEX_CSS, "outputSaveAsCss,record"}
};

STATIC DFA_NODE_S g_astHtmlParserStateCssEnd_L[] =
{
    {DFA_CODE_CHAR('e'), HTML_LEX_CSS_END_E, "save"},
    {DFA_CODE_CHAR('E'), HTML_LEX_CSS_END_E, "save"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsCss"},
    {DFA_CODE_OTHER, HTML_LEX_CSS, "outputSaveAsCss,record"}
};

STATIC DFA_NODE_S g_astHtmlParserStateCssEnd_E[] =
{
    {DFA_CODE_CHAR('>'), HTML_LEX_DATA, "save,CssEnd,outputTag"},
    {DFA_CODE_LWS, HTML_LEX_BEFORE_ATTR_NAME, "CssEnd"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, HTML_LEX_DATA, "outputSaveAsCss"},
    {DFA_CODE_OTHER, HTML_LEX_CSS, "outputSaveAsCss,record"}
};


static DFA_TBL_LINE_S g_astHtmlParserDFA[] =
{
    DFA_TBL_LINE(g_astHtmlParserStateInit),
    DFA_TBL_LINE(g_astHtmlParserStateData),
    DFA_TBL_LINE(g_astHtmlParserStateTagOpen),
    DFA_TBL_LINE(g_astHtmlParserStateEndTagOpen),
    DFA_TBL_LINE(g_astHtmlParserStateSelfClosingStartTag),
    DFA_TBL_LINE(g_astHtmlParserStateTagName),
    DFA_TBL_LINE(g_astHtmlParserStateBeforeAttrName),
    DFA_TBL_LINE(g_astHtmlParserStateAttrName),
    DFA_TBL_LINE(g_astHtmlParserStateAfterAttrName),
    DFA_TBL_LINE(g_astHtmlParserStateBeforeAttrValue),
    DFA_TBL_LINE(g_astHtmlParserStateAttrValueDobuldQuoted),
    DFA_TBL_LINE(g_astHtmlParserStateAttrValueSingleQuoted),
    DFA_TBL_LINE(g_astHtmlParserStateAfterAttrValueQuoted),
    DFA_TBL_LINE(g_astHtmlParserStateAttrValueUnquoted),
    DFA_TBL_LINE(g_astHtmlParserStateMarkDeclaration),
    DFA_TBL_LINE(g_astHtmlParserStateBogusComment),
    DFA_TBL_LINE(g_astHtmlParserStateJs),
    DFA_TBL_LINE(g_astHtmlParserStateJsEndStart),
    DFA_TBL_LINE(g_astHtmlParserStateJsEndSlash),
    DFA_TBL_LINE(g_astHtmlParserStateJsEnd_S),
    DFA_TBL_LINE(g_astHtmlParserStateJsEnd_C),
    DFA_TBL_LINE(g_astHtmlParserStateJsEnd_R),
    DFA_TBL_LINE(g_astHtmlParserStateJsEnd_I),
    DFA_TBL_LINE(g_astHtmlParserStateJsEnd_P),
    DFA_TBL_LINE(g_astHtmlParserStateJsEnd_T),
    DFA_TBL_LINE(g_astHtmlParserStateCss),
    DFA_TBL_LINE(g_astHtmlParserStateCssEndStart),
    DFA_TBL_LINE(g_astHtmlParserStateCssEnd_Slash),
    DFA_TBL_LINE(g_astHtmlParserStateCssEnd_S),
    DFA_TBL_LINE(g_astHtmlParserStateCssEnd_T),
    DFA_TBL_LINE(g_astHtmlParserStateCssEnd_Y),
    DFA_TBL_LINE(g_astHtmlParserStateCssEnd_L),
    DFA_TBL_LINE(g_astHtmlParserStateCssEnd_E),
    DFA_TBL_END
};

static VOID html_parser_NotHtml(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    pstCtrl->uiFlag |= HTML_PARSER_FLAG_NOT_HTML;
}

static VOID html_parser_OutputRecord(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    if (pstCtrl->stData.uiLen == 0)
    {
        return;
    }

    pstCtrl->pfOutput(HTML_PARSER_DATA_TYPE_CONTENT, &pstCtrl->stData, pstCtrl->pUserContext);

    html_parser_ClearRecord(hDfa);
}

static VOID html_parser_OutputJs(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    if (pstCtrl->stData.uiLen == 0)
    {
        return;
    }

    pstCtrl->pfOutput(HTML_PARSER_DATA_TYPE_JS, &pstCtrl->stData, pstCtrl->pUserContext);

    html_parser_ClearRecord(hDfa);
}

static VOID html_parser_OutputCss(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    if (pstCtrl->stData.uiLen == 0)
    {
        return;
    }

    pstCtrl->pfOutput(HTML_PARSER_DATA_TYPE_CSS, &pstCtrl->stData, pstCtrl->pUserContext);

    html_parser_ClearRecord(hDfa);
}

static VOID html_parser_OutputTag(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    if (pstCtrl->stTag.usTagNameLen == 0)
    {
        return;
    }

    pstCtrl->pfOutput(HTML_PARSER_DATA_TYPE_TAG, &pstCtrl->stTag, pstCtrl->pUserContext);

    pstCtrl->stTag.uiTagFlag &= ~HTML_PARSER_TAG_FLAG_TAG_START;

    html_parser_ClearTagAttrName(hDfa);
    html_parser_ClearTagAttrValue(hDfa);
}

static VOID html_parser_RecordData(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    if (pstCtrl->stData.pcData == NULL)
    {
        pstCtrl->stData.pcData = pstCtrl->pcHtmlCurrent;
    }

    pstCtrl->stData.uiLen ++;
}

static VOID html_parser_Save(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    if (pstCtrl->stSaveBuf.uiLen >= HTML_PARSER_MAX_SAVE_BUF_LEN)
    {
        return;
    }

    pstCtrl->stSaveBuf.szBuf[pstCtrl->stSaveBuf.uiLen]
        = (CHAR)(UCHAR)DFA_GetInputCode(hDfa);
    pstCtrl->stSaveBuf.uiLen ++;
}

static VOID html_parser_ClearSave(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    memset(pstCtrl->stSaveBuf.szBuf, 0, sizeof(pstCtrl->stSaveBuf.szBuf));

    pstCtrl->stSaveBuf.uiLen = 0;
}

static VOID html_parser_OutputSave(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);
    LSTR_S stData;

    if (pstCtrl->stSaveBuf.uiLen == 0)
    {
        return;
    }

    stData.pcData = pstCtrl->stSaveBuf.szBuf;
    stData.uiLen = pstCtrl->stSaveBuf.uiLen;

    pstCtrl->pfOutput(HTML_PARSER_DATA_TYPE_CONTENT, &stData, pstCtrl->pUserContext);

    html_parser_ClearSave(hDfa);
}

static VOID html_parser_OutputSaveAsJs(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);
    LSTR_S stData;

    if (pstCtrl->stSaveBuf.uiLen == 0)
    {
        return;
    }

    stData.pcData = pstCtrl->stSaveBuf.szBuf;
    stData.uiLen = pstCtrl->stSaveBuf.uiLen;

    pstCtrl->pfOutput(HTML_PARSER_DATA_TYPE_JS, &stData, pstCtrl->pUserContext);

    html_parser_ClearSave(hDfa);
}

static VOID html_parser_OutputSaveAsCss(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);
    LSTR_S stData;

    if (pstCtrl->stSaveBuf.uiLen == 0)
    {
        return;
    }

    stData.pcData = pstCtrl->stSaveBuf.szBuf;
    stData.uiLen = pstCtrl->stSaveBuf.uiLen;

    pstCtrl->pfOutput(HTML_PARSER_DATA_TYPE_CSS, &stData, pstCtrl->pUserContext);

    html_parser_ClearSave(hDfa);
}

static VOID html_parser_ClearRecord(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    pstCtrl->stData.pcData = NULL;
    pstCtrl->stData.uiLen = 0;
}

static VOID html_parser_ClearTagName(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    memset(pstCtrl->stTag.szTagName, 0, sizeof(pstCtrl->stTag.szTagName));

    pstCtrl->stTag.usTagNameLen = 0;
}

static VOID html_parser_ClearTagAttrName(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    memset(pstCtrl->stTag.szAttrName, 0, sizeof(pstCtrl->stTag.szAttrName));

    pstCtrl->stTag.usAttrNameLen = 0;

    return;
}

static VOID html_parser_ClearTagAttrValue(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    memset(pstCtrl->stTag.szAttrValue, 0, sizeof(pstCtrl->stTag.szAttrValue));

    pstCtrl->stTag.usAttrValueLen = 0;

    return;
}

static VOID html_parser_TagName(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    if (pstCtrl->stTag.usTagNameLen >= HTML_PARSER_MAX_TAG_NAME_LEN)
    {
        return;
    }

    pstCtrl->stTag.szTagName[pstCtrl->stTag.usTagNameLen]
        = (CHAR)(UCHAR)DFA_GetInputCode(hDfa);
    pstCtrl->stTag.usTagNameLen++;
}

static VOID html_parser_TagAttrName(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    if (pstCtrl->stTag.usAttrNameLen >= HTML_PARSER_MAX_ATTR_NAME_LEN)
    {
        return;
    }

    pstCtrl->stTag.szAttrName[pstCtrl->stTag.usAttrNameLen]
        = (CHAR)(UCHAR)DFA_GetInputCode(hDfa);
    pstCtrl->stTag.usAttrNameLen++;

    return;
}

static VOID html_parser_TagAttrValue(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    if (pstCtrl->stTag.usAttrNameLen >= HTML_PARSER_MAX_ATTR_VALUE_LEN)
    {
        return;
    }

    pstCtrl->stTag.szAttrValue[pstCtrl->stTag.usAttrValueLen]
        = (CHAR)(UCHAR)DFA_GetInputCode(hDfa);
    pstCtrl->stTag.usAttrValueLen ++;

    return;
}

static BOOL_T html_parser_TestJs(IN HTML_PARSER_CTRL_S *pstCtrl)
{
    HTML_PARSER_TAG_S *pstTag = &pstCtrl->stTag;

    if (pstTag->uiTagFlag & HTML_PARSER_TAG_FLAG_SELF_CLOSE)
    {
        return FALSE;
    }

    if (pstTag->usTagNameLen != (sizeof("script") - 1))
    {
        return FALSE;
    }

    if (strnicmp(pstTag->szTagName, "script", pstTag->usTagNameLen) != 0)
    {
        return FALSE;
    }

    DFA_SetState(pstCtrl->hDfa, HTML_LEX_JS);

    pstCtrl->pfOutput(HTML_PARSER_DATA_TYPE_JS_START, NULL, pstCtrl->pUserContext);

    return TRUE;
}

static BOOL_T html_parser_TestCss(IN HTML_PARSER_CTRL_S *pstCtrl)
{
    HTML_PARSER_TAG_S *pstTag = &pstCtrl->stTag;

    if (pstTag->uiTagFlag & HTML_PARSER_TAG_FLAG_SELF_CLOSE)
    {
        return FALSE;
    }

    if (pstTag->usTagNameLen != (sizeof("style") - 1))
    {
        return FALSE;
    }

    if (strnicmp(pstTag->szTagName, "style", pstTag->usTagNameLen) != 0)
    {
        return FALSE;
    }

    DFA_SetState(pstCtrl->hDfa, HTML_LEX_CSS);

    pstCtrl->pfOutput(HTML_PARSER_DATA_TYPE_CSS_START, NULL, pstCtrl->pUserContext);

    return TRUE;
}

static VOID html_parser_TestSpecTag(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    if (TRUE == html_parser_TestJs(pstCtrl))
    {
        return;
    }

    if (TRUE == html_parser_TestCss(pstCtrl))
    {
        return;
    }

    return;
}

static VOID html_parser_TagStart(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    memset(&pstCtrl->stTag, 0, sizeof(pstCtrl->stTag));

    pstCtrl->stTag.uiTagFlag |= HTML_PARSER_TAG_FLAG_TAG_START;
}

static VOID html_parser_TagEnd(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    pstCtrl->stTag.uiTagFlag |= HTML_PARSER_TAG_FLAG_TAG_END;
}

static VOID html_parser_EndTagSlash(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    pstCtrl->stTag.uiTagFlag |= HTML_PARSER_TAG_FLAG_WITH_END_SLASH;
}

static VOID html_parser_JsEnd(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    pstCtrl->stTag.uiTagFlag |=
        (HTML_PARSER_TAG_FLAG_TAG_START|HTML_PARSER_TAG_FLAG_TAG_END|HTML_PARSER_TAG_FLAG_WITH_END_SLASH);

    TXT_Strlcpy(pstCtrl->stTag.szTagName, "script", sizeof(pstCtrl->stTag.szTagName));

    pstCtrl->pfOutput(HTML_PARSER_DATA_TYPE_JS_END, NULL, pstCtrl->pUserContext);
}

static VOID html_parser_CssEnd(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    pstCtrl->stTag.uiTagFlag |=
        (HTML_PARSER_TAG_FLAG_TAG_START|HTML_PARSER_TAG_FLAG_TAG_END|HTML_PARSER_TAG_FLAG_WITH_END_SLASH);

    TXT_Strlcpy(pstCtrl->stTag.szTagName, "style", sizeof(pstCtrl->stTag.szTagName));

    pstCtrl->pfOutput(HTML_PARSER_DATA_TYPE_CSS_END, NULL, pstCtrl->pUserContext);
}

static VOID html_parser_SelfClose(IN DFA_HANDLE hDfa)
{
    HTML_PARSER_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    pstCtrl->stTag.uiTagFlag |= HTML_PARSER_TAG_FLAG_SELF_CLOSE;
}

static void html_parser_Init()
{
    static BOOL_T bInit = FALSE;

    if (bInit == FALSE)
    {
        bInit = TRUE;
        DFA_Compile(g_astHtmlParserDFA, g_astHtmlParserActions);
    }
}

HTML_PARSER_HANDLE HTML_Parser_Create(IN PF_HTML_OUTPUT_FUNC pfOutput, IN VOID *pUserContext)
{
    HTML_PARSER_CTRL_S * pstCtrl;

    html_parser_Init();

    pstCtrl = MEM_ZMalloc(sizeof(HTML_PARSER_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstCtrl->hDfa = DFA_Create(g_astHtmlParserDFA, g_astHtmlParserActions, HTML_LEX_INIT);
    if (NULL == pstCtrl->hDfa)
    {
        MEM_Free(pstCtrl);
        return NULL;
    }

    DFA_SetUserData(pstCtrl->hDfa, pstCtrl);

    pstCtrl->pfOutput = pfOutput;
    pstCtrl->pUserContext = pUserContext;

    return pstCtrl;
}

VOID HTML_Parser_Destroy(IN HTML_PARSER_HANDLE hPraser)
{
    HTML_PARSER_CTRL_S *pstCtrl = hPraser;

    DFA_Destory(pstCtrl->hDfa);
    MEM_Free(pstCtrl);
}

VOID HTML_Parser_Run
(
    IN HTML_PARSER_HANDLE hParser,
    IN CHAR *pcHtmlData,
    IN UINT uiHtmlDataLen
)
{
    HTML_PARSER_CTRL_S * pstCtrl = hParser;
    UINT uiIndex;
    LSTR_S stStr;

    if ((NULL == pcHtmlData) || (uiHtmlDataLen == 0))
    {
        return;
    }

    if (pstCtrl->uiFlag & HTML_PARSER_FLAG_NOT_HTML)
    {
        stStr.pcData = pcHtmlData;
        stStr.uiLen = uiHtmlDataLen;
        pstCtrl->pfOutput(HTML_PARSER_DATA_TYPE_CONTENT, &stStr, pstCtrl->pUserContext);

        return;
    }

    pstCtrl->pcHtmlData = pcHtmlData;
    pstCtrl->uiHtmlDataLen = uiHtmlDataLen;
    pstCtrl->pcHtmlCurrent = pcHtmlData;

    for (uiIndex=0; uiIndex<uiHtmlDataLen; uiIndex++)
    {
        if (*pstCtrl->pcHtmlCurrent == 0)
        {
            
            pstCtrl->uiFlag |= HTML_PARSER_FLAG_NOT_HTML;
            break;
        }

        DFA_InputChar(pstCtrl->hDfa, *pstCtrl->pcHtmlCurrent);
        pstCtrl->pcHtmlCurrent ++;

        if (pstCtrl->uiFlag & HTML_PARSER_FLAG_NOT_HTML)
        {
            break;
        }
    }

    DFA_Edge(pstCtrl->hDfa);

    if (pstCtrl->uiFlag & HTML_PARSER_FLAG_NOT_HTML)
    {
        DFA_End(pstCtrl->hDfa);
        stStr.pcData = pstCtrl->pcHtmlCurrent;
        stStr.uiLen = uiHtmlDataLen - uiIndex;
        if (stStr.uiLen > 0)
        {
            pstCtrl->pfOutput(HTML_PARSER_DATA_TYPE_CONTENT, &stStr, pstCtrl->pUserContext);
        }
    }
}

VOID HTML_Parser_End(IN HTML_PARSER_HANDLE hParser)
{
    HTML_PARSER_CTRL_S * pstCtrl = hParser;

    if ((pstCtrl->uiFlag & HTML_PARSER_FLAG_NOT_HTML) == 0)
    {
        DFA_End(pstCtrl->hDfa);
    }
}


