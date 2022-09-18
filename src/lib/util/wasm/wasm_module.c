/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/leb_utl.h"
#include "utl/wasm_utl.h"
#include "wasm_def.h"
#include "wasm_func.h"

static UINT64 _wasm_get_type_mask(WASM_TYPE_S *type)
{
    UINT64 mask = 0x80;
    UINT p;

    if (type->result_count == 1) {
        mask |= 0x80 - type->results[0];
    }
    mask = mask << 4;
    for (p = 0; p < type->param_count; p++) {
        mask = mask << 4;
        mask |= 0x80 - type->params[p];
    }
    return mask;
}

static int _wasm_parse_table_type(WASM_MODULE_S *m, INOUT UINT *pos)
{
    m->table.elem_type = LEB_Read(m->bytes, pos);
    BS_DBGASSERT(m->table.elem_type == WASM_ANYFUNC);

    UINT flags = LEB_Read(m->bytes, pos);
    UINT tsize = LEB_Read(m->bytes, pos);

    m->table.min_size = tsize;
    m->table.cur_size = tsize;

    if (flags & 0x1) {
        tsize = LEB_Read(m->bytes, pos);
        m->table.max_size = MIN(0x10000, tsize);
    } else {
        m->table.max_size = 0x10000;
    }

    return 0;
}

static void _wasm_parse_memory_type(WASM_MODULE_S *m, INOUT UINT *pos)
{
    UINT flags = LEB_Read(m->bytes, pos);
    UINT pages = LEB_Read(m->bytes, pos);
    m->memory.min_size = pages;
    m->memory.cur_size = pages;

    if (flags & 0x1) {
        pages = LEB_Read(m->bytes, pos);
        m->memory.max_size = MIN(0x8000, pages);
    } else {
        m->memory.max_size = 0x8000;
    }
}

static char * _wasm_read_string(UCHAR *bytes, INOUT UINT *pos, UINT *result_len)
{
    UINT str_len = LEB_Read(bytes, pos);
    char *str = malloc(str_len + 1);
    memcpy(str, bytes + *pos, str_len);
    str[str_len] = '\0';
    *pos += str_len;
    if (result_len) {
        *result_len = str_len;
    }

    return str;
}

static void _wasm_run_init_expr(WASM_MODULE_S *m, UCHAR type, UINT *pc)
{
    m->pc = *pc;
    WASM_BLOCK_S block = {
        .block_type = 0x01,
        .type = wasm_get_block_type(type),
        .start_addr = *pc
    };
    wasm_push_block(m, &block, m->sp);
    wasm_interpret(m);
    *pc = m->pc;
    BS_DBGASSERT(m->stack[m->sp].value_type == type);
}

static BOOL_T _wasm_resolve_sym(char *filename, char *symbol, void **val, char **err)
{
    void *handle = NULL;

    dlerror(); //clear errors

    if (filename) {
        handle = dlopen(filename, RTLD_LAZY);
        if (!handle) {
            *err = dlerror();
            return FALSE;
        }
    }

    *val = dlsym(handle, symbol);

    if ((*err = dlerror()) != NULL) {
        return FALSE;
    }
    return TRUE;
}

static int _wasm_load_custom(UCHAR *bytes, UINT curpos, OUT WASM_MODULE_S *m)
{
    return 0;
}

static int _wasm_load_type(UCHAR *bytes, UINT curpos, OUT WASM_MODULE_S *m)
{
    UINT i, j;
    UINT pos = curpos;

    m->type_count = LEB_Read(bytes, &pos);
    m->types = MEM_ZMalloc(m->type_count * sizeof(WASM_TYPE_S));
    for (i = 0; i < m->type_count; i++) {
        WASM_TYPE_S *type = &m->types[i];
        type->form = LEB_Read(bytes, &pos);
        type->param_count = LEB_Read(bytes, &pos);
        type->params = MEM_ZMalloc(type->param_count * sizeof(int));
        for (j = 0; j < type->param_count; j++) {
            type->params[j] = LEB_Read(bytes, &pos);
        }
        type->result_count = LEB_Read(bytes, &pos);
        type->results = MEM_ZMalloc(type->result_count * sizeof(int));
        for (j = 0; j < type->result_count; j++) {
            type->results[j] = LEB_Read(bytes, &pos);
        }
        type->mask = _wasm_get_type_mask(type);
    }

    return 0;
}

