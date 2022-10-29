/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/array_bit.h"

/* 获取一个空位 */
INT64 ArrayBit_GetFree(UINT *data, INT64 bit_size)
{
    INT64 index;

    ARRAYBIT_SCAN_FREE_BEGIN(data, bit_size, index) {
        return index;
    }ARRAYBIT_SCAN_END();

    return -1;
}

/* 从from开始,获取一个free位 */
INT64 ArrayBit_GetFreeFrom(UINT *data, INT64 bit_size, INT64 from)
{
    INT64 i, j;
    INT64 uint_num = NUM_UP_ALIGN(bit_size, 32)/32;
    INT64 uiIndexFound;
    INT64 uiStartUint;
    INT64 uiStartBit;
    INT64 index = from;

    if (index >= bit_size) {
        return -1;
    }

    uiStartUint = index / 32;
    uiStartBit = index % 32;

    for (i=uiStartUint; i<uint_num; i++) {
        if (data[i] == 0xffffffff) {
            uiStartBit = 0;
            continue;
        }
        for (j=uiStartBit; j<32; j++) {
            if (data[i] & (1 << j))
                continue;
            uiIndexFound = i*32 + j;
            if (uiIndexFound >= bit_size) {
                uiIndexFound = -1;
            }
            return uiIndexFound;
        }
    }

    return -1;
}

INT64 ArrayBit_GetFreeAfter(UINT *data, INT64 bit_size, INT64 curr)
{
    return ArrayBit_GetFreeFrom(data, bit_size, curr+1);
}

/* 获取一个setted位 */
INT64 ArrayBit_GetBusy(UINT *data, INT64 bit_size)
{
    INT64 index;

    ARRAYBIT_SCAN_BUSY_BEGIN(data, bit_size, index) {
        return index;
    }ARRAYBIT_SCAN_END();

    return -1;
}

/* 从from开始,获取一个busy位 */
INT64 ArrayBit_GetBusyFrom(UINT *data, INT64 bit_size, INT64 from)
{
    INT64 i, j;
    INT64 uint_num = NUM_UP_ALIGN(bit_size, 32)/32;
    INT64 uiIndexFound;
    INT64 uiStartUint;
    INT64 uiStartBit;
    INT64 index = from;

    if (index >= bit_size) {
        return -1;
    }

    uiStartUint = index / 32;
    uiStartBit = index % 32;

    for (i=uiStartUint; i<uint_num; i++) {
        if (data[i] == 0) {
            uiStartBit = 0;
            continue;
        }

        for (j=uiStartBit; j<32; j++) {
            if ((data[i] & (1 << j)) == 0)
                continue;
            uiIndexFound = i*32 + j;
            if (uiIndexFound >= bit_size) {
                uiIndexFound = -1;
            }
            return uiIndexFound;
        }
    }

    return -1;
}

INT64 ArrayBit_GetBusyAfter(UINT *data, INT64 bit_size, INT64 curr)
{
    return ArrayBit_GetBusyFrom(data, bit_size, curr+1);
}

void ArrayBit_WalkFree(UINT *data, INT64 bit_size, PF_ARRAY_BIT_WALK walk_func, void *ud)
{
    INT64 index;

    ARRAYBIT_SCAN_FREE_BEGIN(data, bit_size, index) {
        if (BS_WALK_STOP == walk_func(index, ud)) {
            return;
        }
    }ARRAYBIT_SCAN_END();
}

void ArrayBit_WalkBusy(UINT *data, INT64 bit_size, PF_ARRAY_BIT_WALK walk_func, void *ud)
{
    INT64 index;

    ARRAYBIT_SCAN_BUSY_BEGIN(data, bit_size, index) {
        if (BS_WALK_STOP == walk_func(index, ud)) {
            return;
        }
    }ARRAYBIT_SCAN_END();
}

UINT64 ArrayBit_GetFreeCount(UINT *data, INT64 bit_size)
{
    INT64 index;
    UINT64 count = 0;

    ARRAYBIT_SCAN_FREE_BEGIN(data, bit_size, index) {
        count ++;
    }ARRAYBIT_SCAN_END();

    return count;
}

UINT64 ArrayBit_GetBusyCount(UINT *data, INT64 bit_size)
{
    INT64 index;
    UINT64 count = 0;

    ARRAYBIT_SCAN_BUSY_BEGIN(data, bit_size, index) {
        count ++;
    }ARRAYBIT_SCAN_END();

    return count;
}

/* 做与操作, data3 = data1 & data2 */
void ArrayBit_And(UINT *data1, UINT *data2, int uint_count, OUT UINT *data3)
{
    int i;

    for (i=0; i<uint_count; i++) {
        data3[i] = data1[i] & data2[i];
    }
}

/* 做或操作, data3 = data1 | data2 */
void ArrayBit_Or(UINT *data1, UINT *data2, int uint_count, OUT UINT *data3)
{
    int i;

    for (i=0; i<uint_count; i++) {
        data3[i] = data1[i] | data2[i];
    }
}

/* 做异或操作, data3 = data1 ^ data2 */
void ArrayBit_Xor(UINT *data1, UINT *data2, int uint_count, OUT UINT *data3)
{
    int i;

    for (i=0; i<uint_count; i++) {
        data3[i] = data1[i] ^ data2[i];
    }
}
