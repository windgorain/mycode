/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */
#ifndef __BPF_TRACING_H__
#define __BPF_TRACING_H__

#include <bpf/bpf_helpers.h>


#if defined(__TARGET_ARCH_x86)
	#define bpf_target_x86
	#define bpf_target_defined
#elif defined(__TARGET_ARCH_s390)
	#define bpf_target_s390
	#define bpf_target_defined
#elif defined(__TARGET_ARCH_arm)
	#define bpf_target_arm
	#define bpf_target_defined
#elif defined(__TARGET_ARCH_arm64)
	#define bpf_target_arm64
	#define bpf_target_defined
#elif defined(__TARGET_ARCH_mips)
	#define bpf_target_mips
	#define bpf_target_defined
#elif defined(__TARGET_ARCH_powerpc)
	#define bpf_target_powerpc
	#define bpf_target_defined
#elif defined(__TARGET_ARCH_sparc)
	#define bpf_target_sparc
	#define bpf_target_defined
#elif defined(__TARGET_ARCH_riscv)
	#define bpf_target_riscv
	#define bpf_target_defined
#elif defined(__TARGET_ARCH_arc)
	#define bpf_target_arc
	#define bpf_target_defined
#elif defined(__TARGET_ARCH_loongarch)
	#define bpf_target_loongarch
	#define bpf_target_defined
#else


#if defined(__x86_64__)
	#define bpf_target_x86
	#define bpf_target_defined
#elif defined(__s390__)
	#define bpf_target_s390
	#define bpf_target_defined
#elif defined(__arm__)
	#define bpf_target_arm
	#define bpf_target_defined
#elif defined(__aarch64__)
	#define bpf_target_arm64
	#define bpf_target_defined
#elif defined(__mips__)
	#define bpf_target_mips
	#define bpf_target_defined
#elif defined(__powerpc__)
	#define bpf_target_powerpc
	#define bpf_target_defined
#elif defined(__sparc__)
	#define bpf_target_sparc
	#define bpf_target_defined
#elif defined(__riscv) && __riscv_xlen == 64
	#define bpf_target_riscv
	#define bpf_target_defined
#elif defined(__arc__)
	#define bpf_target_arc
	#define bpf_target_defined
#elif defined(__loongarch__)
	#define bpf_target_loongarch
	#define bpf_target_defined
#endif 

#endif

#ifndef __BPF_TARGET_MISSING
#define __BPF_TARGET_MISSING "GCC error \"Must specify a BPF target arch via __TARGET_ARCH_xxx\""
#endif

#if defined(bpf_target_x86)



#if defined(__KERNEL__) || defined(__VMLINUX_H__)

#define __PT_PARM1_REG di
#define __PT_PARM2_REG si
#define __PT_PARM3_REG dx
#define __PT_PARM4_REG cx
#define __PT_PARM5_REG r8
#define __PT_PARM6_REG r9

#define __PT_PARM1_SYSCALL_REG __PT_PARM1_REG
#define __PT_PARM2_SYSCALL_REG __PT_PARM2_REG
#define __PT_PARM3_SYSCALL_REG __PT_PARM3_REG
#define __PT_PARM4_SYSCALL_REG r10
#define __PT_PARM5_SYSCALL_REG __PT_PARM5_REG
#define __PT_PARM6_SYSCALL_REG __PT_PARM6_REG

#define __PT_RET_REG sp
#define __PT_FP_REG bp
#define __PT_RC_REG ax
#define __PT_SP_REG sp
#define __PT_IP_REG ip

#else

#ifdef __i386__


#define __PT_PARM1_REG eax
#define __PT_PARM2_REG edx
#define __PT_PARM3_REG ecx

#define __PT_PARM1_SYSCALL_REG ebx
#define __PT_PARM2_SYSCALL_REG ecx
#define __PT_PARM3_SYSCALL_REG edx
#define __PT_PARM4_SYSCALL_REG esi
#define __PT_PARM5_SYSCALL_REG edi
#define __PT_PARM6_SYSCALL_REG ebp

#define __PT_RET_REG esp
#define __PT_FP_REG ebp
#define __PT_RC_REG eax
#define __PT_SP_REG esp
#define __PT_IP_REG eip

#else 

#define __PT_PARM1_REG rdi
#define __PT_PARM2_REG rsi
#define __PT_PARM3_REG rdx
#define __PT_PARM4_REG rcx
#define __PT_PARM5_REG r8
#define __PT_PARM6_REG r9

#define __PT_PARM1_SYSCALL_REG __PT_PARM1_REG
#define __PT_PARM2_SYSCALL_REG __PT_PARM2_REG
#define __PT_PARM3_SYSCALL_REG __PT_PARM3_REG
#define __PT_PARM4_SYSCALL_REG r10
#define __PT_PARM5_SYSCALL_REG __PT_PARM5_REG
#define __PT_PARM6_SYSCALL_REG __PT_PARM6_REG

#define __PT_RET_REG rsp
#define __PT_FP_REG rbp
#define __PT_RC_REG rax
#define __PT_SP_REG rsp
#define __PT_IP_REG rip

#endif 

#endif 

#elif defined(bpf_target_s390)



struct pt_regs___s390 {
	unsigned long orig_gpr2;
};


