/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/mybpf_elf.h"

ELF_SECTION_S * MYBPF_ELF_GetProg(ELF_S *elf, char *func_name, OUT ELF_SECTION_S *sec)
{
    void *iter = NULL;
    char *name;

    while ((iter = ELF_GetNextSection(elf, iter, sec))) {
        if (! ELF_IsProgSection(sec)) {
            continue;
        }

        name = ELF_GetSecSymbolName(elf, sec->sec_id, STT_FUNC, 0);
        if (! name) {
            continue;
        }

        if (strcmp(name, func_name) == 0) {
            return sec;
        }
    }

    return NULL;
}

int MYBPF_ELF_GetMapsSection(ELF_S *elf, OUT MYBPF_MAPS_SEC_S *map_sec)
{
    ELF_SECTION_S sec;

    map_sec->map_count = 0;
    map_sec->map_def_size = 0;
    map_sec->maps = NULL;

    if (ELF_GetSecByName(elf, "maps", &sec) < 0) {
        return 0;
    }

    map_sec->map_count = ELF_SecSymbolCount(elf, sec.sec_id, 0);
    if (map_sec->map_count <= 0) {
        return 0;
    }

    /* 计算map结构体的大小 */
    map_sec->map_def_size = sec.data->d_size / map_sec->map_count;
    map_sec->maps = sec.data->d_buf;
    map_sec->sec_id = sec.sec_id;

    return 0;
}


