/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include <math.h>
#include <inttypes.h>
#include "utl/mem_inline.h"
#include "utl/leb_utl.h"
#include "utl/wasm_utl.h"
#include "wasm_def.h"
#include "wasm_func.h"


#define OP_TRUNC(RES, A, TYPE, RMIN, RMAX)                   \
    if (isnan(A)) {                                          \
        snprintf(m->exception, sizeof(m->exception), "invalid conversion to integer"); \
        return FALSE;                                        \
    }                                                        \
    if ((A) <= (RMIN) || (A) >= (RMAX)) {                    \
        snprintf(m->exception, sizeof(m->exception), "integer overflow");              \
        return FALSE;                                        \
    }                                                        \
    (RES) = (TYPE) (A);

#define OP_I32_TRUNC_F32(RES, A) OP_TRUNC(RES, A, int, -2147483904.0f, 2147483648.0f)
#define OP_U32_TRUNC_F32(RES, A) OP_TRUNC(RES, A, UINT, -1.0f, 4294967296.0f)
#define OP_I32_TRUNC_F64(RES, A) OP_TRUNC(RES, A, int, -2147483649.0, 2147483648.0)
#define OP_U32_TRUNC_F64(RES, A) OP_TRUNC(RES, A, UINT, -1.0, 4294967296.0)

#define OP_I64_TRUNC_F32(RES, A) OP_TRUNC(RES, A, INT64, -9223373136366403584.0f, 9223372036854775808.0f)
#define OP_U64_TRUNC_F32(RES, A) OP_TRUNC(RES, A, UINT64, -1.0f, 18446744073709551616.0f)
#define OP_I64_TRUNC_F64(RES, A) OP_TRUNC(RES, A, INT64, -9223372036854777856.0, 9223372036854775808.0)
#define OP_U64_TRUNC_F64(RES, A) OP_TRUNC(RES, A, UINT64, -1.0, 18446744073709551616.0)




#define OP_TRUNC_SAT(RES, A, TYPE, RMIN, RMAX, IMIN, IMAX) \
    if (isnan(A)) {                                        \
        (RES) = 0;                                         \
    } else if ((A) <= (RMIN)) {                            \
        (RES) = IMIN;                                      \
    } else if ((A) >= (RMAX)) {                            \
        (RES) = IMAX;                                      \
    } else {                                               \
        (RES) = (TYPE) (A);                                \
    }

#define OP_I32_TRUNC_SAT_F32(RES, A) OP_TRUNC_SAT(RES, A, INT, -2147483904.0f, 2147483648.0f, INT32_MIN, INT32_MAX)
#define OP_U32_TRUNC_SAT_F32(RES, A) OP_TRUNC_SAT(RES, A, UINT, -1.0f, 4294967296.0f, 0UL, UINT32_MAX)
#define OP_I32_TRUNC_SAT_F64(RES, A) OP_TRUNC_SAT(RES, A, INT, -2147483649.0, 2147483648.0, INT32_MIN, INT32_MAX)
#define OP_U32_TRUNC_SAT_F64(RES, A) OP_TRUNC_SAT(RES, A, UINT, -1.0, 4294967296.0, 0UL, UINT32_MAX)

#define OP_I64_TRUNC_SAT_F32(RES, A) OP_TRUNC_SAT(RES, A, INT64, -9223373136366403584.0f, 9223372036854775808.0f, INT64_MIN, INT64_MAX)
#define OP_U64_TRUNC_SAT_F32(RES, A) OP_TRUNC_SAT(RES, A, UINT64, -1.0f, 18446744073709551616.0f, 0ULL, UINT64_MAX)
#define OP_I64_TRUNC_SAT_F64(RES, A) OP_TRUNC_SAT(RES, A, INT64, -9223372036854777856.0, 9223372036854775808.0, INT64_MIN, INT64_MAX)
#define OP_U64_TRUNC_SAT_F64(RES, A) OP_TRUNC_SAT(RES, A, UINT64, -1.0, 18446744073709551616.0, 0ULL, UINT64_MAX)


static void _wasm_sext_8_32(UINT *val)
{
    if (*val & 0x80) {
        *val = *val | 0xffffff00;
    }
}

static void _wasm_sext_16_32(UINT *val)
{
    if (*val & 0x8000) {
        *val = *val | 0xffff0000;
    }
}

static void _wasm_sext_8_64(UINT64 *val)
{
    if (*val & 0x80) {
        *val = *val | 0xffffffffffffff00;
    }
}

static void _wasm_sext_16_64(UINT64 *val)
{
    if (*val & 0x8000) {
        *val = *val | 0xffffffffffff0000;
    }
}

static void _wasm_sext_32_64(UINT64 *val)
{
    if (*val & 0x80000000) {
        *val = *val | 0xffffffff00000000;
    }
}


static UINT _wasm_rotl32(UINT n, UINT c)
{
    const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
    c = c % 32;
    c &= mask;
    return (n << c) | (n >> ((-c) & mask));
}


static UINT _wasm_rotr32(UINT n, UINT c)
{
    const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
    c = c % 32;
    c &= mask;
    return (n >> c) | (n << ((-c) & mask));
}


