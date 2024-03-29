#include "bs.h"
#include "utl/vector_utl.h"
#include "utl/vector_int.h"

static int _dynamic_array_int_compare(const void *a, const void *b)
{
    int num1 = *(int *)a;
    int num2 = *(int *)b;

    return num1 - num2;
}

BOOL_T VectorInt_IsExist(VECTOR_S *vec, int val)
{
    int index = VECTOR_Find(vec, &val, _dynamic_array_int_compare);
    if (index < 0) {
        return FALSE;
    }
    return TRUE;
}

void VectorInt_Sort(VECTOR_S *vec)
{
    VECTOR_Sort(vec, _dynamic_array_int_compare);
}

int VectorInt_DelByVal(VECTOR_S *vec, int val)
{
    int index = VECTOR_Find(vec, &val, _dynamic_array_int_compare);
	if (index < 0) {
        return index;
	}

    VECTOR_Remove(vec, index);

	return 0;
}

