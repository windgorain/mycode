/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "pcap.h"
#include "utl/ubpf_utl.h"
#include "ubpf_int.h"
#include "utl/ubpf/ebpf.h"

enum reg {
	R0 = 0, 
	R1 = 1, 
	R2 = 2, 
	R3 = 3, 
	R4 = 4, 
	R5 = 5, 
	R6 = 6,
	R7 = 7,
	R8 = 8,
	R9 = 9,
	R10 = 10, 

	
	REG_A = R0,
	REG_X = R6,
	REG_TMP = R7,
};


#define BPF_MOD 0x90
#define BPF_XOR 0xa0

#define UBPF_INST 4096

static int ubpf_c2e(void *in, uint32_t in_len, void *out, uint32_t *out_len)
{
	const struct bpf_insn *i = in;
	struct ebpf_inst *o_ins = out;
	uint32_t i_len = in_len / sizeof(struct bpf_insn);
	uint32_t o_len = 0, l;
	uint32_t o_ins_order[UBPF_INST + 4];
	uint32_t i_ins_count[UBPF_INST + 4];
	uint32_t i_ins_count_acc[UBPF_INST + 4];

	if (i_len > UBPF_INST) {
		return -1;
	}

#define EMMIT_CODE_RAW(o, d, s, off, im) do \
{\
	if (o_len > UBPF_INST) { \
		return -1; \
	} \
	o_ins->opcode = (o); \
	o_ins->dst = (d); \
	o_ins->src = (s); \
	o_ins->offset = (off); \
	o_ins->imm = (im); \
	++o_ins; \
	++o_len; \
} while(0)

	o_ins_order[o_len] = 0;
	EMMIT_CODE_RAW(EBPF_OP_MOV_IMM, REG_A, 0, 0, 0);
	o_ins_order[o_len] = 0;
	EMMIT_CODE_RAW(EBPF_OP_MOV_IMM, REG_X, 0, 0, 0);
	i_ins_count[0] = 2;
	i_ins_count_acc[0] = 0;

	for (l = 0; l < i_len; ++l) {

		
#define EMMIT_CODE(o, d, s, off, im) do \
{\
	o_ins_order[o_len] = l + 1;\
	++i_ins_count[l + 1]; \
	EMMIT_CODE_RAW(o, d, s, off, im); \
} while (0)
		const struct bpf_insn *n = i + l;
		i_ins_count[l + 1] = 0;
		i_ins_count_acc[l + 1] = i_ins_count_acc[l] + i_ins_count[l];
		switch (BPF_CLASS(n->code)) {
		case BPF_RET:
			switch (BPF_RVAL(n->code)) {
			case BPF_A:
				
				
				EMMIT_CODE(EBPF_OP_EXIT, 0, 0, 0, 0);
				break;
			case BPF_K:
				EMMIT_CODE(EBPF_OP_MOV_IMM, R0, 0, 0, (uint32_t)n->k);
				EMMIT_CODE(EBPF_OP_EXIT, 0, 0, 0, 0);
				break;
			default:
				return -1;
			}
			break;
		case BPF_LD: {
			uint32_t size_mode = BPF_SIZE(n->code) << 8 | BPF_MODE(n->code);
			switch (size_mode) {
			
			case (BPF_W << 8 | BPF_ABS):
				EMMIT_CODE(EBPF_OP_LDXW, REG_A, R1, ((uint16_t)n->k), 0);
				EMMIT_CODE(EBPF_OP_BE, REG_A, 0, 0, 32);
				break;
			case (BPF_H << 8 | BPF_ABS):
				EMMIT_CODE(EBPF_OP_LDXH, REG_A, R1, ((uint16_t)n->k), 0);
				EMMIT_CODE(EBPF_OP_BE, REG_A, 0, 0, 16);
				break;
			case (BPF_B << 8 | BPF_ABS):
				EMMIT_CODE(EBPF_OP_LDXB, REG_A, R1, ((uint16_t)n->k), 0);
				break;

			
			
			
			case (BPF_W << 8 | BPF_IND):
				EMMIT_CODE(EBPF_OP_MOV64_REG, REG_TMP, R1, 0, 0);
				EMMIT_CODE(EBPF_OP_ADD64_REG, REG_TMP, REG_X, 0, 0);
				EMMIT_CODE(EBPF_OP_LDXW, REG_A, REG_TMP, (uint16_t)n->k, 0);
				EMMIT_CODE(EBPF_OP_BE, REG_A, 0, 0, 32);
				break;
			case (BPF_H << 8 | BPF_IND):
				EMMIT_CODE(EBPF_OP_MOV64_REG, REG_TMP, R1, 0, 0);
				EMMIT_CODE(EBPF_OP_ADD64_REG, REG_TMP, REG_X, 0, 0);
				EMMIT_CODE(EBPF_OP_LDXH, REG_A, REG_TMP, (uint16_t)n->k, 0);
				EMMIT_CODE(EBPF_OP_BE, REG_A, 0, 0, 16);
				break;
			case (BPF_B << 8 | BPF_IND):
				EMMIT_CODE(EBPF_OP_MOV64_REG, REG_TMP, R1, 0, 0);
				EMMIT_CODE(EBPF_OP_ADD64_REG, REG_TMP, REG_X, 0, 0);
				EMMIT_CODE(EBPF_OP_LDXB, REG_A, REG_TMP, (uint16_t)n->k, 0);
				break;
			case (BPF_W << 8 | BPF_LEN):
				
				EMMIT_CODE(EBPF_OP_MOV_REG, REG_A, R2, 0, 0);
				break;
			case (BPF_W << 8 | BPF_IMM):
				EMMIT_CODE(EBPF_OP_MOV_IMM, REG_A, 0, 0, (uint32_t)n->k);
				break;
			case (BPF_W << 8 | BPF_MEM):
				EMMIT_CODE(EBPF_OP_LDXW, REG_A, R10, -(1 + (uint16_t)n->k) * 4, 0);
				break;
			default:
				return -1;
			}
			break;
		}
		case BPF_LDX: {
			uint32_t size_mode = BPF_SIZE(n->code) << 8 | BPF_MODE(n->code);
			switch (size_mode) {
			case (BPF_W << 8 | BPF_LEN):
				
				EMMIT_CODE(EBPF_OP_MOV_REG, REG_X, R2, 0, 0);
				break;
			case (BPF_B << 8 | BPF_MSH):
				EMMIT_CODE(EBPF_OP_LDXB, REG_X, R1, (uint16_t)n->k, 0);
				EMMIT_CODE(EBPF_OP_AND_IMM, REG_X, 0, 0, 0xf);
				EMMIT_CODE(EBPF_OP_LSH_IMM, REG_X, 0, 0, 2);
				break;
			case (BPF_W << 8 | BPF_IMM):
				EMMIT_CODE(EBPF_OP_MOV_IMM, REG_X, 0, 0, (uint32_t)n->k);
				break;
			case (BPF_W << 8 | BPF_MEM):
				EMMIT_CODE(EBPF_OP_LDXW, REG_X, R10, -(1 + (uint16_t)n->k) * 4, 0);
				break;
			default:
				return -1;
			}
			break;
		}
		case BPF_ST:
			EMMIT_CODE(EBPF_OP_STXW, R10, REG_A, -(1 + (uint16_t)n->k) * 4, 0);
			break;
		case BPF_STX:
			EMMIT_CODE(EBPF_OP_STXW, R10, REG_X, -(1 + (uint16_t)n->k) * 4, 0);
			break;
		case BPF_JMP:
			if (BPF_OP(n->code) == BPF_JA) {
				EMMIT_CODE(EBPF_OP_JA, 0, 0, (uint16_t)n->k, 0);
			} else {
				uint8_t opc;
				switch (BPF_OP(n->code)) {
				case BPF_JGT:
					opc = EBPF_OP_JGT_REG;
					break;
				case BPF_JGE:
					opc = EBPF_OP_JGE_REG;
					break;
				case BPF_JEQ:
					opc = EBPF_OP_JEQ_REG;
					break;
				case BPF_JSET:
					opc = EBPF_OP_JSET_REG;
					break;
				default:
					return -2;
				}
				switch (BPF_SRC(n->code)) {
				case BPF_K:
					
					
					EMMIT_CODE(EBPF_OP_MOV_IMM, REG_TMP, 0, 0, (uint32_t)n->k);
					EMMIT_CODE(opc, REG_A, REG_TMP, (uint16_t)n->jt, 0);
					if (n->jf) {
						EMMIT_CODE(EBPF_OP_JA, 0, 0, (uint16_t)n->jf, 0);
					}
					break;
				case BPF_X:
					EMMIT_CODE(opc, REG_A, REG_X, (uint16_t)n->jt, 0);
					EMMIT_CODE(EBPF_OP_JA, 0, 0, (uint16_t)n->jf, 0);
					break;
				default:
					return -1;
				}
			}
			break;
		case BPF_ALU:
			if (BPF_OP(n->code) == BPF_NEG) {
				EMMIT_CODE(EBPF_OP_NEG, REG_A, 0, 0, 0);
			} else {
				switch (BPF_SRC(n->code)) {
				case BPF_K: {
					uint8_t opc;
					switch (BPF_OP(n->code)) {
					case BPF_ADD:
						opc = EBPF_OP_ADD_IMM;
						break;
					case BPF_SUB:
						opc = EBPF_OP_SUB_IMM;
						break;
					case BPF_MUL:
						opc = EBPF_OP_MUL_IMM;
						break;
					case BPF_DIV:
						opc = EBPF_OP_DIV_IMM;
						break;
					case BPF_MOD:
						opc = EBPF_OP_MOD_IMM;
						break;
					case BPF_OR:
						opc = EBPF_OP_OR_IMM;
						break;
					case BPF_AND:
						opc = EBPF_OP_AND_IMM;
						break;
					case BPF_XOR:
						opc = EBPF_OP_XOR_IMM;
						break;
					case BPF_LSH:
						opc = EBPF_OP_LSH_IMM;
						break;
					case BPF_RSH:
						opc = EBPF_OP_RSH_IMM;
						break;
					default:
						return -3;
					}
					EMMIT_CODE(opc, REG_A, 0, 0, (uint32_t)n->k);
					break;
				}
				case BPF_X: {
					uint8_t opc;
					switch (BPF_OP(n->code)) {
					case BPF_ADD:
						opc = EBPF_OP_ADD_REG;
						break;
					case BPF_SUB:
						opc = EBPF_OP_SUB_REG;
						break;
					case BPF_MUL:
						opc = EBPF_OP_MUL_REG;
						break;
					case BPF_DIV:
						opc = EBPF_OP_DIV_REG;
						break;
					case BPF_MOD:
						opc = EBPF_OP_MOD_REG;
						break;
					case BPF_OR:
						opc = EBPF_OP_OR_REG;
						break;
					case BPF_AND:
						opc = EBPF_OP_AND_REG;
						break;
					case BPF_XOR:
						opc = EBPF_OP_XOR_REG;
						break;
					case BPF_LSH:
						opc = EBPF_OP_LSH_REG;
						break;
					case BPF_RSH:
						opc = EBPF_OP_RSH_REG;
						break;
					default:
						return -3;
					}
					EMMIT_CODE(opc, REG_A, REG_X, 0, 0);
					break;
				}
				default:
					return -1;
				}
			}
			break;
		case BPF_MISC:
			switch (BPF_MISCOP(n->code)) {
			case BPF_TAX:
				EMMIT_CODE(EBPF_OP_MOV_REG, REG_X, REG_A, 0, 0);
				break;
			case BPF_TXA:
				EMMIT_CODE(EBPF_OP_MOV_REG, REG_A, REG_X, 0, 0);
				break;
			default:
				return -3;
			}
			break;
		default:
			return -1;
		}
	}
	i_ins_count_acc[i_len + 1] = i_ins_count_acc[i_len] + i_ins_count[i_len];

	
	for (l = 0; l < o_len; ++l) {
		o_ins = ((struct ebpf_inst *)out) + l;
		if ((o_ins->opcode & EBPF_CLS_MASK) == EBPF_CLS_JMP) {
			if (i_len >= o_ins_order[l] + o_ins->offset) {
				o_ins->offset = i_ins_count_acc[o_ins_order[l] + o_ins->offset + 1] 
					- l 
					- 1;
			} else {
				return -4;
			}
		}
	}
	*out_len = o_len * sizeof(struct ebpf_inst);
	return 0;
}


