/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#ifndef _MYBPF_VERIFIER_INNER_H
#define _MYBPF_VERIFIER_INNER_H

#include "mybpf_baseos_tnum.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define BPF_COMPLEXITY_LIMIT_INSNS      1000000 /* 1M insns */

enum bpf_reg_liveness {
	REG_LIVE_NONE = 0, /* reg hasn't been read or written this branch */
	REG_LIVE_READ32 = 0x1, /* reg was read, so we're sensitive to initial value */
	REG_LIVE_READ64 = 0x2, /* likewise, but full 64-bit content matters */
	REG_LIVE_READ = REG_LIVE_READ32 | REG_LIVE_READ64,
	REG_LIVE_WRITTEN = 0x4, /* reg was written first, screening off later reads */
	REG_LIVE_DONE = 0x8, /* liveness won't be updating this register anymore */
};

#define MAX_CALL_FRAMES 8
typedef struct bpf_verifier_state {
	/* call stack tracking */
	struct bpf_func_state *frame[MAX_CALL_FRAMES];
	struct bpf_verifier_state *parent;
	u32 branches;
	u32 insn_idx;
	u32 curframe;
	u32 active_spin_lock;
	bool speculative;
	/* first and last insn idx of this verifier state */
	u32 first_insn_idx;
	u32 last_insn_idx;
	/* jmp history recorded from first to last.
	 * backtracking is using it to go from last to first.
	 * For most states jmp_history_cnt is [0-3].
	 * For loops can go up to ~40.
	 */
	struct bpf_idx_pair *jmp_history;
	u32 jmp_history_cnt;
}MYBPF_VERIFIER_STATE_S;

/* verifier_state + insn_idx are pushed to stack when branch is encountered */
struct bpf_verifier_stack_elem {
	/* verifer state is 'st'
	 * before processing instruction 'insn_idx'
	 * and after processing instruction 'prev_insn_idx'
	 */
	struct bpf_verifier_state st;
	int insn_idx;
	int prev_insn_idx;
	struct bpf_verifier_stack_elem *next;
	/* length of verifier log at the time this state was pushed on stack */
	u32 log_pos;
};

/* linked list of verifier states used to prune search */
struct bpf_verifier_state_list {
	struct bpf_verifier_state state;
	struct bpf_verifier_state_list *next;
	int miss_cnt, hit_cnt;
};

typedef struct bpf_verifier_env {
	bool bpf_capable;
    u32 insn_cnt;
	u32 insn_idx;
	u32 prev_insn_idx;
	u32 prev_insn_processed;
    u32 insn_processed;
	u32 id_gen;			/* used to generate unique reg IDs */
	u32 subprog_cnt;
	MYBPF_VERIFIER_STATE_S *cur_state;
	struct bpf_verifier_stack_elem *head; /* stack of verifier states to be processed */
	struct bpf_verifier_state_list **explored_states; /* search pruning optimization */
	struct bpf_verifier_state_list *free_list;
}MYBPF_VERIFIER_ENV_S;

struct bpf_reg_state {
	/* Ordering of fields matters.  See states_equal() */
	enum bpf_reg_type type;
	/* Fixed part of pointer offset, pointer types only */
	s32 off;
	union {
		/* valid when type == PTR_TO_PACKET */
		int range;

		/* valid when type == CONST_PTR_TO_MAP | PTR_TO_MAP_VALUE |
		 *   PTR_TO_MAP_VALUE_OR_NULL
		 */
		struct {
			struct bpf_map *map_ptr;
			/* To distinguish map lookups from outer map
			 * the map_uid is non-zero for registers
			 * pointing to inner maps.
			 */
			u32 map_uid;
		};

		/* for PTR_TO_BTF_ID */
		struct {
			struct btf *btf;
			u32 btf_id;
		};

		u32 mem_size; /* for PTR_TO_MEM | PTR_TO_MEM_OR_NULL */

		/* Max size from any of the above. */
		struct {
			unsigned long raw1;
			unsigned long raw2;
		} raw;

