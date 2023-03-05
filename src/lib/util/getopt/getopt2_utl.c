/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-10-26
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/bit_opt.h"
#include "utl/ip_string.h"
#include "utl/ip_utl.h"
#include "utl/txt_utl.h"
#include "utl/num_list.h"
#include "utl/getopt2_utl.h"

typedef enum
{
    _GETOPT2_RET_OK = 0,    /* 成功 */
    _GETOPT2_RET_SKIP1,     /* 成功并跳过一个参数 */
    _GETOPT2_RET_ERR        /* 失败 */
}_GETOPT2_RET_E;

static char * getopt2_opt_type_2_string(char type)
{
    switch (type) {
        case 's':
            return "STRING";
        case 'u':
            return "UINT";
        case 'b':
            return "BOOL";
        case 'i':
            return "IP";
        case 'r':
            return "RANGE";
    }

    return NULL;
}

static inline BOOL_T getopt2_is_opt_type(char type)
{
    if ((type == 'o') || (type == 'O')) {
        return TRUE;
    }
    return FALSE;
}

static inline BOOL_T getopt2_is_param_type(char type)
{
    if ((type == 'p') || (type == 'P')) {
        return TRUE;
    }
    return FALSE;
}

/* 是否设置了必选标志 */
static inline BOOL_T getopt2_is_must(GETOPT2_NODE_S *node)
{
    /* 检查是否设置了必选参数 */
    if (node->opt_type == 'P') {
        return TRUE;
    }

    /* 检查是否设置了必选选项 */
    if (node->opt_type == 'O') {
        return TRUE;
    }

    return FALSE;
}

static int getopt2_parse_value_u(GETOPT2_NODE_S *pstNode, CHAR *pcValue)
{
    UINT uiData;

    if (BS_OK != TXT_Atoui(pcValue, &uiData)) {
        return BS_ERR;
    }

    *((UINT*)pstNode->value) = uiData;

    return 0;
}

static int getopt2_parse_value_b(GETOPT2_NODE_S *pstNode, CHAR *pcValue)
{
    if ((strcmp(pcValue, "1") == 0) || (stricmp(pcValue, "true") == 0) || (stricmp(pcValue, "yes") == 0)) {
        *((BOOL_T*)pstNode->value) = TRUE;
    } else {
        *((BOOL_T*)pstNode->value) = FALSE;
    }

    return 0;
}

static int getopt2_parse_value_s(GETOPT2_NODE_S *pstNode, CHAR *pcValue)
{
    *((CHAR**)pstNode->value) = pcValue;
    return 0;
}

/* ip/prefix */
static int getopt2_parse_value_i(GETOPT2_NODE_S *pstNode, CHAR *pcValue)
{
    IP_PREFIX_S *p = pstNode->value;
    return IPString_ParseIpPrefix(pcValue, p);
}

/* num range */
static int getopt2_parse_value_r(GETOPT2_NODE_S *pstNode, CHAR *pcValue)
{
    LSTR_S lstr;
    GETOPT2_NUM_RANGE_S *r = pstNode->value;

    lstr.pcData = pcValue;
    lstr.uiLen = strlen(pcValue);

    return NumList_ParseElement(&lstr, &r->min, &r->max);
}

static BS_STATUS getopt2_ParseValue(GETOPT2_NODE_S *pstNode, CHAR *pcValue)
{
    switch (pstNode->value_type) {
        case 'u':
            return getopt2_parse_value_u(pstNode, pcValue);
        case 'b':
            return getopt2_parse_value_b(pstNode, pcValue);
        case 's':
            return getopt2_parse_value_s(pstNode, pcValue);
        case 'i':
            return getopt2_parse_value_i(pstNode, pcValue);
        case 'r':
            return getopt2_parse_value_r(pstNode, pcValue);
    }

    RETURN(BS_NOT_SUPPORT);
}

