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


#define BEGIN(x, y) "\033[" #x ";" #y "m"// x: 背景，y: 前景
#define CLOSE "\033[0m"                  // 关闭所有属性

static void wasmc_usage(char *prog)
{
    fprintf(stderr, "%s --repl WASM_FILE\n", prog);
    fprintf(stderr, "%s        WASM_FILE CMD [ARG...]\n", prog);
    exit(2);
}

static char * _wasm_main_value_repr(WASM_STACKVALUE_S *v)
{
    static char value[256];

    switch (v->value_type) {
        case WASM_I32:
            snprintf(value, sizeof(value), "0x%x:i32", v->value.uint32);
            break;
        case WASM_I64:
            snprintf(value, sizeof(value), "%llu:i64", v->value.uint64);
            break;
        case WASM_F32:
            snprintf(value, sizeof(value), "%.7g:f32", v->value.f32);
            break;
        case WASM_F64:
            snprintf(value, sizeof(value), "%.7g:f64", v->value.f64);
            break;
    }
    return value;
}

static void _wasm_main_run_line(WASM_MODULE_S *m, char *line)
{
    BOOL_T res = WASM_RunLine(m, line);

    if (! res) {
        char *exception = WASM_GetException(m);
        if (exception && (exception[0] != '\0')) {
            BS_PRINT_ERR("Exception: %s\n", exception);
        }
        return;
    }

    WASM_STACKVALUE_S *result = WASM_StackPop(m);
    if (result) {
        printf("%s\n", _wasm_main_value_repr(result));
    }
}

int main(int argc, char **argv)
{
    char *mod_path;
    UCHAR *bytes = NULL;
    char *line = NULL;
    char *string;
    FILE_MEM_S *mem;

    if (argc != 2) {
        wasmc_usage(argv[0]);
        return 2;
    }

    mod_path = argv[1];
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

    WASM_MODULE_S * m = WASM_Load(bytes, mem->uiFileLen, NULL);

    while (1) {
        line = readline(BEGIN(49, 34) "wasm$ " CLOSE);

        if (!line) {
            break;
        }

        string = TXT_Strim(line);

        if (string[0] == '\0') {
            continue;
        }

        if (strcmp(string, "quit") == 0) {
            free(line);
            break;
        }

        add_history(string);

        _wasm_main_run_line(m, string);

        free(line);
    }

    WASM_Free(m);
    FILE_MemFree(mem);

    return 0;
}