static int _wasm_load_import_id(UCHAR *bytes, UINT curpos, OUT WASM_MODULE_S *m)
{
    UINT pos = curpos;
    UINT idx;

    UINT import_count = LEB_Read(bytes, &pos);

    for (idx = 0; idx < import_count; idx++) {
        UINT module_len, field_len;
        char *import_module = _wasm_read_string(bytes, &pos, &module_len);
        char *import_field = _wasm_read_string(bytes, &pos, &field_len);
        UINT external_kind = bytes[pos++];
        UINT type_index, fidx;
        UCHAR global_type;

        switch (external_kind) {
            case WASM_KIND_FUNCTION:
                type_index = LEB_Read(bytes, &pos);
                break;
            case WASM_KIND_TABLE:
                _wasm_parse_table_type(m, &pos);
                break;
            case WASM_KIND_MEMORY:
                _wasm_parse_memory_type(m, &pos);
                break;
            case WASM_KIND_GLOBAL:
                global_type = LEB_Read(bytes, &pos);
                LEB_Read(bytes, &pos); // mutability
                break;
            default:
                break;
        }

        void *val;
        char *err;
        char *sym = malloc(module_len + field_len + 5);

        do {
            if (_wasm_resolve_sym(import_module, import_field, &val, &err)) {
                break;
            }

            sprintf(sym, "_%s__%s_", import_module, import_field);
            TXT_ReplaceChar(sym, '-', '_');
            if (_wasm_resolve_sym(NULL, sym, &val, &err)) {
                break; 
            }

            // If enabled, try without the leading underscore (added
            // by emscripten for external symbols)
            if (m->options.dlsym_trim_underscore &&
                    (strncmp("env", import_module, 4) == 0) &&
                    (strncmp("_", import_field, 1) == 0)) {
                sprintf(sym, "%s", import_field+1);
                if (_wasm_resolve_sym(NULL, sym, &val, &err)) {
                    break;
                }
            }

            // Try the plain symbol by itself with module name/handle
            sprintf(sym, "%s", import_field);
            if (_wasm_resolve_sym(NULL, sym, &val, &err)) {
                break; 
            }
        }while(0);

        if (! val) {
            printf("import %s:%s failed\n", import_module, import_field);
            continue;
        }

        switch (external_kind) {
            case WASM_KIND_FUNCTION:
                fidx = m->function_count;
                m->import_func_count += 1;
                m->function_count += 1;
                m->functions = MEM_ZRealloc(m->functions,
                        fidx*sizeof(WASM_BLOCK_S),
                        m->import_func_count*sizeof(WASM_BLOCK_S));
                WASM_BLOCK_S *func = &m->functions[fidx];
                func->import_module = import_module;
                func->import_field = import_field;
                func->func_ptr = val;
                func->type = &m->types[type_index];
                break;
            case WASM_KIND_TABLE:
                BS_DBGASSERT(! m->table.entries);
                WASM_TABLE_S *tval = val;
                m->table.entries = val;
                BS_DBGASSERT(m->table.cur_size <= tval->max_size);
                m->table.entries = *(UINT **) val;
                m->table.cur_size = tval->cur_size;
                m->table.max_size = tval->max_size;
                m->table.entries = tval->entries;
                break;
            case WASM_KIND_MEMORY:
                BS_DBGASSERT(!m->memory.bytes);
                WASM_MEMORY_S *mval = val;
                BS_DBGASSERT(m->memory.cur_size <= mval->max_size);
                m->memory.cur_size = mval->cur_size;
                m->memory.max_size = mval->max_size;
                m->memory.bytes = mval->bytes;
                break;
            case WASM_KIND_GLOBAL:
                m->global_count += 1;
                m->globals = MEM_ZRealloc(m->globals,
                        (m->global_count - 1) * sizeof(WASM_STACKVALUE_S),
                        m->global_count * sizeof(WASM_STACKVALUE_S));
                WASM_STACKVALUE_S *glob = &m->globals[m->global_count - 1];
                glob->value_type = global_type;
                switch (global_type) {
                    case WASM_I32:
                        memcpy(&glob->value.uint32, val, 4);
                        break;
                    case WASM_I64:
                        memcpy(&glob->value.uint64, val, 8);
                        break;
                    case WASM_F32:
                        memcpy(&glob->value.f32, val, 4);
                        break;
                    case WASM_F64:
                        memcpy(&glob->value.f64, val, 8);
                        break;
                    default:
                        break;
                }
                break;
            default:
                BS_DBGASSERT(0);
                return -1;
        }
    }

    return 0;
}

