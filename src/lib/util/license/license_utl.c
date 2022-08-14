/*================================================================
*   Created by LiXingang
*   Description:license基本构成如下: 
*     [licence_index]            #License索引,用于区分多个licence
*     module=module_name         #必须字段,表明这段license作用于什么功能
*     _signature_=xxxx           #必须字段,是对所有其他字段的签名
*     version=1                  #可选字段,表明此license的版本
*     hostid=hex_string          #可选字段,由硬件信息或用户信息计算得出,如果不设置此字段,则表示其是一个通用license
*     create=time                #可选字段,表明此license的创建时间. UTC Seconds
*     expire=time                #可选字段,表明license的过期时间. UTC Seconds
*     vendor=...                 #可选字段,表示license提供商
*     description=...            #描述信息
*     tip=...                    #提示信息
*     xxx=enable                 #可选字段,开启xxx某功能, 例如: NAT=enable,   EIP=disable
*     yyy=NUMBER                 #可选字段,yyy限制为NUMBER个, 例如: users_number=1000
*     zzz=mmm                    #可选, 用户自定义的一些字段和值
================================================================*/
#include "bs.h"

#include "utl/time_utl.h"
#include "utl/cff_utl.h"
#include "utl/cff_sign.h"
#include "utl/rsa_utl.h"
#include "utl/md5_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/license_utl.h"

int LICENSE_Init(IN CFF_HANDLE hCff)
{
    char *duplicate;

    duplicate = CFF_GetTagDuplicate(hCff);
    if (NULL != duplicate) {
        return ERR_VSet(BS_CONFLICT, "%s conflict", duplicate);
    }

    return 0;
}

BS_STATUS LICENSE_X_SetModule(IN CFF_HANDLE hCff, IN char *lic_index, IN char *module)
{
    return CFF_SetPropAsString(hCff, lic_index, "module", module);
}

BS_STATUS LICENSE_X_SetVersion(IN CFF_HANDLE hCff, IN char *lic_index, IN UINT version)
{
    return CFF_SetPropAsUint(hCff, lic_index, "version", version);
}

BS_STATUS LICENSE_X_SetHostID(IN CFF_HANDLE hCff, IN char *lic_index, IN char *id)
{
    return CFF_SetPropAsString(hCff, lic_index, "hostid", id);
}

BS_STATUS LICENSE_X_EnableFeature(IN CFF_HANDLE hCff, IN char *lic_index, IN char *feature)
{
    return CFF_SetPropAsString(hCff, lic_index, feature, "enable");
}

BS_STATUS LICENSE_X_SetFeatureLimition(IN CFF_HANDLE hCff, IN char *lic_index, IN char *feature, IN UINT limit)
{
    return CFF_SetPropAsUint(hCff, lic_index, feature, limit);
}

BS_STATUS LICENSE_X_SetCreateTime(IN CFF_HANDLE hCff, IN char *lic_index)
{
    UINT64 now = TM_NowInSec();
    CHAR szTimeString[TM_STRING_TIME_LEN + 1];

    if (BS_OK != TM_Utc2Gmt(now, szTimeString)) {
        RETURN(BS_ERR);
    }

    return CFF_SetPropAsString(hCff, lic_index, "create", szTimeString);
}

BS_STATUS LICENSE_X_SetExpire(IN CFF_HANDLE hCff, IN char *lic_index, IN UINT64 expire)
{
    CHAR szTimeString[TM_STRING_TIME_LEN + 1];

    if (BS_OK != TM_Utc2Gmt(expire, szTimeString)) {
        RETURN(BS_ERR);
    }

    return CFF_SetPropAsString(hCff, lic_index, "expire", szTimeString);
}

BS_STATUS LICENSE_X_SetVendor(IN CFF_HANDLE hCff, IN char *lic_index, IN char *vendor)
{
    return CFF_SetPropAsString(hCff, lic_index, "vendor", vendor);
}

