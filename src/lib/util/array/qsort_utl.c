/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/qsort_utl.h"

static int qsort_dft_cmp(void *d1, void *d2, void *ud)
{
    int ele_size = HANDLE_UINT(ud);
    return memcmp(d1, d2, ele_size);
}

static void qsort_do(void *base, int count, int ele_size, PF_CMP_FUNC cmp_func, void *ud)
{
    char *ptr = base;
    int i = 0;
    int j = count - 1;

    while (i != j) {
        /* 从右向左寻找第一个小于基准元素的值 */
        while ((i < j) && (cmp_func(ptr + (j*ele_size), base, ud) >= 0)) {
            j --;
        }
        /* 从左向右寻找第一个大于基准元素的值 */
        while ((i < j) && (cmp_func(ptr + (i*ele_size), base, ud) <= 0)) {
            i ++;
        }

        /* 交换i/j */
        if (i < j) {
            MEM_Exchange(ptr+(i*ele_size), ptr+(j*ele_size), ele_size);
        }
    }

    if (i == 0) {
        qsort_do(ptr + ele_size, count - 1, ele_size, cmp_func, ud);
        return;
    }

    /* 将i和j共同指向的元素与基准元素互换 */
    MEM_Exchange(ptr+(i*ele_size), ptr, ele_size);

    /* 左边 */
    if (i > 2) {
        qsort_do(base, i-1, ele_size, cmp_func, ud);
    }

    /* 右边 */
    if (count - i > 2) {
        void *right_base = ptr + ((i+1) * ele_size);
        int right_count = count - i - 1;
        qsort_do(right_base, right_count, ele_size, cmp_func, ud);
    }
}

void QSORT_Do(void *base, int count, int ele_size, PF_CMP_FUNC cmp_func /* 为NULL则使用默认比较函数 */, void *ud)
{
    if (count <= 1) {
        return;
    }

    if (! cmp_func) {
        cmp_func = qsort_dft_cmp;
        ud = UINT_HANDLE(ele_size);
    }

    qsort_do(base, count, ele_size, cmp_func, ud);
}