static int _wasm_load_func_id(UCHAR *bytes, UINT curpos, OUT WASM_MODULE_S *m)
{
    UINT i;
    UINT pos = curpos;

    m->function_count += LEB_Read(bytes, &pos);

    WASM_BLOCK_S *functions;
    functions = MEM_ZMalloc(m->function_count * sizeof(WASM_BLOCK_S));

    if (m->import_func_count != 0) {
        memcpy(functions, m->functions, sizeof(WASM_BLOCK_S) * m->import_func_count);
        MEM_Free(m->functions);
    }
    m->functions = functions;

    for (i = m->import_func_count; i < m->function_count; i++) {
        m->functions[i].fidx = i;
        UINT tidx = LEB_Read(bytes, &pos);
        m->functions[i].type = &m->types[tidx];
    }

    return 0;
}

static int _wasm_load_table_id(UCHAR *bytes, UINT cur_pos, OUT WASM_MODULE_S *m)
{
    UINT pos = cur_pos;
    UINT table_count = LEB_Read(bytes, &pos);
    if (table_count != 1) {
        BS_DBGASSERT(0);
        return -1;
    }

    _wasm_parse_table_type(m, &pos);
    m->options.mangle_table_index = 0;
    m->table.entries = MEM_ZMalloc(m->table.cur_size * sizeof(UINT));

    return 0;
}

static int _wasm_load_mem_id(UCHAR *bytes, UINT cur_pos, OUT WASM_MODULE_S *m)
{
    UINT pos = cur_pos;
    UINT memory_count = LEB_Read(bytes, &pos);
    (void)memory_count;
    BS_DBGASSERT(memory_count == 1);
    _wasm_parse_memory_type(m, &pos);
    m->memory.bytes = MEM_ZMalloc(m->memory.cur_size * WASM_PAGE_SIZE * sizeof(UINT));

    return 0;
}

static int _wasm_load_global_id(UCHAR *bytes, UINT cur_pos, OUT WASM_MODULE_S *m)
{
    UINT pos = cur_pos;
    UINT global_count = LEB_Read(bytes, &pos);
    UINT g;

    for (g = 0; g < global_count; g++) {
        UCHAR type = LEB_Read(bytes, &pos);
        LEB_Read(bytes, &pos); //mutability
        UINT gidx = m->global_count;
        m->global_count += 1;
        m->globals = MEM_ZRealloc(m->globals,
                gidx*sizeof(WASM_STACKVALUE_S),
                m->global_count*sizeof(WASM_STACKVALUE_S));
        m->globals[gidx].value_type = type;
        _wasm_run_init_expr(m, type, &pos);
        m->globals[gidx] = m->stack[m->sp--];
    }

    return 0;
}

static int _wasm_load_export_id(UCHAR *bytes, UINT cur_pos, OUT WASM_MODULE_S *m)
{
    UINT e;
    UINT pos = cur_pos;
    UINT export_count = LEB_Read(bytes, &pos);

    for (e = 0; e < export_count; e++) {
        char *name = _wasm_read_string(bytes, &pos, NULL);
        UINT external_kind = bytes[pos++];
        UINT index = LEB_Read(bytes, &pos);
        UINT eidx = m->export_count;

        m->export_count += 1;

        m->exports = MEM_ZRealloc(m->exports,
                eidx*sizeof(WASM_EXPORT_S),
                m->export_count*sizeof(WASM_EXPORT_S));
        m->exports[eidx].export_name = name;
        m->exports[eidx].external_kind = external_kind;

        switch (external_kind) {
            case WASM_KIND_FUNCTION:
                m->exports[eidx].value = &m->functions[index];
                break;
            case WASM_KIND_TABLE:
                BS_DBGASSERT(index == 0);
                m->exports[eidx].value = &m->table;
                break;
            case WASM_KIND_MEMORY:
                BS_DBGASSERT(index == 0);
                m->exports[eidx].value = &m->memory;
                break;
            case WASM_KIND_GLOBAL:
                m->exports[eidx].value = &m->globals[index];
                break;
            default:
                break;
        }
    }

    return 0;
}

