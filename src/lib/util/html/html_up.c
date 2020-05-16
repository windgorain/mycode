/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-10-24
* Description: HTML URL Parser
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/html_parser.h"
#include "utl/html_up.h"
#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/plkv_utl.h"

typedef struct
{
    HTML_PARSER_HANDLE hHtmlParser;
    PF_HTML_UP_OUTPUT pfOutput;
    VOID *pUserContext;
}HTML_UP_S;

typedef VOID (*PF_HTML_UP_ATTR_VALUE_SPEC_PROCESS)(IN HTML_UP_S *pstUp, IN HTML_PARSER_TAG_S *pstTag);  /* attr value特殊处理 */

typedef struct
{
    CHAR *pcTagName;
    CHAR *pcAttrName;
    USHORT usTagNameLen;
    USHORT usAttrNameLen;
    PF_HTML_UP_ATTR_VALUE_SPEC_PROCESS pfSpecProcess;
}HTML_UP_LINK_S;

#define HTML_UP_LINK(_tagname,_attrname,_pfCheck) \
    {(_tagname), (_attrname), sizeof(_tagname) - 1, sizeof(_attrname) - 1, (_pfCheck)}


static VOID html_up_SpecProcessMetaContent(IN HTML_UP_S *pstUp, IN HTML_PARSER_TAG_S *pstTag);


static HTML_UP_LINK_S g_astHtmlUpLinks[] =
{
    HTML_UP_LINK("param", "value", NULL),
    HTML_UP_LINK("meta", "content", html_up_SpecProcessMetaContent),
    HTML_UP_LINK("*", "href", NULL),
    HTML_UP_LINK("*", "src", NULL),
    HTML_UP_LINK("*", "action", NULL),
    HTML_UP_LINK("*", "archive", NULL),
    HTML_UP_LINK("*", "codebase", NULL),
    HTML_UP_LINK("*", "cite", NULL),
    HTML_UP_LINK("*", "background", NULL),
    HTML_UP_LINK("*", "bgsound", NULL),
    HTML_UP_LINK("*", "profile", NULL),
    HTML_UP_LINK("*", "lowsrc", NULL),
    HTML_UP_LINK("*", "usemap", NULL),
    HTML_UP_LINK("*", "dynsrc", NULL),
    HTML_UP_LINK("*", "data", NULL),
    HTML_UP_LINK("*", "borderimage", NULL),
    HTML_UP_LINK("*", "classid", NULL),
    HTML_UP_LINK("*", "longdesc", NULL),
    HTML_UP_LINK("*", "pluginspage", NULL),
    HTML_UP_LINK("*", "pluginurl", NULL),
};

/* 检查是否URL:
   1. 必须以"字母/."开头, 否则不是URL
   2. 如果前缀是javascript或者script,则不认为是URL
*/
static BOOL_T html_up_UrlCheck(IN CHAR *pcString)
{
    CHAR *pcStringTmp;
    UCHAR ucFirstChar;

    if ((*pcString == '\"') || (*pcString == '\''))
    {
        pcString ++;
    }

    pcStringTmp = TXT_StrimHead(pcString, strlen(pcString), " \t\r\n");

    if ((strnicmp(pcString, "javascript", sizeof("javascript") - 1) == 0)
        || (strnicmp(pcString, "script", sizeof("script") - 1) == 0))
    {
        return FALSE;
    }

    ucFirstChar = *pcStringTmp;

    if (isalpha(ucFirstChar)
        || isdigit(ucFirstChar)
        || (ucFirstChar == '/')
        || (ucFirstChar == '.')
        || (ucFirstChar == '\\'))
    {
        return TRUE;
    }

    return FALSE;
}

static inline BOOL_T html_up_Match(IN HTML_PARSER_TAG_S *pstTag, IN HTML_UP_LINK_S *pstLink)
{
    if (pstLink->pcTagName[0] != '*')
    {
        if ((pstLink->usTagNameLen != pstTag->usTagNameLen)
            || (0 != stricmp(pstLink->pcTagName,pstTag->szTagName)))
        {
            return FALSE;
        }
    }

    if ((pstLink->usAttrNameLen != pstTag->usAttrNameLen)
        || (0 != stricmp(pstLink->pcAttrName,pstTag->szAttrName)))
    {
        return FALSE;
    }

    return TRUE;
}

static HTML_UP_LINK_S * html_up_AttrValueMatchLink(IN HTML_PARSER_TAG_S *pstTag)
{
    UINT uiIndex;

    for (uiIndex=0; uiIndex<sizeof(g_astHtmlUpLinks)/sizeof(HTML_UP_LINK_S); uiIndex++)
    {
        if (TRUE == html_up_Match(pstTag, &g_astHtmlUpLinks[uiIndex]))
        {
            return &g_astHtmlUpLinks[uiIndex];
        }
    }

    return NULL;
}

static VOID html_up_OutputCtrl(IN HTML_UP_S *pstUp, IN HTML_UP_TYPE_E enType)
{
    HTML_UP_PARAM_S stParam;

    stParam.enType = enType;
    stParam.pcData = "";
    stParam.uiDataLen = 0;

    pstUp->pfOutput(&stParam, pstUp->pUserContext);
}