#define __PT_REGS_CAST(x) ((const user_pt_regs *)(x))
#define __PT_PARM1_REG gprs[2]
#define __PT_PARM2_REG gprs[3]
#define __PT_PARM3_REG gprs[4]
#define __PT_PARM4_REG gprs[5]
#define __PT_PARM5_REG gprs[6]

#define __PT_PARM1_SYSCALL_REG orig_gpr2
#define __PT_PARM2_SYSCALL_REG __PT_PARM2_REG
#define __PT_PARM3_SYSCALL_REG __PT_PARM3_REG
#define __PT_PARM4_SYSCALL_REG __PT_PARM4_REG
#define __PT_PARM5_SYSCALL_REG __PT_PARM5_REG
#define __PT_PARM6_SYSCALL_REG gprs[7]
#define PT_REGS_PARM1_SYSCALL(x) PT_REGS_PARM1_CORE_SYSCALL(x)
#define PT_REGS_PARM1_CORE_SYSCALL(x) \
	BPF_CORE_READ((const struct pt_regs___s390 *)(x), __PT_PARM1_SYSCALL_REG)

#define __PT_RET_REG gprs[14]
#define __PT_FP_REG gprs[11]	
#define __PT_RC_REG gprs[2]
#define __PT_SP_REG gprs[15]
#define __PT_IP_REG psw.addr

#elif defined(bpf_target_arm)



#define __PT_PARM1_REG uregs[0]
#define __PT_PARM2_REG uregs[1]
#define __PT_PARM3_REG uregs[2]
#define __PT_PARM4_REG uregs[3]

#define __PT_PARM1_SYSCALL_REG __PT_PARM1_REG
#define __PT_PARM2_SYSCALL_REG __PT_PARM2_REG
#define __PT_PARM3_SYSCALL_REG __PT_PARM3_REG
#define __PT_PARM4_SYSCALL_REG __PT_PARM4_REG
#define __PT_PARM5_SYSCALL_REG uregs[4]
#define __PT_PARM6_SYSCALL_REG uregs[5]
#define __PT_PARM7_SYSCALL_REG uregs[6]

#define __PT_RET_REG uregs[14]
#define __PT_FP_REG uregs[11]	
#define __PT_RC_REG uregs[0]
#define __PT_SP_REG uregs[13]
#define __PT_IP_REG uregs[12]

#elif defined(bpf_target_arm64)



struct pt_regs___arm64 {
	unsigned long orig_x0;
};


#define __PT_REGS_CAST(x) ((const struct user_pt_regs *)(x))
#define __PT_PARM1_REG regs[0]
#define __PT_PARM2_REG regs[1]
#define __PT_PARM3_REG regs[2]
#define __PT_PARM4_REG regs[3]
#define __PT_PARM5_REG regs[4]
#define __PT_PARM6_REG regs[5]
#define __PT_PARM7_REG regs[6]
#define __PT_PARM8_REG regs[7]

#define __PT_PARM1_SYSCALL_REG orig_x0
#define __PT_PARM2_SYSCALL_REG __PT_PARM2_REG
#define __PT_PARM3_SYSCALL_REG __PT_PARM3_REG
#define __PT_PARM4_SYSCALL_REG __PT_PARM4_REG
#define __PT_PARM5_SYSCALL_REG __PT_PARM5_REG
#define __PT_PARM6_SYSCALL_REG __PT_PARM6_REG
#define PT_REGS_PARM1_SYSCALL(x) PT_REGS_PARM1_CORE_SYSCALL(x)
#define PT_REGS_PARM1_CORE_SYSCALL(x) \
	BPF_CORE_READ((const struct pt_regs___arm64 *)(x), __PT_PARM1_SYSCALL_REG)

#define __PT_RET_REG regs[30]
#define __PT_FP_REG regs[29]	
#define __PT_RC_REG regs[0]
#define __PT_SP_REG sp
#define __PT_IP_REG pc

#elif defined(bpf_target_mips)



#define __PT_PARM1_REG regs[4]
#define __PT_PARM2_REG regs[5]
#define __PT_PARM3_REG regs[6]
#define __PT_PARM4_REG regs[7]
#define __PT_PARM5_REG regs[8]
#define __PT_PARM6_REG regs[9]
#define __PT_PARM7_REG regs[10]
#define __PT_PARM8_REG regs[11]

#define __PT_PARM1_SYSCALL_REG __PT_PARM1_REG
#define __PT_PARM2_SYSCALL_REG __PT_PARM2_REG
#define __PT_PARM3_SYSCALL_REG __PT_PARM3_REG
#define __PT_PARM4_SYSCALL_REG __PT_PARM4_REG
#define __PT_PARM5_SYSCALL_REG __PT_PARM5_REG 
#define __PT_PARM6_SYSCALL_REG __PT_PARM6_REG 

#define __PT_RET_REG regs[31]
#define __PT_FP_REG regs[30]	
#define __PT_RC_REG regs[2]
#define __PT_SP_REG regs[29]
#define __PT_IP_REG cp0_epc

#elif defined(bpf_target_powerpc)



#define __PT_PARM1_REG gpr[3]
#define __PT_PARM2_REG gpr[4]
#define __PT_PARM3_REG gpr[5]
#define __PT_PARM4_REG gpr[6]
#define __PT_PARM5_REG gpr[7]
#define __PT_PARM6_REG gpr[8]
#define __PT_PARM7_REG gpr[9]
#define __PT_PARM8_REG gpr[10]


