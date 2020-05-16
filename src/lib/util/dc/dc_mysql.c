#include "bs.h"


#include "utl/cff_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/dc_utl.h"
#include "utl/txt_utl.h"
#include "utl/xml_cfg.h"

#include "dc_proto_tbl.h"

#include "mysql/mysql.h"

static BOOL_T g_bDcMysqlInit = FALSE;

typedef struct
{
    MYSQL stMysql;
}DC_MYSQL_CTRL_S;

HANDLE DC_Mysql_OpenInstance(IN VOID *pParam)
{
    DC_MYSQL_PARAM_S *pstParam = pParam;
    DC_MYSQL_CTRL_S *pstCtrl;

    if (g_bDcMysqlInit == FALSE)
    {
        g_bDcMysqlInit = TRUE;
        mysql_library_init(0, NULL, NULL);
    }

    pstCtrl = MEM_ZMalloc(sizeof(DC_MYSQL_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    if (NULL == mysql_init(&pstCtrl->stMysql))
    {
        MEM_Free(pstCtrl);
        return NULL;
    }
    
    if (NULL == mysql_real_connect(&pstCtrl->stMysql,
                                   pstParam->pcHost,
                                   pstParam->pcUserName,
                                   pstParam->pcPassWord,
                                   pstParam->pcDbName,
                                   pstParam->usPort, NULL, 0))
    {
        MEM_Free(pstCtrl);
        return NULL;
    }

    return pstCtrl;
}

VOID DC_Mysql_CloseInstance(IN HANDLE hHandle)
{
    DC_MYSQL_CTRL_S *pstCtrl = hHandle;
    
    mysql_close(&pstCtrl->stMysql);

    MEM_Free(pstCtrl);
}

static MYSQL_RES * dc_mysql_GetFieldValue
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName
)
{
    DC_MYSQL_CTRL_S *pstCtrl = hHandle;
    CHAR szSql[512];
    CHAR szTmp[256];
    UINT i;

    snprintf(szSql, sizeof(szSql), "select %s from %s where %s=\'%s\'",
        pcFieldName, pcTableName,
        pstKey->astKeyValue[0].pcKey, pstKey->astKeyValue[0].pcValue);

    for (i=1; i<pstKey->uiNum; i++)
    {
        snprintf(szTmp, sizeof(szTmp), " and %s=\'%s\'",
            pstKey->astKeyValue[i].pcKey, pstKey->astKeyValue[i].pcValue);

        TXT_Strlcat(szSql, szTmp, sizeof(szSql));
    }

    if (0 != mysql_query(&pstCtrl->stMysql, szSql))
    {
        return NULL;
    }

    return mysql_store_result(&pstCtrl->stMysql);
}

BS_STATUS DC_Mysql_GetFieldValueAsUint
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    OUT UINT *puiValue
)
{
    MYSQL_RES *pstRes;
    MYSQL_ROW row;

    *puiValue = 0;

    pstRes = dc_mysql_GetFieldValue(hHandle, pcTableName, pstKey, pcFieldName);
    if (NULL == pstRes)
    {
        return BS_ERR;
    }

    row = mysql_fetch_row(pstRes);
    if ((NULL == row) || (row[0] == NULL))
    {
        return BS_ERR;
    }

    TXT_Atoui(row[0], puiValue);

    mysql_free_result(pstRes);

    return BS_OK;
}

BS_STATUS DC_Mysql_CpyFieldValueAsString
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    OUT CHAR *pcValue,
    IN UINT uiValueMaxSize
)
{
    MYSQL_RES *pstRes;
    MYSQL_ROW row;

    *pcValue = '\0';

    pstRes = dc_mysql_GetFieldValue(hHandle, pcTableName, pstKey, pcFieldName);
    if (NULL == pstRes)
    {
        return BS_ERR;
    }

    row = mysql_fetch_row(pstRes);
    if ((NULL == row) || (row[0] == NULL))
    {
        return BS_ERR;
    }

    TXT_Strlcpy(pcValue, row[0], uiValueMaxSize);

    mysql_free_result(pstRes);

    return BS_OK;
}