static VOID html_up_OutputData(IN HTML_UP_S *pstUp, IN CHAR *pcData, IN UINT uiDataLen)
{
    HTML_UP_PARAM_S stParam;

    stParam.enType = HTML_UP_DATA;
    stParam.pcData = pcData;
    stParam.uiDataLen = uiDataLen;

    pstUp->pfOutput(&stParam, pstUp->pUserContext);
}

static VOID html_up_OutputUrl(IN HTML_UP_S *pstUp, IN CHAR *pcUrl, IN UINT uiUrlLen)
{
    HTML_UP_PARAM_S stParam;

    stParam.enType = HTML_UP_URL;
    stParam.pcData = pcUrl;
    stParam.uiDataLen = uiUrlLen;

    pstUp->pfOutput(&stParam, pstUp->pUserContext);
}

static VOID html_up_OutputString(IN HTML_UP_S *pstUp, IN CHAR *pcString)
{
    html_up_OutputData(pstUp, pcString, strlen(pcString));
}

static VOID html_up_ProcessData(IN HTML_UP_S *pstUp, IN LSTR_S *pstData)
{
    html_up_OutputData(pstUp, pstData->pcData, pstData->uiLen);
}

static VOID html_up_ProcessTagContent(IN HTML_UP_S *pstUp, IN CHAR *pcData, IN UINT uiDataLen)
{
    HTML_UP_PARAM_S stParam;

    stParam.enType = HTML_UP_TAG_CONTENT;
    stParam.pcData = pcData;
    stParam.uiDataLen = uiDataLen;

    pstUp->pfOutput(&stParam, pstUp->pUserContext);
}

static VOID html_up_OutputTagString(IN HTML_UP_S *pstUp, IN CHAR *pcString)
{
    html_up_ProcessTagContent(pstUp, pcString, strlen(pcString));
}

static VOID html_up_NormalProcessLink(IN HTML_UP_S *pstUp, IN HTML_PARSER_TAG_S *pstTag)
{
    if (TRUE != html_up_UrlCheck(pstTag->szAttrValue))
    {
        html_up_ProcessTagContent(pstUp, pstTag->szAttrValue, pstTag->usAttrValueLen);
        return;
    }

    html_up_OutputUrl(pstUp, pstTag->szAttrValue, pstTag->usAttrValueLen);

    return;
}

static VOID html_up_SpecProcessMetaContent(IN HTML_UP_S *pstUp, IN HTML_PARSER_TAG_S *pstTag)
{
    CHAR *pcAttrValue;
    PLKV_HANDLE hKV;
    LSTR_S stAttrValue;
    LSTR_S *pstUrl;
    UINT uiLen;

    hKV = PLKV_Create(0);
    if (NULL == hKV)
    {
        return;
    }

    pcAttrValue = pstTag->szAttrValue;
    stAttrValue.pcData = pcAttrValue;
    stAttrValue.uiLen = pstTag->usAttrValueLen;

    LSTR_RemoveQuotation(&stAttrValue);

    PLKV_Parse(hKV, &stAttrValue, ';', '=');

    pstUrl = PLKV_GetKeyValue(hKV, "url");
    if (NULL == pstUrl)
    {
        PLKV_Destroy(hKV);
        html_up_ProcessTagContent(pstUp, pstTag->szAttrValue, pstTag->usAttrValueLen);
        return;
    }

    /* 1.输出url前面的内容 */
    uiLen = pstUrl->pcData - pcAttrValue;
    if (uiLen > 0)
    {
        html_up_ProcessTagContent(pstUp, pcAttrValue, uiLen);
    }

    if (pstUrl->uiLen > 0)
    {
        html_up_OutputUrl(pstUp, pstUrl->pcData, pstUrl->uiLen);
    }

    uiLen = (pcAttrValue + pstTag->usAttrValueLen) - (pstUrl->pcData + pstUrl->uiLen);
    if (uiLen > 0)
    {
        html_up_ProcessTagContent(pstUp, pstUrl->pcData + pstUrl->uiLen, uiLen);
    }

    PLKV_Destroy(hKV);

    return;
}

static VOID html_up_ProcessTagAttrValue(IN HTML_UP_S *pstUp, IN HTML_PARSER_TAG_S *pstTag)
{
    HTML_UP_LINK_S *pstLink;

    pstLink = html_up_AttrValueMatchLink(pstTag);
    if (NULL == pstLink)
    {
        html_up_ProcessTagContent(pstUp, pstTag->szAttrValue, pstTag->usAttrValueLen);
        return;
    }

    if (pstLink->pfSpecProcess != NULL)
    {
        pstLink->pfSpecProcess(pstUp, pstTag);
        return;
    }

    html_up_NormalProcessLink(pstUp, pstTag);

    return;
}

