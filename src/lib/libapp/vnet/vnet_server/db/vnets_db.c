#include "bs.h"

#include "utl/local_info.h"
#include "utl/cff_utl.h"

#include "../inc/vnets_db.h"

#define _VNETS_DB_CONFI_FILE_NAME "vnets_db.ini"

static DB_MYSQL_PARAM_S g_stVnetsDbParam;

BS_STATUS VNETS_DB_Init()
{
    CFF_HANDLE hCff;
    CHAR *pcHost, *pcUserName, *pcPassword, *pcDbName;
    UINT uiPort = 0;
    CHAR szLocalPath[FILE_MAX_PATH_LEN + 1];

    Mem_Zero(&g_stVnetsDbParam, sizeof(g_stVnetsDbParam));

    LOCAL_INFO_ExpandToConfPath(_VNETS_DB_CONFI_FILE_NAME, szLocalPath);

    hCff = CFF_INI_Open(szLocalPath, CFF_FLAG_READ_ONLY);
    if (NULL == hCff)
    {
        return BS_CAN_NOT_OPEN;
    }
    
    if (BS_OK != CFF_GetPropAsString(hCff, "mysql", "host", &pcHost))
    {
        CFF_Close(hCff);
        return BS_ERR;
    }

    if (BS_OK != CFF_GetPropAsString(hCff, "mysql", "user-name", &pcUserName))
    {
        CFF_Close(hCff);
        return BS_ERR;
    }

    if (BS_OK != CFF_GetPropAsString(hCff, "mysql", "password", &pcPassword))
    {
        CFF_Close(hCff);
        return BS_ERR;
    }

    if (BS_OK != CFF_GetPropAsString(hCff, "mysql", "db-name", &pcDbName))
    {
        CFF_Close(hCff);
        return BS_ERR;
    }

    if (BS_OK != CFF_GetPropAsUint(hCff, "mysql", "port", &uiPort))
    {
        CFF_Close(hCff);
        return BS_ERR;
    }

    g_stVnetsDbParam.pcHost = pcHost;
    g_stVnetsDbParam.pcUserName = pcUserName;
    g_stVnetsDbParam.pcPassWord = pcPassword;
    g_stVnetsDbParam.pcDbName = pcDbName;
    g_stVnetsDbParam.usPort = uiPort;

    return BS_OK;
}

DB_MYSQL_HANDLE VNETS_DB_Open()
{
    return DB_Mysql_Create(&g_stVnetsDbParam);
}

VOID VNETS_DB_Close(IN DB_MYSQL_HANDLE hDb)
{
    DB_Mysql_Destroy(hDb);
}

