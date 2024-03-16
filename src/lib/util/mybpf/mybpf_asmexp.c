/*********************************************************
*   Copyright (C) LiXingang
*   Description: bpf exp
*
********************************************************/
#include "bs.h"
#include "utl/file_utl.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_asm.h"
#include "utl/mybpf_runtime.h"

#define MYBPF_EXP_INSTANCE_NAME "convert_exp"

static int _mybpf_exp_write_headers(FILE *fp)
{
    fprintf(fp, "#include \"bs.h\" \n"
            "#include \"utl/mybpf_regs.h\" \n"
            "#include \"utl/mybpf_asmdef.h\" \n"
            "#include \"utl/bpfasm_utl.h\" \n"
            "\n");

    return 0;
}

static ELF_PROG_INFO_S * _mybpf_exp_is_prog_begin(MYBPF_LOADER_NODE_S *n, int insn_idx)
{
    for (int i=0; i<n->progs_count; i++) {
        if (n->progs[i].prog_offset == insn_idx * sizeof(MYBPF_INSN_S)) {
            return &n->progs[i];
        }
    }

    return NULL;
}

static int _mybpf_exp_write_binary_exps(MYBPF_LOADER_NODE_S *n, FILE *fp)
{
    UCHAR *c = n->insts;
    int insn_idx = 0;
    int insn_count = n->insts_len / sizeof(MYBPF_INSN_S);

    fprintf(fp, "static UCHAR g_bpfasm_insts[] = { \n");

    for (insn_idx = 0; insn_idx < insn_count; insn_idx++) {
        fprintf(fp, "0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x, \n",
                c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);
        c += 8;
    }

    fprintf(fp, "}; \n\n");

    return 0;
}

static int _mybpf_exp_write_exps(MYBPF_LOADER_NODE_S *n, FILE *fp)
{
    char exp_string[128];
    MYBPF_INSN_S *insn = n->insts;
    int insn_idx = 0;
    int insn_count = n->insts_len / sizeof(MYBPF_INSN_S);
    int insn_consume;

    fprintf(fp, "static MYBPF_INSN_S g_bpfasm_insts[] = { \n");

    while (insn_idx < insn_count) {
        ELF_PROG_INFO_S *prog = _mybpf_exp_is_prog_begin(n, insn_idx);
        if (prog) {
            fprintf(fp, "\n");
        }

        insn_consume = MYBPF_ASM_Insn2Exp(insn, insn_count, insn_idx, exp_string, sizeof(exp_string));
        fprintf(fp, "    %s, \n", exp_string);
        insn_idx += insn_consume;
    }

    fprintf(fp, "}; \n\n");

    return 0;
}

static int _mybpf_exp_write_cfuncs(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *n, FILE *fp)
{
    MYBPF_PROG_NODE_S *prog = NULL;
    char *begin = n->insts;
    char *prog_begin;
    int offset;

    while ((prog = MYBPF_PROG_GetNext(runtime, MYBPF_EXP_INSTANCE_NAME, NULL, prog))) {
        char *name = prog->prog_name;
        if (name[0] == '\0') {
            name = prog->sec_name;
        }

        char *funcname = FILE_GetFileNameFromPath(name);

        prog_begin = (void*)prog->insn;
        offset = (prog_begin - begin);

        fprintf(fp, "U64 %s(U64 p1, U64 p2, U64 p3, U64 p4, U64 p5) \n", funcname);
        fprintf(fp, "{ \n"
                "    MYBPF_PARAM_S p; \n"
                "    MYBPF_CTX_S ctx = {0}; \n\n"
                "    ctx.begin_addr = g_bpfasm_insts; \n"
                "    ctx.end_addr = (char*)(void*)g_bpfasm_insts + sizeof(g_bpfasm_insts); \n");

        if (offset) {
            fprintf(fp, "    ctx.insts = (char*)g_bpfasm_insts + %d; \n", offset);
        } else {
            fprintf(fp, "    ctx.insts = (char*)g_bpfasm_insts; \n");
        }

        fprintf(fp,
                "    p.p[0]=p1; p.p[1]=p2; p.p[2]=p3; p.p[3]=p4; p.p[4]=p5; \n"
                "    int ret = MYBPF_DefultRun(&ctx, &p); \n"
                "    if (ret < 0) return ret; \n"
                "    return ctx.bpf_ret; \n"
                "} \n\n");
     }

     return 0;
}

static int _mybpf_exp_convert(MYBPF_RUNTIME_S *runtime, char *bpf_file, int binary, FILE *fp)
{
    int ret;
    MYBPF_LOADER_PARAM_S p = {0};

    p.instance = MYBPF_EXP_INSTANCE_NAME;
    p.filename = bpf_file;

    if ((ret = MYBPF_LoaderLoad(runtime, &p)) < 0) {
        return ret;
    }

    MYBPF_LOADER_NODE_S *n = MYBPF_LoaderGet(runtime, p.instance);
    if (! n) {
        RETURN(BS_ERR);
    }

    
    if (n->map_count > 0) {
        RETURNI(BS_NOT_SUPPORT, "This file have maps");
    }

    _mybpf_exp_write_headers(fp);
    if (binary) {
        _mybpf_exp_write_binary_exps(n, fp);
    } else {
        _mybpf_exp_write_exps(n, fp);
    }
    _mybpf_exp_write_cfuncs(runtime, n, fp);

    return ret;
}

static int _mybpf_exp_convert_file(char *bpf_file, char *exp_file, int binary)
{
    MYBPF_RUNTIME_S runtime;
    int ret;
    FILE *fp;

    if (! FILE_IsFileExist(bpf_file)) {
        RETURN(BS_NOT_FOUND);
    }

    if ((ret = MYBPF_RuntimeInit(&runtime)) < 0) {
        return ret;
    }

    fp = FILE_Open(exp_file, TRUE, "wb+");
    if (! fp) {
        MYBPF_RuntimeFini(&runtime);
        RETURN(BS_CAN_NOT_OPEN);
    }

    ret = _mybpf_exp_convert(&runtime, bpf_file, binary, fp);

    fclose(fp);
    MYBPF_RuntimeFini(&runtime);

    if (ret < 0) {
        FILE_DelFile(exp_file);
    }

    return ret;
}

int MYBPF_ASMEXP_ConvertFile(char *bpf_file, char *exp_file, int binary)
{
    return _mybpf_exp_convert_file(bpf_file, exp_file, binary);
}

