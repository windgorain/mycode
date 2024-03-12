/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "utl/args_utl.h"
#include "utl/time_utl.h"
#include "utl/subcmd_utl.h"
#include "utl/getopt2_utl.h"
#include "utl/mmap_utl.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_vm.h"
#include "utl/mybpf_elf.h"
#include "utl/mybpf_relo.h"
#include "utl/mybpf_file.h"
#include "utl/mybpf_jit.h"
#include "utl/mybpf_simple.h"
#include "utl/mybpf_dbg.h"
#include "utl/mybpf_asm.h"
#include "utl/mybpf_asmexp.h"
#include "utl/ubpf_utl.h"
#include "utl/file_utl.h"
#include "utl/mybpf_merge.h"
#include "utl/mybpf_bare.h"
#include "utl/umap_def.h"
#include "mybpf_loader_func.h"

#define MYBPF_BARE_MAGIC 0x7781648d
#define MYBPF_BARE_BSS_BLOCK_SIZE 16

typedef struct {
    U32 magic;
    U32 size; 
    U8 ver;
    U8 jit_arch;
    U16 bss_size; 
    U8 reserved[4];
}MYBPF_BARE_HDR_S;

static int _mybpf_bare_get_bss_size(FILE_MEM_S *m, MYBPF_SIMPLE_CONVERT_PARAM_S *p)
{
    U32 flags;
    UMAP_ELF_MAP_S *map;

    int map_count = MYBPF_SIMPLE_GetMapCount(m);
    if (map_count == 0) {
        return 0;
    }

    
    if (! p->jit_arch) {
        RETURNI(BS_NOT_SUPPORT, "Not support map");
    }

    if (map_count > 1) {
        RETURNI(BS_NOT_SUPPORT, "Not support many map");
    }

    
    flags = MYBPF_SIMPLE_GetMapFlags(m);
    if ((flags & MYBPF_SIMPLE_MAP_FLAG_BSS) == 0) {
        RETURNI(BS_NOT_SUPPORT, "Not support !bss map");
    }

    map = MYBPF_SIMPLE_GetMap(m, 0);
    if (! map) {
        RETURNI(BS_ERR, "Can't get bss map");
    }

    return (map->size_value + (MYBPF_BARE_BSS_BLOCK_SIZE - 1)) / MYBPF_BARE_BSS_BLOCK_SIZE;
}

static inline int _mybpf_bare_add_hdr(VBUF_S *vbuf, MYBPF_SIMPLE_CONVERT_PARAM_S *p, int bss_size)
{
    MYBPF_BARE_HDR_S *hdr;
    int ret;

    ret = VBUF_AddHead(vbuf, sizeof(MYBPF_BARE_HDR_S));
    if (ret < 0) {
        RETURNI(BS_OUT_OF_RANGE, "Can't add head");
    }

    hdr = VBUF_GetData(vbuf);
    memset(hdr, 0, sizeof(*hdr));

    int len = VBUF_GetDataLength(vbuf);
    hdr->magic = htonl(MYBPF_BARE_MAGIC);
    hdr->size = htonl(len);
    hdr->jit_arch = p->jit_arch;
    hdr->bss_size = htons(bss_size);

    return 0;
}

static int _mybpf_bare_convert_file(char *src_filename, char *dst_filename, MYBPF_SIMPLE_CONVERT_PARAM_S *p, OUT VBUF_S *vbuf)
{
    int ret;
    FILE_MEM_S m;
    int len;
    int bss_size;

    ret = MYBPF_SIMPLE_Convert2Buf(src_filename, p, vbuf);
    if (ret < 0) {
        return ret;
    }

    m.data = VBUF_GetData(vbuf);
    m.len = VBUF_GetDataLength(vbuf);

    bss_size = _mybpf_bare_get_bss_size(&m, p);
    if (bss_size < 0) {
        return bss_size;
    }

    void *progs = MYBPF_SIMPLE_GetProgs(&m);
    int progs_size = MYBPF_SIMPLE_GetProgsSize(&m);
    if (progs_size <= 0) {
        RETURNI(BS_OUT_OF_RANGE, "Can't get progs");
    }

    len = (char*)progs - (char*)m.data;
    VBUF_EarseHead(vbuf, len);
    VBUF_CutTail(vbuf, (m.len - len) - progs_size);

    ret = _mybpf_bare_add_hdr(vbuf, p, bss_size);
    if (ret < 0) {
        return ret;
    }

    ret = VBUF_WriteFile(dst_filename, vbuf);
    if (ret < 0) {
        RETURNI(BS_OUT_OF_RANGE, "Can't write file");
    }

    return 0;
}

