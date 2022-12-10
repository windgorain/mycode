/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/time_utl.h"
#include "utl/cff_utl.h"
#include "utl/cff_sign.h"
#include "utl/rsa_utl.h"
#include "utl/cpu_utl.h"
#include "utl/hard_disk.h"
#include "utl/md5_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/license_utl.h"
#include "utl/license_verify.h"

static char * license_hostid(OUT char *hostid)
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

    info[0] = '\0';
    HD_GetDiskSN(info, sizeof(info));
    MD5UTL_Update(&ctx, (UCHAR*)info, strlen(info));

    MD5UTL_Final(md5_data, &ctx);

    DH_Data2HexString(md5_data, MD5_LEN, hostid);
    printf("Hostid=%s\r\n", hostid);

    return hostid;
}

int license_verify()
{
    CFF_HANDLE hCff;
    EVP_PKEY *pub_key;
    int verify_ok = 0;
    char hostid[256] = "";

    license_hostid(hostid);

    hCff = CFF_INI_Open("ali_ids.license", 0);
    if (hCff == NULL) {
        printf("Can't open ali_ids.license \r\n");
        return -1;
    }

    if (0 != LICENSE_Init(hCff)) {
        CFF_Close(hCff);
        printf("Can't init ali_ids.license \r\n");
        return -1;
    }

    LICENSE_ShowTip(hCff, NULL, NULL);

    pub_key = RSA_DftPublicKey();
    verify_ok = LICENSE_IsEnabled(hCff, "ali_ids", hostid, "ids", pub_key);
    EVP_PKEY_free(pub_key);

    CFF_Close(hCff);

    if (! verify_ok) {
        printf("License failed\r\n");
        return -1;
    }

    return 0;
}
