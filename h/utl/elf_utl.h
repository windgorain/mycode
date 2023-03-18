/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _ELF_UTL_H
#define _ELF_UTL_H

#include <libelf.h>
#include <gelf.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define ELF_DBG_PROCESS 0x1

typedef struct {
    int fd;
    Elf *elf_info;
    GElf_Ehdr ehdr;
    void *symbols;
	int symbols_shndx;
	int strtabidx;
}ELF_S;

typedef struct {
    int sec_id;
    char *shname;
    GElf_Shdr shdr;
	Elf_Data *data;
}ELF_SECTION_S;

#define ELF_MAX_GLOBAL_DATA_SEC_NUM 32

typedef struct {
    UINT sec_count: 8;
    UINT rodata_count: 8;
    UINT have_bss:1;
    UINT have_data:1;
    ELF_SECTION_S bss_sec;
    ELF_SECTION_S data_sec;
    ELF_SECTION_S rodata_sec[ELF_MAX_GLOBAL_DATA_SEC_NUM];
}ELF_GLOBAL_DATA_S;

typedef struct {
    char *sec_name;
    char *func_name;
    UINT sec_offset;
    UINT prog_offset;
    UINT size;
    int  sec_id;
}ELF_PROG_INFO_S;

int ELF_Open(char *path, OUT ELF_S *elf);
void ELF_Close(ELF_S *elf);
void * ELF_GetNextSection(ELF_S *elf, void *iter, OUT ELF_SECTION_S *sec);
int ELF_SecCount(ELF_S *elf);
int ELF_GetSecIDByName(ELF_S *elf, char *sec_name);
int ELF_GetSecByID(ELF_S *elf, int id, OUT ELF_SECTION_S *sec);
int ELF_GetSecByName(ELF_S *elf, char *sec_name, OUT ELF_SECTION_S *sec);
int ELF_SecSymbolCount(ELF_S *elf, int sec_id, int type);
Elf64_Sym * ELF_GetSymbol(ELF_S *elf, int id);
Elf64_Sym * ELF_GetSecSymbol(ELF_S *elf, int sec_id, int type, int index);
char * ELF_GetSecSymbolName(ELF_S *elf, int sec_id, int type, int index);
char * ELF_GetSymbolName(ELF_S *elf, Elf64_Sym * sym);
GElf_Rel * ELF_GetRel(Elf_Data *relo_data, int id, OUT GElf_Rel *rel);

BOOL_T ELF_IsProgSection(ELF_SECTION_S *sec);
BOOL_T ELF_IsDataSection(ELF_SECTION_S *sec);

int ELF_GetGlobalData(ELF_S *elf, OUT ELF_GLOBAL_DATA_S *global_data);

int ELF_GetProgsSize(ELF_S *elf);
int ELF_GetProgsCount(ELF_S *elf);
int ELF_CopyProgs(ELF_S *elf, OUT void *mem, int mem_size);
void * ELF_DupProgs(ELF_S *elf);
int ELF_GetProgsInfo(ELF_S *elf, OUT ELF_PROG_INFO_S *progs, int max_prog_count);
void ELF_ClearProgsInfo(ELF_PROG_INFO_S *progs, int prog_count);
typedef int (*PF_ELF_WALK_PROG)(void *data, int offset, int len, char *sec_name, char *func_name, void *ud);
int ELF_WalkProgs(ELF_S *elf, PF_ELF_WALK_PROG walk_func, void *ud);

#ifdef __cplusplus
}
#endif
#endif //ELF_UTL_H_