UBPF_VM_HANDLE UBPF_C2e(void *bpfprog)
{
	unsigned ebpf_buffer[UBPF_INST];
	uint32_t ebpf_len;
    struct bpf_program *bpf_prog = bpfprog;

    if(ubpf_c2e(bpf_prog->bf_insns, bpf_prog->bf_len * sizeof(struct bpf_insn),
                ebpf_buffer, &ebpf_len) < 0) {
        return NULL;
	}	

    return UBPF_CreateLoad(ebpf_buffer, ebpf_len, NULL, 0);
}


ubpf_jit_fn UBPF_C2j(void *bpfprog, OUT UBPF_JIT_S *jit)
{
    struct bpf_program *bpf_prog = bpfprog;
    UBPF_VM_HANDLE vm = UBPF_C2e(bpf_prog);
    if (NULL == vm) {
        return NULL;
    }

    ubpf_jit_fn fn = UBPF_E2j(vm);
    if (fn == NULL) {
        UBPF_Destroy(vm);
        return NULL;
    }

    jit->vm = vm;
    jit->jit_func = fn;

    return fn;
}

ubpf_jit_fn UBPF_E2j(UBPF_VM_HANDLE vm)
{
	char* errmsg = NULL;
    ubpf_jit_fn fn;

	fn = ubpf_compile(vm, &errmsg);
    if (errmsg) {
        free(errmsg);
    }

    return fn;
}