static int _wasm_load_start_id(UCHAR *bytes, UINT cur_pos, OUT WASM_MODULE_S *m)
{
    m->start_function = LEB_Read(bytes, &cur_pos);
    return 0;
}

static int _wasm_load_elem_id(UCHAR *bytes, UINT cur_pos, OUT WASM_MODULE_S *m)
{
    UINT pos = cur_pos;
    UINT elem_count = LEB_Read(bytes, &pos);
    UINT c, n;

    for (c = 0; c < elem_count; c++) {
        UINT index = LEB_Read(bytes, &pos);
        BS_DBGASSERT(index == 0);
        (void)index;
        _wasm_run_init_expr(m, WASM_I32, &pos);
        UINT offset = m->stack[m->sp--].value.uint32;
        if (m->options.mangle_table_index) {
            offset = offset - (UINT)(ULONG)m->table.entries;
        }
        UINT num_elem = LEB_Read(bytes, &pos);
        if (!m->options.disable_memory_bounds) {
            BS_DBGASSERT(offset+num_elem <= m->table.cur_size);
        }
        for (n = 0; n < num_elem; n++) {
            m->table.entries[offset + n] = LEB_Read(bytes, &pos);
        }
    }

    return 0;
}

static int _wasm_load_code_id(UCHAR *bytes, UINT curpos, OUT WASM_MODULE_S *m)
{
    UINT pos = curpos;
    UINT code_count = LEB_Read(bytes, &pos); // 代码项数量
    UCHAR val_type;
    UINT save_pos, lidx, lecount;
    UINT c, l, n;

    for (c = 0; c < code_count; c++) {
        WASM_BLOCK_S *function = &m->functions[m->import_func_count + c];

        UINT code_size = LEB_Read(bytes, &pos);
        UINT payload_start = pos;
        UINT local_count = LEB_Read(bytes, &pos);

        save_pos = pos;
        function->local_count = 0;

        for (l = 0; l < local_count; l++) {
            lecount = LEB_Read(bytes, &pos);
            function->local_count += lecount;
            LEB_Read(bytes, &pos);
        }

        function->locals = MEM_ZMalloc(function->local_count * sizeof(UINT));

        pos = save_pos;
        lidx = 0;

        for (l = 0; l < local_count; l++) {
            lecount = LEB_Read(bytes, &pos);
            val_type = LEB_Read(bytes, &pos);
            for (n = 0; n < lecount; n++) {
                function->locals[lidx++] = val_type;
            }
        }

        function->start_addr = pos;
        function->end_addr = payload_start + code_size - 1;
        function->br_addr = function->end_addr;

        BS_DBGASSERT(bytes[function->end_addr] == 0x0b);

        pos = function->end_addr + 1;
    }

    return 0;
}

static int _wasm_load_data_id(UCHAR *bytes, UINT curpos, OUT WASM_MODULE_S *m)
{
    UINT pos = curpos;
    UINT mem_count = LEB_Read(bytes, &pos);
    UINT s;

    for (s = 0; s < mem_count; s++) {
        UINT index = LEB_Read(bytes, &pos);
        BS_DBGASSERT(index == 0);
        (void)index;
        _wasm_run_init_expr(m, WASM_I32, &pos);
        ULONG offset = m->stack[m->sp--].value.uint32;
        UINT size = LEB_Read(bytes, &pos);
        if (!m->options.disable_memory_bounds) {
            BS_DBGASSERT(offset+size <= m->memory.cur_size*WASM_PAGE_SIZE);
        }
        memcpy(m->memory.bytes + offset, bytes + pos, size);
        pos += size;
    }

    return 0;
}

