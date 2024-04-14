/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/mem_utl.h"
#include "utl/qsort_utl.h"

#define CUTOFF 8

/* 使用的是非递归方式, 有一个自定义的栈式结构 */
/* 栈大小的需求不会大于1+log2(num)，STKSIZ足够使用 */
#define STKSIZ (8*sizeof(void*) - 2) /* 栈的大小 */

/* 当快速排序的循环中遇到大小小于CUTOFF的数组时，就不再使用快速排序了,转为使用简单排序 */
static void _qsort_shortsort(char *lo, char *hi, size_t width, PF_CMP_FUNC cmp_func)
{
    char *p, *max;

    while (hi > lo) {
        max = lo;

        /* lo到hi的元素中，选出最大的一个，max指针指向这个最大项 */
        for (p = lo+width; p <= hi; p += width) {
            if (cmp_func(p, max) > 0) {
                max = p;
            }
        }

        /* 把最大项和hi指向的项向交换 */
        MEM_Swap(max, hi, width);

        hi -= width;
    }
}

void QSORT_Do(void *base, int num, int width, PF_CMP_FUNC cmp_func)
{
    char *lo, *hi;              /* 数组的两端项指针，用来指明数组的上界和下界*/
    char *mid;                  /* 数组的中间项指针*/
    char *loguy, *higuy;        /* 循环中的游动指针*/
    int size;                   /* 数组的大小*/
    int stkptr;                 /* 栈顶指针*/
    char *lostk[STKSIZ], *histk[STKSIZ];

    /*如果只有一个或以下的元素，则退出*/
    if (num < 2 || width == 0) {
        return;
    }

    stkptr = 0;

    lo = base;
    hi = (char *)base + width * (num-1);

    /*这个标签是伪递归的开始*/
recurse:

    /* 计算有多少个元素 */
    size = ((U32)(hi - lo) / (U32)width) + 1;

    /* 当size小于CUTOFF时，使用简单算法 */
    if (size <= CUTOFF) {
        _qsort_shortsort(lo, hi, width, cmp_func);
    }
    else {
        /*首先我们要选择一个分区项。算法的高效性要求我们找到一个近似数组中间值
          的项，但我们要保证能够很快找到它。我们选择数组的第一项、中间项和最后一项的中
          间值，来避免最坏情况下的低效率。测试表明，选择三个数的中间值，比单纯选择数组
          的中间项的效率要高 */
        /* 解释一下为什么要避免最坏情况和怎样避免。在最坏情况下，快速排序算法
           的运行时间复杂度是O(n^2)。这种情况的一个例子是已经排序的文件。如果我们选择最
           后一个项作为划分项，也就是已排序数组中的最大项，我们分区的结果是分成了一个大
           小为N-1的数组和一个大小为1的数组，这样的话，我们需要的比较次数是N + N-1 + N-2 
           + N-3 +...+2+1=(N+1)N/2=O(n^2)。而如果选择前 中 后三个数的中间值，这种最坏情况的
           数组也能够得到很好的处理。*/

        mid = lo + (size / 2) * width;

        /*第一项 中间项 最后项, 三个元素排序*/
        if (cmp_func(lo, mid) > 0) {
            MEM_Swap(lo, mid, width);
        }
        if (cmp_func(lo, hi) > 0) {
            MEM_Swap(lo, hi, width);
        }
        if (cmp_func(mid, hi) > 0) {
            MEM_Swap(mid, hi, width);
        }

        /*下面要把数组分区成三块，一块是小于分区项的，一块是等于分区项的，而另一块是大于分区项的。*/
        /*这里初始化的loguy 和 higuy两个指针，是在循环中用于移动来指示需要交换的两个元素的。
          higuy递减，loguy递增，所以下面的for循环总是可以终止。*/

        /* 循环中的游动指针*/
        loguy = lo; 
        higuy = hi;

        for (;;) {
            /*开始移动loguy指针，直到A[loguy]>A[mid]*/
            if (mid > loguy) {
                do  {
                    loguy += width;
                } while (loguy < mid && cmp_func(loguy, mid) <= 0);
            }

            /*如果移动到loguy>=mid的时候，就继续向后移动，使得A[loguy]>A[mid]。
              这一步实际上作用就是使得移动完loguy之后，loguy指针之前的元素都是不大于划分值的元素。*/
            if (mid <= loguy) {
                do  {
                    loguy += width;
                } while (loguy <= hi && cmp_func(loguy, mid) <= 0);
            }
 
           /*执行到这里的时候，loguy指针之前的项都比A[mid]要小或者等于它*/
           
            /*下面移动higuy指针，直到A[higuy]<＝A[mid]*/
            do  {
                higuy -= width;
            } while (higuy > mid && cmp_func(higuy, mid) > 0);
 
           /*如果两个指针交叉了，则退出循环。*/
            if (higuy < loguy)
                break;
 
            /* 此时A[loguy]>A[mid],A[higuy]<=A[mid],loguy<=hi，higuy>lo。*/
           /*交换两个指针指向的元素*/
            MEM_Swap(loguy, higuy, width);
 
            /*如果划分元素的位置移动了，我们要跟踪它。
              因为在前面对loguy处理的两个循环中的第二个循环已经保证了loguy>mid,
              即loguy指针不和mid指针相等。
              所以我们只需要看一下higuy指针是否等于mid指针，
              如果原来是mid==higuy成立了，那么经过刚才的交换，中间值项已经到了loguy指向的位置
              注意：刚才是值交换了，但是并没有交换指针。当higuy和mid相等，交换higuy和loguy指向的内容，higuy依然等于mid,
              所以让mid＝loguy，重新跟踪中间值。*/

            if (mid == higuy)
                mid = loguy;
        }

        /*上一个循环结束之后，因为还没有执行loguy指针和higuy指针内容的交换，
          所以loguy指针的前面的数组元素都不大于划分值，而higuy指针之后的数组元素都大于划分值，所以此时有两种情况：
          1)  higuy＝loguy－1
          2)  higuy＝hi－1，loguy＝hi+1
          其中第二种情况发生在一开始选择三个元素的时候，hi指向的元素和mid指向的元素值相等，
          而hi前面的元素全部都不大于划分值，使得移动loguy指针的时候，一直移动到了hi+1才停止，
          再移动higuy指针的时候，higuy指针移动一步就停止了，停在hi－1处。
         */
        higuy += width;
        if (mid < higuy) {
            do  {
                higuy -= width;
            } while (higuy > mid && cmp_func(higuy, mid) == 0);
        }
        if (mid >= higuy) {
            do  {
                higuy -= width;
            } while (higuy > lo && cmp_func(higuy, mid) == 0);
        }

        /*
           可以想像一下，对于一个已经排序的数组，如果每次分成N-1和1的数组，
           而每次都先处理N-1那一半，那么递归深度就是和N成比例，这样对于大N，栈空间的开销是很大的。
           如果先处理1的那一半，栈里面最多只有2项。当划分元素刚好在数组中间时，栈的长度是logN。
           对于栈的操作，就是先把大的数组信息入栈。
         */
        if ( higuy - lo >= hi - loguy ) {
            if (lo < higuy) {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy;
                ++stkptr;
            }

            if (loguy < hi) {
                lo = loguy;
                goto recurse;           /* do small recursion */
            }
        } else {
            if (loguy < hi) {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr;               /* save big recursion for later */
            }

            if (lo < higuy) {
                hi = higuy;
                goto recurse;           /* do small recursion */
            }
        }
    }

    /*出栈操作，直到栈为空，退出循环*/

    --stkptr;
    if (stkptr >= 0) {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse;           /* pop subarray from stack */
    } else {
        return;                 /* all subarrays done */
    }
}