#define PT_REGS_SYSCALL_REGS(ctx) ctx
#define __PT_PARM1_SYSCALL_REG orig_gpr3
#define __PT_PARM2_SYSCALL_REG __PT_PARM2_REG
#define __PT_PARM3_SYSCALL_REG __PT_PARM3_REG
#define __PT_PARM4_SYSCALL_REG __PT_PARM4_REG
#define __PT_PARM5_SYSCALL_REG __PT_PARM5_REG
#define __PT_PARM6_SYSCALL_REG __PT_PARM6_REG
#if !defined(__arch64__)
#define __PT_PARM7_SYSCALL_REG __PT_PARM7_REG 
#endif

#define __PT_RET_REG regs[31]
#define __PT_FP_REG __unsupported__
#define __PT_RC_REG gpr[3]
#define __PT_SP_REG sp
#define __PT_IP_REG nip

#elif defined(bpf_target_sparc)



#define __PT_PARM1_REG u_regs[UREG_I0]
#define __PT_PARM2_REG u_regs[UREG_I1]
#define __PT_PARM3_REG u_regs[UREG_I2]
#define __PT_PARM4_REG u_regs[UREG_I3]
#define __PT_PARM5_REG u_regs[UREG_I4]
#define __PT_PARM6_REG u_regs[UREG_I5]

#define __PT_PARM1_SYSCALL_REG __PT_PARM1_REG
#define __PT_PARM2_SYSCALL_REG __PT_PARM2_REG
#define __PT_PARM3_SYSCALL_REG __PT_PARM3_REG
#define __PT_PARM4_SYSCALL_REG __PT_PARM4_REG
#define __PT_PARM5_SYSCALL_REG __PT_PARM5_REG
#define __PT_PARM6_SYSCALL_REG __PT_PARM6_REG

#define __PT_RET_REG u_regs[UREG_I7]
#define __PT_FP_REG __unsupported__
#define __PT_RC_REG u_regs[UREG_I0]
#define __PT_SP_REG u_regs[UREG_FP]

#if defined(__arch64__)
#define __PT_IP_REG tpc
#else
#define __PT_IP_REG pc
#endif

#elif defined(bpf_target_riscv)



#define __PT_REGS_CAST(x) ((const struct user_regs_struct *)(x))
#define __PT_PARM1_REG a0
#define __PT_PARM2_REG a1
#define __PT_PARM3_REG a2
#define __PT_PARM4_REG a3
#define __PT_PARM5_REG a4
#define __PT_PARM6_REG a5
#define __PT_PARM7_REG a6
#define __PT_PARM8_REG a7


#define PT_REGS_SYSCALL_REGS(ctx) ctx
#define __PT_PARM1_SYSCALL_REG __PT_PARM1_REG
#define __PT_PARM2_SYSCALL_REG __PT_PARM2_REG
#define __PT_PARM3_SYSCALL_REG __PT_PARM3_REG
#define __PT_PARM4_SYSCALL_REG __PT_PARM4_REG
#define __PT_PARM5_SYSCALL_REG __PT_PARM5_REG
#define __PT_PARM6_SYSCALL_REG __PT_PARM6_REG

#define __PT_RET_REG ra
#define __PT_FP_REG s0
#define __PT_RC_REG a0
#define __PT_SP_REG sp
#define __PT_IP_REG pc

#elif defined(bpf_target_arc)




#define __PT_REGS_CAST(x) ((const struct user_regs_struct *)(x))
#define __PT_PARM1_REG scratch.r0
#define __PT_PARM2_REG scratch.r1
#define __PT_PARM3_REG scratch.r2
#define __PT_PARM4_REG scratch.r3
#define __PT_PARM5_REG scratch.r4
#define __PT_PARM6_REG scratch.r5
#define __PT_PARM7_REG scratch.r6
#define __PT_PARM8_REG scratch.r7


#define PT_REGS_SYSCALL_REGS(ctx) ctx
#define __PT_PARM1_SYSCALL_REG __PT_PARM1_REG
#define __PT_PARM2_SYSCALL_REG __PT_PARM2_REG
#define __PT_PARM3_SYSCALL_REG __PT_PARM3_REG
#define __PT_PARM4_SYSCALL_REG __PT_PARM4_REG
#define __PT_PARM5_SYSCALL_REG __PT_PARM5_REG
#define __PT_PARM6_SYSCALL_REG __PT_PARM6_REG

#define __PT_RET_REG scratch.blink
#define __PT_FP_REG scratch.fp
#define __PT_RC_REG scratch.r0
#define __PT_SP_REG scratch.sp
#define __PT_IP_REG scratch.ret

#elif defined(bpf_target_loongarch)




#define __PT_REGS_CAST(x) ((const struct user_pt_regs *)(x))
#define __PT_PARM1_REG regs[4]
#define __PT_PARM2_REG regs[5]
#define __PT_PARM3_REG regs[6]
#define __PT_PARM4_REG regs[7]
#define __PT_PARM5_REG regs[8]
#define __PT_PARM6_REG regs[9]
#define __PT_PARM7_REG regs[10]
#define __PT_PARM8_REG regs[11]


