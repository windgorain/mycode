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

#ifdef __cplusplus
}
#endif
#endif //ELF_UTL_H_
