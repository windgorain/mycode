/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "utl/elf_utl.h"
#include "utl/elf_lib.h"
#include "utl/vbuf_utl.h"
#include "utl/mybpf_utl.h"
#include "utl/mybpf_spf_sec.h"
#include "utl/mybpf_asmdef.h"

static BOOL_T _elflib_reduce_is_matched(LLDATA_S *d, Elf64_Shdr *shdr, char **prefix, int count)
{
    char *sec_name = ELFLIB_GetSecName(d, shdr);
    if (! sec_name) {
        return FALSE;
    }

    for (int i=0; i<count; i++) {
        if (strncmp(sec_name, prefix[i], strlen(prefix[i])) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}


void ELFLIB_Reduce(LLDATA_S *d, char **prefix, int count)
{
    Elf64_Shdr *shdr = NULL;

    while ((shdr = ELFLIB_GetNextSec(d, shdr))) {
        if (_elflib_reduce_is_matched(d, shdr, prefix, count)) {
            shdr->sh_type = SHT_NULL;
        }
    }
}


void ELFLIB_ReduceDebug(LLDATA_S *d)
{
    char p1[] = ".debug";
    char p2[] = ".rel.debug";
    char *prefix[] = {p1, p2};

    ELFLIB_Reduce(d, prefix, ARRAY_SIZE(prefix));
}


void ELFLIB_ReduceBTF(LLDATA_S *d)
{
    char p1[] = ".BTF";
    char p2[] = ".rel.BTF";
    char *prefix[] = {p1, p2};

    ELFLIB_Reduce(d, prefix, ARRAY_SIZE(prefix));
}


void ELFLIB_ReduceLLVM(LLDATA_S *d)
{
    char p1[] = ".llvm";
    char *prefix[] = {p1};

    ELFLIB_Reduce(d, prefix, ARRAY_SIZE(prefix));
}

