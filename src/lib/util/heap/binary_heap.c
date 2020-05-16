/*================================================================
*   Created by LiXingang: 2018.12.07
*   Description: 最小二叉堆
*
================================================================*/
#include "bs.h"
#include "utl/darray_utl.h"
#include "utl/binary_heap.h"

int BinaryHeap_Init(IN BINARY_HEAP_S *heap, IN PF_BINARY_HEAP_CMP pfCmp)
{
    heap->hDarray = DARRAY_Create(1024);
    if (heap->hDarray == NULL) {
        RETURN(BS_NO_MEMORY);
    }

    heap->pfCmp = pfCmp;

    return 0;
}

void BinaryHeap_SetCmpFunc(IN BINARY_HEAP_S *heap, IN PF_BINARY_HEAP_CMP pfCmp)
{
    heap->pfCmp = pfCmp;
}

UINT binaryheap_GetIndex(IN BINARY_HEAP_S *heap, IN void *data, IN PF_BINARY_HEAP_CMP pfCmp)
{
    DARRAY_HANDLE hDarray = heap->hDarray;
    UINT count = DARRAY_GetCount(hDarray);
    UINT index;

    for (index=0; index<count; index++) {
        if (pfCmp(data, DARRAY_Get(hDarray, index)) == 0) {
            return index;
        }
    }

    return DARRAY_INVALID_INDEX;
}

static void binaryheap_Up(IN BINARY_HEAP_S *heap, IN UINT index, IN PF_BINARY_HEAP_CMP pfCmp)
{
    DARRAY_HANDLE hDarray = heap->hDarray;
    void *current, *parrent;
    UINT c;          // 当前节点(current)的位置
    UINT p;          // 父(parent)结点的位置 

    c = index;
    current = DARRAY_Get(hDarray, c);

    while (c > 0) {
        p = (c-1)/2;
        parrent = DARRAY_Get(hDarray, p);

        if (pfCmp(current, parrent) >= 0) {
            break;
        }

        DARRAY_Set(hDarray, c, parrent);
        c = p;
    }

    DARRAY_Set(hDarray, c, current);
}

/* 选取最小的孩子 */
static UINT binaryheap_GetMinChild(IN BINARY_HEAP_S *heap, IN UINT index, IN PF_BINARY_HEAP_CMP pfCmp)
{
    DARRAY_HANDLE hDarray = heap->hDarray;
    UINT count = DARRAY_GetCount(hDarray);
    UINT l = 2*index + 1;
    UINT r = l + 1;
    void *left, *right;

    if (l >= count) {
        return DARRAY_INVALID_INDEX;
    }

    if (r >= count) {
        return l;
    }

    left = DARRAY_Get(hDarray, l);
    right = DARRAY_Get(hDarray, r);

    if (pfCmp(left, right) < 0) {
        return l;
    }

    return r;
}

static void binaryheap_Down(IN BINARY_HEAP_S *heap, IN UINT index, IN PF_BINARY_HEAP_CMP pfCmp)
{
    DARRAY_HANDLE hDarray = heap->hDarray;
    void *current, *next;
    UINT c, n;
    UINT end = DARRAY_GetCount(hDarray);

    c = index;
    current = DARRAY_Get(hDarray, c);

    while (c < end) {
        n = binaryheap_GetMinChild(heap, c, pfCmp);
        if (n == DARRAY_INVALID_INDEX) {
            break;
        }

        next = DARRAY_Get(hDarray, n);

        if (pfCmp(current, next) <= 0) {
            break;
        }

        DARRAY_Set(hDarray, c, next);
        c = n;
    }

    DARRAY_Set(hDarray, c, current);
}

int BinaryHeap_Insert(IN BINARY_HEAP_S *heap, IN void *new_data)
{
    PF_BINARY_HEAP_CMP pfCmp = heap->pfCmp;
    UINT index;

    index = DARRAY_Add(heap->hDarray, new_data);
    if (index == DARRAY_INVALID_INDEX) {
        RETURN(BS_ERR);
    }

    binaryheap_Up(heap, index, pfCmp);

    return 0;
}

/* 删除指定index位置的元素, 并返回这个元素 */
void * BinaryHeap_RemoveIndex(IN BINARY_HEAP_S *heap, IN UINT index)
{
    DARRAY_HANDLE hDarray = heap->hDarray;
    PF_BINARY_HEAP_CMP pfCmp = heap->pfCmp;
    void *deleted;
    void *last;
    UINT count = DARRAY_GetCount(hDarray);

    if (index >= count) {
        return NULL;
    }

    last = DARRAY_Del(hDarray, DARRAY_GetCount(heap->hDarray) - 1);
    if (index == count - 1) {
        return last;
    }

    deleted = DARRAY_Get(hDarray, index);
    DARRAY_Set(hDarray, index, last);

    binaryheap_Up(heap, index, pfCmp);
    binaryheap_Down(heap, index, pfCmp);

    return deleted;
}

int BinaryHeap_RemoveData(IN BINARY_HEAP_S *heap, IN void *data)
{
    UINT index = binaryheap_GetIndex(heap, data, heap->pfCmp);

    if (index == DARRAY_INVALID_INDEX) {
        RETURN(BS_NOT_FOUND);
    }

    BinaryHeap_RemoveIndex(heap, index);

    return 0;
}