#define PT_REGS_SYSCALL_REGS(ctx) ctx
#define __PT_PARM1_SYSCALL_REG __PT_PARM1_REG
#define __PT_PARM2_SYSCALL_REG __PT_PARM2_REG
#define __PT_PARM3_SYSCALL_REG __PT_PARM3_REG
#define __PT_PARM4_SYSCALL_REG __PT_PARM4_REG
#define __PT_PARM5_SYSCALL_REG __PT_PARM5_REG
#define __PT_PARM6_SYSCALL_REG __PT_PARM6_REG

#define __PT_RET_REG regs[1]
#define __PT_FP_REG regs[22]
#define __PT_RC_REG regs[4]
#define __PT_SP_REG regs[3]
#define __PT_IP_REG csr_era

#endif

#if defined(bpf_target_defined)

struct pt_regs;


#ifndef __PT_REGS_CAST
#define __PT_REGS_CAST(x) (x)
#endif


#ifndef __PT_PARM4_REG
#define __PT_PARM4_REG __unsupported__
#endif
#ifndef __PT_PARM5_REG
#define __PT_PARM5_REG __unsupported__
#endif
#ifndef __PT_PARM6_REG
#define __PT_PARM6_REG __unsupported__
#endif
#ifndef __PT_PARM7_REG
#define __PT_PARM7_REG __unsupported__
#endif
#ifndef __PT_PARM8_REG
#define __PT_PARM8_REG __unsupported__
#endif

#ifndef __PT_PARM7_SYSCALL_REG
#define __PT_PARM7_SYSCALL_REG __unsupported__
#endif

#define PT_REGS_PARM1(x) (__PT_REGS_CAST(x)->__PT_PARM1_REG)
#define PT_REGS_PARM2(x) (__PT_REGS_CAST(x)->__PT_PARM2_REG)
#define PT_REGS_PARM3(x) (__PT_REGS_CAST(x)->__PT_PARM3_REG)
#define PT_REGS_PARM4(x) (__PT_REGS_CAST(x)->__PT_PARM4_REG)
#define PT_REGS_PARM5(x) (__PT_REGS_CAST(x)->__PT_PARM5_REG)
#define PT_REGS_PARM6(x) (__PT_REGS_CAST(x)->__PT_PARM6_REG)
#define PT_REGS_PARM7(x) (__PT_REGS_CAST(x)->__PT_PARM7_REG)
#define PT_REGS_PARM8(x) (__PT_REGS_CAST(x)->__PT_PARM8_REG)
#define PT_REGS_RET(x) (__PT_REGS_CAST(x)->__PT_RET_REG)
#define PT_REGS_FP(x) (__PT_REGS_CAST(x)->__PT_FP_REG)
#define PT_REGS_RC(x) (__PT_REGS_CAST(x)->__PT_RC_REG)
#define PT_REGS_SP(x) (__PT_REGS_CAST(x)->__PT_SP_REG)
#define PT_REGS_IP(x) (__PT_REGS_CAST(x)->__PT_IP_REG)

#define PT_REGS_PARM1_CORE(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM1_REG)
#define PT_REGS_PARM2_CORE(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM2_REG)
#define PT_REGS_PARM3_CORE(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM3_REG)
#define PT_REGS_PARM4_CORE(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM4_REG)
#define PT_REGS_PARM5_CORE(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM5_REG)
#define PT_REGS_PARM6_CORE(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM6_REG)
#define PT_REGS_PARM7_CORE(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM7_REG)
#define PT_REGS_PARM8_CORE(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM8_REG)
#define PT_REGS_RET_CORE(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_RET_REG)
#define PT_REGS_FP_CORE(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_FP_REG)
#define PT_REGS_RC_CORE(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_RC_REG)
#define PT_REGS_SP_CORE(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_SP_REG)
#define PT_REGS_IP_CORE(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_IP_REG)

#if defined(bpf_target_powerpc)

#define BPF_KPROBE_READ_RET_IP(ip, ctx)		({ (ip) = (ctx)->link; })
#define BPF_KRETPROBE_READ_RET_IP		BPF_KPROBE_READ_RET_IP

#elif defined(bpf_target_sparc)

#define BPF_KPROBE_READ_RET_IP(ip, ctx)		({ (ip) = PT_REGS_RET(ctx); })
#define BPF_KRETPROBE_READ_RET_IP		BPF_KPROBE_READ_RET_IP

#else

#define BPF_KPROBE_READ_RET_IP(ip, ctx)					    \
	({ bpf_probe_read_kernel(&(ip), sizeof(ip), (void *)PT_REGS_RET(ctx)); })
#define BPF_KRETPROBE_READ_RET_IP(ip, ctx)				    \
	({ bpf_probe_read_kernel(&(ip), sizeof(ip), (void *)(PT_REGS_FP(ctx) + sizeof(ip))); })

#endif

