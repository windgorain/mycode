
#ifndef __VNETS_DB_H_
#define __VNETS_DB_H_

#include "utl/db_mysql.h"

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS VNETS_DB_Init();

DB_MYSQL_HANDLE VNETS_DB_Open();

VOID VNETS_DB_Close(IN DB_MYSQL_HANDLE hDb);


#ifdef __cplusplus
    }
#endif 

#endif 


