/*================================================================
*   Created by LiXingang
*   Description: 动态增长的数组
*
================================================================*/
#include "bs.h"
#include "utl/vector_utl.h"
#include "utl/mem_cap.h"
#include "utl/qsort_utl.h"
#include "utl/bsearch_utl.h"

#define VECTOR_DEFAULT_RESIZE_FACTOR (50)

static int _vector_make_room(VECTOR_S *vec, UINT size)
{
	UINT new_size;
	char *new_array;
	UINT sz;

	if (vec->size >= size) {
		return 0;
	}

	new_size = size * (100 + vec->resize_factor) / 100;

	sz = new_size * vec->ele_size;

	new_array = MemCap_Malloc(vec->memcap, sz);
	if (!new_array) {
        RETURN(BS_NO_MEMORY);
	}

	if (vec->array) {
		UINT old_size = vec->count * vec->ele_size;
		memcpy(new_array, vec->array, old_size);
        MemCap_Free(vec->memcap, vec->array);
	}

	vec->array = new_array;
	vec->size = new_size;

	return 0;
}

static inline void * _vector_index_2_ptr(VECTOR_S *vec, int index)
{
    return vec->array + (index * vec->ele_size);
}

static inline int _vector_ptr_2_index(VECTOR_S *vec, char *ptr)
{
    return (ptr - vec->array) / vec->ele_size;
}

static int _vector_find_slow(VECTOR_S *vec, void *value, PF_CMP_FUNC cmp_func)
{
	char *p;
	int pos;

	for (pos = 0; pos < vec->count; pos++) {
		p = _vector_index_2_ptr(vec, pos);
        if (cmp_func(p, value) == 0) {
			return pos;
		}
	}

	return -1;
}

static int _vector_find_fast(VECTOR_S *vec, void *value, PF_CMP_FUNC cmp_func)
{
    char *node = BSEARCH_Do(vec->array, vec->count, vec->ele_size, value, cmp_func);
    if (! node) {
        return -1;
    }

    return _vector_ptr_2_index(vec, node);
}

int VECTOR_Init(VECTOR_S *vec, VECTOR_PARAM_S *p)
{
    memset(vec, 0, sizeof(VECTOR_S));

    vec->ele_size = p->ele_size;
    vec->resize_factor = p->resize_factor ? p->resize_factor : VECTOR_DEFAULT_RESIZE_FACTOR;

	if (_vector_make_room(vec, p->init_size) < 0) {
		VECTOR_Finit(vec);
        RETURN(BS_NO_MEMORY);
	}

	return 0;
}

void VECTOR_Finit(VECTOR_S *vec)
{
	if (! vec) {
		return;
	}

	if (vec->array) {
        MemCap_Free(vec->memcap, vec->array);
	}

    memset(vec, 0, sizeof(VECTOR_S));

	return;
}

VECTOR_S * VECTOR_Create(VECTOR_PARAM_S *p)
{
    VECTOR_S *vec = MemCap_ZMalloc(p->memcap, sizeof(VECTOR_S));
    if (! vec) {
        return NULL;
    }

    if (VECTOR_Init(vec, p) < 0) {
        MemCap_Free(p->memcap, vec);
        return NULL;
    }

    return vec;
}

void VECTOR_Destroy(VECTOR_S *vec)
{
	if (vec->array) {
        MemCap_Free(vec->memcap, vec->array);
	}
    MemCap_Free(vec->memcap, vec);
}


int VECTOR_AddBySize(VECTOR_S *vec, void *val, int val_size)
{
    if (vec->ele_size < val_size) {
        BS_DBGASSERT(0);
        RETURN(BS_OUT_OF_RANGE);
    }

    int ret = _vector_make_room(vec, vec->count + 1);
    if (ret < 0) {
        return ret;
	}

    char *node = vec->array + (vec->ele_size* vec->count);

	MEM_Copy(node, val, val_size);

    
    if (vec->ele_size > val_size) {
        memset(node + val_size, 0, vec->ele_size - val_size);
    }

	vec->count++;
	vec->sorted = 0;

	return 0;
}

int VECTOR_Add(VECTOR_S *vec, void *val)
{
    return VECTOR_AddBySize(vec, val, vec->ele_size);
}

int VECTOR_AddN(VECTOR_S *vec, void *val, UINT num)
{
	if (_vector_make_room(vec, vec->count + num) < 0) {
        RETURN(BS_ERR);
	}

	memcpy(vec->array + (vec->count * vec->ele_size), val, vec->ele_size * num);
	vec->count += num;

	vec->sorted = 0;

	return 0;
}

int VECTOR_Append(VECTOR_S *vec_dst, VECTOR_S *vec_src)
{
    if (!vec_dst || !vec_src) {
        RETURN(BS_BAD_PARA);
    }

    return VECTOR_AddN(vec_dst, vec_src->array, vec_src->count);
}

void VECTOR_Sort(VECTOR_S *vec, PF_CMP_FUNC cmp_func)
{
    QSORT_Do(vec->array, vec->count, vec->ele_size, cmp_func);
	vec->sorted = 1;
	return;
}

void VECTOR_Remove(VECTOR_S *vec, UINT pos)
{
	char *p;
	char *q;

	if (pos >= vec->count) {
		return;
	}

	if (pos == vec->count - 1) {
		vec->count--;
		return;
	}

	p = (char *)vec->array + pos * vec->ele_size;
	q = p + vec->ele_size;
	memmove(p, q,  vec->ele_size * (vec->count - pos - 1));
	vec->count --;

	return;
}

void VECTOR_Delete(VECTOR_S *vec, void *value, PF_CMP_FUNC cmp_func)
{
    int pos;

    pos = VECTOR_Find(vec, value, cmp_func);
    if (pos < 0) {
        return;
    }

    VECTOR_Remove(vec, pos);
}

int VECTOR_Find(VECTOR_S *vec, void *value, PF_CMP_FUNC cmp_func)
{
    if (vec->sorted) {
        return _vector_find_fast(vec, value, cmp_func);
    }

    return _vector_find_slow(vec, value, cmp_func);
}

void * VECTOR_Get(VECTOR_S *vec, int idx)
{
	if (idx < 0 || idx >= vec->count) {
        return NULL;
	}

	return vec->array + (vec->ele_size * idx);
}

int VECTOR_Copy(VECTOR_S *vec, int idx, OUT void *val)
{
	if (idx < 0 || idx >= vec->count) {
        RETURN(BS_ERR);
	}

	memcpy(val, vec->array + (vec->ele_size * idx), vec->ele_size);

	return 0;
}

