
#ifndef __DB_MYSQL_H_
#define __DB_MYSQL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef HANDLE DB_MYSQL_HANDLE;

typedef struct
{
    CHAR *pcHost;
    CHAR *pcUserName;
    CHAR *pcPassWord;
    CHAR *pcDbName;
    USHORT usPort;      /* 主机序 */
}DB_MYSQL_PARAM_S;

DB_MYSQL_HANDLE DB_Mysql_Create(IN DB_MYSQL_PARAM_S *pstParam);
VOID DB_Mysql_Destroy(IN DB_MYSQL_HANDLE hMysql);
BS_STATUS DB_Mysql_Query(IN DB_MYSQL_HANDLE hMysql, IN CHAR *pcFmt, ...);
VOID * DB_Mysql_GetResult(IN DB_MYSQL_HANDLE hMysql);
CHAR ** DB_Mysql_FetchRow(IN VOID *pResult);
VOID DB_Mysql_FreeResult(IN VOID *pResult);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__DB_MYSQL_H_*/

