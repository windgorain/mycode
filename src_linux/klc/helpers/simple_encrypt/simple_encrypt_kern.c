#include "klc/klc_base.h"
#include "helpers/simple_encrypt_klc.h"

#define KLC_MODULE_NAME SIMPLE_ENCRYPT_KLC_MODULE_NAME
KLC_DEF_MODULE();

static inline void _simple_encrypt_do(char *data, char *data_end, char *passwd, int passwd_len)
{
    int i = 0;

    while (data < data_end) {
        *data = *data ^ passwd[i];
        data ++;
        i ++;
        if (i >= passwd_len) {
            i = 0;
        }
    }
}

SEC_NAME_FUNC(SIMPLE_ENCRYPT_KLC_ENC_NAME)
int simple_encrypt_klc_encrypt(SIMPLE_ENCRYPT_S *info)
{
    _simple_encrypt_do(info->data, info->data_end, info->passwd, info->passwd_len);
    return 0;
}

SEC_NAME_FUNC(SIMPLE_ENCRYPT_KLC_ENC_NAME)
int simple_encrypt_klc_decrypt(SIMPLE_ENCRYPT_S *info)
{
    _simple_encrypt_do(info->data, info->data_end, info->passwd, info->passwd_len);
    return 0;
}

