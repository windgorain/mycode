
#ifndef _ACL_H
#define _ACL_H
#ifdef __cplusplus
extern "C"
{
#endif

#define ACL_INVALID_ID    0 /* 无效ID */
#define ACL_NAME_MAX_LEN  63 /* ACL名称长度1-63 */

#define ACL_TYPE_DEF \
    _(ACL_TYPE_IP, "ip") /* ip-acl */ \
    _(ACL_TYPE_DOMAIN, "domain") /*domain-acl*/\
    _(ACL_TYPE_URL,  "url") /* uri-acl */ \

/* ACL类型枚举 */
typedef enum {
#define _(a, b) a,
    ACL_TYPE_DEF
#undef _

    ACL_TYPE_MAX
}ACL_TYPEL_E;


char * ACL_GetStrByType(int type);
int ACL_GetTypeByStr(char *type_str);

#ifdef __cplusplus
}
#endif
#endif //ACL_H_