BS_STATUS LICENSE_X_SetExpireFromNow(IN CFF_HANDLE hCff, IN char *lic_index, IN UINT64 seconds_from_now)
{
    UINT64 now = TM_NowInSec();

    return LICENSE_X_SetExpire(hCff, lic_index, now + seconds_from_now);
}

BS_STATUS LICENSE_X_SetKeyValue(IN CFF_HANDLE hCff, IN char *lic_index, IN char *key, IN char *value)
{
    return CFF_SetPropAsString(hCff, lic_index, key, value);
}

/* 对license进行签名 */
int LICENSE_X_Sign(IN CFF_HANDLE hCff, IN char *lic_index, IN void *pri_key)
{
    return CFFSign_PrivateSign(hCff, lic_index, pri_key, 0);
}

/* 验证license的签名 */
int LICENSE_X_VerifySignature(IN CFF_HANDLE hCff, IN char *lic_index, IN void *pub_key)
{
    return CFFSign_PublicVerify(hCff, lic_index, pub_key, 0);
}

char * LICENSE_X_GetModule(IN CFF_HANDLE hCff, IN char *lic_index)
{
    return LICENSE_X_GetKeyValue(hCff, lic_index, "module");
}

char * LICENSE_X_GetHostID(IN CFF_HANDLE hCff, IN char *lic_index)
{
    return LICENSE_X_GetKeyValue(hCff, lic_index, "hostid");
}

char *LICENSE_X_GetVendor(IN CFF_HANDLE hCff, IN char *lic_index)
{
    return LICENSE_X_GetKeyValue(hCff, lic_index, "vendor");
}

UINT LICENSE_X_GetVersion(IN CFF_HANDLE hCff, IN char *lic_index)
{
    UINT version = 0;
    CFF_GetPropAsUint(hCff, lic_index, "version", &version);
    return version;
}

/* 获取特性是否使能 */
BOOL_T LICENSE_X_IsEnabled(IN CFF_HANDLE hCff, IN char *lic_index, IN char *feature)
{
    char *enabled = NULL;

    if (BS_OK != CFF_GetPropAsString(hCff, lic_index, feature, &enabled)) {
        return FALSE;
    }

    if (strcmp(enabled, "enable") == 0) {
        return TRUE;
    }

    return FALSE;
}

/* 验证License的有效性, 检查项包括:
  1.是否指定module; 2.是否过期; 3.是否verify通过;*/
LICENSE_VERIFY_RET LICENSE_X_Verify(IN CFF_HANDLE hCff, IN char *lic_index, IN char *module/* NULL表示不关心 */, IN RSA *pub_key)
{
    char *tmp;

    tmp = LICENSE_X_GetModule(hCff, lic_index);
    if (tmp == NULL) {
        return LICENSE_VERIFY_MODULE_NOT_EXIST;
    }

    if ((NULL != module) && (strcmp(module, tmp) != 0)) {
        return LICENSE_VERIFY_MODULE_NOT_MATCH;
    }

    if (LICENSE_X_IsExpired(hCff, lic_index)) {
        return LICENSE_VERIFY_EXPIRED;
    }

    if (0 != LICENSE_X_VerifySignature(hCff, lic_index, pub_key)) {
        return LICENSE_VERIFY_SIGNATURE_FAILED;
    }

    return LICENSE_VERIFY_OK;
}

/* 获取特性限制 */
UINT LICENSE_X_GetLimition(IN CFF_HANDLE hCff, IN char *lic_index, IN char *feature)
{
    UINT limit = 0;
    CFF_GetPropAsUint(hCff, lic_index, feature, &limit);
    return limit;
}

UINT64 LICENSE_X_GetCreateTime(IN CFF_HANDLE hCff, IN char *lic_index)
{
    char *create_time = NULL;

    if (BS_OK != CFF_GetPropAsString(hCff, lic_index, "create", &create_time)) {
        return 0;
    }

    return TM_Gmt2Utc(create_time, strlen(create_time));
}

UINT64 LICENSE_X_GetExpireTime(IN CFF_HANDLE hCff, IN char *lic_index)
{
    char *expire_time = NULL;

    if (BS_OK != CFF_GetPropAsString(hCff, lic_index, "expire", &expire_time)) {
        return 0;
    }

    return TM_Gmt2Utc(expire_time, strlen(expire_time));
}

