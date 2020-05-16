
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/db_mysql.h"

#include "mysql/mysql.h"

typedef struct
{
    MYSQL stMysql;
}DB_MYSQL_CTRL_S;

static BOOL_T g_bDcMysqlInit = FALSE;

DB_MYSQL_HANDLE DB_Mysql_Create(IN DB_MYSQL_PARAM_S *pstParam)
{
    DB_MYSQL_CTRL_S *pstCtrl;

    if (g_bDcMysqlInit == FALSE)
    {
        g_bDcMysqlInit = TRUE;
        mysql_library_init(0, NULL, NULL);
    }

    pstCtrl = MEM_ZMalloc(sizeof(DB_MYSQL_CTRL_S));
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

VOID DB_Mysql_Destroy(IN DB_MYSQL_HANDLE hMysql)
{
    DB_MYSQL_CTRL_S *pstCtrl = hMysql;
    
    mysql_close(&pstCtrl->stMysql);

    MEM_Free(pstCtrl);
}

static BS_STATUS db_mysql_Query(IN DB_MYSQL_HANDLE hMysql, IN CHAR *pcSql)
{
    DB_MYSQL_CTRL_S *pstCtrl = hMysql;

    if (0 != mysql_query(&pstCtrl->stMysql, pcSql))
    {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS DB_Mysql_Query(IN DB_MYSQL_HANDLE hMysql, IN CHAR *pcFmt, ...)
{
    va_list args;
    char szMsg[1024];

    va_start(args, pcFmt);
    vsnprintf(szMsg, sizeof(szMsg), pcFmt, args);
    va_end(args);

    return db_mysql_Query(hMysql, szMsg);
}

VOID * DB_Mysql_GetResult(IN DB_MYSQL_HANDLE hMysql)
{
    DB_MYSQL_CTRL_S *pstCtrl = hMysql;

    return mysql_store_result(&pstCtrl->stMysql);
}

CHAR ** DB_Mysql_FetchRow(IN VOID *pResult)
{
    if (pResult == NULL)
    {
        return NULL;
    }
    
    return mysql_fetch_row(pResult);
}

VOID DB_Mysql_FreeResult(IN VOID *pResult)
{
    if (pResult != NULL)
    {
        mysql_free_result(pResult);
    }
}