#ifndef PT_REGS_PARM1_SYSCALL
#define PT_REGS_PARM1_SYSCALL(x) (__PT_REGS_CAST(x)->__PT_PARM1_SYSCALL_REG)
#define PT_REGS_PARM1_CORE_SYSCALL(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM1_SYSCALL_REG)
#endif
#ifndef PT_REGS_PARM2_SYSCALL
#define PT_REGS_PARM2_SYSCALL(x) (__PT_REGS_CAST(x)->__PT_PARM2_SYSCALL_REG)
#define PT_REGS_PARM2_CORE_SYSCALL(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM2_SYSCALL_REG)
#endif
#ifndef PT_REGS_PARM3_SYSCALL
#define PT_REGS_PARM3_SYSCALL(x) (__PT_REGS_CAST(x)->__PT_PARM3_SYSCALL_REG)
#define PT_REGS_PARM3_CORE_SYSCALL(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM3_SYSCALL_REG)
#endif
#ifndef PT_REGS_PARM4_SYSCALL
#define PT_REGS_PARM4_SYSCALL(x) (__PT_REGS_CAST(x)->__PT_PARM4_SYSCALL_REG)
#define PT_REGS_PARM4_CORE_SYSCALL(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM4_SYSCALL_REG)
#endif
#ifndef PT_REGS_PARM5_SYSCALL
#define PT_REGS_PARM5_SYSCALL(x) (__PT_REGS_CAST(x)->__PT_PARM5_SYSCALL_REG)
#define PT_REGS_PARM5_CORE_SYSCALL(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM5_SYSCALL_REG)
#endif
#ifndef PT_REGS_PARM6_SYSCALL
#define PT_REGS_PARM6_SYSCALL(x) (__PT_REGS_CAST(x)->__PT_PARM6_SYSCALL_REG)
#define PT_REGS_PARM6_CORE_SYSCALL(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM6_SYSCALL_REG)
#endif
#ifndef PT_REGS_PARM7_SYSCALL
#define PT_REGS_PARM7_SYSCALL(x) (__PT_REGS_CAST(x)->__PT_PARM7_SYSCALL_REG)
#define PT_REGS_PARM7_CORE_SYSCALL(x) BPF_CORE_READ(__PT_REGS_CAST(x), __PT_PARM7_SYSCALL_REG)
#endif

#else 

#define PT_REGS_PARM1(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM2(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM3(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM4(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM5(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM6(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM7(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM8(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_RET(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_FP(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_RC(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_SP(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_IP(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })

#define PT_REGS_PARM1_CORE(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM2_CORE(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM3_CORE(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM4_CORE(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM5_CORE(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM6_CORE(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM7_CORE(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM8_CORE(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_RET_CORE(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_FP_CORE(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_RC_CORE(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_SP_CORE(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_IP_CORE(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })

#define BPF_KPROBE_READ_RET_IP(ip, ctx) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define BPF_KRETPROBE_READ_RET_IP(ip, ctx) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })

#define PT_REGS_PARM1_SYSCALL(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM2_SYSCALL(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM3_SYSCALL(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM4_SYSCALL(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM5_SYSCALL(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM6_SYSCALL(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM7_SYSCALL(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })

#define PT_REGS_PARM1_CORE_SYSCALL(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM2_CORE_SYSCALL(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM3_CORE_SYSCALL(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM4_CORE_SYSCALL(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM5_CORE_SYSCALL(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM6_CORE_SYSCALL(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })
#define PT_REGS_PARM7_CORE_SYSCALL(x) ({ _Pragma(__BPF_TARGET_MISSING); 0l; })

#endif 


#ifndef PT_REGS_SYSCALL_REGS

#define PT_REGS_SYSCALL_REGS(ctx) ((struct pt_regs *)PT_REGS_PARM1(ctx))
#endif

#ifndef ___bpf_concat
#define ___bpf_concat(a, b) a ## b
#endif
#ifndef ___bpf_apply
#define ___bpf_apply(fn, n) ___bpf_concat(fn, n)
#endif
#ifndef ___bpf_nth
#define ___bpf_nth(_, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, N, ...) N
#endif
#ifndef ___bpf_narg
#define ___bpf_narg(...) ___bpf_nth(_, ##__VA_ARGS__, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#endif

#define ___bpf_ctx_cast0()            ctx
#define ___bpf_ctx_cast1(x)           ___bpf_ctx_cast0(), (void *)ctx[0]
#define ___bpf_ctx_cast2(x, args...)  ___bpf_ctx_cast1(args), (void *)ctx[1]
#define ___bpf_ctx_cast3(x, args...)  ___bpf_ctx_cast2(args), (void *)ctx[2]
#define ___bpf_ctx_cast4(x, args...)  ___bpf_ctx_cast3(args), (void *)ctx[3]
#define ___bpf_ctx_cast5(x, args...)  ___bpf_ctx_cast4(args), (void *)ctx[4]
#define ___bpf_ctx_cast6(x, args...)  ___bpf_ctx_cast5(args), (void *)ctx[5]
#define ___bpf_ctx_cast7(x, args...)  ___bpf_ctx_cast6(args), (void *)ctx[6]
#define ___bpf_ctx_cast8(x, args...)  ___bpf_ctx_cast7(args), (void *)ctx[7]
#define ___bpf_ctx_cast9(x, args...)  ___bpf_ctx_cast8(args), (void *)ctx[8]
#define ___bpf_ctx_cast10(x, args...) ___bpf_ctx_cast9(args), (void *)ctx[9]
#define ___bpf_ctx_cast11(x, args...) ___bpf_ctx_cast10(args), (void *)ctx[10]
#define ___bpf_ctx_cast12(x, args...) ___bpf_ctx_cast11(args), (void *)ctx[11]
#define ___bpf_ctx_cast(args...)      ___bpf_apply(___bpf_ctx_cast, ___bpf_narg(args))(args)


