/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  
* Description: 
* History:     
******************************************************************************/
#include "klc/klc_base.h"
#include "helpers/xdp_klc.h"

#define KLC_MODULE_NAME XDP_KLC_MODULE_NAME
KLC_DEF_MODULE();

SEC_NAME_FUNC(XDP_KLC_GET_STRUCT_INFO)
int xdp_klc_get_struct_info(OUT KLC_XDP_STRUCT_INFO_S *info)
{
    info->struct_size = sizeof(struct xdp_buff);
    info->data_offset = offsetof(struct xdp_buff, data);
    info->data_end_offset = offsetof(struct xdp_buff, data_end);
    info->data_meta_offset = offsetof(struct xdp_buff, data_meta);
    info->data_hard_start_offset = offsetof(struct xdp_buff, data_hard_start);

    return 0;
}