ubpf_jit_fn UBPF_GetJittedFunc(UBPF_VM_HANDLE vm)
{
    struct ubpf_vm *uvm = vm;
    return uvm->jitted;
}

int UBPF_GetJittedSize(UBPF_VM_HANDLE vm)
{
    struct ubpf_vm *uvm = vm;
    return uvm->jitted_size;
}

UBPF_VM_HANDLE UBPF_Create()
{
    return ubpf_create();
}

void UBPF_Destroy(UBPF_VM_HANDLE vm)
{
    ubpf_destroy(vm);
}

int UBPF_Load(UBPF_VM_HANDLE vm, void *ebpf_code, int ebpf_len)
{
	char* errmsg = NULL;
    int ret;

	ret = ubpf_load(vm, ebpf_code, ebpf_len, &errmsg);
    if (errmsg) {
        printf("%s \r\n", errmsg);
        free(errmsg);
    }

    return ret;
}

UBPF_VM_HANDLE UBPF_CreateLoad(void *ebpf_code, int ebpf_len, void **funcs, int funcs_count)
{
    UBPF_VM_HANDLE vm;
    int ret;
    int i;

    vm = UBPF_Create();
    if (! vm) {
        return NULL;
    }

    for (i=0; i<funcs_count; i++) {
        if (funcs[i]) {
            ubpf_register(vm, i, NULL, funcs[i]);
        }
    }

    ret = UBPF_Load(vm, ebpf_code, ebpf_len);
    if (ret < 0) {
        UBPF_Destroy(vm);
        return NULL;
    }

    return vm;
}


int BPF_Check(void *insts, int num_insts, OUT char * error_msg, int error_msg_size)
{
    struct ebpf_inst *insn = insts;
    int i;

    for (i = 0; i < num_insts; i++) {
        struct ebpf_inst inst = insn[i];
        switch (inst.opcode) {
            case EBPF_OP_LDDW:
                if (i + 1 >= num_insts || insn[i+1].opcode != 0) {
                    scnprintf(error_msg, error_msg_size, "incomplete lddw at PC %d", i);
                    RETURN(BS_ERR);
                }
                i++;
                break;
            case EBPF_OP_CALL:
                if (inst.imm < 0) {
                    scnprintf(error_msg, error_msg_size, "invalid call immediate at PC %d", i);
                    RETURN(BS_ERR);
                }
                break;
        }
    }

    return 0;
}