#define BPF_PROG(name, args...)						    \
name(unsigned long long *ctx);						    \
static __always_inline typeof(name(0))					    \
____##name(unsigned long long *ctx, ##args);				    \
typeof(name(0)) name(unsigned long long *ctx)				    \
{									    \
	_Pragma("GCC diagnostic push")					    \
	_Pragma("GCC diagnostic ignored \"-Wint-conversion\"")		    \
	return ____##name(___bpf_ctx_cast(args));			    \
	_Pragma("GCC diagnostic pop")					    \
}									    \
static __always_inline typeof(name(0))					    \
____##name(unsigned long long *ctx, ##args)

#ifndef ___bpf_nth2
#define ___bpf_nth2(_, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13,	\
		    _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, N, ...) N
#endif
#ifndef ___bpf_narg2
#define ___bpf_narg2(...)	\
	___bpf_nth2(_, ##__VA_ARGS__, 12, 12, 11, 11, 10, 10, 9, 9, 8, 8, 7, 7,	\
		    6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0)
#endif

#define ___bpf_treg_cnt(t) \
	__builtin_choose_expr(sizeof(t) == 1, 1,	\
	__builtin_choose_expr(sizeof(t) == 2, 1,	\
	__builtin_choose_expr(sizeof(t) == 4, 1,	\
	__builtin_choose_expr(sizeof(t) == 8, 1,	\
	__builtin_choose_expr(sizeof(t) == 16, 2,	\
			      (void)0)))))

#define ___bpf_reg_cnt0()		(0)
#define ___bpf_reg_cnt1(t, x)		(___bpf_reg_cnt0() + ___bpf_treg_cnt(t))
#define ___bpf_reg_cnt2(t, x, args...)	(___bpf_reg_cnt1(args) + ___bpf_treg_cnt(t))
#define ___bpf_reg_cnt3(t, x, args...)	(___bpf_reg_cnt2(args) + ___bpf_treg_cnt(t))
#define ___bpf_reg_cnt4(t, x, args...)	(___bpf_reg_cnt3(args) + ___bpf_treg_cnt(t))
#define ___bpf_reg_cnt5(t, x, args...)	(___bpf_reg_cnt4(args) + ___bpf_treg_cnt(t))
#define ___bpf_reg_cnt6(t, x, args...)	(___bpf_reg_cnt5(args) + ___bpf_treg_cnt(t))
#define ___bpf_reg_cnt7(t, x, args...)	(___bpf_reg_cnt6(args) + ___bpf_treg_cnt(t))
#define ___bpf_reg_cnt8(t, x, args...)	(___bpf_reg_cnt7(args) + ___bpf_treg_cnt(t))
#define ___bpf_reg_cnt9(t, x, args...)	(___bpf_reg_cnt8(args) + ___bpf_treg_cnt(t))
#define ___bpf_reg_cnt10(t, x, args...)	(___bpf_reg_cnt9(args) + ___bpf_treg_cnt(t))
#define ___bpf_reg_cnt11(t, x, args...)	(___bpf_reg_cnt10(args) + ___bpf_treg_cnt(t))
#define ___bpf_reg_cnt12(t, x, args...)	(___bpf_reg_cnt11(args) + ___bpf_treg_cnt(t))
#define ___bpf_reg_cnt(args...)	 ___bpf_apply(___bpf_reg_cnt, ___bpf_narg2(args))(args)

#define ___bpf_union_arg(t, x, n) \
	__builtin_choose_expr(sizeof(t) == 1, ({ union { __u8 z[1]; t x; } ___t = { .z = {ctx[n]}}; ___t.x; }), \
	__builtin_choose_expr(sizeof(t) == 2, ({ union { __u16 z[1]; t x; } ___t = { .z = {ctx[n]} }; ___t.x; }), \
	__builtin_choose_expr(sizeof(t) == 4, ({ union { __u32 z[1]; t x; } ___t = { .z = {ctx[n]} }; ___t.x; }), \
	__builtin_choose_expr(sizeof(t) == 8, ({ union { __u64 z[1]; t x; } ___t = {.z = {ctx[n]} }; ___t.x; }), \
	__builtin_choose_expr(sizeof(t) == 16, ({ union { __u64 z[2]; t x; } ___t = {.z = {ctx[n], ctx[n + 1]} }; ___t.x; }), \
			      (void)0)))))

