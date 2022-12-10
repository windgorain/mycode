#include "klc/klc_base.h"
#include "helpers/mem_klc.h"

#define KLC_MODULE_NAME MEM_MODULE_NAME
KLC_DEF_MODULE();

SEC_NAME_FUNC(MEM_INVERT_NAME)
int mem_invert(void *in, int len, OUT void *out)
{
    int i;
    unsigned char * inc = (unsigned char*)in + (len -1);
    unsigned char * outc = out;

    for (i=0; i<len; i++) {
        *outc = *inc;
        outc ++;
        inc --;
    }

    return 0;
}

SEC_NAME_FUNC(MEM_PRINT_NAME)
static inline int mem_print(char *data, char *data_end)
{
    unsigned int *d = (void *)data;
    int loop = 0;
    unsigned char *d8;
    unsigned char tmp[4] = {0};

    BPF_Print("data len %d", data_end - data);

    if (data_end - data > 1024) {
        data_end = data + 1024;
    }

    while (d + 1 <= (unsigned int *)data_end) {
        BPF_Print("%d addr:%p %08x", loop, d, ntohl(*d));
        d++;
        loop++;
    }

    if ((char *)d >= data_end) {
        return 0;
    }

    d8 = (UCHAR *)d;

    int i = 0;
    while (d8 < (unsigned char *)data_end) {
        tmp[i] = *d8;
        d8++;
        i++;
    }

    BPF_Print("%d addr:%p %08x", loop, d8, ntohl(*(int*)tmp));

    return 0;
}


