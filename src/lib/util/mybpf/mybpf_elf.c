/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/mybpf_utl.h"
#include "utl/mybpf_elf.h"
#include "utl/mybpf_relo.h"
#include "utl/mybpf_dbg.h"
#include "utl/ulc_user.h"

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

static void _mybpf_def_get_global_maps(ELF_S *elf, OUT ELF_GLOBAL_DATA_S *data)
{
    ELF_GetGlobalData(elf, data);

    if (data->have_bss) {
        if (! MYBPF_RELO_IsMapUsed(elf, data->bss_sec.sec_id, 0, 1)) {
            data->have_bss = 0;
            data->sec_count --;
        }
    }

    if (data->have_data) {
        if (! MYBPF_RELO_IsMapUsed(elf, data->data_sec.sec_id, 0, 1)) {
            data->have_data = 0;
            data->sec_count --;
        }
    }

    for (int i=0; i<data->rodata_count; i++) {
        if (! MYBPF_RELO_IsMapUsed(elf, data->rodata_sec[i].sec_id, 0, 1)) {
            int left_count = data->rodata_count - (i + 1);
            if (left_count > 0) {
                memcpy(&data->rodata_sec[i], &data->rodata_sec[i + 1], sizeof(ELF_SECTION_S) * left_count);
            }

            data->sec_count --;
            data->rodata_count --;
            i --;
        }
    }
}


void MYBPF_ELF_GetGlobalDataUsed(ELF_S *elf, OUT ELF_GLOBAL_DATA_S *global_data)
{
    _mybpf_def_get_global_maps(elf, global_data);
}

int MYBPF_ELF_GetMapsSection(ELF_S *elf, OUT MYBPF_MAPS_SEC_S *map_sec)
{
    ELF_SECTION_S sec;
    int map_count;

    map_sec->map_count = 0;
    map_sec->map_def_size = 0;
    map_sec->maps = NULL;

    if (ELF_GetSecByName(elf, "maps", &sec) < 0) {
        if (ELF_GetSecByName(elf, ".maps", &sec) < 0) {
            return -1;
        }
    }

    map_count = ELF_SecSymbolCount(elf, sec.sec_id, 0);
    if (map_count <= 0) {
        return 0;
    }

    map_sec->map_count = map_count;
    map_sec->map_def_size = sec.data->d_size / map_count; 
    map_sec->maps = sec.data->d_buf;
    map_sec->sec_id = sec.sec_id;

    return 0;
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
        ret = walk_func(i, (void*)map, map_sec.map_def_size, map_name, ud);
        if (ret < 0) {
            break;
        }
        map = map + map_sec.map_def_size;
    }

    return ret;
}

int MYBPF_ELF_WalkMapByFile(char *file, PF_MYBPF_ELF_WALK_MAP walk_func, void *ud)
{
    ELF_S elf = {0};

    int ret = ELF_Open(file, &elf);
    if (ret < 0) {
        RETURNI(BS_CAN_NOT_OPEN, "Can't open file %s \r\n", file);
    }

    ret = MYBPF_ELF_WalkMap(&elf, walk_func, ud);

    ELF_Close(&elf);

    return ret;
}

int MYBPF_ELF_WalkProgByFile(char *file, PF_ELF_WALK_PROG walk_func, void *ud)
{
    ELF_S elf = {0};

    int ret = ELF_Open(file, &elf);
    if (ret < 0) {
        RETURNI(BS_CAN_NOT_OPEN, "Can't open file %s \r\n", file);
    }

    ret = ELF_WalkProgs(&elf, walk_func, ud);

    ELF_Close(&elf);

    return ret;
}

