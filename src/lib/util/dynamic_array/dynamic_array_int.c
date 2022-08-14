#include "bs.h"
#include "utl/vector_utl.h"
#include "utl/vector_int.h"

static int _dynamic_array_int_compare(void *a, void *b, void *ud)
{
    int num1 = *(int32_t *)a;
    int num2 = *(int32_t *)b;

    return num1 - num2;
}

BOOL_T VectorInt_IsExist(VECTOR_INT_HDL vec, int val)
{
    int index = VECTOR_Find(vec, &val, _dynamic_array_int_compare, NULL);
    if (index < 0) {
        return FALSE;
    }
    return TRUE;
}

void VectorInt_Sort(VECTOR_INT_HDL vec)
{
    VECTOR_Sort(vec, _dynamic_array_int_compare, NULL);
}

int VectorInt_DelByVal(VECTOR_INT_HDL vec, int val)
{
    int index = VECTOR_Find(vec, &val, _dynamic_array_int_compare, NULL);
	if (index < 0) {
        return index;
	}

    VECTOR_Remove(vec, index);

	return 0;
}

