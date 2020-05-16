/*==============================================================================
*   Created by LiXingang
*   Description: 带空洞的连续buf.每个字节对应一个bit,其中空洞所对应的bit为0.
*
==============================================================================*/
#include "bs.h"
#include "utl/mem_cap.h"
#include "utl/bitmap_utl.h"
#include "utl/hole_data.h"


typedef struct {
    HOLE_DATA_S hole_data;
    MEM_CAP_S *mem_cap;
    void *data;
    void *bitmap_data;
}HOLE_DATA_INSTANCE_S;

/* 初始化方法一 */
int HoleData_Init(HOLE_DATA_S *hole_data, void *data, int max_size, void *bits)
{
    hole_data->data = data;
    hole_data->max_size = max_size;
    hole_data->filled_size = 0;

    BITMAP_Init(&hole_data->bitmap, max_size, bits);

    return 0;
}

void HoleData_Final(HOLE_DATA_S *hole_data)
{
    hole_data->filled_size = 0;
    BITMAP_Fini(&hole_data->bitmap);
    return;
}

void HoleData_Reset(HOLE_DATA_S *hole_data)
{
    hole_data->filled_size = 0;
    BITMAP_ClrAll(&hole_data->bitmap);
}

/* 初始化方法二 */
HOLE_DATA_S * HoleData_Create(int max_size, MEM_CAP_S *mem_cap/* 允许NULL */)
{
    HOLE_DATA_INSTANCE_S *ins;

    ins = MemCap_ZAlloc(mem_cap, sizeof(HOLE_DATA_INSTANCE_S));
    if (! ins) {
        return NULL;
    }

    ins->mem_cap = mem_cap;

    ins->data = MemCap_Alloc(mem_cap, max_size);
    if (! ins->data) {
        HoleData_Destroy(&ins->hole_data);
        return NULL;
    }

    ins->bitmap_data = MemCap_Alloc(mem_cap, (max_size + 7)/8);
    if (! ins->bitmap_data) {
        HoleData_Destroy(&ins->hole_data);
        return NULL;
    }

    HoleData_Init(&ins->hole_data, ins->data, max_size,
            ins->bitmap_data);

    return &ins->hole_data;
}

void HoleData_Destroy(HOLE_DATA_S *hole_data)
{
    HOLE_DATA_INSTANCE_S *ins;

    ins = container_of(hole_data, HOLE_DATA_INSTANCE_S, hole_data);
    if (ins->data) {
        MemCap_Free(ins->mem_cap, ins->data);
    }

    if (ins->bitmap_data) {
        MemCap_Free(ins->mem_cap, ins->bitmap_data);
    }

    MemCap_Free(ins->mem_cap, ins);
}

/* 反回处理了多长的数据 */
int HoleData_Input(HOLE_DATA_S *hole_data, void *data, int offset, int len)
{
    int copy_len = len;

    if (offset + len > hole_data->max_size) {
        copy_len = hole_data->max_size - offset;
    }

    if (copy_len <= 0) {
        return 0;
    }

    memcpy(hole_data->data + offset, data, copy_len);
    hole_data->filled_size += copy_len;

    BITMAP_SetBits(&hole_data->bitmap, offset, copy_len);

    return copy_len;
}

BOOL_T HoleData_IsFinished(HOLE_DATA_S *hole_data)
{
    return BITMAP_IsAllSetted(&hole_data->bitmap);
}

int HoleData_GetFilledSize(HOLE_DATA_S *hole_data)
{
    return hole_data->filled_size;
}

/* 获取从起始位置连续的内存长度 */
int HoleData_GetContineLen(HOLE_DATA_S *hole_data)
{
    UINT index;

    index = BITMAP_GetAUnsettedBitIndex(&hole_data->bitmap);
    if (index == BITMAP_INVALID_INDEX) {
        return hole_data->max_size;
    }

    return index;
}

