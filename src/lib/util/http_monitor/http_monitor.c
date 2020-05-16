/*================================================================
*   Created by LiXingang
*   Description: 旁路方式监控
*
================================================================*/
#include "bs.h"
#include "utl/http_lib.h"
#include "utl/http_monitor.h"

int HttpMonitor_Init(HTTP_MONITOR_S *ctrl, HTTP_SOURCE_E type)
{
    memset(ctrl, 0, sizeof(HTTP_MONITOR_S));
    ctrl->state = HTTP_MONITOR_STATE_HEAD;
    ctrl->type = type;
    ctrl->head_parser = HTTP_CreateHeadParser();
    if (NULL == ctrl->head_parser) {
        RETURN(BS_NO_MEMORY);
    }

    return 0;
}

int HttpMonitor_Reset(HTTP_MONITOR_S *ctrl)
{
    HttpMonitor_Final(ctrl);
    return HttpMonitor_Init(ctrl, ctrl->type);
}

void HttpMonitor_Final(HTTP_MONITOR_S *ctrl)
{
    if (ctrl == NULL) {
        return;
    }
    if (ctrl->head_parser) {
        HTTP_DestoryHeadParser(ctrl->head_parser);
        ctrl->head_parser = NULL;
    }
    if (ctrl->body_parser) {
        HTTP_BODY_DestroyParser(ctrl->body_parser);
        ctrl->body_parser = NULL;
    }
}

static BS_STATUS httpmonitor_Body(UCHAR *pucData, UINT uiDataLen,
        USER_HANDLE_S *pstUserPointer)
{
    HTTP_MONITOR_S *ctrl = pstUserPointer->ahUserHandle[0];

    return ctrl->output(ctrl, HTTP_MONITOR_DATA_BODY,
            pucData, uiDataLen, ctrl->ud);
}

static int httpmonitor_ProcessHead(HTTP_MONITOR_S *ctrl, UCHAR *data, int len)
{
    UINT headlen;
    int ret;

    headlen = HTTP_GetHeadLen((char*)data, len);
    if (headlen == 0) {
        return 0;
    }

    ret = HTTP_ParseHead(ctrl->head_parser, (char*)data, headlen, ctrl->type);
    if (ret < 0) {
        return ret;
    }

    ctrl->output(ctrl, HTTP_MONITOR_DATA_HEAD, data, headlen, ctrl->ud);

    if (HTTP_BODY_TRAN_TYPE_NONE != HTTP_GetBodyTranType(ctrl->head_parser)) {
        if (ctrl->body_parser) {
            HTTP_BODY_DestroyParser(ctrl->body_parser);
        }

        USER_HANDLE_S stUserHandle;
        stUserHandle.ahUserHandle[0] = ctrl;
        ctrl->body_parser = HTTP_BODY_CreateParser(ctrl->head_parser,
                httpmonitor_Body, &stUserHandle);
        if (NULL == ctrl->body_parser) {
            RETURN(BS_NO_MEMORY);
        }

        ctrl->state = HTTP_MONITOR_STATE_BODY;
    } else {
        ctrl->output(ctrl, HTTP_MONITOR_DATA_END, NULL, 0, ctrl->ud);
    }

    return headlen;
}

static int httpmonitor_ProcessBody(HTTP_MONITOR_S *ctrl, UCHAR *data, int len)
{
    UINT parsed_len;
    int ret;

    ret = HTTP_BODY_Parse(ctrl->body_parser, data, len, &parsed_len);
    if (ret == BS_NOT_COMPLETE) {
        return parsed_len;
    }

    if (ret == BS_OK) {
        ctrl->output(ctrl, HTTP_MONITOR_DATA_END, NULL, 0, ctrl->ud);
        ctrl->state = HTTP_MONITOR_STATE_HEAD;
        HTTP_BODY_DestroyParser(ctrl->body_parser);
        ctrl->body_parser = NULL;
        return parsed_len;
    }

    return ret;
}

static int httpmonitor_Process(HTTP_MONITOR_S *ctrl, UCHAR *data, int len)
{
    int process_len = 0;

    switch (ctrl->state) {
        case HTTP_MONITOR_STATE_HEAD:
            process_len = httpmonitor_ProcessHead(ctrl, data, len);
            break;
        case HTTP_MONITOR_STATE_BODY:
            process_len = httpmonitor_ProcessBody(ctrl, data, len);
            break;
        default:
            RETURN(BS_ERR);
    }

    return process_len;
}

int HttpMonitor_Input(HTTP_MONITOR_S *ctrl, UINT64 offset, UCHAR *data,
        int len, PF_HTTP_MONITOR_OUTPUT output, void *ud)
{
    int process_len = 0;

    if (ctrl->state == HTTP_MONITOR_STATE_ERR) {
        RETURN(BS_ERR);
    }

    if (ctrl->offset < offset) { /* 有丢包 */
        ctrl->state = HTTP_MONITOR_STATE_ERR;
        output(ctrl, HTTP_MONITOR_DATA_ERR, NULL, 0, ud);
        RETURN(BS_OUT_OF_RANGE);
    }

    ctrl->output = output;
    ctrl->ud = ud;

    UCHAR *data_in = data + (ctrl->offset - offset);
    int len_in = len - (ctrl->offset - offset);

    do {
        UCHAR *data_tmp = data_in + process_len;
        int len_tmp = len_in - process_len;
        int ret = httpmonitor_Process(ctrl, data_tmp, len_tmp);
        if (ret < 0) {
            ctrl->state = HTTP_MONITOR_STATE_ERR;
            output(ctrl, HTTP_MONITOR_DATA_ERR, NULL, 0, ud);
            return ret;
        }
        if (ret == 0) {
            break;
        }
        process_len += ret;
        ctrl->offset += ret;
    }while (process_len < len_in);

    return process_len;
}