static uint64_t _wasm_rotl64(UINT64 n, UINT c)
{
    const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
    c = c % 64;
    c &= mask;
    return (n << c) | (n >> ((-c) & mask));
}


static uint64_t _wasm_rotr64(UINT64 n, UINT c)
{
    const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
    c = c % 64;
    c &= mask;
    return (n >> c) | (n << ((-c) & mask));
}


static float _wasm_fmaxf(float a, float b)
{
    float c = fmaxf(a, b);
    if (c == 0 && a == b) {
        return signbit(a) ? b : a;
    }
    return c;
}


static float _wasm_fminf(float a, float b)
{
    float c = fminf(a, b);
    if (c == 0 && a == b) {
        return signbit(a) ? a : b;
    }
    return c;
}


static double _wasm_fmax(double a, double b)
{
    double c = fmax(a, b);
    if (c == 0 && a == b) {
        return signbit(a) ? b : a;
    }
    return c;
}


static double _wasm_fmin(double a, double b)
{
    double c = fmin(a, b);
    if (c == 0 && a == b) {
        return signbit(a) ? a : b;
    }
    return c;
}

static WASM_BLOCK_S * _wasm_pop_block(WASM_MODULE_S *m)
{
    WASM_FRAME_S *frame = &m->callstack[m->csp--];
    WASM_TYPE_S *t = frame->block->type;

    if (t->result_count == 1) {
        if (m->stack[m->sp].value_type != t->results[0]) {
            snprintf(m->exception, sizeof(m->exception), "call type mismatch");
            return NULL;
        }
    }

    if (t->result_count == 1) {
        if (frame->sp < m->sp) {
            m->stack[frame->sp + 1] = m->stack[m->sp];
            m->sp = frame->sp + 1;
        }
    } else {
        if (frame->sp < m->sp) {
            m->sp = frame->sp;
        }
    }

    m->fp = frame->fp;

    if (frame->block->block_type == 0x00) {
        m->pc = frame->ra;
    }

    return frame->block;
}

void wasm_setup_call(WASM_MODULE_S *m, UINT fidx)
{
    WASM_BLOCK_S *func = &m->functions[fidx];
    WASM_TYPE_S *type = func->type; 
    UINT lidx;

    wasm_push_block(m, func, m->sp - (int) type->param_count);
    m->fp = m->sp - (int) type->param_count + 1;

    
    for (lidx = 0; lidx < func->local_count; lidx++) {
        m->sp += 1;
        m->stack[m->sp].value_type = func->locals[lidx];
        m->stack[m->sp].value.uint64 = 0;
    }

    m->pc = func->start_addr;
}

void wasm_push_block(WASM_MODULE_S *m, WASM_BLOCK_S *block, int sp)
{
    m->csp += 1;
    m->callstack[m->csp].block = block;
    m->callstack[m->csp].sp = sp;
    m->callstack[m->csp].fp = m->fp;
    m->callstack[m->csp].ra = m->pc;
}


