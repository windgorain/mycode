/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _MYBPF_OSBASE_H
#define _MYBPF_OSBASE_H

#include "os/linux_kernel_pile.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Register numbers */
enum {
	BPF_REG_0 = 0,
	BPF_REG_1,
	BPF_REG_2,
	BPF_REG_3,
	BPF_REG_4,
	BPF_REG_5,
	BPF_REG_6,
	BPF_REG_7,
	BPF_REG_8,
	BPF_REG_9,
	BPF_REG_10,
	__MAX_BPF_REG,
};

#define MAX_BPF_REG	__MAX_BPF_REG


#define BPF_REG_ARG1	BPF_REG_1
#define BPF_REG_ARG2	BPF_REG_2
#define BPF_REG_ARG3	BPF_REG_3
#define BPF_REG_ARG4	BPF_REG_4
#define BPF_REG_ARG5	BPF_REG_5
#define BPF_REG_CTX	BPF_REG_6
#define BPF_REG_FP	BPF_REG_10

/* Additional register mappings for converted user programs. */
#define BPF_REG_A	BPF_REG_0
#define BPF_REG_X	BPF_REG_7
#define BPF_REG_TMP	BPF_REG_2	/* scratch reg */
#define BPF_REG_D	BPF_REG_8	/* data, callee-saved */
#define BPF_REG_H	BPF_REG_9	/* hlen, callee-saved */

/* Kernel hidden auxiliary/helper register. */
#define BPF_REG_AX		MAX_BPF_REG
#define MAX_BPF_EXT_REG		(MAX_BPF_REG + 1)
#define MAX_BPF_JIT_REG		MAX_BPF_EXT_REG


#ifndef __bpf_call_base_args 
#define __bpf_call_base_args \
	((U64 (*)(U64, U64, U64, U64, U64, const struct bpf_insn *)) \
	 __bpf_call_base)
#endif

/* instruction classes */
#define BPF_JMP32	0x06	/* jmp mode in word width */

#define BPF_JNE		0x50	/* jump != */
#define BPF_JLT		0xa0	/* LT is unsigned, '<' */
#define BPF_JLE		0xb0	/* LE is unsigned, '<=' */
#define BPF_JSGT	0x60	/* SGT is signed '>', GT in x86 */
#define BPF_JSGE	0x70	/* SGE is signed '>=', GE in x86 */
#define BPF_JSLT	0xc0	/* SLT is signed, '<' */
#define BPF_JSLE	0xd0	/* SLE is signed, '<=' */
#define BPF_CALL	0x80	/* function call */
#define BPF_EXIT	0x90	/* function return */

#define BPF_REG_AX		MAX_BPF_REG
#define MAX_BPF_EXT_REG		(MAX_BPF_REG + 1)

#ifndef MAX_BPF_JIT_REG		
#define MAX_BPF_JIT_REG		MAX_BPF_EXT_REG
#endif

#define BPF_TAIL_CALL	0xf0
#define BPF_CALL_ARGS	0xe0
#define BPF_NOSPEC	0xc0
#define BPF_PROBE_MEM	0x20

#define BPF_FETCH	0x01	/* not an opcode on its own, used to build others */
#define BPF_XCHG	(0xe0 | BPF_FETCH)	/* atomic exchange */
#define BPF_CMPXCHG	(0xf0 | BPF_FETCH)	/* atomic compare-and-write */

#define BPF_PSEUDO_CALL		1

#define FIELD_SIZEOF(t, f) (sizeof(((t*)0)->f))

#define bytes_to_bpf_size(bytes)				\
({								\
	int bpf_size = -EINVAL;					\
								\
	if (bytes == sizeof(U8))				\
		bpf_size = BPF_B;				\
	else if (bytes == sizeof(U16))				\
		bpf_size = BPF_H;				\
	else if (bytes == sizeof(U32))				\
		bpf_size = BPF_W;				\
	else if (bytes == sizeof(U64))				\
		bpf_size = BPF_DW;				\
								\
	bpf_size;						\
})

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

#define BPF_FIELD_SIZEOF(type, field)				\
	({							\
		const int __size = bytes_to_bpf_size(FIELD_SIZEOF(type, field)); \
		BUILD_BUG_ON(__size < 0);			\
		__size;						\
	})

#define BPF_LDX_MEM(SIZE, DST, SRC, OFF)			\
	((struct bpf_insn) {					\
		.code  = BPF_LDX | BPF_SIZE(SIZE) | BPF_MEM,	\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = 0 })

#if 0
struct bpf_insn {
	UCHAR code;		/* opcode */
	UCHAR dst_reg:4;	/* dest register */
	UCHAR src_reg:4;	/* source register */
	SHORT off;		/* signed offset */
	INT imm;		/* signed immediate constant */
};
#endif

struct xdp_md {
	UINT data;
	UINT data_end;
	UINT data_meta;
	/* Below access go through struct xdp_rxq_info */
	UINT ingress_ifindex; /* rxq->dev->ifindex */
	UINT rx_queue_index;  /* rxq->queue_index  */
	UINT egress_ifindex;  /* txq->dev->ifindex */
};

enum xdp_action {
	XDP_ABORTED = 0,
	XDP_DROP,
	XDP_PASS,
	XDP_TX,
	XDP_REDIRECT,
};

enum bpf_reg_type {
	NOT_INIT = 0,		 /* nothing was written into register */
	UNKNOWN_VALUE,		 /* reg doesn't contain a valid pointer */
	PTR_TO_CTX,		 /* reg points to bpf_context */
	CONST_PTR_TO_MAP,	 /* reg points to struct bpf_map */
	PTR_TO_MAP_VALUE,	 /* reg points to map element value */
	PTR_TO_MAP_VALUE_OR_NULL,/* points to map elem value or NULL */
	FRAME_PTR,		 /* reg == frame_pointer */
	PTR_TO_STACK,		 /* reg == frame_pointer + imm */
	CONST_IMM,		 /* constant integer value */
};

struct reg_state {
	enum bpf_reg_type type;
	union {
		int imm;
		void *map_ptr;
	};
};

#ifdef __cplusplus
}
#endif
#endif //MYBPF_OSBASE_H_