		u32 subprogno; /* for PTR_TO_FUNC */
	};
	/* For PTR_TO_PACKET, used to find other pointers with the same variable
	 * offset, so they can share range knowledge.
	 * For PTR_TO_MAP_VALUE_OR_NULL this is used to share which map value we
	 * came from, when one is tested for != NULL.
	 * For PTR_TO_MEM_OR_NULL this is used to identify memory allocation
	 * for the purpose of tracking that it's freed.
	 * For PTR_TO_SOCKET this is used to share which pointers retain the
	 * same reference to the socket, to determine proper reference freeing.
	 */
	u32 id;
	/* PTR_TO_SOCKET and PTR_TO_TCP_SOCK could be a ptr returned
	 * from a pointer-cast helper, bpf_sk_fullsock() and
	 * bpf_tcp_sock().
	 *
	 * Consider the following where "sk" is a reference counted
	 * pointer returned from "sk = bpf_sk_lookup_tcp();":
	 *
	 * 1: sk = bpf_sk_lookup_tcp();
	 * 2: if (!sk) { return 0; }
	 * 3: fullsock = bpf_sk_fullsock(sk);
	 * 4: if (!fullsock) { bpf_sk_release(sk); return 0; }
	 * 5: tp = bpf_tcp_sock(fullsock);
	 * 6: if (!tp) { bpf_sk_release(sk); return 0; }
	 * 7: bpf_sk_release(sk);
	 * 8: snd_cwnd = tp->snd_cwnd;  // verifier will complain
	 *
	 * After bpf_sk_release(sk) at line 7, both "fullsock" ptr and
	 * "tp" ptr should be invalidated also.  In order to do that,
	 * the reg holding "fullsock" and "sk" need to remember
	 * the original refcounted ptr id (i.e. sk_reg->id) in ref_obj_id
	 * such that the verifier can reset all regs which have
	 * ref_obj_id matching the sk_reg->id.
	 *
	 * sk_reg->ref_obj_id is set to sk_reg->id at line 1.
	 * sk_reg->id will stay as NULL-marking purpose only.
	 * After NULL-marking is done, sk_reg->id can be reset to 0.
	 *
	 * After "fullsock = bpf_sk_fullsock(sk);" at line 3,
	 * fullsock_reg->ref_obj_id is set to sk_reg->ref_obj_id.
	 *
	 * After "tp = bpf_tcp_sock(fullsock);" at line 5,
	 * tp_reg->ref_obj_id is set to fullsock_reg->ref_obj_id
	 * which is the same as sk_reg->ref_obj_id.
	 *
	 * From the verifier perspective, if sk, fullsock and tp
	 * are not NULL, they are the same ptr with different
	 * reg->type.  In particular, bpf_sk_release(tp) is also
	 * allowed and has the same effect as bpf_sk_release(sk).
	 */
	u32 ref_obj_id;
	/* For scalar types (SCALAR_VALUE), this represents our knowledge of
	 * the actual value.
	 * For pointer types, this represents the variable part of the offset
	 * from the pointed-to object, and is shared with all bpf_reg_states
	 * with the same id as us.
	 */
	struct tnum var_off;
	/* Used to determine if any memory access using this register will
	 * result in a bad access.
	 * These refer to the same value as var_off, not necessarily the actual
	 * contents of the register.
	 */
	s64 smin_value; /* minimum possible (s64)value */
	s64 smax_value; /* maximum possible (s64)value */
	u64 umin_value; /* minimum possible (u64)value */
	u64 umax_value; /* maximum possible (u64)value */
	s32 s32_min_value; /* minimum possible (s32)value */
	s32 s32_max_value; /* maximum possible (s32)value */
	u32 u32_min_value; /* minimum possible (u32)value */
	u32 u32_max_value; /* maximum possible (u32)value */
	/* parentage chain for liveness checking */
	struct bpf_reg_state *parent;
	/* Inside the callee two registers can be both PTR_TO_STACK like
	 * R1=fp-8 and R2=fp-8, but one of them points to this function stack
	 * while another to the caller's stack. To differentiate them 'frameno'
	 * is used which is an index in bpf_verifier_state->frame[] array
	 * pointing to bpf_func_state.
	 */
	u32 frameno;
	/* Tracks subreg definition. The stored value is the insn_idx of the
	 * writing insn. This is safe because subreg_def is used before any insn
	 * patching which only happens after main verification finished.
	 */
	s32 subreg_def;
	enum bpf_reg_liveness live;
	/* if (!precise && SCALAR_VALUE) min/max/tnum don't affect safety */
	bool precise;
};

/* state of the program:
 * type of all registers and stack info
 */
typedef struct bpf_func_state {
	struct bpf_reg_state regs[MAX_BPF_REG];
	/* index of call instruction that called into this func */
	int callsite;
	/* stack frame number of this function state from pov of
	 * enclosing bpf_verifier_state.
	 * 0 = main function, 1 = first callee.
	 */
	u32 frameno;
	/* subprog number == index within subprog_info
	 * zero == main subprog
	 */
	u32 subprogno;
	/* Every bpf_timer_start will increment async_entry_cnt.
	 * It's used to distinguish:
	 * void foo(void) { for(;;); }
	 * void foo(void) { bpf_timer_set_callback(,foo); }
	 */
	u32 async_entry_cnt;
	bool in_callback_fn;
	bool in_async_callback_fn;

	/* The following fields should be last. See copy_func_state() */
	int acquired_refs;
	struct bpf_reference_state *refs;
	int allocated_stack;
	struct bpf_stack_state *stack;
}MYBPF_BPF_FUNC_STATE_S;

/* extract base type from bpf_{arg, return, reg}_type. */
static inline u32 base_type(u32 type)
{
	return type & 0xff;
}

#ifdef __cplusplus
}
#endif
#endif //MYBPF_VERIFIER_INNER_H_