static void _wasm_skip_immediate(UCHAR *bytes, UINT *pos)
{
    UINT opcode = bytes[*pos];
    UINT count;
    UINT i;
    *pos = *pos + 1;

    switch (opcode) {
        case Block_:
        case Loop:
        case If:
            LEB_Read(bytes, pos);
            break;
        case Br:
        case BrIf:
            LEB_Read(bytes, pos);
            break;
        case BrTable:
            count = LEB_Read(bytes, pos);
            for (i = 0; i < count; i++) {
                LEB_Read(bytes, pos);
            }
            LEB_Read(bytes, pos);
            break;
        case Call:
            LEB_Read(bytes, pos);
            break;
        case CallIndirect:
            LEB_Read(bytes, pos);
            LEB_Read(bytes, pos);
            break;
        case LocalGet:
        case LocalSet:
        case LocalTee:
        case GlobalGet:
        case GlobalSet:
            LEB_Read(bytes, pos);
            break;
        case I32Load ... I64Store32:
            LEB_Read(bytes, pos);
            LEB_Read(bytes, pos);
            break;
        case MemorySize:
        case MemoryGrow:
            LEB_Read(bytes, pos);
            break;
        case I32Const:
            LEB_Read(bytes, pos);
            break;
        case I64Const:
            LEB_Read(bytes, pos);
            break;
        case F32Const:
            *pos += 4;
            break;
        case F64Const:
            *pos += 8;
            break;
        default:
            break;
    }
}

static int _wasm_find_blocks(WASM_MODULE_S *m)
{
    WASM_BLOCK_S *function;
    WASM_BLOCK_S *block;
    WASM_BLOCK_S *blockstack[WASM_BLOCKSTACK_SIZE];
    int top = -1;
    UCHAR opcode = Unreachable;
    UINT f;

    for (f = m->import_func_count; f < m->function_count; f++) {
        function = &m->functions[f];
        UINT pos = function->start_addr;
        while (pos <= function->end_addr) {
            opcode = m->bytes[pos];
            switch (opcode) {
                case Block_:
                case Loop:
                case If:
                    block = MEM_ZMalloc(sizeof(WASM_BLOCK_S));
                    block->block_type = opcode;
                    block->type = wasm_get_block_type(m->bytes[pos + 1]);
                    block->start_addr = pos;
                    blockstack[++top] = block;
                    m->block_lookup[pos] = block;
                    break;
                case Else_:
                    BS_DBGASSERT(blockstack[top]->block_type == If);
                    blockstack[top]->else_addr = pos + 1;
                    break;
                case End_:
                    if (pos == function->end_addr) {
                        break;
                    }
                    BS_DBGASSERT(top >= 0);
                    block = blockstack[top--];
                    block->end_addr = pos;
                    if (block->block_type == Loop) {
                        block->br_addr = block->start_addr + 2;
                    } else {
                        block->br_addr = pos;
                    }
                    break;
                default:
                    break;
            }
            _wasm_skip_immediate(m->bytes, &pos);
        }
        BS_DBGASSERT(top == -1);
        if (opcode != End_) {
            BS_WARNNING(("pos 0x%x invalid \n", pos));
            BS_DBGASSERT(0);
        }
    }

    return 0;
}

