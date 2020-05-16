/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-10-26
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/bit_opt.h"
#include "utl/txt_utl.h"
#include "utl/getopt2_utl.h"

typedef enum
{
    _GETOPT2_RET_OK = 0,    /* 成功 */
    _GETOPT2_RET_SKIP1,     /* 成功并跳过一个参数 */
    _GETOPT2_RET_ERR        /* 失败 */
}_GETOPT2_RET_E;

static BS_STATUS getopt2_ParseValue(GETOPT2_NODE_S *pstNode, CHAR *pcValue)
{
    UINT uiData;
    
    switch (pstNode->value_type) {
        case 'u': {
            uiData = atoi(pcValue);
            *((UINT*)pstNode->value) = uiData;
            break;
        }

        case 'b': {
            if ((strcmp(pcValue, "1") == 0)
                || (stricmp(pcValue, "true") == 0)
                || (stricmp(pcValue, "yes") == 0)) {
                *((BOOL_T*)pstNode->value) = TRUE;
            } else {
                *((BOOL_T*)pstNode->value) = FALSE;
            }
            break;
        }

        case 's': {
            *((CHAR**)pstNode->value) = pcValue;
            break;
        }

        default: {
            return BS_ERR;
        }
    }

    return BS_OK;
}

static _GETOPT2_RET_E getopt2_ParseOpt(UINT uiArgc, CHAR **ppcArgv, GETOPT2_NODE_S *pstNodes, UINT uiOptIndex, int is_long_opt)
{
    CHAR cOpt = ppcArgv[uiOptIndex][1];
    GETOPT2_NODE_S *pstNode;
    _GETOPT2_RET_E eRet = _GETOPT2_RET_OK;
    char *pcOptName = &ppcArgv[uiOptIndex][2];
    char *pcSplit;
    char *pcValue = NULL;

    if (is_long_opt) {
        pcSplit = strchr(pcOptName, '=');
        if (NULL != pcSplit) {
            *pcSplit = '\0';
            pcValue = pcSplit + 1;
        }
    }

    for (pstNode=pstNodes; pstNode->opt_type != 0; pstNode++) {
        if (pstNode->opt_type != 'o') {
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
        if (pstNode->opt_type != 'p') {
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

static void getopt2_Init(GETOPT2_NODE_S *pstNodes)
{
    GETOPT2_NODE_S *node;

    for (node=pstNodes; node->opt_type!=0; node++) {
        node->flag = 0;
        switch (node->value_type) {
            case 'u': {
               *((UINT*)node->value) = 0;
               break;
            }

            case 'b': {
               *((BOOL_T*)node->value) = FALSE;
               break;
            }

            case 's': {
               *((CHAR**)node->value) = NULL;
               break;
            }
        }
    }
}

int GETOPT2_ParseFromArgv0(UINT uiArgc, CHAR **ppcArgv, INOUT GETOPT2_NODE_S *opts)
{
    UINT i;
    _GETOPT2_RET_E eRet;

    getopt2_Init(opts);

    for (i=0; i<uiArgc; i++) {
        /* 判断是否"-" */
        if (ppcArgv[i][0] == '-') {
            /* 判断是否是"--" */
            if (ppcArgv[i][1] == '-') {
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
    return GETOPT2_ParseFromArgv0(uiArgc-1, ppcArgv+1, opts);
}

static int getopt2_build_param_help(GETOPT2_NODE_S *nodes, OUT char *buf, int buf_size)
{
    GETOPT2_NODE_S *node;
    int len = 0;
    int is_have_param_help = 0;

    for (node=nodes; node->opt_type!=0; node++) {
        if (node->opt_type != 'p') {
            continue;
        }

        if (node->opt_long_name != NULL) {
            if (! is_have_param_help) {
                len += snprintf(buf+len, buf_size-len, "Usage: [OPTIONS]");
                is_have_param_help = 1;
            }
            if (node->opt_short_name) { /* 必须的param */
                len += snprintf(buf+len, buf_size-len, " %s", node->opt_long_name);
            } else { /* 可选的param */
                len += snprintf(buf+len, buf_size-len, " [%s]", node->opt_long_name);
            }
        }
    }

    if (is_have_param_help) {
        len += snprintf(buf+len, buf_size-len, "\r\n");
    }

    return len;
}

static int getopt2_build_opt_help(GETOPT2_NODE_S *nodes, OUT char *buf, int buf_size)
{
    GETOPT2_NODE_S *node;
    int len = 0;

    len += snprintf(buf+len, buf_size-len, "Options:\r\n");

    for (node=nodes; node->opt_type!=0; node++) {
        if (node->opt_type != 'o') {
            continue;
        }
        len += snprintf(buf+len, buf_size-len, " ");
        if (node->opt_short_name != 0) {
            len += snprintf(buf+len, buf_size-len, "-%c ", node->opt_short_name);
        }
        if (node->opt_long_name != 0) {
            len += snprintf(buf+len, buf_size-len, "--%-20s ", node->opt_long_name);
        }
        len += snprintf(buf+len, buf_size-len, "%s\r\n", node->help_info == NULL ? "": node->help_info);
    }

    return len;
}

char * GETOPT2_BuildHelpinfo(GETOPT2_NODE_S *nodes, OUT char *buf, int buf_size)
{
    int len = 0;

    buf[0] = '\0';

    len = getopt2_build_param_help(nodes, buf, buf_size);
    len += getopt2_build_opt_help(nodes, buf+len, buf_size-len);

    return buf;
}

int GETOPT2_IsOptSetted(GETOPT2_NODE_S *nodes, char short_opt_name, char *long_opt_name)
{
    GETOPT2_NODE_S *node;

    for (node=nodes; node->opt_type!=0; node++) {
        if ((short_opt_name != 0) && (short_opt_name == node->opt_short_name)) {
            return (node->flag & GETOPT2_OUT_FLAG_ISSET);
        } else if ((NULL != long_opt_name) && (NULL != node->opt_long_name) && (strcmp(long_opt_name, node->opt_long_name) == 0)) {
            return (node->flag & GETOPT2_OUT_FLAG_ISSET);
        }
    }

    return 0;
}

int GETOPT2_IsHaveError(GETOPT2_NODE_S *opts)
{
    GETOPT2_NODE_S *node;

    for (node=opts; node->opt_type!=0; node++) {
        if (node->opt_type != 'p') {
            continue;
        }
        if (node->opt_short_name) {
            if (! (node->flag & GETOPT2_OUT_FLAG_ISSET)) {
                return 1;
            }
        }
    }

    return 0;
}

