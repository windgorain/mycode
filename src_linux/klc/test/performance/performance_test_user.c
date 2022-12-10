#include <stdio.h>
#include "bpf/bpf.h"
#include "bpf/bpf_load.h"
#include "utl/trace_pipe.h"

int main(int argc, char **argv)
{
    char obj[256];

    snprintf(obj, sizeof(obj), "%s_kern.o", argv[0]);

    if(load_bpf_file(obj) != 0) {
        printf("The kernel didn't load BPF program\n");
        return -1;
    }

    my_read_trace_pipe();
    return 0;
}
