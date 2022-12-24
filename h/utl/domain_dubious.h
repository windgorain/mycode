/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _DOMAIN_DUBIOUS_H
#define _DOMAIN_DUBIOUS_H
#include "utl/trie_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

enum {
    DOMAIN_DUBIOUS_ADD_WHITE = 1, /* 白名单 */
    DOMAIN_DUBIOUS_ADD_BLACK,     /* 黑名单 */
};

enum {
    DOMAIN_DUBIOUS_RESULT_OK = 0, /* 检测通过 */
    DOMAIN_DUBIOUS_RESULT_DGA,    /* 随机域名 */
    DOMAIN_DUBIOUS_RESULT_BLACK,  /* 在黑名单中 */
};

typedef struct {
    TRIE_HANDLE domain_name_trie;
}DOMAIN_DUBIOUS_S;

int DomainDubious_Init(DOMAIN_DUBIOUS_S *ctrl);

/* 添加一条域名,比如: www.163.com, *.baidu.com */
/* type: DOMAIN_DUBIOUS_ADD_WHITE  or DOMAIN_DUBIOUS_ADD_BLACK */
int DomainDubious_Add(DOMAIN_DUBIOUS_S *ctrl, char *domain_name, int domain_name_len, int type);
int DomainDubious_LoadFile(DOMAIN_DUBIOUS_S *ctrl, char *file, int type);

/* return: DOMAIN_DUBIOUS_RESULT_XXX */
int DomainDubious_Check(DOMAIN_DUBIOUS_S *ctrl, char *domain_name, int domain_name_len);

#ifdef __cplusplus
}
#endif
#endif //DOMAIN_DUBIOUS_H_