static VOID html_up_ProcessTag(IN HTML_UP_S *pstUp, IN HTML_PARSER_TAG_S *pstTag)
{
    if (pstTag->uiTagFlag & HTML_PARSER_TAG_FLAG_TAG_START)
    {
        html_up_OutputCtrl(pstUp, HTML_UP_TAG_START);
 
        if (pstTag->uiTagFlag & HTML_PARSER_TAG_FLAG_WITH_END_SLASH)
        {
            html_up_OutputTagString(pstUp, "</");
        }
        else
        {
            html_up_OutputTagString(pstUp, "<");
        }

        html_up_OutputString(pstUp, pstTag->szTagName);
    }

    if (pstTag->usAttrNameLen != 0)
    {
        html_up_OutputTagString(pstUp, " ");
        html_up_OutputTagString(pstUp, pstTag->szAttrName);
    }

    if (pstTag->usAttrValueLen != 0)
    {
        html_up_OutputString(pstUp, "=");
        html_up_ProcessTagAttrValue(pstUp, pstTag);
    }

    if (pstTag->uiTagFlag & HTML_PARSER_TAG_FLAG_TAG_END)
    {
        if (pstTag->uiTagFlag & HTML_PARSER_TAG_FLAG_SELF_CLOSE)
        {
            html_up_OutputTagString(pstUp, " /");
        }

        html_up_OutputTagString(pstUp, ">");

        html_up_OutputCtrl(pstUp, HTML_UP_TAG_END);
    }
}

static VOID html_up_ProcessJs(IN HTML_UP_S *pstUp, IN LSTR_S *pstData)
{
    HTML_UP_PARAM_S stParam;

    stParam.enType = HTML_UP_JS;
    stParam.pcData = pstData->pcData;
    stParam.uiDataLen = pstData->uiLen;

    pstUp->pfOutput(&stParam, pstUp->pUserContext);
}

static VOID html_up_ProcessCss(IN HTML_UP_S *pstUp, IN LSTR_S *pstData)
{
    HTML_UP_PARAM_S stParam;

    stParam.enType = HTML_UP_CSS;
    stParam.pcData = pstData->pcData;
    stParam.uiDataLen = pstData->uiLen;

    pstUp->pfOutput(&stParam, pstUp->pUserContext);
}

static VOID html_up_ParserOutput
(
    IN HTML_PARSE_DATA_TYPE_E enType,
    IN VOID *pData,
    IN VOID *pUserContext
)
{
    HTML_UP_S *pstUp = pUserContext;

    switch (enType)
    {
        case HTML_PARSER_DATA_TYPE_CONTENT:
        {
            html_up_ProcessData(pstUp, pData);
            break;
        }
        case HTML_PARSER_DATA_TYPE_TAG:
        {
            html_up_ProcessTag(pstUp, pData);
            break;
        }
        case HTML_PARSER_DATA_TYPE_JS_START:
        {
            html_up_OutputCtrl(pstUp, HTML_UP_JS_START);
            break;
        }
        case HTML_PARSER_DATA_TYPE_JS:
        {
            html_up_ProcessJs(pstUp, pData);
            break;
        }
        case HTML_PARSER_DATA_TYPE_JS_END:
        {
            html_up_OutputCtrl(pstUp, HTML_UP_JS_END);
            break;
        }
        case HTML_PARSER_DATA_TYPE_CSS_START:
        {
            html_up_OutputCtrl(pstUp, HTML_UP_CSS_START);
            break;
        }
        case HTML_PARSER_DATA_TYPE_CSS:
        {
            html_up_ProcessCss(pstUp, pData);
            break;
        }
        case HTML_PARSER_DATA_TYPE_CSS_END:
        {
            html_up_OutputCtrl(pstUp, HTML_UP_CSS_END);
            break;
        }
        default:
        {
            break;
        }
    }
}

HTML_UP_HANDLE HTML_UP_Create
(
    IN PF_HTML_UP_OUTPUT pfOutput,
    IN VOID *pUserContext
)
{
    HTML_UP_S *pstUp;

    pstUp = MEM_ZMalloc(sizeof(HTML_UP_S));
    if (NULL == pstUp)
    {
        return NULL;
    }

    pstUp->hHtmlParser = HTML_Parser_Create(html_up_ParserOutput, pstUp);
    if (NULL == pstUp->hHtmlParser)
    {
        MEM_Free(pstUp);
        return NULL;
    }

    pstUp->pfOutput = pfOutput;
    pstUp->pUserContext = pUserContext;

    return pstUp;
}

VOID HTML_UP_Destroy(IN HTML_UP_HANDLE hUp)
{
    HTML_UP_S *pstUp = hUp;

    if (pstUp->hHtmlParser != NULL)
    {
        HTML_Parser_Destroy(pstUp->hHtmlParser);
    }

    MEM_Free(pstUp);
}

VOID HTML_UP_InputHtml(IN HTML_UP_HANDLE hUp, IN CHAR *pcHtml, IN UINT uiHtmlLen)
{
    HTML_UP_S *pstUp = hUp;

    HTML_Parser_Run(pstUp->hHtmlParser, pcHtml, uiHtmlLen);
}

VOID HTML_UP_InputHtmlEnd(IN HTML_UP_HANDLE hUp)
{
    HTML_UP_S *pstUp = hUp;

    HTML_Parser_End(pstUp->hHtmlParser);
}