static _GETOPT2_RET_E getopt2_ParseOpt(UINT uiArgc, CHAR **ppcArgv, GETOPT2_NODE_S *pstNodes, UINT uiOptIndex, int is_long_opt)
{
    CHAR cOpt = ppcArgv[uiOptIndex][1];
    GETOPT2_NODE_S *pstNode;
    _GETOPT2_RET_E eRet = _GETOPT2_RET_OK;
    char *pcOptName = &ppcArgv[uiOptIndex][1];
    char *pcSplit;
    char *pcValue = NULL;

    if (*pcOptName == '-') {
        pcOptName ++;
    }

    if (is_long_opt) {
        pcSplit = strchr(pcOptName, '=');
        if (NULL != pcSplit) {
            *pcSplit = '\0';
            pcValue = pcSplit + 1;
        }
    }

    for (pstNode=pstNodes; pstNode->opt_type != 0; pstNode++) {
        if (! getopt2_is_opt_type(pstNode->opt_type)) {
            continue;
        }

        if (is_long_opt) {
            if (0 != strcmp(pcOptName, pstNode->opt_long_name)) {
                continue;
            }
        } else {
            if (cOpt != pstNode->opt_short_name) {
                continue;
            }
        }

        if (pstNode->value_type != 0) {
            if (pcValue == NULL) {
                if (uiOptIndex + 1 < uiArgc) {
                    pcValue = ppcArgv[uiOptIndex + 1];
                    eRet = _GETOPT2_RET_SKIP1;
                }
            }

            if (pcValue == NULL) {
                return _GETOPT2_RET_ERR;
            }

            if (BS_OK != getopt2_ParseValue(pstNode, pcValue)) {
                return _GETOPT2_RET_ERR;
            }
        }

        BIT_SET(pstNode->flag, GETOPT2_OUT_FLAG_ISSET);

        return eRet;
    }

    return _GETOPT2_RET_ERR;
}

static _GETOPT2_RET_E getopt2_ParseParam (CHAR *pcParam, GETOPT2_NODE_S *pstNodes)
{
    GETOPT2_NODE_S *pstNode;

    for (pstNode=pstNodes; pstNode->opt_type!=0; pstNode++) {
        if (! getopt2_is_param_type(pstNode->opt_type)) {
            continue;
        }

        if (pstNode->flag & GETOPT2_OUT_FLAG_ISSET) {
            continue;
        }

        if (BS_OK != getopt2_ParseValue(pstNode, pcParam)) {
            return _GETOPT2_RET_ERR;
        }

        BIT_SET(pstNode->flag, GETOPT2_OUT_FLAG_ISSET);

        return _GETOPT2_RET_OK;
    }

    return _GETOPT2_RET_ERR;
}


static void getopt2_init(GETOPT2_NODE_S *pstNodes)
{
    GETOPT2_NODE_S *node;

    for (node=pstNodes; node->opt_type!=0; node++) {
        node->flag = 0;
    }
}

static BOOL_T _getopt2_is_long_opt(char *opt)
{
    if (opt[0] != '-') {
        return FALSE;
    }

    opt ++;

    if (*opt == '\0') {
        return FALSE;
    }

    if (*opt == '-') {
        return TRUE;
    }

    opt ++;

    if ((*opt == '\0') || (*opt == ' ') || (*opt == '\t')) {
        return FALSE;
    }

    return TRUE;
}

/* 从开始就已经是需要处理的参数/选项 */
int GETOPT2_ParseFromArgv0(UINT uiArgc, CHAR **ppcArgv, INOUT GETOPT2_NODE_S *opts)
{
    UINT i;
    _GETOPT2_RET_E eRet;

    getopt2_init(opts);

    for (i=0; i<uiArgc; i++) {
        /* 判断是否"-" */
        if (ppcArgv[i][0] == '-') {
            /* 判断是否long option */
            if (_getopt2_is_long_opt(ppcArgv[i])) {
                eRet = getopt2_ParseOpt(uiArgc, ppcArgv, opts, i, 1);
            } else {
                eRet = getopt2_ParseOpt(uiArgc, ppcArgv, opts, i, 0);
            }
        } else {
            eRet = getopt2_ParseParam(ppcArgv[i], opts);
        }
        
        if (eRet == _GETOPT2_RET_SKIP1) {
            i++;
        } else if (eRet == _GETOPT2_RET_ERR) {
            return -1;
        }
    }

    if (GETOPT2_IsHaveError(opts)) {
        return -1;
    }

    return 0;
}

int GETOPT2_Parse(UINT uiArgc, CHAR **ppcArgv, INOUT GETOPT2_NODE_S *opts)
{
    /* 有时候第一个argv是可执行程序名,从第二个开始才是需要处理的参数/选项,所以需要跳过第一个argv */
    return GETOPT2_ParseFromArgv0(uiArgc - 1, ppcArgv + 1, opts);
}

#if 0
static BOOL_T getopt2_is_have_param(GETOPT2_NODE_S *nodes)
{
    GETOPT2_NODE_S *node;

    for (node=nodes; node->opt_type!=0; node++) {
        if (getopt2_is_param_type(node->opt_type)) {
            return TRUE;
        }
    }

    return FALSE;
}
#endif

static BOOL_T getopt2_is_have_opt(GETOPT2_NODE_S *nodes)
{
    GETOPT2_NODE_S *node;

    for (node=nodes; node->opt_type!=0; node++) {
        if (getopt2_is_opt_type(node->opt_type)) {
            return TRUE;
        }
    }

    return FALSE;
}

