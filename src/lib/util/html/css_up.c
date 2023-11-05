/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-10-20
* Description: 查找css文件中的url
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/html_parser.h"
#include "utl/dfa_utl.h"
#include "utl/action_utl.h"
#include "utl/txt_utl.h"
#include "utl/url_lib.h"
#include "utl/bit_opt.h"
#include "utl/css_up.h"

#define CSS_UP_MAX_SAVE_VALUEBUF_LEN 1024


typedef struct
{
    CHAR acUrlValueBuf[CSS_UP_MAX_SAVE_VALUEBUF_LEN  + 1];
    UINT uiLen;
}CSS_UP_SAVE_URLVALUE_S;


typedef struct
{
    DFA_HANDLE hDfa;
    CSS_UP_OUTPUT_PF pfOutput;
    VOID *pUserContext;
    CHAR *pcCssData;
    CHAR *pcCssCurrent; 
    CSS_UP_SAVE_URLVALUE_S stUrlValueBuf;
}CSS_UP_CTRL_S;


typedef enum tagCSS_UP_LEX_STATE
{
    CSS_UP_LEX_DATA = 0,
    CSS_UP_LEX_URL_U,
    CSS_UP_LEX_URL_R,
    CSS_UP_LEX_URL_L,
    CSS_UP_LEX_URL_BEFORE_VALUE_QUOTED,
    CSS_UP_LEX_URL_AFTER_VALUE_QUOTED,
    CSS_UP_LEX_LEFT_BRACES,
    CSS_UP_LEX_MAX
}CSS_UP_LEX_STATE_E;


STATIC VOID _cssup_ClearUrlValue(IN DFA_HANDLE hDfa);
STATIC VOID _cssup_SaveUrlValue(IN DFA_HANDLE hDfa);
STATIC VOID _cssup_ProcessUrlValue(IN DFA_HANDLE hDfa);
STATIC VOID _cssup_OutputData(IN DFA_HANDLE hDfa);

static ACTION_S g_astCssUpActions[] =
{
    ACTION_LINE("OutputData", _cssup_OutputData),
    ACTION_LINE("ClearUrlValue", _cssup_ClearUrlValue),
    ACTION_LINE("SaveUrlValue", _cssup_SaveUrlValue),
    ACTION_LINE("ProcessUrlValue", _cssup_ProcessUrlValue),
    ACTION_END
};

STATIC DFA_NODE_S g_astCssUpStateData[] =
{
    {DFA_CODE_CHAR('u'), CSS_UP_LEX_URL_U, NULL},
    {DFA_CODE_CHAR('U'), CSS_UP_LEX_URL_U, NULL},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "OutputData"},
    {DFA_CODE_END, CSS_UP_LEX_DATA, NULL},
    {DFA_CODE_OTHER, DFA_STATE_SELF, NULL}
};

STATIC DFA_NODE_S g_astCssUpStateUrl_U[] =
{
    {DFA_CODE_CHAR('r'), CSS_UP_LEX_URL_R, NULL},

    {DFA_CODE_CHAR('R'), CSS_UP_LEX_URL_R, NULL},

    {DFA_CODE_EDGE, DFA_STATE_SELF, "OutputData"},

    {DFA_CODE_END, CSS_UP_LEX_DATA, NULL},

    {DFA_CODE_OTHER, CSS_UP_LEX_DATA, NULL}
};


STATIC DFA_NODE_S g_astCssUpStateUrl_R[] =
{
    {DFA_CODE_CHAR('l'), CSS_UP_LEX_URL_L, NULL},

    {DFA_CODE_CHAR('L'), CSS_UP_LEX_URL_L, NULL},

    {DFA_CODE_EDGE, DFA_STATE_SELF, "OutputData"},

    {DFA_CODE_END, CSS_UP_LEX_DATA, NULL},

    {DFA_CODE_OTHER, CSS_UP_LEX_DATA, NULL}
};

STATIC DFA_NODE_S g_astCssUpStateUrl_L[] =
{
    {DFA_CODE_CHAR('('), CSS_UP_LEX_URL_BEFORE_VALUE_QUOTED, "OutputData,ClearUrlValue"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "OutputData"},
    {DFA_CODE_END, CSS_UP_LEX_DATA, NULL},
    {DFA_CODE_OTHER, CSS_UP_LEX_DATA, NULL}
};


STATIC DFA_NODE_S g_astCssUpStateUrlBeforeValueQuoted[] =
{
    {DFA_CODE_CHAR('"'), CSS_UP_LEX_URL_AFTER_VALUE_QUOTED, "ClearUrlValue,OutputData"},
    {DFA_CODE_CHAR('\''), CSS_UP_LEX_URL_AFTER_VALUE_QUOTED, "ClearUrlValue,OutputData"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, "OutputData"},
    {DFA_CODE_END, CSS_UP_LEX_DATA, NULL},
    {DFA_CODE_OTHER, CSS_UP_LEX_LEFT_BRACES , "SaveUrlValue"},
};

STATIC DFA_NODE_S g_astCssUpStateUrlAfterValueQuoted[] =
{
    {DFA_CODE_CHAR('"'), CSS_UP_LEX_DATA, "ProcessUrlValue"},
    {DFA_CODE_CHAR('\''), CSS_UP_LEX_DATA, "ProcessUrlValue"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, CSS_UP_LEX_DATA, NULL},
    {DFA_CODE_OTHER, DFA_STATE_SELF, "SaveUrlValue"},
};



STATIC DFA_NODE_S g_astCssUpState_Left_Braces[] =
{
    {DFA_CODE_CHAR(')'), CSS_UP_LEX_DATA, "ProcessUrlValue"},
    {DFA_CODE_EDGE, DFA_STATE_SELF, NULL},
    {DFA_CODE_END, CSS_UP_LEX_DATA, NULL},
    {DFA_CODE_OTHER, DFA_STATE_SELF, "SaveUrlValue"}
};



