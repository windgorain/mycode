/*================================================================
*   Created by LiXingang: 2018.11.15
*   Description: 
*
================================================================*/
#ifndef _ACL_STRING_H
#define _ACL_STRING_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    char *action;
    char *pattern;
}ACL_STR_S;

/* 处理简单规则, 如permit *.baidu.com */
int ACLSTR_Simple_Parse(char *aclstring, ACL_STR_S *node);

#ifdef __cplusplus
}
#endif
#endif //ACL_STRING_H_
