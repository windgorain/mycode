/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/mybpf_utl.h"
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
    int map_count;

    map_sec->map_count = 0;
    map_sec->map_def_size = 0;
    map_sec->maps = NULL;

    if (ELF_GetSecByName(elf, "maps", &sec) < 0) {
        return -1;
    }

    map_count = ELF_SecSymbolCount(elf, sec.sec_id, 0);
    if (map_count <= 0) {
        return 0;
    }

    map_sec->map_count = map_count;
    map_sec->map_def_size = sec.data->d_size / map_count; /* 计算map结构体的大小 */
    map_sec->maps = sec.data->d_buf;
    map_sec->sec_id = sec.sec_id;

    return 0;
}

int MYBPF_ELF_WalkProg(ELF_S *elf, PF_MYBPF_ELF_WALK_PROG walk_func, void *ud)
{
    void *iter = NULL;
    ELF_SECTION_S sec;
    int ret = 0;
    char *func_name;

    while ((iter = ELF_GetNextSection(elf, iter, &sec))) {
        if (! ELF_IsProgSection(&sec)) {
            continue;
        }

        func_name = ELF_GetSecSymbolName(elf, sec.sec_id, STT_FUNC, 0);
        if (! func_name) {
            continue;
        }

        ret = walk_func(sec.data->d_buf, sec.data->d_size, sec.shname, func_name, ud);
        if (ret < 0) {
            break;
        }
    }

    return ret;
}

int MYBPF_ELF_WalkMap(ELF_S *elf, PF_MYBPF_ELF_WALK_MAP walk_func, void *ud)
{
    char *map;
    char *map_name;
    int i;
    int ret = 0;
    MYBPF_MAPS_SEC_S map_sec;

    ret = MYBPF_ELF_GetMapsSection(elf, &map_sec);
    if (ret < 0) {
        return ret;
    }

    if (map_sec.map_count == 0) {
        return 0;
    }

    map = map_sec.maps;

    for (i=0; i<map_sec.map_count; i++) {
        map_name = ELF_GetSecSymbolName(elf, map_sec.sec_id, 0, i);
        ret = walk_func(map, map_sec.map_def_size, map_name, ud);
        if (ret < 0) {
            break;
        }
        map = map + map_sec.map_def_size;
    }

    return ret;
}

