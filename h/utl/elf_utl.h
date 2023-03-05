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

typedef struct {
    int fd;
    Elf *elf_info;
    GElf_Ehdr ehdr;
    void *symbols;
	int symbols_shndx; /* symbols section的id */
	int strtabidx;
}ELF_S;

typedef struct {
    int sec_id;
    char *shname;
    GElf_Shdr shdr;
	Elf_Data *data;
}ELF_SECTION_S;

typedef struct {
    UINT sec_count: 8; /* global data sec count */
    UINT have_data:1;
    UINT have_rodata:1;
    UINT have_bss:1;
    ELF_SECTION_S data_sec;
    ELF_SECTION_S rodata_sec;
    ELF_SECTION_S bss_sec;
}ELF_GLOBAL_DATA_S;

typedef struct {
    char *sec_name;
    char *func_name;
    UINT offset; /* 在 progs 中的字节为单位的offset */
    UINT size; /* prog 大小, 字节为单位 */
}ELF_PROG_INFO_S;

int ELF_Open(char *path, OUT ELF_S *elf);
void ELF_Close(ELF_S *elf);
/* 返回 iter, NULL表示结束 */
void * ELF_GetNextSection(ELF_S *elf, void *iter/* NULL表示获取第一个 */, OUT ELF_SECTION_S *sec);
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

#ifdef __cplusplus
}
#endif
#endif //ELF_UTL_H_