static int getopt2_build_param_help(GETOPT2_NODE_S *nodes, OUT char *buf, int buf_size)
{
    GETOPT2_NODE_S *node;
    int len = 0;

    for (node=nodes; node->opt_type!=0; node++) {
        if (! getopt2_is_param_type(node->opt_type)) {
            continue;
        }

        if (node->opt_long_name != NULL) {
            if (getopt2_is_must(node)) { /* 必须的param */
                len += snprintf(buf+len, buf_size-len, " %s", node->opt_long_name);
            } else { /* 可选的param */
                len += snprintf(buf+len, buf_size-len, " [%s]", node->opt_long_name);
            }
        }
    }

    len += snprintf(buf+len, buf_size-len, "\r\n");

    for (node=nodes; node->opt_type!=0; node++) {
        if (! getopt2_is_param_type(node->opt_type)) {
            continue;
        }

        if (node->opt_long_name != NULL) {
            len += snprintf(buf+len, buf_size-len, " %s: %s\r\n", 
                    node->opt_long_name,
                    node->help_info == NULL ? "": node->help_info);
        }
    }

    return len;
}

static void getopt2_build_opt_one(GETOPT2_NODE_S *node, OUT char *buf, int buf_size)
{
    int len = 0;

    if (node->opt_short_name) {
        len += snprintf(buf+len, buf_size-len, "-%c ", node->opt_short_name);
    }

    if (node->opt_long_name) {
        len += snprintf(buf+len, buf_size-len, "--%s ", node->opt_long_name);
    }

    if (node->value_type) {
        len += snprintf(buf+len, buf_size-len, "%s ", getopt2_opt_type_2_string(node->value_type));
    }
}

static int getopt2_build_opt_help(GETOPT2_NODE_S *nodes, OUT char *buf, int buf_size)
{
    GETOPT2_NODE_S *node;
    int len = 0;
    int must;
    char optbuf[128];

    if (getopt2_is_have_opt(nodes)) {
        len += snprintf(buf+len, buf_size-len, "Options:\r\n");
    }

    for (node=nodes; node->opt_type!=0; node++) {
        if (! getopt2_is_opt_type(node->opt_type)) {
            continue;
        }

        len += snprintf(buf+len, buf_size-len, " ");

        must = getopt2_is_must(node);

        if (must) {
            len += snprintf(buf+len, buf_size-len, "* ");
        } else {
            len += snprintf(buf+len, buf_size-len, "  ");
        }

        getopt2_build_opt_one(node, optbuf, sizeof(optbuf));

        len += snprintf(buf+len, buf_size-len, "%-32s %s\r\n", optbuf, node->help_info == NULL ? "": node->help_info);
    }

    return len;
}

char * GETOPT2_BuildHelpinfo(GETOPT2_NODE_S *nodes, OUT char *buf, int buf_size)
{
    int len = 0;

    buf[0] = '\0';

    if (getopt2_is_have_opt(nodes)) {
        len += snprintf(buf+len, buf_size-len, "Commond: [Options]");
    } else {
        len += snprintf(buf+len, buf_size-len, "Commond: ");
    }

    len += getopt2_build_param_help(nodes, buf+len, buf_size-len);

    len += getopt2_build_opt_help(nodes, buf+len, buf_size-len);

    return buf;
}

int GETOPT2_IsOptSetted(GETOPT2_NODE_S *nodes, char short_opt_name, char *long_opt_name)
{
    GETOPT2_NODE_S *node;

    for (node=nodes; node->opt_type!=0; node++) {
        if ((short_opt_name != 0) && (short_opt_name == node->opt_short_name)) {
            return (node->flag & GETOPT2_OUT_FLAG_ISSET);
        } else if ((NULL != long_opt_name) && (NULL != node->opt_long_name)
                && (strcmp(long_opt_name, node->opt_long_name) == 0)) {
            return (node->flag & GETOPT2_OUT_FLAG_ISSET);
        }
    }

    return 0;
}

int GETOPT2_IsHaveError(GETOPT2_NODE_S *opts)
{
    GETOPT2_NODE_S *node;

    for (node=opts; node->opt_type!=0; node++) {
        /* 检查是否设置了必选 */
        if (getopt2_is_must(node)) {
            if (! (node->flag & GETOPT2_OUT_FLAG_ISSET)) {
                return 1;
            }
        }
    }

    return 0;
}

void GETOPT2_PrintHelp(GETOPT2_NODE_S *opts)
{
    char buf[1024];
    printf("%s\r\n", GETOPT2_BuildHelpinfo(opts, buf, sizeof(buf)));
}

