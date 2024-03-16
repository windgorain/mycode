/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "utl/mmap_utl.h"
#include "utl/arch_utl.h"
#include "utl/mybpf_prog_def.h"
#include "utl/bpf_helper_utl.h"
#include "utl/file_utl.h"
#include "utl/mybpf_bare.h"

static void ** _mybpf_bare_malloc_bss(int bss_size)
{
    void **bss;

    bss = MEM_Malloc(sizeof(void*));
    if (! bss) {
        return NULL;
    }

    *bss = MEM_ZMalloc(bss_size);
    if (! (*bss)) {
        MEM_Free(bss);
        return NULL;
    }

    return bss;
}

static void _mybpf_bare_free_bss(void **bss)
{
    if (bss) {
        if (*bss) {
            MEM_Free(*bss);
        }
        MEM_Free(bss);
    }
}

static int _mybpf_bare_check_depends(MYBPF_BARE_HDR_S *hdr, const void **tmp_helpers)
{
    int *helpers;
    int depend_count;
    int i;
    void *fn;

    depend_count = ntohs(hdr->depends_count);
    if (depend_count == 0) {
        return 0;
    }

    helpers = (void*)(hdr + 1);
    for (i=0; i<depend_count; i++) {
        fn = BpfHelper_GetFuncExt(ntohl(helpers[i]), tmp_helpers);
        if (! fn) {
            RETURN(BS_NOT_SUPPORT);
        }
    }

    return 0;
}


static int _mybpf_bare_check(void *mem, int mem_len, const void **tmp_helpers)
{
    MYBPF_BARE_HDR_S *hdr = mem;

    if (mem_len <= sizeof(*hdr)) {
        
        RETURNI(BS_TOO_SMALL, "Too small");
    }

    if (hdr->magic != htonl(MYBPF_BARE_MAGIC)) {
        
        RETURNI(BS_NOT_MATCHED, "Magic not match");
    }

    int size = ntohl(hdr->size);
    if (size > mem_len) {
        
        RETURNI(BS_WRONG_FILE, "File length not valid");
    }

    if ((! hdr->jit_arch) || (hdr->jit_arch != ARCH_LocalArch())) {
        RETURNI(BS_NOT_SUPPORT, "Jit arch not matched");
    }

    return _mybpf_bare_check_depends(hdr, tmp_helpers);
}

static int _mybpf_bare_load(void *data, int len, const void **tmp_helpers, OUT MYBPF_BARE_S *bare)
{
    MYBPF_BARE_HDR_S *hdr = data;
    void **bss = NULL;

    int ret = _mybpf_bare_check(data, len, tmp_helpers);
    if (ret < 0) {
        return ret;
    }

    int bss_size = ntohs(hdr->bss_size);
    if (bss_size) {
        bss = _mybpf_bare_malloc_bss(bss_size * MYBPF_BARE_BSS_BLOCK_SIZE);
        if (! bss) {
            RETURNI(BS_NO_MEMORY, "Can't malloc bss memory");
        }
    }

    int offset = sizeof(*hdr) + (sizeof(int) * ntohs(hdr->depends_count));
    void *prog_begin = (char*)data + offset;
    int prog_size = ntohl(hdr->size) - offset;

    void *fn = MMAP_Map(prog_begin, prog_size, 0);
    if (! fn) {
        _mybpf_bare_free_bss(bss);
        RETURN(BS_ERR);
    }

    MMAP_MakeExe(fn, prog_size);

    bare->prog = fn;
    bare->prog_len = prog_size;
    bare->bss = bss;

    return 0;
}

static U64 _mybpf_bare_run(MYBPF_BARE_S *bare, const void **tmp_helpers, MYBPF_PARAM_S *p)
{
    MYBPF_AOT_PROG_CTX_S ctx = {0};
    int (*fn)(U64, U64, U64, U64, U64, void*) = bare->prog;

    if (! bare->prog) {
        RETURN(BS_ERR);
    }

    
    ctx.base_helpers = BpfHelper_BaseHelper();
    ctx.sys_helpers = BpfHelper_SysHelper();
    ctx.user_helpers = BpfHelper_UserHelper();
    ctx.tmp_helpers = tmp_helpers;
    ctx.global_map_data = bare->bss;

    return fn(p->p[0], p->p[1], p->p[2], p->p[3], p->p[4], &ctx);
}

BOOL_T MYBPF_IsBareFile(char *filename)
{
    MYBPF_BARE_HDR_S hdr = {0};

    int len = FILE_MemTo(filename, &hdr, sizeof(hdr));
    if (len != sizeof(hdr)) {
        return FALSE;
    }

    if (hdr.magic != htonl(MYBPF_BARE_MAGIC)) {
        return FALSE;
    }

    return TRUE;
}

int MYBPF_LoadBare(void *data, int len, const void **tmp_helpers, OUT MYBPF_BARE_S *bare)
{
    return _mybpf_bare_load(data, len, tmp_helpers, bare);
}

void MYBPF_UnloadBare(MYBPF_BARE_S *bare)
{
    if (bare) {
        _mybpf_bare_free_bss(bare->bss);
        MMAP_Unmap(bare->prog, bare->prog_len);
        memset(bare, 0, sizeof(*bare));
    }
}

int MYBPF_LoadBareFile(char *file, const void **tmp_helpers, OUT MYBPF_BARE_S *bare)
{
    int ret;
    FILE_MEM_S m = {0};

    ret = FILE_Mem2m(file, &m);
    if (ret < 0) {
        RETURNI(BS_CAN_NOT_OPEN, "Can't open file");
    }

    ret = _mybpf_bare_load(m.data, m.len, tmp_helpers, bare);

    FILE_FreeMem(&m);

    return ret;
}

U64 MYBPF_RunBare(MYBPF_BARE_S *bare, const void **tmp_helpers, MYBPF_PARAM_S *p)
{
    return _mybpf_bare_run(bare, tmp_helpers, p);
}


U64 MYBPF_RunBareFile(char *file, const void **tmp_helpers, MYBPF_PARAM_S *p)
{
    MYBPF_BARE_S bare;
    int ret;
    U64 ret64;

    ret = MYBPF_LoadBareFile(file, tmp_helpers, &bare);
    if (ret < 0) {
        return ret;
    }

    ret64 = _mybpf_bare_run(&bare, tmp_helpers, p);

    MYBPF_UnloadBare(&bare);

    return ret64;
}

