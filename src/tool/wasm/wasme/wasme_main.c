/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include <readline/history.h>
#include <readline/readline.h>
#include "utl/file_utl.h"
#include "utl/wasm_utl.h"

static void wasme_usage(char *prog)
{
    fprintf(stderr, "%s WASM_FILE [ARG...]\n", prog);
}

int main(int argc, char **argv)
{
    char *mod_path;
    UCHAR *bytes = NULL;
    FILE_MEM_S *mem;
    int ret = 0;

    if (argc < 2) {
        wasme_usage(argv[0]);
        return -1;
    }

    mod_path = argv[1];

    WASME_Init();

    mem = FILE_Mem(mod_path);
    if (! mem) {
        BS_PRINT_ERR("Could not load %s", mod_path);
        return 2;
    }

    bytes = mem->pucFileData;

    if (bytes == NULL) {
        BS_PRINT_ERR("Could not load %s", mod_path);
        return 2;
    }

    Options opts = { .disable_memory_bounds = 1,
                     .mangle_table_index    = 1,
                     .dlsym_trim_underscore = 1};

    WASM_MODULE_S * m = WASM_Load(bytes, mem->uiFileLen, &opts);

    WASME_InitThunk(m);

    
    WASM_BLOCK_S *func = WASM_GetExport(m, "__post_instantiate");
    if (func) {
        WASM_Run(m, func->fidx);
    }

    WASM_StackPushValue(m, WASM_I32, argc-1);
    WASM_StackPushValue(m, WASM_I64, (ULONG)(void*)(argv+1));

    func = WASM_GetExport(m, "main");
    if (!func) {
        func = WASM_GetExport(m, "_main");
    }
	if (!func) {
	    FATAL("no exported function named 'main' or '_main'\n");
	}

    int res = WASM_Run(m, func->fidx);

    if (!res) {
        char *exception = WASM_GetException(m);
        if (exception && (exception[0] != '\0')) {
            BS_PRINT_ERR("Exception: %s\n", exception);
        }
        ret = -1;
    } else {
        WASM_STACKVALUE_S *retv = WASM_StackPop(m);
        if (retv) {
            ret = retv->value.uint32;
        }
    }

    WASM_Free(m);
    FILE_MemFree(mem);

    return ret;
}

