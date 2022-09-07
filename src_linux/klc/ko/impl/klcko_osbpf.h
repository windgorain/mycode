/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/* Copyright (c) 2011-2014 PLUMgrid, http://plumgrid.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 */
#ifndef _KLCKO_OSBPF_H__
#define _KLCKO_OSBPF_H__

#ifndef __annotate_jump_table 
#define __annotate_jump_table __section(".rodata..c_jump_table")
#endif


#ifndef __bpf_call_base_args 
#define __bpf_call_base_args \
	((u64 (*)(u64, u64, u64, u64, u64, const struct bpf_insn *)) \
	 __bpf_call_base)
#endif

/* instruction classes */
#define BPF_JMP32	0x06	/* jmp mode in word width */

#define BPF_DW		0x18	/* double word (64-bit) */
#define BPF_ATOMIC	0xc0	/* atomic memory ops - op type in immediate */
#define BPF_XADD	0xc0	/* exclusive add - legacy name */

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


#ifndef atomic_fetch_add
#define atomic_fetch_add(i,v) (atomic_add_return(i,v) - (i))
#endif

#ifndef atomic64_fetch_add
#define atomic64_fetch_add(i,v) (atomic64_add_return(i,v) - (i))
#endif

#ifndef atomic_fetch_and /* 注意这可能不再是原子的了 */
#define atomic_fetch_and(i,v) ({u32 old=(v)->counter;atomic_and(i,v); old;})
#endif

#ifndef atomic64_fetch_and /* 注意这可能不再是原子的了 */
#define atomic64_fetch_and(i,v) ({u64 old=(v)->counter;atomic64_and(i,v); old;})
#endif

#ifndef atomic_fetch_or /* 注意这可能不再是原子的了 */
#define atomic_fetch_or(i,v) ({u32 old=(v)->counter;atomic_or(i,v); old;})
#endif

#ifndef atomic64_fetch_or /* 注意这可能不再是原子的了 */
#define atomic64_fetch_or(i,v) ({u64 old=(v)->counter;atomic64_or(i,v); old;})
#endif

#ifndef atomic_fetch_xor /* 注意这可能不再是原子的了 */
#define atomic_fetch_xor(i,v) ({u32 old=(v)->counter;atomic_xor(i,v); old;})
#endif

#ifndef atomic64_fetch_xor /* 注意这可能不再是原子的了 */
#define atomic64_fetch_xor(i,v) ({u64 old=(v)->counter;atomic64_xor(i,v); old;})
#endif

#endif 

