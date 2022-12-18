/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/license_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/rsa_utl.h"
#include "utl/md5_utl.h"
#include "utl/cpu_utl.h"
#include "utl/hard_disk.h"
#include "utl/txt_utl.h"
#include "utl/cff_utl.h"
#include "utl/time_utl.h"
#include "utl/subcmd_utl.h"

int license_hostid(int argc, char **argv);
static int license_create(int argc, char **argv);
static int license_verify(int argc, char **argv);

static SUB_CMD_NODE_S g_license_subcmds[] = 
{
    {"hostid", license_hostid},
    {"create", license_create},
    {"verify", license_verify},
    {NULL, NULL}
};

static void license_help_create()
{
    printf("Usage: license create [OPTIONS] license_file\r\n");
    printf("Options:\r\n");
    printf("  -h,       Help\r\n");
    printf("  -e days,  Expire days. eg: -e 365\r\n");
    return;
}

static void license_help_verify()
{
    printf("Usage: license verify license_file\r\n");
    printf("Options:\r\n");
    printf("  -h,   Help\r\n");
    return;
}

static void license_help_hostid()
{
    printf("Usage: license hostid\r\n");
    printf("Options:\r\n");
    printf("  -h,   Help\r\n");
    return;
}

static char * license_get_hostid(OUT char *hostid)
{
    char info[256];
    unsigned char md5_data[MD5_LEN];
    MD5_CTX ctx;

    MD5UTL_Init(&ctx);

    info[0] = '\0';
    CPU_GetCompany(info, sizeof(info));
    printf("Company=%s\r\n", info);
    MD5UTL_Update(&ctx, (UCHAR*)info, strlen(info));
    info[0] = '\0';
    CPU_GetBrand(info, sizeof(info));
    printf("Brand=%s\r\n", info);
    MD5UTL_Update(&ctx, (UCHAR*)info, strlen(info));
    info[0] = '\0';
    CPU_GetBaseParam(info, sizeof(info));
    printf("Param=%s\r\n", info);
    MD5UTL_Update(&ctx, (UCHAR*)info, strlen(info));

#ifdef IN_LINUX
    info[0] = '\0';
    HD_GetDiskSN(info, sizeof(info));
    printf("DiskSn=%s\r\n", info);
    MD5UTL_Update(&ctx, (UCHAR*)info, strlen(info));
#endif

    MD5UTL_Final(md5_data, &ctx);

    DH_Data2HexString(md5_data, MD5_LEN, hostid);

    return hostid;
}

int license_hostid(int argc, char **argv)
{
    char hostid[36] = "";
    int c;

    while ((c = getopt(argc, argv, "h")) != -1) {
        switch (c) {
            case 'h':
                license_help_hostid();
                return 0;
                break;
            default:
                printf("Unknown option -%c\r\n", c);
                license_help_hostid();
                return -1;
        }
    }

    license_get_hostid(hostid);

    printf("HostID: %s\r\n", hostid);

    return 0;
}

int license_create(int argc, char **argv)
{
    CFF_HANDLE hCff;
    char *tagname;
    char *days_str= NULL;
    UINT64 days;
    UINT64 seconds = 0;
    EVP_PKEY *pri_key;
    int c;

    if (argc < 2) {
        license_help_create();
        return -1;
    }

    while ((c = getopt(argc, argv, "he:")) != -1) {
        switch (c) {
            case 'h':
                license_help_create();
                return 0;
                break;
            case 'e':
                days_str = optarg;
                break;
            default:
                printf("Unknown option -%c\r\n", c);
                license_help_create();
                return -1;
        }
    }

    hCff = CFF_INI_Open(argv[optind], 0);
    if (NULL == hCff) {
        printf("Can't open file %s\r\n", argv[optind]);
        return -1;
    }

    if (LICENSE_Init(hCff) != 0) {
        printf("%s\r\n", ErrCode_GetInfo());
        CFF_Close(hCff);
        return -1;
    }

    if (days_str) {
        days = TXT_Str2Ui(days_str);
        seconds = days * 24 * 60 * 60;
    }

    pri_key = RSA_DftPrivateKey();
    BS_DBGASSERT(pri_key);

    CFF_SCAN_TAG_START(hCff, tagname) {
        LICENSE_X_SetCreateTime(hCff, tagname);
        if (days_str) {
            LICENSE_X_SetExpireFromNow(hCff, tagname, seconds);
        }
        LICENSE_X_Sign(hCff, tagname, pri_key);
    }CFF_SCAN_END();

    EVP_PKEY_free(pri_key);

    CFF_Save(hCff);
    CFF_Close(hCff);

    return 0;
}

int license_verify(int argc, char **argv)
{
    CFF_HANDLE hCff;
    char *tagname;
    EVP_PKEY *pub_key;
    int ret;
    char *tip;

    if (argc < 2) {
        license_help_verify();
        return -1;
    }

    hCff = CFF_INI_Open(argv[1], 0);
    if (NULL == hCff) {
        printf("Can't open file %s\r\n", argv[1]);
        return -1;
    }

    if (LICENSE_Init(hCff) != 0) {
        printf("%s\r\n", ErrCode_GetInfo());
        CFF_Close(hCff);
        return -1;
    }

    pub_key = RSA_DftPublicKey();
    BS_DBGASSERT(pub_key);

    CFF_SCAN_TAG_START(hCff, tagname) {
        tip = LICENSE_X_GetTip(hCff, tagname);
        if (NULL != tip) {
            printf("[%s] %s\r\n", tagname, tip);
        }
        ret = LICENSE_X_Verify(hCff, tagname, NULL, pub_key);
        printf("[%s] %s\r\n", tagname, LICENSE_VerifyResultInfo(ret));
    }CFF_SCAN_END();

    EVP_PKEY_free(pub_key);

    CFF_Close(hCff);

    return 0;
}

int main(int argc, char **argv)
{
    return SUBCMD_Do(g_license_subcmds, argc, argv);
}