BOOL_T wasm_interpret(WASM_MODULE_S *m)
{
    UCHAR *bytes = m->bytes;
    WASM_STACKVALUE_S *stack = m->stack;   
    UCHAR opcode;
    UINT cur_pc;
    WASM_BLOCK_S *block; 
    UINT cond; 
    UINT depth; 
    UINT fidx; 
    UINT idx;
    UCHAR *maddr; 
    UINT addr; 
    UINT offset;
    UINT a, b, c;
    UINT64 d, e, f;
    float g, h, i; 
    double j, k, l; 

    while (m->pc < m->byte_count) {
        opcode = bytes[m->pc];
        cur_pc = m->pc;
        m->pc += 1;

        switch (opcode) {
            case Unreachable:
                snprintf(m->exception, sizeof(m->exception), "%s", "unreachable");
                return FALSE;
            case Nop:
                continue;
            case Block_:
            case Loop:
                LEB_Read(bytes, &m->pc);
                if (m->csp >= WASM_CALLSTACK_SIZE) {
                    snprintf(m->exception, sizeof(m->exception), "call stack exhausted");
                    return FALSE;
                }
                block = m->block_lookup[cur_pc];
                wasm_push_block(m, block, m->sp);
                continue;
            case If:
                LEB_Read(bytes, &m->pc);
                if (m->csp >= WASM_CALLSTACK_SIZE) {
                    snprintf(m->exception, sizeof(m->exception), "call stack exhausted");
                    return FALSE;
                }
                block = m->block_lookup[cur_pc];
                wasm_push_block(m, block, m->sp);
                cond = stack[m->sp--].value.uint32;
                if (cond == 0) {
                    if (block->else_addr == 0) {
                        m->pc = block->br_addr + 1;
                        m->csp -= 1;
                    } else {
                        m->pc = block->else_addr;
                    }
                }
                continue;
            case Else_:
                block = m->callstack[m->csp].block;
                m->pc = block->br_addr;
                continue;
            case End_:
                block = _wasm_pop_block(m);
                if (block == NULL) {
                    return FALSE;
                }
                if (block->block_type == 0x00) {
                    
                    if (m->csp == -1) {
                        return TRUE;
                    }
                } else if (block->block_type == 0x01) {
                    
                    return TRUE;
                }
                
                continue;
            case Br:
                depth = LEB_Read(bytes, &m->pc);
                m->csp -= (int) depth;
                m->pc = m->callstack[m->csp].block->br_addr;
                continue;
            case BrIf:
                depth = LEB_Read(bytes, &m->pc);
                cond = stack[m->sp--].value.uint32;
                if (cond) {
                    m->csp -= (int) depth;
                    m->pc = m->callstack[m->csp].block->br_addr;
                }
                continue;
            case BrTable: {
                UINT count = LEB_Read(bytes, &m->pc);
                if (count > WASM_BR_TABLE_SIZE) {
                    
                    snprintf(m->exception, sizeof(m->exception), "br_table size %d exceeds max %d\n", count, WASM_BR_TABLE_SIZE);
                    return FALSE;
                }
                for (UINT n = 0; n < count; n++) {
                    m->br_table[n] = LEB_Read(bytes, &m->pc);
                }
                depth = LEB_Read(bytes, &m->pc);
                
                int didx = stack[m->sp--].value.int32;
                if (didx >= 0 && didx < (int32_t) count) {
                    depth = m->br_table[didx];
                }
                m->csp -= (int) depth;
                m->pc = m->callstack[m->csp].block->br_addr;
                continue;
            }
            case Return:
                while (m->csp >= 0 && m->callstack[m->csp].block->block_type != 0x00) {
                    m->csp--;
                }
                m->pc = m->callstack[m->csp].block->end_addr;
                continue;
            case Call:
                fidx = LEB_Read(bytes, &m->pc);
                if (fidx < m->import_func_count) {
                    thunk_out(m, fidx);
                } else {
                    if (m->csp >= WASM_CALLSTACK_SIZE) {
                        snprintf(m->exception, sizeof(m->exception), "call stack exhausted");
                        return FALSE;
                    }
                    wasm_setup_call(m, fidx);
                }
                continue;
            case CallIndirect: {
                UINT tidx = LEB_Read(bytes, &m->pc);
                LEB_Read(bytes, &m->pc);
                UINT val = stack[m->sp--].value.uint32;

                if (m->options.mangle_table_index) {
                    val = val - (UINT)(ULONG)m->table.entries;
                }
                if (val >= m->table.max_size) {
                    snprintf(m->exception, sizeof(m->exception),
                            "undefined element 0x%x (max: 0x%x) in table",
                            val, m->table.max_size);
                    return FALSE;
                }
                fidx = m->table.entries[val];
                if (fidx < m->import_func_count) {
                    thunk_out(m, fidx);
                } else {
                    WASM_BLOCK_S *func = &m->functions[fidx];
                    WASM_TYPE_S *ftype = func->type;
                    if (m->csp >= WASM_CALLSTACK_SIZE) {
                        snprintf(m->exception, sizeof(m->exception), "call stack exhausted");
                        return FALSE;
                    }
                    if (ftype->mask != m->types[tidx].mask) { 
                        snprintf(m->exception, sizeof(m->exception),
                                "indirect call type mismatch (call type and function type differ)");
                        return FALSE;
                    }
                    wasm_setup_call(m, fidx);
                    if (ftype->param_count + func->local_count != m->sp - m->fp + 1) {
                        snprintf(m->exception, sizeof(m->exception),
                                "indirect call type mismatch (param counts differ)");
                        return FALSE;
                    }
                    for (UINT n = 0; n < ftype->param_count; n++) {
                        if (ftype->params[n] != m->stack[m->fp + n].value_type) {
                            snprintf(m->exception, sizeof(m->exception),
                                    "indirect call type mismatch (param types differ)");
                            return FALSE;
                        }
                    }
                }
                continue;
            }
            case Drop:
                m->sp--;
                continue;
            case Select:
                BS_DBGASSERT(stack[m->sp].value_type == WASM_I32);
                cond = stack[m->sp--].value.uint32;
                m->sp--;
                if (!cond) {
                    stack[m->sp] = stack[m->sp + 1];
                }
                continue;
            case LocalGet:
                idx = LEB_Read(bytes, &m->pc);
                
                stack[++m->sp] = stack[m->fp + idx];
                continue;
            case LocalSet:
                idx = LEB_Read(bytes, &m->pc);
                
                stack[m->fp + idx] = stack[m->sp--];
                continue;
            case LocalTee:
                
                idx = LEB_Read(bytes, &m->pc);
                stack[m->fp + idx] = stack[m->sp];
                continue;
            case GlobalGet:
                idx = LEB_Read(bytes, &m->pc);
                stack[++m->sp] = m->globals[idx];
                continue;
            case GlobalSet:
                idx = LEB_Read(bytes, &m->pc);
                m->globals[idx] = stack[m->sp--];
                continue;
            case I32Load ... I64Load32U:
                LEB_Read(bytes, &m->pc);
                offset = LEB_Read(bytes, &m->pc);
                addr = stack[m->sp--].value.uint32;
                maddr = m->memory.bytes + offset + addr;
                
                stack[++m->sp].value.uint64 = 0;
                switch (opcode) {
                    case I32Load:
                        memcpy(&stack[m->sp].value, maddr, 4);
                        stack[m->sp].value_type = WASM_I32;
                        break;

                    case I64Load:
                        memcpy(&stack[m->sp].value, maddr, 8);
                        stack[m->sp].value_type = WASM_I64;
                        break;
                    case F32Load:
                        memcpy(&stack[m->sp].value, maddr, 4);
                        stack[m->sp].value_type = WASM_F32;
                        break;
                    case F64Load:
                        memcpy(&stack[m->sp].value, maddr, 8);
                        stack[m->sp].value_type = WASM_F64;
                        break;
                    case I32Load8S:
                        memcpy(&stack[m->sp].value, maddr, 1);
                        _wasm_sext_8_32(&stack[m->sp].value.uint32);
                        stack[m->sp].value_type = WASM_I32;
                        break;
                    case I32Load8U:
                        memcpy(&stack[m->sp].value, maddr, 1);
                        stack[m->sp].value_type = WASM_I32;
                        break;
                    case I32Load16S:
                        memcpy(&stack[m->sp].value, maddr, 2);
                        _wasm_sext_16_32(&stack[m->sp].value.uint32);
                        stack[m->sp].value_type = WASM_I32;
                        break;
                    case I32Load16U:
                        memcpy(&stack[m->sp].value, maddr, 2);
                        stack[m->sp].value_type = WASM_I32;
                        break;
                    case I64Load8S:
                        memcpy(&stack[m->sp].value, maddr, 1);
                        _wasm_sext_8_64(&stack[m->sp].value.uint64);
                        stack[m->sp].value_type = WASM_I64;
                        break;
                    case I64Load8U:
                        memcpy(&stack[m->sp].value, maddr, 1);
                        stack[m->sp].value_type = WASM_I64;
                        break;
                    case I64Load16S:
                        memcpy(&stack[m->sp].value, maddr, 2);
                        _wasm_sext_16_64(&stack[m->sp].value.uint64);
                        stack[m->sp].value_type = WASM_I64;
                        break;
                    case I64Load16U:
                        memcpy(&stack[m->sp].value, maddr, 2);
                        stack[m->sp].value_type = WASM_I64;
                        break;
                    case I64Load32S:
                        memcpy(&stack[m->sp].value, maddr, 4);
                        _wasm_sext_32_64(&stack[m->sp].value.uint64);
                        stack[m->sp].value_type = WASM_I64;
                        break;
                    case I64Load32U:
                        memcpy(&stack[m->sp].value, maddr, 4);
                        stack[m->sp].value_type = WASM_I64;
                        break;
                    default:
                        break;
                }
                continue;
            case I32Store ... I64Store32:
                LEB_Read(bytes, &m->pc);
                offset = LEB_Read(bytes, &m->pc);
                WASM_STACKVALUE_S *sval = &stack[m->sp--];
                addr = stack[m->sp--].value.uint32;
                maddr = m->memory.bytes + offset + addr;
                
                switch (opcode) {
                    case I32Store:
                        memcpy(maddr, &sval->value.uint32, 4);
                        break;
                    case I64Store:
                        memcpy(maddr, &sval->value.uint64, 8);
                        break;
                    case F32Store:
                        memcpy(maddr, &sval->value.f32, 4);
                        break;
                    case F64Store:
                        memcpy(maddr, &sval->value.f64, 8);
                        break;
                    case I32Store8:
                        memcpy(maddr, &sval->value.uint32, 1);
                        break;
                    case I32Store16:
                        memcpy(maddr, &sval->value.uint32, 2);
                        break;
                    case I64Store8:
                        memcpy(maddr, &sval->value.uint64, 1);
                        break;
                    case I64Store16:
                        memcpy(maddr, &sval->value.uint64, 2);
                        break;
                    case I64Store32:
                        memcpy(maddr, &sval->value.uint64, 4);
                        break;
                    default:
                        break;
                }
                continue;
            case MemorySize:
                LEB_Read(bytes, &m->pc);
                stack[++m->sp].value_type = WASM_I32;
                stack[m->sp].value.uint32 = m->memory.cur_size;
                continue;
            case MemoryGrow:
                LEB_Read(bytes, &m->pc);
                UINT prev_pages = m->memory.cur_size;
                UINT delta = stack[m->sp].value.uint32;
                stack[m->sp].value.uint32 = prev_pages;
                if (delta == 0 || delta + prev_pages > m->memory.max_size) {
                    continue;
                }
                
                m->memory.cur_size += delta;
                m->memory.bytes = MEM_MallocAndCopy(m->memory.bytes,
                        prev_pages * WASM_PAGE_SIZE, m->memory.cur_size * WASM_PAGE_SIZE);
                continue;
            case I32Const:
                stack[++m->sp].value_type = WASM_I32;
                stack[m->sp].value.uint32 = LEB_ReadSigned(bytes, &m->pc);
                continue;
            case I64Const:
                stack[++m->sp].value_type = WASM_I64;
                stack[m->sp].value.int64 = (int64_t) LEB_ReadSigned(bytes, &m->pc);
                continue;
            case F32Const:
                stack[++m->sp].value_type = WASM_F32;
                memcpy(&stack[m->sp].value.uint32, bytes + m->pc, 4);
                m->pc += 4;
                continue;
            case F64Const:
                stack[++m->sp].value_type = WASM_F64;
                memcpy(&stack[m->sp].value.uint64, bytes + m->pc, 8);
                m->pc += 8;
                continue;
            case I32Eqz:
                stack[m->sp].value_type = WASM_I32;
                stack[m->sp].value.uint32 = stack[m->sp].value.uint32 == 0;
                continue;
            case I64Eqz:
                stack[m->sp].value_type = WASM_I32;
                stack[m->sp].value.uint32 = stack[m->sp].value.uint64 == 0;
                continue;
            case I32Eq ... I32GeU:
                a = stack[m->sp - 1].value.uint32;
                b = stack[m->sp].value.uint32;
                m->sp -= 1;
                switch (opcode) {
                    case I32Eq:
                        c = a == b;
                        break;
                    case I32Ne:
                        c = a != b;
                        break;
                    case I32LtS:
                        c = (UINT) a < (UINT) b;
                        break;
                    case I32LtU:
                        c = a < b;
                        break;
                    case I32GtS:
                        c = (UINT) a > (UINT) b;
                        break;
                    case I32GtU:
                        c = a > b;
                        break;
                    case I32LeS:
                        c = (UINT) a <= (UINT) b;
                        break;
                    case I32LeU:
                        c = a <= b;
                        break;
                    case I32GeS:
                        c = (UINT) a >= (UINT) b;
                        break;
                    case I32GeU:
                        c = a >= b;
                        break;
                    default:
                        c = 0;
                        break;
                }
                stack[m->sp].value_type = WASM_I32;
                stack[m->sp].value.uint32 = c;
                continue;
            case I64Eq ... I64GeU:
                d = stack[m->sp - 1].value.uint64;
                e = stack[m->sp].value.uint64;
                m->sp -= 1;
                switch (opcode) {
                    case I64Eq:
                        c = d == e;
                        break;
                    case I64Ne:
                        c = d != e;
                        break;
                    case I64LtS:
                        c = (uint64_t) d < (uint64_t) e;
                        break;
                    case I64LtU:
                        c = d < e;
                        break;
                    case I64GtS:
                        c = (uint64_t) d > (uint64_t) e;
                        break;
                    case I64GtU:
                        c = d > e;
                        break;
                    case I64LeS:
                        c = (uint64_t) d <= (uint64_t) e;
                        break;
                    case I64LeU:
                        c = d <= e;
                        break;
                    case I64GeS:
                        c = (uint64_t) d >= (uint64_t) e;
                        break;
                    case I64GeU:
                        c = d >= e;
                        break;
                    default:
                        c = 0;
                        break;
                }
                stack[m->sp].value_type = WASM_I32;
                stack[m->sp].value.uint32 = c;
                continue;
            case F32Eq ... F32Ge:
                g = stack[m->sp - 1].value.f32;
                h = stack[m->sp].value.f32;
                m->sp -= 1;
                switch (opcode) {
                    case F32Eq:
                        c = g == h;
                        break;
                    case F32Ne:
                        c = g != h;
                        break;
                    case F32Lt:
                        c = g < h;
                        break;
                    case F32Gt:
                        c = g > h;
                        break;
                    case F32Le:
                        c = g <= h;
                        break;
                    case F32Ge:
                        c = g >= h;
                        break;
                    default:
                        c = 0;
                        break;
                }
                stack[m->sp].value_type = WASM_I32;
                stack[m->sp].value.uint32 = c;
                continue;
            case F64Eq ... F64Ge:
                j = stack[m->sp - 1].value.f64;
                k = stack[m->sp].value.f64;
                m->sp -= 1;
                switch (opcode) {
                    case F64Eq:
                        c = j == k;
                        break;
                    case F64Ne:
                        c = j != k;
                        break;
                    case F64Lt:
                        c = j < k;
                        break;
                    case F64Gt:
                        c = j > k;
                        break;
                    case F64Le:
                        c = j <= k;
                        break;
                    case F64Ge:
                        c = j >= k;
                        break;
                    default:
                        c = 0;
                        break;
                }
                stack[m->sp].value_type = WASM_I32;
                stack[m->sp].value.uint32 = c;
                continue;
            case I32Clz ... I32PopCnt:
                a = stack[m->sp].value.uint32;
                switch (opcode) {
                    case I32Clz:
                        c = a == 0 ? 32 : __builtin_clz(a);
                        break;
                    case I32Ctz:
                        c = a == 0 ? 32 : __builtin_ctz(a);
                        break;
                    case I32PopCnt:
                        c = __builtin_popcount(a);
                        break;
                    default:
                        c = 0;
                        break;
                }

                stack[m->sp].value.uint32 = c;
                continue;
            case I32Add ... I32Rotr:
                a = stack[m->sp - 1].value.uint32;
                b = stack[m->sp].value.uint32;
                m->sp -= 1;
                if (opcode >= I32DivS && opcode <= I32RemU && b == 0) {
                    snprintf(m->exception, sizeof(m->exception), "integer divide by zero");
                    return FALSE;
                }
                switch (opcode) {
                    case I32Add:
                        c = a + b;
                        break;
                    case I32Sub:
                        c = a - b;
                        break;
                    case I32Mul:
                        c = a * b;
                        break;
                    case I32DivS:
                        if (a == 0x80000000 && b == -1) {
                            snprintf(m->exception, sizeof(m->exception), "integer overflow");
                            return FALSE;
                        }
                        c = (int32_t) a / (int32_t) b;
                        break;
                    case I32DivU:
                        c = a / b;
                        break;
                    case I32RemS:
                        if (a == 0x80000000 && b == -1) {
                            c = 0;
                        } else {
                            c = (int32_t) a % (int32_t) b;
                        }
                        break;
                    case I32RemU:
                        c = a % b;
                        break;
                    case I32And:
                        c = a & b;
                        break;
                    case I32Or:
                        c = a | b;
                        break;
                    case I32Xor:
                        c = a ^ b;
                        break;
                    case I32Shl:
                        c = a << b;
                        break;
                    case I32ShrS:
                        c = ((int32_t) a) >> b;
                        break;
                    case I32ShrU:
                        c = a >> b;
                        break;
                    case I32Rotl:
                        c = _wasm_rotl32(a, b);
                        break;
                    case I32Rotr:
                        c = _wasm_rotr32(a, b);
                        break;
                    default:
                        c = 0;
                        break;
                }

                stack[m->sp].value.uint32 = c;
                continue;
            case I64Clz ... I64PopCnt:
                d = stack[m->sp].value.uint64;
                switch (opcode) {
                    case I64Clz:
                        f = d == 0 ? 64 : __builtin_clzll(d);
                        break;
                    case I64Ctz:
                        f = d == 0 ? 64 : __builtin_ctzll(d);
                        break;
                    case I64PopCnt:
                        f = __builtin_popcountll(d);
                        break;
                    default:
                        f = 0;
                        break;
                }
                stack[m->sp].value.uint64 = f;
                continue;
            case I64Add ... I64Rotr:
                d = stack[m->sp - 1].value.uint64;
                e = stack[m->sp].value.uint64;
                m->sp -= 1;
                if (opcode >= I64DivS && opcode <= I64RemU && e == 0) {
                    snprintf(m->exception, sizeof(m->exception), "integer divide by zero");
                    return FALSE;
                }
                switch (opcode) {
                    case I64Add:
                        f = d + e;
                        break;
                    case I64Sub:
                        f = d - e;
                        break;
                    case I64Mul:
                        f = d * e;
                        break;
                    case I64DivS:
                        if (d == 0x80000000 && e == -1) {
                            snprintf(m->exception, sizeof(m->exception), "integer overflow");
                            return FALSE;
                        }
                        f = (int64_t) d / (int64_t) e;
                        break;
                    case I64DivU:
                        f = d / e;
                        break;
                    case I64RemS:
                        if (d == 0x80000000 && e == -1) {
                            f = 0;
                        } else {
                            f = (int64_t) d % (int64_t) e;
                        }
                        break;
                    case I64RemU:
                        f = d % e;
                        break;
                    case I64And:
                        f = d & e;
                        break;
                    case I64Or:
                        f = d | e;
                        break;
                    case I64Xor:
                        f = d ^ e;
                        break;
                    case I64Shl:
                        f = d << e;
                        break;
                    case I64ShrS:
                        f = ((int64_t) d) >> e;
                        break;
                    case I64ShrU:
                        f = d >> e;
                        break;
                    case I64Rotl:
                        f = _wasm_rotl64(d, e);
                        break;
                    case I64Rotr:
                        f = _wasm_rotr64(d, e);
                        break;
                    default:
                        f = 0;
                        break;
                }
                stack[m->sp].value.uint64 = f;
                continue;
            case F32Abs:
                stack[m->sp].value.f32 = fabsf(stack[m->sp].value.f32);
                continue;
            case F32Neg:
                stack[m->sp].value.f32 = -stack[m->sp].value.f32;
                continue;
            case F32Ceil:
                stack[m->sp].value.f32 = ceilf(stack[m->sp].value.f32);
                continue;
            case F32Floor:
                stack[m->sp].value.f32 = floorf(stack[m->sp].value.f32);
                continue;
            case F32Trunc:
                stack[m->sp].value.f32 = truncf(stack[m->sp].value.f32);
                continue;
            case F32Nearest:
                stack[m->sp].value.f32 = rintf(stack[m->sp].value.f32);
                continue;
            case F32Sqrt:
                stack[m->sp].value.f32 = sqrtf(stack[m->sp].value.f32);
                continue;
            case F32Add ... F32CopySign:
                g = stack[m->sp - 1].value.f32;
                h = stack[m->sp].value.f32;
                m->sp -= 1;
                switch (opcode) {
                    case F32Add:
                        i = g + h;
                        break;
                    case F32Sub:
                        i = g - h;
                        break;
                    case F32Mul:
                        i = g * h;
                        break;
                    case F32Div:
                        if (h == 0) {
                            snprintf(m->exception, sizeof(m->exception), "integer divide by zero");
                            return FALSE;
                        }
                        i = g / h;
                        break;
                    case F32Min:
                        i = _wasm_fminf(g, h);
                        break;
                    case F32Max:
                        i = _wasm_fmaxf(g, h);
                        break;
                    case F32CopySign:
                        i = signbit(h) ? -fabsf(g) : fabsf(g);
                        break;
                    default:
                        i = 0;
                        break;
                }

                stack[m->sp].value.f32 = i;
                continue;
            case F64Abs:
                stack[m->sp].value.f32 = (float) fabs(stack[m->sp].value.f64);
                continue;
            case F64Neg:
                stack[m->sp].value.f64 = -stack[m->sp].value.f64;
                continue;
            case F64Ceil:
                stack[m->sp].value.f64 = ceil(stack[m->sp].value.f64);
                continue;
            case F64Floor:
                stack[m->sp].value.f64 = floor(stack[m->sp].value.f64);
                continue;
            case F64Trunc:
                stack[m->sp].value.f64 = trunc(stack[m->sp].value.f64);
                continue;
            case F64Nearest:
                stack[m->sp].value.f64 = rint(stack[m->sp].value.f64);
                continue;
            case F64Sqrt:
                stack[m->sp].value.f64 = sqrt(stack[m->sp].value.f64);
                continue;
            case F64Add ... F64CopySign:
                j = stack[m->sp - 1].value.f64;
                k = stack[m->sp].value.f64;
                m->sp -= 1;

                switch (opcode) {
                    case F64Add:
                        l = j + k;
                        break;
                    case F64Sub:
                        l = j - k;
                        break;
                    case F64Mul:
                        l = j * k;
                        break;
                    case F64Div:
                        if (k == 0) {
                            snprintf(m->exception, sizeof(m->exception), "integer divide by zero");
                            return FALSE;
                        }
                        l = j / k;
                        break;
                    case F64Min:
                        l = _wasm_fmin(j, k);
                        break;
                    case F64Max:
                        l = _wasm_fmax(j, k);
                        break;
                    case F64CopySign:
                        l = signbit(k) ? -fabs(j) : fabs(j);
                        break;
                    default:
                        l = 0;
                        break;
                }

                stack[m->sp].value.f64 = l;
                continue;
            case I32WrapI64:
                stack[m->sp].value.uint64 &= 0x00000000ffffffff;
                stack[m->sp].value_type = WASM_I32;
                continue;
            case I32TruncF32S:
                OP_I32_TRUNC_F32(stack[m->sp].value.int32, stack[m->sp].value.f32)
                stack[m->sp].value_type = WASM_I32;
                continue;
            case I32TruncF32U:
                OP_U32_TRUNC_F32(stack[m->sp].value.uint32, stack[m->sp].value.f32)
                stack[m->sp].value_type = WASM_I32;
                continue;
            case I32TruncF64S:
                OP_I32_TRUNC_F64(stack[m->sp].value.int32, stack[m->sp].value.f64)
                stack[m->sp].value_type = WASM_I32;
                continue;
            case I32TruncF64U:
                OP_U32_TRUNC_F64(stack[m->sp].value.uint32, stack[m->sp].value.f64)
                stack[m->sp].value_type = WASM_I32;
                continue;
            case I64ExtendI32S:
                stack[m->sp].value.uint64 = stack[m->sp].value.uint32;
                _wasm_sext_32_64(&stack[m->sp].value.uint64);
                stack[m->sp].value_type = WASM_I64;
                continue;
            case I64ExtendI32U:
                stack[m->sp].value.uint64 = stack[m->sp].value.uint32;
                stack[m->sp].value_type = WASM_I64;
                continue;
            case I64TruncF32S:
                OP_I64_TRUNC_F32(stack[m->sp].value.int64, stack[m->sp].value.f32)
                stack[m->sp].value_type = WASM_I64;
                continue;
            case I64TruncF32U:
                OP_U64_TRUNC_F32(stack[m->sp].value.uint64, stack[m->sp].value.f32)
                stack[m->sp].value_type = WASM_I64;
                continue;
            case I64TruncF64S:
                OP_I64_TRUNC_F64(stack[m->sp].value.int64, stack[m->sp].value.f64)
                stack[m->sp].value_type = WASM_I64;
                continue;
            case I64TruncF64U:
                OP_U64_TRUNC_F64(stack[m->sp].value.uint64, stack[m->sp].value.f64)
                stack[m->sp].value_type = WASM_I64;
                continue;
            case F32ConvertI32S:
                stack[m->sp].value.f32 = (float) stack[m->sp].value.int32;
                stack[m->sp].value_type = WASM_F32;
                continue;
            case F32ConvertI32U:
                stack[m->sp].value.f32 = (float) stack[m->sp].value.uint32;
                stack[m->sp].value_type = WASM_F32;
                continue;
            case F32ConvertI64S:
                stack[m->sp].value.f32 = (float) stack[m->sp].value.int64;
                stack[m->sp].value_type = WASM_F32;
                continue;
            case F32ConvertI64U:
                stack[m->sp].value.f32 = (float) stack[m->sp].value.uint64;
                stack[m->sp].value_type = WASM_F32;
                continue;
            case F32DemoteF64:
                stack[m->sp].value.f32 = (float) stack[m->sp].value.f64;
                stack[m->sp].value_type = WASM_F32;
                continue;
            case F64ConvertI32S:
                stack[m->sp].value.f64 = stack[m->sp].value.int32;
                stack[m->sp].value_type = WASM_F64;
                continue;
            case F64ConvertI32U:
                stack[m->sp].value.f64 = stack[m->sp].value.uint32;
                stack[m->sp].value_type = WASM_F64;
                continue;
            case F64ConvertI64S:
                stack[m->sp].value.f64 = (double) stack[m->sp].value.int64;
                stack[m->sp].value_type = WASM_F64;
                continue;
            case F64ConvertI64U:
                stack[m->sp].value.f64 = (double) stack[m->sp].value.uint64;
                stack[m->sp].value_type = WASM_F64;
                continue;
            case F64PromoteF32:
                stack[m->sp].value.f64 = stack[m->sp].value.f32;
                stack[m->sp].value_type = WASM_F64;
                continue;
            case I32ReinterpretF32:
                stack[m->sp].value_type = WASM_I32;
                continue;
            case I64ReinterpretF64:
                stack[m->sp].value_type = WASM_I64;
                continue;
            case F32ReinterpretI32:
                stack[m->sp].value_type = WASM_F32;
                continue;
            case F64ReinterpretI64:
                stack[m->sp].value_type = WASM_F64;
                continue;
            case I32Extend8S:
                stack[m->sp].value.int32 = ((int32_t) (int8_t) stack[m->sp].value.int32);
                continue;
            case I32Extend16S:
                stack[m->sp].value.int32 = ((int32_t) (int16_t) stack[m->sp].value.int32);
                continue;
            case I64Extend8S:
                stack[m->sp].value.int64 = ((int64_t) (int8_t) stack[m->sp].value.int64);
                continue;
            case I64Extend16S:
                stack[m->sp].value.int64 = ((int64_t) (int16_t) stack[m->sp].value.int64);
                continue;
            case I64Extend32S:
                stack[m->sp].value.int64 = ((int64_t) (int32_t) stack[m->sp].value.int64);
                continue;
            case TruncSat: {
                UCHAR type = LEB_Read(bytes, &m->pc);
                switch (type) {
                    case 0x00:
                        OP_I32_TRUNC_SAT_F32(stack[m->sp].value.int32, stack[m->sp].value.f32)
                        stack[m->sp].value_type = WASM_I32;
                        break;
                    case 0x01:
                        OP_U32_TRUNC_SAT_F32(stack[m->sp].value.uint32, stack[m->sp].value.f32)
                        stack[m->sp].value_type = WASM_I32;
                        break;
                    case 0x02:
                        OP_I32_TRUNC_SAT_F64(stack[m->sp].value.int32, stack[m->sp].value.f64)
                        stack[m->sp].value_type = WASM_I32;
                        break;
                    case 0x03:
                        OP_U32_TRUNC_SAT_F64(stack[m->sp].value.uint32, stack[m->sp].value.f64)
                        stack[m->sp].value_type = WASM_I32;
                        break;
                    case 0x04:
                        OP_I64_TRUNC_SAT_F32(stack[m->sp].value.int64, stack[m->sp].value.f32)
                        stack[m->sp].value_type = WASM_I64;
                        break;
                    case 0x05:
                        OP_U64_TRUNC_SAT_F32(stack[m->sp].value.uint64, stack[m->sp].value.f32)
                        stack[m->sp].value_type = WASM_I64;
                        break;
                    case 0x06:
                        OP_I64_TRUNC_SAT_F64(stack[m->sp].value.int64, stack[m->sp].value.f64)
                        stack[m->sp].value_type = WASM_I64;
                        break;
                    case 0x07:
                        OP_U64_TRUNC_SAT_F64(stack[m->sp].value.uint64, stack[m->sp].value.f64)
                        stack[m->sp].value_type = WASM_I64;
                        break;
                    default:
                        break;
                }
                continue;
            }
            default:
                return FALSE;
        }
    }

    return FALSE;
}

