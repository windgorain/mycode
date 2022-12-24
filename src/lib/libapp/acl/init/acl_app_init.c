/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-18
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mutex_utl.h"
#include "utl/socket_utl.h"
#include "utl/ip_acl.h"
#include "utl/dpi_acl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/list_rule.h"
#include "utl/bit_opt.h"
#include "comp/comp_acl.h"
#include "../h/acl_muc.h"
#include "../h/acl_app_func.h"
#include "../h/acl_ip_group.h"
#include "../h/acl_port_group.h"

BS_STATUS AclApp_Init()
{
    AclMuc_Init();
    AclAppIP_Init();
    AclAppURL_Init();  
    AclIPGroup_Init();
    AclPortGroup_Init();
    AclDomainGroup_Init();
    return BS_OK;
}

PLUG_API BS_STATUS AclApp_Save(IN HANDLE hFile)
{
    // ACL地址池的配置必须在最初进行配置置保存，因为下面有配配置依赖关系 ip-acl 会引用地址池，所以恢复的时候也需要先灰复地址池
    AclIPGroup_Save(hFile);
    AclPortGroup_Save(hFile);
    AclAppIP_Save(hFile);
    AclAppURL_Save(hFile);
    AclDomainGroup_Save(hFile);
    AclDomain_Save(hFile);

    return BS_OK;
}

