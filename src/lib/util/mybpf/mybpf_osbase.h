/*================================================================
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


#define BPF_REG_A	BPF_REG_0
#define BPF_REG_X	BPF_REG_7
#define BPF_REG_TMP	BPF_REG_2	
#define BPF_REG_D	BPF_REG_8	
#define BPF_REG_H	BPF_REG_9	


#define BPF_REG_AX		MAX_BPF_REG
#define MAX_BPF_EXT_REG		(MAX_BPF_REG + 1)
#define MAX_BPF_JIT_REG		MAX_BPF_EXT_REG


#ifndef __bpf_call_base_args 
#define __bpf_call_base_args \
	((U64 (*)(U64, U64, U64, U64, U64, const MYBPF_INSN_S *)) \
	 __bpf_call_base)
#endif


#define BPF_JMP32	0x06	

#define BPF_JNE		0x50	
#define BPF_JLT		0xa0	
#define BPF_JLE		0xb0	
#define BPF_JSGT	0x60	
#define BPF_JSGE	0x70	
#define BPF_JSLT	0xc0	
#define BPF_JSLE	0xd0	
#define BPF_CALL	0x80	
#define BPF_EXIT	0x90	

#define BPF_REG_AX		MAX_BPF_REG
#define MAX_BPF_EXT_REG		(MAX_BPF_REG + 1)

#ifndef MAX_BPF_JIT_REG		
#define MAX_BPF_JIT_REG		MAX_BPF_EXT_REG
#endif

#define BPF_TAIL_CALL	0xf0
#define BPF_CALL_ARGS	0xe0
#define BPF_NOSPEC	0xc0
#define BPF_PROBE_MEM	0x20

#define BPF_FETCH	0x01	
#define BPF_XCHG	(0xe0 | BPF_FETCH)	
#define BPF_CMPXCHG	(0xf0 | BPF_FETCH)	

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
	((MYBPF_INSN_S) {					\
		.opcode  = BPF_LDX | BPF_SIZE(SIZE) | BPF_MEM,	\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = 0 })

enum bpf_reg_type {
	NOT_INIT = 0,		 
	SCALAR_VALUE,		 
	PTR_TO_CTX,		 
	CONST_PTR_TO_MAP,	 
	PTR_TO_MAP_VALUE,	 
	PTR_TO_MAP_KEY,		 
	PTR_TO_STACK,		 
	PTR_TO_PACKET_META,	 
	PTR_TO_PACKET,		 
	PTR_TO_PACKET_END,	 
	PTR_TO_FLOW_KEYS,	 
	PTR_TO_SOCKET,		 
	PTR_TO_SOCK_COMMON,	 
	PTR_TO_TCP_SOCK,	 
	PTR_TO_TP_BUFFER,	 
	PTR_TO_XDP_SOCK,	 
	
	PTR_TO_BTF_ID,
	
	PTR_TO_MEM,		 
	PTR_TO_BUF,		 
	PTR_TO_PERCPU_BTF_ID,	 
	PTR_TO_FUNC,		 
	__BPF_REG_TYPE_MAX,

#define	PTR_MAYBE_NULL (0x100)
#define MEM_RDONLY	   (0x200)
#define BPF_TYPE_LIMIT		(MEM_RDONLY	| (MEM_RDONLY-1)) 

	
	PTR_TO_MAP_VALUE_OR_NULL	= PTR_MAYBE_NULL | PTR_TO_MAP_VALUE,
	PTR_TO_SOCKET_OR_NULL		= PTR_MAYBE_NULL | PTR_TO_SOCKET,
	PTR_TO_SOCK_COMMON_OR_NULL	= PTR_MAYBE_NULL | PTR_TO_SOCK_COMMON,
	PTR_TO_TCP_SOCK_OR_NULL		= PTR_MAYBE_NULL | PTR_TO_TCP_SOCK,
	PTR_TO_BTF_ID_OR_NULL		= PTR_MAYBE_NULL | PTR_TO_BTF_ID,

	
	__BPF_REG_TYPE_LIMIT	= BPF_TYPE_LIMIT,
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
#endif 