char * LICENSE_X_GetDescription(IN CFF_HANDLE hCff, IN char *lic_index)
{
    return LICENSE_X_GetKeyValue(hCff, lic_index, "description");
}

char * LICENSE_X_GetTip(IN CFF_HANDLE hCff, IN char *lic_index)
{
    return LICENSE_X_GetKeyValue(hCff, lic_index, "tip");
}

/* 判断是否过期 */
BOOL_T LICENSE_X_IsExpired(IN CFF_HANDLE hCff, IN char *lic_index)
{
    UINT64 now;
    UINT64 expire = 0;
    UINT64 created;

    now = TM_NowInSec();

    created = LICENSE_X_GetCreateTime(hCff, lic_index);
    if (now < created) { //比创建时间早
        return TRUE;
    }

    expire = LICENSE_X_GetExpireTime(hCff, lic_index);
    if (0 == expire) { //未设置
        return FALSE;
    }

    if (now < expire) {
        return FALSE;
    }

    return TRUE;
}

char * LICENSE_X_GetKeyValue(IN CFF_HANDLE hCff, IN char *lic_index, IN char *key)
{
    char *value = NULL;

    if (BS_OK != CFF_GetPropAsString(hCff, lic_index, key, &value)) {
        return NULL;
    }

    return value;
}

BOOL_T LICENSE_X_CheckHostID(IN CFF_HANDLE hCff, IN char *lic_index, IN char *hostid)
{
    char *lic_hostid;

    lic_hostid = LICENSE_X_GetHostID(hCff, lic_index);
    if (lic_hostid == NULL) { /* 此license未绑定hostid，认为是通用license */
        return TRUE;
    }

    if (hostid == NULL) {
        return FALSE;
    }

    if (strcmp(hostid, lic_hostid) != 0) {
        return FALSE;
    }

    return TRUE;
}

char * LICENSE_VerifyResultInfo(IN int result)
{
    switch (result) {
        case LICENSE_VERIFY_OK:
            return "License verify OK";
        case LICENSE_VERIFY_MODULE_NOT_EXIST:
            return "License module not exist";
        case LICENSE_VERIFY_MODULE_NOT_MATCH:
            return "License module name not matched";
        case LICENSE_VERIFY_EXPIRED:
            return "License expired";
        case LICENSE_VERIFY_SIGNATURE_FAILED:
            return "License signature verify failed";
        default:
            return "Invalid return code";
    }

    return "";
}

BOOL_T LICENSE_IsEnabled(IN CFF_HANDLE hCff, IN char *module, IN char *hostid, IN char *feature, IN RSA *pub_key)
{
    char *tagname;

    if (! pub_key) {
        return FALSE;
    }

    CFF_SCAN_TAG_START(hCff, tagname) {
        if (! LICENSE_X_IsEnabled(hCff, tagname, feature)) {
            continue;
        }
        if (! LICENSE_X_CheckHostID(hCff, tagname, hostid)) {
            continue;
        } 
        if (LICENSE_VERIFY_OK == LICENSE_X_Verify(hCff, tagname, module, pub_key)) {
            return TRUE;
        }
    }CFF_SCAN_END();

    return FALSE;
}

static void license_print(IN char *str, IN void *user_data)
{
    printf("%s\r\n", str);
}

void LICENSE_ShowTip(IN CFF_HANDLE hCff, IN PF_LICENSE_PRINT_FUNC print_func/* 为NULL时使用缺省输出 */, IN void *user_data)
{
    char *tagname;
    char *tip;

    if (NULL == print_func) {
        print_func = license_print;
    }

    CFF_SCAN_TAG_START(hCff, tagname) {
        tip = LICENSE_X_GetTip(hCff, tagname);
        if ((NULL != tip) && (tip[0] != '\0')) {
            print_func(tip, user_data);
        }
    }CFF_SCAN_END();
}