static int _mybpf_bare_run_aoted(void *data, int len, void **tmp_helpers, MYBPF_PARAM_S *p, int bss_size)
{
    MYBPF_AOT_PROG_CTX_S ctx = {0};
    int (*fn)(U64, U64, U64, U64, U64, void*);
    int ret;
    void *bss = NULL;

    if (bss_size) {
        int real_bss_size = bss_size * MYBPF_BARE_BSS_BLOCK_SIZE;
        bss = MEM_ZMalloc(real_bss_size);
        if (! bss) {
            RETURN(BS_NO_MEMORY);
        }
    }

    ctx.agent_func = MYBPF_CallAgent;
    ctx.base_helpers = BpfHelper_BaseHelper();
    ctx.sys_helpers = BpfHelper_SysHelper();
    ctx.user_helpers = BpfHelper_UserHelper();
    ctx.tmp_helpers = NULL;
    ctx.global_map_data = &bss;

    fn = _MYBPF_MakeExe(&ctx, data, len);
    ret = fn(p->p[0], p->p[1], p->p[2], p->p[3], p->p[4], &ctx);
    _MYBPF_UnmapExe(fn, len);

    return ret;
}

static inline int _mybpf_bare_run_byte_code(void *begin, void *end, void **tmp_helpers, MYBPF_PARAM_S *p)
{
    U64 ret = -1;
    MYBPF_DefultRunCode(begin, end, begin, &ret, tmp_helpers, p);
    return ret;
}

static int _mybpf_get_bare_size(void *mem, int mem_len)
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

    return size;
}


int MYBPF_BARE_Convert2File(char *src_filename, char *dst_filename, MYBPF_SIMPLE_CONVERT_PARAM_S *p)
{
    VBUF_S vbuf;

    VBUF_Init(&vbuf);

    int ret = _mybpf_bare_convert_file(src_filename, dst_filename, p, &vbuf);

    VBUF_Finit(&vbuf);

    return ret;
}

int MYBPF_RunBare(void *mem, int mem_len, void **tmp_helpers, MYBPF_PARAM_S *p)
{
    int ret;
    int size;
    void *begin, *end;
    MYBPF_BARE_HDR_S *hdr = mem;

    size = _mybpf_get_bare_size(mem, mem_len);
    if (size < 0) {
        return size;
    }

    begin = (char*)mem + sizeof(MYBPF_BARE_HDR_S);
    end = (char*)mem + size;

    if (! hdr->jit_arch) {
        ret = _mybpf_bare_run_byte_code(begin, end, tmp_helpers, p);
    } else if (hdr->jit_arch == MYBPF_JIT_LocalArch()) {
        ret = _mybpf_bare_run_aoted(begin, size - sizeof(MYBPF_BARE_HDR_S), tmp_helpers, p, ntohs(hdr->bss_size));
    } else {
        RETURNI(BS_NOT_SUPPORT, "Jit arch not matched");
    }

    return ret;
}

int MYBPF_RunBareFile(char *file, void **tmp_helpers, char *params)
{
    int ret;
    char *argv[32];
    int argc = 0;
    MYBPF_PARAM_S p = {0};

    if (params) {
        argc = ARGS_Split(params, argv, ARRAY_SIZE(argv));
    }

    FILE_MEM_S *m = FILE_Mem(file);
    if (! m) {
        RETURNI(BS_CAN_NOT_OPEN, "Can't open file");
    }

    p.p[0] = argc;
    p.p[1] = (long)argv;

    ret = MYBPF_RunBare(m->data, m->len, tmp_helpers, &p);

    FILE_MemFree(m);

    return ret;
}