static int _wasm_load(UCHAR *bytes, UINT byte_count, OUT WASM_MODULE_S *m)
{
    int ret = 0;
    UINT pos = 0;

    UINT magic = ((UINT*) (bytes + pos))[0];
    pos += 4;
    if (magic != WASM_MAGIC) {
        RETURN(BS_ERR);
    }

    UINT version = ((UINT*) (bytes + pos))[0];
    pos += 4;
    if (version != WASM_VERSION) {
        RETURN(BS_ERR);
    }

    while (pos < byte_count) {
        UINT id = LEB_Read(bytes, &pos);
        UINT slen = LEB_Read(bytes, &pos);

        switch (id) {
            case WASM_ID_CUSTOM:
                ret = _wasm_load_custom(bytes, pos, m);
                break;
            case WASM_ID_TYPE:
                ret = _wasm_load_type(bytes, pos, m);
                break;
            case WASM_ID_IMPORT:
                ret = _wasm_load_import_id(bytes, pos, m);
                break;
            case WASM_ID_FUNC:
                ret = _wasm_load_func_id(bytes, pos, m);
                break;
            case WASM_ID_TABLE:
                ret = _wasm_load_table_id(bytes, pos, m);
                break;
            case WASM_ID_MEM:
                ret = _wasm_load_mem_id(bytes, pos, m);
                break;
            case WASM_ID_GLOBAL:
                ret = _wasm_load_global_id(bytes, pos, m);
                break;
            case WASM_ID_EXPORT:
                ret = _wasm_load_export_id(bytes, pos, m);
                break;
            case WASM_ID_START:
                ret = _wasm_load_start_id(bytes, pos, m);
                break;
            case WASM_ID_ELEM:
                ret = _wasm_load_elem_id(bytes, pos, m);
                break;
            case WASM_ID_CODE:
                ret = _wasm_load_code_id(bytes, pos, m);
                break;
            case WASM_ID_DATA:
                ret = _wasm_load_data_id(bytes, pos, m);
                break;
            default:
                BS_DBGASSERT(0);
                ret = -1;
                break;
        }

        if (ret != 0) {
            break;
        }

        pos += slen;
    }

    if (ret != 0) {
        return ret;
    }

    _wasm_find_blocks(m);

    if (m->start_function >= 0) {
        if (m->start_function < m->import_func_count) {
            thunk_out(m, m->start_function);
        }
        if (FALSE == WASM_Run(m, m->start_function)) {
            ret = -1;
        }
    }

    return ret;
}

WASM_TYPE_S * wasm_get_block_type(UCHAR value_type)
{
    static UINT results[][1] = {{WASM_I32}, {WASM_I64}, {WASM_F32}, {WASM_F64}};
    static WASM_TYPE_S block_types[] = {
        { .result_count = 0, },
        { .result_count = 1, .results = results[0], },
        { .result_count = 1, .results = results[1], },
        { .result_count = 1, .results = results[2], },
        { .result_count = 1, .results = results[3], },
    };

    switch (value_type) {
        case WASM_BLOCK:
            return &block_types[0];
        case WASM_I32:
            return &block_types[1];
        case WASM_I64:
            return &block_types[2];
        case WASM_F32:
            return &block_types[3];
        case WASM_F64:
            return &block_types[4];
        default:
            BS_PRINT_ERR("type=0x%x \n", value_type);
            BS_DBGASSERT(0);
            return NULL;
    }
}

void * WASM_GetExport(WASM_MODULE_S *m, char *name)
{
    UINT e;

    for (e = 0; e < m->export_count; e++) {
        char *export_name = m->exports[e].export_name;
        if (!export_name) {
            continue;
        }
        if (strncmp(name, export_name, 1024) == 0) {
            return m->exports[e].value;
        }
    }
    return NULL;
}

WASM_MODULE_S * WASM_Load(UCHAR *bytes, UINT byte_count, Options *opt)
{
    WASM_MODULE_S *m;

    if ((! bytes) || (! byte_count)) {
        return NULL;
    }

    m = MEM_ZMalloc(sizeof(WASM_MODULE_S));
    if (! m) {
        return NULL;
    }

    if (opt) {
        m->options = *opt;
    }
    m->sp = -1;
    m->fp = -1;
    m->csp = -1;
    m->start_function = -1;

    m->bytes = bytes;
    m->byte_count = byte_count;
    m->block_lookup = MEM_ZMalloc(m->byte_count * sizeof(WASM_BLOCK_S*));
    if (! m->block_lookup) {
        WASM_Free(m);
        return NULL;
    }

    if (_wasm_load(bytes, byte_count, m) != 0) {
        WASM_Free(m);
        return NULL;
    }

    return m;
}

void WASM_Free(WASM_MODULE_S *m)
{
    if (! m) {
        return;
    }

    MEM_Free(m);
}

