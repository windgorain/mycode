/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _WASM_UTL_H
#define _WASM_UTL_H
#ifdef __cplusplus
extern "C"
{
#endif

#define WASM_I32 0x7f
#define WASM_I64 0x7e
#define WASM_F32 0x7d
#define WASM_F64 0x7c
#define WASM_ANYFUNC 0x70
#define WASM_FUNC 0x60
#define WASM_BLOCK 0x40

typedef struct tagWASM_MODULE_S WASM_MODULE_S;

typedef struct tagWASM_STACKVALUE_S {
    UCHAR value_type;// 值类型
    union {
        UINT uint32;
        int int32;
        UINT64 uint64;
        INT64 int64;
        float f32;
        double f64;
    } value;// 值
}WASM_STACKVALUE_S;

// 内存结构体
typedef struct tagWASM_MEMORY_S {
    UINT min_size;// 最小页数
    UINT max_size;// 最大页数
    UINT cur_size;// 当前页数
    UCHAR *bytes;   // 用于存储数据
} WASM_MEMORY_S;

typedef struct tagWASM_TYPE_S {
    UCHAR form;
    UINT param_count; // 参数数量
    UINT *params;     // 参数类型集合
    UINT result_count;// 返回值数量
    UINT *results;    // 返回值类型集合
    UINT64 mask;      // 基于控制块（包含函数）签名计算的唯一掩码值
}WASM_TYPE_S;

// 控制块（包含函数）结构体
typedef struct tagBlock {
    UCHAR block_type;

    UINT fidx;
    UINT local_count;
    UINT start_addr;
    UINT end_addr;
    UINT else_addr;
    UINT br_addr;

    UINT *locals;
    WASM_TYPE_S *type;
    char *import_module;
    char *import_field;
    void *(*func_ptr)();
} WASM_BLOCK_S;

typedef struct Options {
    // when true: host memory addresses will be outside allocated memory area
    // so do not do bounds checking
    BOOL_T disable_memory_bounds;

    // when true, table entries are accessed like this:
    //   m->table.entries[m->table.entries-index]
    // when false, table entires are accessed like this:
    //   m->table.entries[index]
    BOOL_T mangle_table_index;

    BOOL_T dlsym_trim_underscore;
} Options;

void WASME_Init();
void WASME_InitThunk(WASM_MODULE_S *m);

WASM_MODULE_S * WASM_Load(UCHAR *bytes, UINT byte_count, Options *opt);
void WASM_Free(WASM_MODULE_S *m);

void * WASM_GetExport(WASM_MODULE_S *m, char *name);

BOOL_T WASM_Run(WASM_MODULE_S *m, UINT fidx);
BOOL_T WASM_RunLine(WASM_MODULE_S *m, char *line);

WASM_STACKVALUE_S * WASM_StackGet(WASM_MODULE_S *m, int off);
WASM_STACKVALUE_S * WASM_StackPop(WASM_MODULE_S *m);
int WASM_StackPush(WASM_MODULE_S *m, IN WASM_STACKVALUE_S *v);
int WASM_StackPushValue(WASM_MODULE_S *m, UCHAR type, UINT64 value);
char * WASM_GetException(WASM_MODULE_S *m);

#ifdef __cplusplus
}
#endif
#endif //WASM_UTL_H_