#define ___bpf_ctx_arg0(n, args...)
#define ___bpf_ctx_arg1(n, t, x)		, ___bpf_union_arg(t, x, n - ___bpf_reg_cnt1(t, x))
#define ___bpf_ctx_arg2(n, t, x, args...)	, ___bpf_union_arg(t, x, n - ___bpf_reg_cnt2(t, x, args)) ___bpf_ctx_arg1(n, args)
#define ___bpf_ctx_arg3(n, t, x, args...)	, ___bpf_union_arg(t, x, n - ___bpf_reg_cnt3(t, x, args)) ___bpf_ctx_arg2(n, args)
#define ___bpf_ctx_arg4(n, t, x, args...)	, ___bpf_union_arg(t, x, n - ___bpf_reg_cnt4(t, x, args)) ___bpf_ctx_arg3(n, args)
#define ___bpf_ctx_arg5(n, t, x, args...)	, ___bpf_union_arg(t, x, n - ___bpf_reg_cnt5(t, x, args)) ___bpf_ctx_arg4(n, args)
#define ___bpf_ctx_arg6(n, t, x, args...)	, ___bpf_union_arg(t, x, n - ___bpf_reg_cnt6(t, x, args)) ___bpf_ctx_arg5(n, args)
#define ___bpf_ctx_arg7(n, t, x, args...)	, ___bpf_union_arg(t, x, n - ___bpf_reg_cnt7(t, x, args)) ___bpf_ctx_arg6(n, args)
#define ___bpf_ctx_arg8(n, t, x, args...)	, ___bpf_union_arg(t, x, n - ___bpf_reg_cnt8(t, x, args)) ___bpf_ctx_arg7(n, args)
#define ___bpf_ctx_arg9(n, t, x, args...)	, ___bpf_union_arg(t, x, n - ___bpf_reg_cnt9(t, x, args)) ___bpf_ctx_arg8(n, args)
#define ___bpf_ctx_arg10(n, t, x, args...)	, ___bpf_union_arg(t, x, n - ___bpf_reg_cnt10(t, x, args)) ___bpf_ctx_arg9(n, args)
#define ___bpf_ctx_arg11(n, t, x, args...)	, ___bpf_union_arg(t, x, n - ___bpf_reg_cnt11(t, x, args)) ___bpf_ctx_arg10(n, args)
#define ___bpf_ctx_arg12(n, t, x, args...)	, ___bpf_union_arg(t, x, n - ___bpf_reg_cnt12(t, x, args)) ___bpf_ctx_arg11(n, args)
#define ___bpf_ctx_arg(args...)	___bpf_apply(___bpf_ctx_arg, ___bpf_narg2(args))(___bpf_reg_cnt(args), args)

#define ___bpf_ctx_decl0()
#define ___bpf_ctx_decl1(t, x)			, t x
#define ___bpf_ctx_decl2(t, x, args...)		, t x ___bpf_ctx_decl1(args)
#define ___bpf_ctx_decl3(t, x, args...)		, t x ___bpf_ctx_decl2(args)
#define ___bpf_ctx_decl4(t, x, args...)		, t x ___bpf_ctx_decl3(args)
#define ___bpf_ctx_decl5(t, x, args...)		, t x ___bpf_ctx_decl4(args)
#define ___bpf_ctx_decl6(t, x, args...)		, t x ___bpf_ctx_decl5(args)
#define ___bpf_ctx_decl7(t, x, args...)		, t x ___bpf_ctx_decl6(args)
#define ___bpf_ctx_decl8(t, x, args...)		, t x ___bpf_ctx_decl7(args)
#define ___bpf_ctx_decl9(t, x, args...)		, t x ___bpf_ctx_decl8(args)
#define ___bpf_ctx_decl10(t, x, args...)	, t x ___bpf_ctx_decl9(args)
#define ___bpf_ctx_decl11(t, x, args...)	, t x ___bpf_ctx_decl10(args)
#define ___bpf_ctx_decl12(t, x, args...)	, t x ___bpf_ctx_decl11(args)
#define ___bpf_ctx_decl(args...)	___bpf_apply(___bpf_ctx_decl, ___bpf_narg2(args))(args)


#define BPF_PROG2(name, args...)						\
name(unsigned long long *ctx);							\
static __always_inline typeof(name(0))						\
____##name(unsigned long long *ctx ___bpf_ctx_decl(args));			\
typeof(name(0)) name(unsigned long long *ctx)					\
{										\
	return ____##name(ctx ___bpf_ctx_arg(args));				\
}										\
static __always_inline typeof(name(0))						\
____##name(unsigned long long *ctx ___bpf_ctx_decl(args))

struct pt_regs;

#define ___bpf_kprobe_args0()           ctx
#define ___bpf_kprobe_args1(x)          ___bpf_kprobe_args0(), (void *)PT_REGS_PARM1(ctx)
#define ___bpf_kprobe_args2(x, args...) ___bpf_kprobe_args1(args), (void *)PT_REGS_PARM2(ctx)
#define ___bpf_kprobe_args3(x, args...) ___bpf_kprobe_args2(args), (void *)PT_REGS_PARM3(ctx)
#define ___bpf_kprobe_args4(x, args...) ___bpf_kprobe_args3(args), (void *)PT_REGS_PARM4(ctx)
#define ___bpf_kprobe_args5(x, args...) ___bpf_kprobe_args4(args), (void *)PT_REGS_PARM5(ctx)
#define ___bpf_kprobe_args6(x, args...) ___bpf_kprobe_args5(args), (void *)PT_REGS_PARM6(ctx)
#define ___bpf_kprobe_args7(x, args...) ___bpf_kprobe_args6(args), (void *)PT_REGS_PARM7(ctx)
#define ___bpf_kprobe_args8(x, args...) ___bpf_kprobe_args7(args), (void *)PT_REGS_PARM8(ctx)
#define ___bpf_kprobe_args(args...)     ___bpf_apply(___bpf_kprobe_args, ___bpf_narg(args))(args)


#define BPF_KPROBE(name, args...)					    \
name(struct pt_regs *ctx);						    \
static __always_inline typeof(name(0))					    \
____##name(struct pt_regs *ctx, ##args);				    \
typeof(name(0)) name(struct pt_regs *ctx)				    \
{									    \
	_Pragma("GCC diagnostic push")					    \
	_Pragma("GCC diagnostic ignored \"-Wint-conversion\"")		    \
	return ____##name(___bpf_kprobe_args(args));			    \
	_Pragma("GCC diagnostic pop")					    \
}									    \
static __always_inline typeof(name(0))					    \
____##name(struct pt_regs *ctx, ##args)

