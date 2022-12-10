/*================================================================
*   Created by LiXingang: 2018.12.17
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/cff_utl.h"
#include "utl/rsa_utl.h"
#include "utl/cpu_utl.h"
#include "utl/hard_disk.h"
#include "utl/md5_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/license_utl.h"

#define FAKECERT_LICENSE_FILE "tproxy.license"

static char * fakecert_license_hostid(OUT char *hostid)
{
    char info[256];
    unsigned char md5_data[MD5_LEN];
    MD5_CTX ctx;

    MD5UTL_Init(&ctx);

    info[0] = '\0';
    CPU_GetCompany(info, sizeof(info));
    MD5UTL_Update(&ctx, (UCHAR*)info, strlen(info));
    info[0] = '\0';
    CPU_GetBrand(info, sizeof(info));
    MD5UTL_Update(&ctx, (UCHAR*)info, strlen(info));
    info[0] = '\0';
    CPU_GetBaseParam(info, sizeof(info));
    MD5UTL_Update(&ctx, (UCHAR*)info, strlen(info));

#ifdef IN_LINUX
    info[0] = '\0';
    HD_GetDiskSN(info, sizeof(info));
    MD5UTL_Update(&ctx, (UCHAR*)info, strlen(info));
#endif

    MD5UTL_Final(md5_data, &ctx);

    DH_Data2HexString(md5_data, MD5_LEN, hostid);
    printf("Hostid=%s\r\n", hostid);

    return hostid;
}

int fakecert_license_verify()
{
    CFF_HANDLE hCff;
    EVP_PKEY *pub_key;
    int verify_ok = 0;
    char hostid[256] = "";

    fakecert_license_hostid(hostid);

    hCff = CFF_INI_Open(FAKECERT_LICENSE_FILE, 0);
    if (hCff == NULL) {
        printf("Can't open %s\r\n", FAKECERT_LICENSE_FILE);
        return -1;
    }

    if (0 != LICENSE_Init(hCff)) {
        CFF_Close(hCff);
        printf("Can't init %s\r\n", FAKECERT_LICENSE_FILE);
        return -1;
    }

    LICENSE_ShowTip(hCff, NULL, NULL);

    pub_key = RSA_DftPublicKey();
    verify_ok = LICENSE_IsEnabled(hCff, "tproxy", hostid, "fakecert", pub_key);
    EVP_PKEY_free(pub_key);

    CFF_Close(hCff);

    if (! verify_ok) {
        printf("License fakecert failed\r\n");
        return -1;
    }

    return 0;
}
