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

#define BPF_COMPLEXITY_LIMIT_INSNS      1000000 

enum bpf_reg_liveness {
	REG_LIVE_NONE = 0, 
	REG_LIVE_READ32 = 0x1, 
	REG_LIVE_READ64 = 0x2, 
	REG_LIVE_READ = REG_LIVE_READ32 | REG_LIVE_READ64,
	REG_LIVE_WRITTEN = 0x4, 
	REG_LIVE_DONE = 0x8, 
};

#define MAX_CALL_FRAMES 8
typedef struct bpf_verifier_state {
	
	struct bpf_func_state *frame[MAX_CALL_FRAMES];
	struct bpf_verifier_state *parent;
	u32 branches;
	u32 insn_idx;
	u32 curframe;
	u32 active_spin_lock;
	bool speculative;
	
	u32 first_insn_idx;
	u32 last_insn_idx;
	
	struct bpf_idx_pair *jmp_history;
	u32 jmp_history_cnt;
}MYBPF_VERIFIER_STATE_S;


struct bpf_verifier_stack_elem {
	
	struct bpf_verifier_state st;
	int insn_idx;
	int prev_insn_idx;
	struct bpf_verifier_stack_elem *next;
	
	u32 log_pos;
};


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
	u32 id_gen;			
	u32 subprog_cnt;
	MYBPF_VERIFIER_STATE_S *cur_state;
	struct bpf_verifier_stack_elem *head; 
	struct bpf_verifier_state_list **explored_states; 
	struct bpf_verifier_state_list *free_list;
}MYBPF_VERIFIER_ENV_S;

struct bpf_reg_state {
	
	enum bpf_reg_type type;
	
	s32 off;
	union {
		
		int range;

		
		struct {
			struct bpf_map *map_ptr;
			
			u32 map_uid;
		};

		
		struct {
			struct btf *btf;
			u32 btf_id;
		};

		u32 mem_size; 

		
		struct {
			unsigned long raw1;
			unsigned long raw2;
		} raw;

		u32 subprogno; 
	};
	
	u32 id;
	
	u32 ref_obj_id;
	
	struct tnum var_off;
	
	s64 smin_value; 
	s64 smax_value; 
	u64 umin_value; 
	u64 umax_value; 
	s32 s32_min_value; 
	s32 s32_max_value; 
	u32 u32_min_value; 
	u32 u32_max_value; 
	
	struct bpf_reg_state *parent;
	
	u32 frameno;
	
	s32 subreg_def;
	enum bpf_reg_liveness live;
	
	bool precise;
};


typedef struct bpf_func_state {
	struct bpf_reg_state regs[MAX_BPF_REG];
	
	int callsite;
	
	u32 frameno;
	
	u32 subprogno;
	
	u32 async_entry_cnt;
	bool in_callback_fn;
	bool in_async_callback_fn;

	
	int acquired_refs;
	struct bpf_reference_state *refs;
	int allocated_stack;
	struct bpf_stack_state *stack;
}MYBPF_BPF_FUNC_STATE_S;


static inline u32 base_type(u32 type)
{
	return type & 0xff;
}

#ifdef __cplusplus
}
#endif
#endif 