BOOL_T WASM_Run(WASM_MODULE_S *m, UINT fidx)
{
    wasm_setup_call(m, fidx);
    return wasm_interpret(m);
}

WASM_STACKVALUE_S * WASM_StackGet(WASM_MODULE_S *m, int off)
{
    int sp = m->sp + off;
    if (sp < 0) {
        return NULL;
    }
    return &m->stack[sp];
}

WASM_STACKVALUE_S * WASM_StackPop(WASM_MODULE_S *m)
{
    if (m->sp < 0) {
        return NULL;
    }
    m->sp --;
    return &m->stack[m->sp + 1];
}

int WASM_StackPush(WASM_MODULE_S *m, IN WASM_STACKVALUE_S *v)
{
    if ((m->sp + 1) >= WASM_STACK_SIZE) {
        return -1;
    }

    m->sp++;
    m->stack[m->sp] = *v;

    return 0;
}

int WASM_StackPushValue(WASM_MODULE_S *m, UCHAR type, UINT64 value)
{
    WASM_STACKVALUE_S v;

    v.value_type = type;

    switch (type) {
        case WASM_I32:
            v.value.int32 = value;
            break;
        case WASM_I64:
            v.value.int64 = value;
            break;
        case WASM_F32:
            v.value.f32 = value;
            break;
        case WASM_F64:
            v.value.f64 = value;
            break;
    }

    return WASM_StackPush(m, &v);
}

char * WASM_GetException(WASM_MODULE_S *m)
{
    return m->exception;
}

