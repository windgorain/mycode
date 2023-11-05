/*================================================================
*   Created by LiXingang
*   Description: 
*    设: h = hash(data[12])
*    设: X = [x1,x2,...,xn]
*    定义: y = f(h, X)
*          Y = {y1,y2,...,yn}
*    求解X,使得Y内的元素不冲突,增加新的Data后重新求解X依然使得Y不冲突
*
================================================================*/
#include "bs.h"
#include "utl/jhash_utl.h"
#include "utl/rand_utl.h"

#define EQUATION_MAX_DATA 1000000
#define EQUATION_SLOVE_MAX_STEP 0 
#define EQUATION_DEPTH (1024 * 16)
#define EQUATION_WITH (16)

typedef struct {
    UCHAR data[12];
}EQUATION_DATA_S;

typedef struct {
    int X[EQUATION_WITH][EQUATION_DEPTH];
    char ref[EQUATION_WITH][EQUATION_DEPTH];
    char flag[EQUATION_WITH][EQUATION_DEPTH];
    int Y[EQUATION_MAX_DATA];
    int y_count;
    EQUATION_DATA_S data[EQUATION_MAX_DATA];
}EQUATION_EMU_S;

static int _equation_check();
static int _equation_f(UCHAR *data, int factor);
static void _equation_print_X();

static EQUATION_EMU_S g_equation;


static int _equation_is_exist_y(UINT y)
{
    int i;

    for (i=0; i<g_equation.y_count; i++) {
        if (y == g_equation.Y[i]) {
            return 1;
        }
    }

    return 0;
}

static void _equation_build_index(UCHAR *data, OUT UINT *index)
{
    int i;

    for (i=0; i<EQUATION_WITH; i++) {
        index[i] = JHASH_GeneralBuffer(data, 12, i+1);
        index[i] %= EQUATION_DEPTH;
    }
}


static void _equation_process_ref(UCHAR *data)
{
    UINT index[EQUATION_WITH];
    int i;

    _equation_build_index(data, index);

    for (i=0; i<EQUATION_WITH; i++) {
        g_equation.ref[i][index[i]] ++;
    }
}


static int _equation_try_add(UCHAR *data)
{
    int y = _equation_f(data, 0);

    
    if (_equation_is_exist_y(y)) {
        return -1;
    }

    g_equation.Y[g_equation.y_count] = y;
    g_equation.y_count ++;

    _equation_process_ref(data);

    return 0;
}



static char * _equation_get_flag_min_ref(UINT *index)
{
    int i;
    UINT ref;
    char *pos;
    char *found = NULL;

    for (i=0; i<EQUATION_WITH; i++) {
        pos = &g_equation.flag[i][index[i]];
        if (*pos) { 
            continue;
        }
        if ((!found) || (ref < g_equation.ref[i][index[i]])) {
            found = pos;
            ref = g_equation.ref[i][index[i]];
        }
    }

    return found;
}


static char * _equation_get_flag_random(UINT *index)
{
    int i;
    char *pos;

    
    for (i=0; i<EQUATION_WITH*4; i++) {
        int rand = RAND_Get() % EQUATION_WITH;
        pos = &g_equation.flag[rand][index[rand]];
        if (*pos == 0) {
            return pos;
        }
    }

    
    for (i=0; i<EQUATION_WITH; i++) {
        pos = &g_equation.flag[i][index[i]];
        if (*pos == 0) {
            return pos;
        }
    }

    return NULL;
}


static char * _equation_get_flag(UCHAR *data)
{
    UINT index[EQUATION_WITH];
    char *pos;

    _equation_build_index(data, index);

    int rand = RAND_Get() % 1024;

    
    if (rand < 820) {
        pos = _equation_get_flag_min_ref(index);
        if (pos) {
            return pos;
        }
    }

    return _equation_get_flag_random(index);
}


static int _equation_f(UCHAR *data, int factor)
{
    int i;
    int v = 0;
    UINT index[EQUATION_WITH];
    int h = JHASH_GeneralBuffer(data, 12, 0);

    _equation_build_index(data, index);

    for (i=0; i<EQUATION_WITH; i++) {
        v ^= g_equation.X[i][index[i]];
    }

    v ^= factor;
    v ^= h;

    
    return  JHASH_GeneralBuffer(&v, sizeof(v), 0) % (EQUATION_MAX_DATA * 2);
}


