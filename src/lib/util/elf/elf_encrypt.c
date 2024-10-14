/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "utl/gauss_encrypt.h"
#include "utl/elf_utl.h"
#include "utl/elf_lib.h"
#include "utl/elf_encrypt.h"

#define	ELF_ENCRYPT_MAGIC "\177ENC"
#define	ELF_ENCRYPT_MAGIC_SIZE 4
#define ELF_ENCRYPT_PASSWORD "spf_encrypt"

BOOL_T ELFLIB_IsEncrypted(LLDATA_S *d)
{
    if (d->len <= ELF_ENCRYPT_MAGIC_SIZE) {
        return FALSE;
    }

    char magic[] = ELF_ENCRYPT_MAGIC;
    if (memcmp(d->data, magic, STR_LEN(magic)) == 0) {
        return TRUE;
    }

    return FALSE;
}

int ELFLIB_Encrypt(LLDATA_S *d)
{
    if (d->len <= ELF_ENCRYPT_MAGIC_SIZE) {
        RETURN(BS_ERR);
    }

    
    U8 *data = (U8*)d->data + ELF_ENCRYPT_MAGIC_SIZE;
    U64 enc_len = d->len - ELF_ENCRYPT_MAGIC_SIZE;

    U8 pwd[] = ELF_ENCRYPT_PASSWORD;
    GAUSS_EncryptIV(data, enc_len, pwd, sizeof(pwd), 0);

    Elf64_Ehdr *ehdr = (void*)d->data;
    char magic[] = ELF_ENCRYPT_MAGIC;
    memcpy(ehdr->e_ident, magic, STR_LEN(magic));

    return 0;
}

int ELFLIB_Decrypt(LLDATA_S *d)
{
    if (! ELFLIB_IsEncrypted(d)) {
        return 0;
    }

    U8 *data = (U8*)d->data + ELF_ENCRYPT_MAGIC_SIZE;
    U64 enc_len = d->len - ELF_ENCRYPT_MAGIC_SIZE;

    U8 pwd[] = ELF_ENCRYPT_PASSWORD;
    GAUSS_DecryptIV(data, enc_len, pwd, sizeof(pwd), 0);

    Elf64_Ehdr *ehdr = (void*)d->data;
    char magic[] = ELFMAG;
    memcpy(ehdr->e_ident, magic, STR_LEN(magic));

    return 0;
}

int ELFLIB_EncryptVbuf(VBUF_S *vbuf)
{
    LLDATA_S d;
    d.data = VBUF_GetData(vbuf);
    d.len = VBUF_GetDataLength(vbuf);
    return ELFLIB_Encrypt(&d);
}

int ELFLIB_DecryptVbuf(VBUF_S *vbuf)
{
    LLDATA_S d;
    d.data = VBUF_GetData(vbuf);
    d.len = VBUF_GetDataLength(vbuf);
    return ELFLIB_Decrypt(&d);
}


