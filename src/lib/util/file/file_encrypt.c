/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

#include "sys/stat.h"
#include "utl/mem_utl.h"
#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/cjson.h"
#include "utl/passwd_utl.h"

cJSON * FILE_LoadJson(const char *filename, BOOL_T is_encrypt)
{
    FILE_MEM_S m;

    if (0 != FILE_Mem((char*)filename, &m)) {
        return NULL;
    }

    int decrypt_len = m.len/2 + 1;

    CHAR *decrypt = (CHAR*)m.data;
    CHAR *decrypt_file = MEM_ZMalloc(decrypt_len);
    if (!decrypt_file) {
        FILE_FreeMem(&m);
        return NULL;
    }

    if (is_encrypt) {
        PW_HexDecrypt(decrypt, decrypt_file, decrypt_len);
        decrypt = decrypt_file;
    }

    cJSON *rule = cJSON_Parse(decrypt);

    FILE_FreeMem(&m);
    MEM_Free(decrypt_file);

    return rule;
}
