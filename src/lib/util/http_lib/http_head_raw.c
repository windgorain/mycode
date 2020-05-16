/*================================================================
*   Created by LiXingang
*   Description: 相比于http head parser, 它不进行解码的工作
*                也不进行内存复制,只是使用指针指向原始内存
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/http_lib.h"

typedef struct {
    DLL_NODE_S link_node;
    LSTR_S field;
    LSTR_S value;
}HTTP_HEAD_RAW_FIELD_S;

static int http_head_raw_cb(char *field, int field_len, char *value, int value_len, void *ud)
{
    HTTP_HEAD_RAW_S *ctrl = ud;

    if (field == NULL) {
        ctrl->first_line.pcData = value;
        ctrl->first_line.uiLen = value_len;
        return 0;
    }

    HTTP_HEAD_RAW_FIELD_S *node = MEM_ZMalloc(sizeof(HTTP_HEAD_RAW_FIELD_S));
    if (node == NULL) {
        RETURN(BS_NO_MEMORY);
    }

    node->field.pcData = field;
    node->field.uiLen = field_len;
    node->value.pcData = value;
    node->value.uiLen = value_len;

    DLL_ADD(&ctrl->field_list, node);

    return 0;
}

static int http_HeadRawLine(int first_line, char *line, int len,
        PF_HTTP_RAW_SCAN out_func, void *ud)
{
    char *split;
    LSTR_S field;
    LSTR_S value;

    if (first_line) {
        return out_func(NULL, 0, line, len, ud);
    }

    field.pcData = line;
    field.uiLen = len;
    value.pcData = NULL;
    value.uiLen = 0;

    split = TXT_Strnchr(line, ':', len);
    if (split) {
        field.uiLen = split - line;
        value.pcData = split + 1;
        value.uiLen = (len - field.uiLen) - 1;
    }

    LSTR_Strim(&field, " \r\n", &field);
    if (value.uiLen > 0) {
        LSTR_Strim(&value, " \r\n", &value);
    }

    return out_func(field.pcData, field.uiLen, value.pcData, value.uiLen, ud);
}

int HTTP_HeadRawScan(char *head, int head_len, PF_HTTP_RAW_SCAN out_func, void *ud)
{
    char *line = NULL, *line_tmp;
    UINT len = 0, len_tmp;
    int ret = 0;
    int first_line = 1;

    TXT_SCAN_N_LINE_BEGIN(head, head_len, line_tmp, len_tmp) {
        line = line ? line : line_tmp;
        len += len_tmp;
        if ((len_tmp > 0) && (line_tmp[len_tmp-1] == '\\')) {
            continue;
        }
        if (len > 0) {
            ret = http_HeadRawLine(first_line, line, len, out_func, ud);
            if (ret != 0) {
                break;
            }
            first_line = 0;
        }
        line = NULL;
        len = 0;
    }TXT_SCAN_LINE_END();

    return ret;
}

int HTTP_HeadRawInit(HTTP_HEAD_RAW_S *ctrl)
{
    memset(ctrl, 0, sizeof(HTTP_HEAD_RAW_S));
    DLL_INIT(&ctrl->field_list);
    return 0;
}

int HTTP_HeadRawReset(HTTP_HEAD_RAW_S *ctrl)
{
    HTTP_HEAD_RAW_FIELD_S *field;

    while ((field = DLL_Get(&ctrl->field_list))) {
        MEM_Free(field);
    }

    return HTTP_HeadRawInit(ctrl);
}

int HTTP_HeadRawFin(HTTP_HEAD_RAW_S *ctrl)
{
    return HTTP_HeadRawReset(ctrl);
}

int HTTP_HeadRawParse(HTTP_HEAD_RAW_S *ctrl, char *head, int head_len)
{
    return HTTP_HeadRawScan(head, head_len, http_head_raw_cb, ctrl);
}

LSTR_S * HTTP_HeadRawGetField(HTTP_HEAD_RAW_S *ctrl, char *field)
{
    HTTP_HEAD_RAW_FIELD_S *node;

    DLL_SCAN(&ctrl->field_list, node) {
        if (0 == LSTR_StrCmp(&node->field, field)) {
            return &node->value;
        }
    }

    return NULL;
}