STATIC DFA_TBL_LINE_S g_astCssUpDFA[] =
{
    DFA_TBL_LINE(g_astCssUpStateData),
    DFA_TBL_LINE(g_astCssUpStateUrl_U),
    DFA_TBL_LINE(g_astCssUpStateUrl_R),
    DFA_TBL_LINE(g_astCssUpStateUrl_L),
    DFA_TBL_LINE(g_astCssUpStateUrlBeforeValueQuoted),
    DFA_TBL_LINE(g_astCssUpStateUrlAfterValueQuoted),
    DFA_TBL_LINE(g_astCssUpState_Left_Braces),
    DFA_TBL_END
};


STATIC VOID _cssup_ClearUrlValue(IN DFA_HANDLE hDfa)
{
    CSS_UP_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    pstCtrl->stUrlValueBuf.uiLen = 0;

    return;
}

STATIC VOID _cssup_SaveUrlValue(IN DFA_HANDLE hDfa)
{
    CSS_UP_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);
    if (pstCtrl->stUrlValueBuf.uiLen >= CSS_UP_MAX_SAVE_VALUEBUF_LEN)
    {
        return;
    }

    pstCtrl->stUrlValueBuf.acUrlValueBuf[pstCtrl->stUrlValueBuf.uiLen] = *pstCtrl->pcCssCurrent;
    pstCtrl->stUrlValueBuf.uiLen ++;

    return;
}

STATIC VOID _cssup_ProcessUrlValue(IN DFA_HANDLE hDfa)
{
    CSS_UP_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);
    URL_LIB_URL_TYPE_E enUrlType;
    CSS_UP_TYPE_E eCssUpType = CSS_UP_TYPE_URL;

    enUrlType = URL_LIB_GetUrlType(pstCtrl->stUrlValueBuf.acUrlValueBuf, pstCtrl->stUrlValueBuf.uiLen);
    if (URL_TYPE_NOT_URL == enUrlType)
    {
        eCssUpType = CSS_UP_TYPE_DATA;
    }
    
    pstCtrl->pfOutput(eCssUpType, pstCtrl->stUrlValueBuf.acUrlValueBuf,
                      (ULONG)pstCtrl->stUrlValueBuf.uiLen, pstCtrl->pUserContext);

    pstCtrl->pcCssData = pstCtrl->pcCssCurrent;

    return;
}



STATIC VOID _cssup_OutputData(IN DFA_HANDLE hDfa)
{
    CHAR *pcData = NULL;
    ULONG ulDataLen = 0;
    CSS_UP_CTRL_S *pstCtrl = DFA_GetUserData(hDfa);

    pcData = pstCtrl->pcCssData;
    ulDataLen = (ULONG)(pstCtrl->pcCssCurrent + 1) - (ULONG)(pstCtrl->pcCssData);
    if (0 == ulDataLen )
    {
        return;
    }

    pstCtrl->pfOutput(CSS_UP_TYPE_DATA, pcData, ulDataLen, pstCtrl->pUserContext);

    pstCtrl->pcCssData += ulDataLen;

    return;
}

static VOID _css_up_Init()
{
    static BOOL_T bInit = FALSE;

    if (bInit == FALSE) {
        bInit = TRUE;
        DFA_Compile(g_astCssUpDFA, g_astCssUpActions);
    }
}

CSS_UP_HANDLE CSS_UP_Create
(
    IN CSS_UP_OUTPUT_PF pfOutput,
    IN VOID *pUserContext
)
{
     CSS_UP_CTRL_S * pstCtrl;

     _css_up_Init();

    pstCtrl = MEM_ZMalloc(sizeof(CSS_UP_CTRL_S));
    if (unlikely(NULL == pstCtrl))
    {
        return NULL;
    }

    pstCtrl->hDfa = DFA_Create(g_astCssUpDFA, g_astCssUpActions, CSS_UP_LEX_DATA);
    if (NULL == pstCtrl->hDfa)
    {
        MEM_Free(pstCtrl);
        return NULL;
    }

    DFA_SetUserData(pstCtrl->hDfa, pstCtrl);

    pstCtrl->pfOutput = pfOutput;
    pstCtrl->pUserContext = pUserContext;
    pstCtrl->stUrlValueBuf.uiLen = 0;

    return pstCtrl;
}

VOID CSS_UP_Destroy(IN CSS_UP_HANDLE hParser)
{
    CSS_UP_CTRL_S * pstCtrl = hParser;

    if (NULL != pstCtrl)
    {
        DFA_Destory(pstCtrl->hDfa);
        MEM_Free(pstCtrl);
    }

    return;
}


VOID CSS_UP_End(IN CSS_UP_HANDLE hParser)
{
    CSS_UP_CTRL_S * pstCtrl = hParser;

    DFA_End(pstCtrl->hDfa);
}

VOID CSS_UP_Run
(
    IN CSS_UP_HANDLE hParser,
    IN CHAR *pcCssData,
    IN ULONG ulCssDataLen
)
{
    CHAR *pcCssEnd = pcCssData + ulCssDataLen;
    CSS_UP_CTRL_S * pstCtrl = hParser;

    pstCtrl->pcCssData = pcCssData;
    pstCtrl->pcCssCurrent = pcCssData;

    while (pstCtrl->pcCssCurrent < pcCssEnd)
    {

        DFA_InputChar(pstCtrl->hDfa, *pstCtrl->pcCssCurrent);
        pstCtrl->pcCssCurrent ++;
    }

    pstCtrl->pcCssCurrent--;

    DFA_Edge(pstCtrl->hDfa);

    return;
}