#define ___bpf_kretprobe_args0()       ctx
#define ___bpf_kretprobe_args1(x)      ___bpf_kretprobe_args0(), (void *)PT_REGS_RC(ctx)
#define ___bpf_kretprobe_args(args...) ___bpf_apply(___bpf_kretprobe_args, ___bpf_narg(args))(args)


#define BPF_KRETPROBE(name, args...)					    \
name(struct pt_regs *ctx);						    \
static __always_inline typeof(name(0))					    \
____##name(struct pt_regs *ctx, ##args);				    \
typeof(name(0)) name(struct pt_regs *ctx)				    \
{									    \
	_Pragma("GCC diagnostic push")					    \
	_Pragma("GCC diagnostic ignored \"-Wint-conversion\"")		    \
	return ____##name(___bpf_kretprobe_args(args));			    \
	_Pragma("GCC diagnostic pop")					    \
}									    \
static __always_inline typeof(name(0)) ____##name(struct pt_regs *ctx, ##args)


#define ___bpf_syscall_args0()           ctx
#define ___bpf_syscall_args1(x)          ___bpf_syscall_args0(), (void *)PT_REGS_PARM1_SYSCALL(regs)
#define ___bpf_syscall_args2(x, args...) ___bpf_syscall_args1(args), (void *)PT_REGS_PARM2_SYSCALL(regs)
#define ___bpf_syscall_args3(x, args...) ___bpf_syscall_args2(args), (void *)PT_REGS_PARM3_SYSCALL(regs)
#define ___bpf_syscall_args4(x, args...) ___bpf_syscall_args3(args), (void *)PT_REGS_PARM4_SYSCALL(regs)
#define ___bpf_syscall_args5(x, args...) ___bpf_syscall_args4(args), (void *)PT_REGS_PARM5_SYSCALL(regs)
#define ___bpf_syscall_args6(x, args...) ___bpf_syscall_args5(args), (void *)PT_REGS_PARM6_SYSCALL(regs)
#define ___bpf_syscall_args7(x, args...) ___bpf_syscall_args6(args), (void *)PT_REGS_PARM7_SYSCALL(regs)
#define ___bpf_syscall_args(args...)     ___bpf_apply(___bpf_syscall_args, ___bpf_narg(args))(args)


#define ___bpf_syswrap_args0()           ctx
#define ___bpf_syswrap_args1(x)          ___bpf_syswrap_args0(), (void *)PT_REGS_PARM1_CORE_SYSCALL(regs)
#define ___bpf_syswrap_args2(x, args...) ___bpf_syswrap_args1(args), (void *)PT_REGS_PARM2_CORE_SYSCALL(regs)
#define ___bpf_syswrap_args3(x, args...) ___bpf_syswrap_args2(args), (void *)PT_REGS_PARM3_CORE_SYSCALL(regs)
#define ___bpf_syswrap_args4(x, args...) ___bpf_syswrap_args3(args), (void *)PT_REGS_PARM4_CORE_SYSCALL(regs)
#define ___bpf_syswrap_args5(x, args...) ___bpf_syswrap_args4(args), (void *)PT_REGS_PARM5_CORE_SYSCALL(regs)
#define ___bpf_syswrap_args6(x, args...) ___bpf_syswrap_args5(args), (void *)PT_REGS_PARM6_CORE_SYSCALL(regs)
#define ___bpf_syswrap_args7(x, args...) ___bpf_syswrap_args6(args), (void *)PT_REGS_PARM7_CORE_SYSCALL(regs)
#define ___bpf_syswrap_args(args...)     ___bpf_apply(___bpf_syswrap_args, ___bpf_narg(args))(args)


#define BPF_KSYSCALL(name, args...)					    \
name(struct pt_regs *ctx);						    \
extern _Bool LINUX_HAS_SYSCALL_WRAPPER __kconfig;			    \
static __always_inline typeof(name(0))					    \
____##name(struct pt_regs *ctx, ##args);				    \
typeof(name(0)) name(struct pt_regs *ctx)				    \
{									    \
	struct pt_regs *regs = LINUX_HAS_SYSCALL_WRAPPER		    \
			       ? (struct pt_regs *)PT_REGS_PARM1(ctx)	    \
			       : ctx;					    \
	_Pragma("GCC diagnostic push")					    \
	_Pragma("GCC diagnostic ignored \"-Wint-conversion\"")		    \
	if (LINUX_HAS_SYSCALL_WRAPPER)					    \
		return ____##name(___bpf_syswrap_args(args));		    \
	else								    \
		return ____##name(___bpf_syscall_args(args));		    \
	_Pragma("GCC diagnostic pop")					    \
}									    \
static __always_inline typeof(name(0))					    \
____##name(struct pt_regs *ctx, ##args)

#define BPF_KPROBE_SYSCALL BPF_KSYSCALL


#define BPF_UPROBE(name, args...)  BPF_KPROBE(name, ##args)
#define BPF_URETPROBE(name, args...)  BPF_KRETPROBE(name, ##args)

#endif
