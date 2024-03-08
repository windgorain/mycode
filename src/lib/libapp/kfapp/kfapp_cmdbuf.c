/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/cmd_buf.h"
#include "utl/txt_utl.h"
#include "utl/json_utl.h"
#include "utl/rp_utl.h"
#include "comp/comp_kfapp.h"
#include "kfapp_func.h"

#define KFAPP_CMDBUF_LIST_ELE_MAX 32

static CMD_BUF_HDL g_kfapp_cmdbuf;

static BS_STATUS _kfapp_cmdbuf_list(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    char *cmd;
    CHAR *pcLineNext;
    CHAR *pcBufTmp;
    UINT ulLineLen;
    BOOL_T bIsFoundLineEnd;
    char *ele_names[KFAPP_CMDBUF_LIST_ELE_MAX];
    char *eles[KFAPP_CMDBUF_LIST_ELE_MAX];

    cmd = MIME_GetKeyValue(hMime, "cmd");
    if ((NULL == cmd) || (cmd[0] == '\0')) {
        JSON_SetFailed(pstParam->pstJson, "Empty cmd");
        return 0;
    }

    int ret = CmdBuf_RunCmd(g_kfapp_cmdbuf, cmd);
    if (ret != 0) {
        JSON_SetFailed(pstParam->pstJson, "failed");
        return 0;
    }

    char *buf = CmdBuf_Buf(g_kfapp_cmdbuf);
    if (! buf) {
        JSON_SetFailed(pstParam->pstJson, "failed");
        return 0;
    }

    /* 处理首行 */
    char *first_line = buf;
    TXT_GetLine(first_line, &ulLineLen, &bIsFoundLineEnd, &pcLineNext);
    first_line[ulLineLen] = '\0';
    int ele_name_count = TXT_StrToToken(first_line, " \t", ele_names, KFAPP_CMDBUF_LIST_ELE_MAX);
    (void)ele_name_count;

    if (pcLineNext == NULL) {
        JSON_SetSuccess(pstParam->pstJson);
        return 0;
    }

    /* 跳过隔离行 */
    pcBufTmp = pcLineNext;
    TXT_GetLine(pcBufTmp, &ulLineLen, &bIsFoundLineEnd, &pcLineNext);

    cJSON *pstArray = cJSON_CreateArray();
    if (NULL == pstArray) {
        JSON_SetFailed(pstParam->pstJson, "Not enough memory");
        return 0;
    }

    while ((pcBufTmp = pcLineNext) != NULL) {
        TXT_GetLine(pcBufTmp, &ulLineLen, &bIsFoundLineEnd, &pcLineNext);
        pcBufTmp[ulLineLen] = '\0';
        int ele_count = TXT_StrToToken(pcBufTmp, " \t", eles, KFAPP_CMDBUF_LIST_ELE_MAX);
        if (ele_count == 0) {
            continue;
        }

        BS_DBGASSERT(ele_count == ele_name_count);

        cJSON *pstResJson = cJSON_CreateObject();
        if (NULL == pstResJson) {
            continue;
        }

        int i;
        for (i=0; i<ele_count; i++) {
            cJSON_AddStringToObject(pstResJson, ele_names[i], eles[i]);
        }
        cJSON_AddItemToArray(pstArray, pstResJson);
    }

    cJSON_AddItemToObject(pstParam->pstJson, "data", pstArray);
    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS _kfapp_cmdbuf_jshow(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    char *cmd;

    cmd = MIME_GetKeyValue(hMime, "cmd");
    if ((NULL == cmd) || (cmd[0] == '\0')) {
        JSON_SetFailed(pstParam->pstJson, "Empty cmd");
        return 0;
    }

    int ret = CmdBuf_RunCmd(g_kfapp_cmdbuf, cmd);
    if (ret != 0) {
        JSON_SetFailed(pstParam->pstJson, "failed");
        return 0;
    }

    char *buf = CmdBuf_Buf(g_kfapp_cmdbuf);
    if (! buf) {
        JSON_SetFailed(pstParam->pstJson, "failed");
        return 0;
    }

    pstParam->json_string = buf;

    return BS_OK;
}

static BS_STATUS _kfapp_cmdbuf_run(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    char *cmd;

    cmd = MIME_GetKeyValue(hMime, "cmd");
    if ((NULL == cmd) || (cmd[0] == '\0')) {
        JSON_SetFailed(pstParam->pstJson, "Empty cmd");
        return 0;
    }

    int ret = CmdBuf_RunCmd(g_kfapp_cmdbuf, cmd);
    if (ret != 0) {
        JSON_SetFailed(pstParam->pstJson, "failed");
        return 0;
    }

    char *buf = CmdBuf_Buf(g_kfapp_cmdbuf);
    if (! buf) {
        JSON_SetFailed(pstParam->pstJson, "failed");
        return 0;
    }

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static int _kfapp_cmdbuf_kv_rp(MIME_HANDLE hMime, OUT char *cmd, int cmd_size)
{
    char *key;
    RP_TAG_S tag;

    key = MIME_GetKeyValue(hMime, "cmd");
    if ((NULL == key) || (key[0] == '\0')) {
        RETURN(BS_ERR);
    }

    char * value = KFAPP_KV_Get(key);
    if (! value) {
        RETURN(BS_ERR);
    }

    tag.tag_start = '<';
    tag.tag_stop = '>';
    int ret = RP_Do(&tag, hMime, value, cmd, sizeof(cmd));
    if (ret < 0) {
        RETURN(BS_ERR);
    }

    return 0;
}

static BS_STATUS _kfapp_cmdbuf_kv(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    int ret;
    char cmd[1024];

    ret = _kfapp_cmdbuf_kv_rp(hMime, cmd, sizeof(cmd));
    if (ret < 0) {
        JSON_SetFailed(pstParam->pstJson, "failed");
        return 0;
    }

    ret = CmdBuf_RunCmd(g_kfapp_cmdbuf, cmd);
    if (ret != 0) {
        JSON_SetFailed(pstParam->pstJson, "failed");
        return 0;
    }

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

int KFAPP_CmdBuf_Init()
{
    g_kfapp_cmdbuf = CmdBuf_Create();
    if (! g_kfapp_cmdbuf) {
        RETURN(BS_ERR);
    }

    KFAPP_RegFunc("cmd.list", _kfapp_cmdbuf_list, NULL);
    KFAPP_RegFunc("cmd.jshow", _kfapp_cmdbuf_jshow, NULL);
    KFAPP_RegFunc("cmd.run", _kfapp_cmdbuf_run, NULL);
    KFAPP_RegFunc("cmd.kv", _kfapp_cmdbuf_kv, NULL);

    return 0;
}
