/*================================================================
*   Created by LiXingang: 2018.12.13
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

int main(int argc, char **argv)
{
    char hostid[36] = "";

    license_get_hostid(hostid);

    printf("HostID: %s\r\n", hostid);

    return 0;
}