static int _equation_update(UCHAR *data)
{
    int i, j;
    int factor;

    for (factor=1; factor<0xfffffff; factor++) {
        for (i=0; i<EQUATION_WITH; i++) {
            for (j=0; j<EQUATION_DEPTH; j++) {
                if (g_equation.flag[i][j]) {
                    g_equation.X[i][j] ^= factor;
                }
            }
        }

        if (_equation_try_add(data) == 0) {
            break;
        }
    }

    if (factor >= 256) {
        return -1;
    }

    return 0;
}

static void _equation_pre_solve(UCHAR *data)
{
    UINT index[EQUATION_WITH];

    _equation_build_index(data, index);

    memset(g_equation.flag, 0, sizeof(g_equation.flag));
    int rand = RAND_Get() % EQUATION_WITH;
    g_equation.flag[rand][index[rand]] = 1;
}


static int _equation_is_y_change(UCHAR *data)
{
    int v = 0;
    int i;
    UINT index[EQUATION_WITH];

    _equation_build_index(data, index);

    for (i=0; i<EQUATION_WITH; i++) {
        v ^= g_equation.flag[i][index[i]];
    }

    return v;
}


static int _equation_adjust_flag()
{
    int changed;
    int i;
    char *p;

    do {
        changed = 0;

        for (i=0; i<g_equation.y_count; i++) {
            UCHAR *data = g_equation.data[i].data;

            if (_equation_is_y_change(data)) { 
                changed = 1;

                
                p = _equation_get_flag(data);
                if (! p) {
                    return -1;
                }
                *p = 1;
            }
        }
    } while(changed);

    return 0;
}


static int _equation_solve(UCHAR *data)
{
    int count = 0;
    UINT64 count2 = 0;

    while (count <= EQUATION_SLOVE_MAX_STEP) {

        count2++;
        printf("\r\t\t\t\t");
        printf("\r Add %d Step %llu  ", g_equation.y_count, count2);
        fflush(stdout);

        if (EQUATION_SLOVE_MAX_STEP) {
            count ++;
        }

        _equation_pre_solve(data);
        if (_equation_adjust_flag() == 0) {
            if (_equation_is_y_change(data)) {
                
                break;
            }
        }
    }

    printf("\n");

    if (EQUATION_SLOVE_MAX_STEP) {
        if (count > EQUATION_SLOVE_MAX_STEP) {
            return -1;
        }
    }

    return _equation_update(data);
}

static int _equation_add(UCHAR *data)
{
    if (_equation_try_add(data) == 0) {
        return 0;
    }

    
    return _equation_solve(data);
}

static void _equation_print_X()
{
    int i,j;

    for (j=0; j<EQUATION_DEPTH; j++) {
        for (i=0; i<EQUATION_WITH; i++) {
            printf("%-5u ", (UCHAR)g_equation.X[i][j]);
        }
        printf("\n");
    }
}


static int _equation_is_y_conflict()
{
    int i;
    int j;

    for (i=0; i<g_equation.y_count; i++) {
        for (j=i+1; j<g_equation.y_count; j++) {
            if (g_equation.Y[i] == g_equation.Y[j]) {
                return 1;
            }
        }
    }

    return 0;
}


static int _equation_check()
{
    int i;
    int y;
    int ret = 0;
    int count = g_equation.y_count;

    for (i=0; i<count; i++) {
        y = _equation_f(g_equation.data[i].data, 0);
        if (y != g_equation.Y[i]) {
            PRINT_RED("Data %d Check failed, Y[i]=%d, y=%d \n",
                    i, g_equation.Y[i], y);
            ret = -1;
        }
    }

    if (_equation_is_y_conflict()) {
        PRINT_RED("Y conflict");
        ret = -1;
    }

    return ret;
}

int main(int argc, char **argv)
{
    int i;

    for (i=0; i<EQUATION_MAX_DATA; i++) {
        printf("\r\t\t\t\t");
        printf("\r Add %d", i);
        fflush(stdout);

        memcpy(g_equation.data[i].data, &i, sizeof(i));

        if (_equation_add(g_equation.data[i].data) < 0) {
            break;
        }
    }

    printf("\n");

    _equation_print_X();

    _equation_check();

    PRINT_GREEN("Added %d data", i);

    return 0;
}


