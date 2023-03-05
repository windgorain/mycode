/*********************************************************
*   Created by LiXingang, Copyright LiXingang
*   Date: 2017.10.2
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/ufd_utl.h"
#include "utl/umap_utl.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_insn.h"
#include "utl/bpf_helper_utl.h"
#include "utl/num_limits.h"
#include "mybpf_osbase.h"
#include "mybpf_def.h"
#include "mybpf_verifier_inner.h"

const struct tnum tnum_unknown = { .value = 0, .mask = -1 };

/* Reset the min/max bounds of a register */
static void __mark_reg_unbounded(struct bpf_reg_state *reg)
{
	reg->smin_value = S64_MIN;
	reg->smax_value = S64_MAX;
	reg->umin_value = 0;
	reg->umax_value = U64_MAX;

	reg->s32_min_value = S32_MIN;
	reg->s32_max_value = S32_MAX;
	reg->u32_min_value = 0;
	reg->u32_max_value = U32_MAX;
}

/* Mark a register as having a completely unknown (scalar) value. */
static void __mark_reg_unknown(const struct bpf_verifier_env *env,
			       struct bpf_reg_state *reg)
{
	/*
	 * Clear type, id, off, and union(map_ptr, range) and
	 * padding between 'type' and union
	 */
	memset(reg, 0, offsetof(struct bpf_reg_state, var_off));
	reg->type = SCALAR_VALUE;
	reg->var_off = tnum_unknown;
	reg->frameno = 0;
	reg->precise = env->subprog_cnt > 1 || !env->bpf_capable;
	__mark_reg_unbounded(reg);
}

/* This helper doesn't clear reg->id */
static void ___mark_reg_known(struct bpf_reg_state *reg, u64 imm)
{
	reg->var_off = tnum_const(imm);
	reg->smin_value = (s64)imm;
	reg->smax_value = (s64)imm;
	reg->umin_value = imm;
	reg->umax_value = imm;

	reg->s32_min_value = (s32)imm;
	reg->s32_max_value = (s32)imm;
	reg->u32_min_value = (u32)imm;
	reg->u32_max_value = (u32)imm;
}

static void __mark_reg_known(struct bpf_reg_state *reg, u64 imm)
{
	/* Clear id, off, and union(map_ptr, range) */
	memset(((u8 *)reg) + sizeof(reg->type), 0,
	       offsetof(struct bpf_reg_state, var_off) - sizeof(reg->type));
	___mark_reg_known(reg, imm);
}

static void __mark_reg_known_zero(struct bpf_reg_state *reg)
{
	__mark_reg_known(reg, 0);
}

static void mark_reg_known_zero(struct bpf_verifier_env *env,
				struct bpf_reg_state *regs, u32 regno)
{
    BS_DBGASSERT(regno < MAX_BPF_REG);
	__mark_reg_known_zero(regs + regno);
}


static void mark_reg_unknown(struct bpf_verifier_env *env,
			     struct bpf_reg_state *regs, u32 regno)
{
    BS_DBGASSERT(regno < MAX_BPF_REG);
	__mark_reg_unknown(env, regs + regno);
}

static void __mark_reg_not_init(const struct bpf_verifier_env *env,
				struct bpf_reg_state *reg)
{
	__mark_reg_unknown(env, reg);
	reg->type = NOT_INIT;
}

static void mark_reg_not_init(struct bpf_verifier_env *env,
			      struct bpf_reg_state *regs, u32 regno)
{
    BS_DBGASSERT(regno < MAX_BPF_REG);
	__mark_reg_not_init(env, regs + regno);
}

#define DEF_NOT_SUBREG	(0)
static void init_reg_state(struct bpf_verifier_env *env,
			   struct bpf_func_state *state)
{
	struct bpf_reg_state *regs = state->regs;
	int i;

	for (i = 0; i < MAX_BPF_REG; i++) {
		mark_reg_not_init(env, regs, i);
		regs[i].live = REG_LIVE_NONE;
		regs[i].parent = NULL;
		regs[i].subreg_def = DEF_NOT_SUBREG;
	}

	/* frame pointer */
	regs[BPF_REG_FP].type = PTR_TO_STACK;
	mark_reg_known_zero(env, regs, BPF_REG_FP);
	regs[BPF_REG_FP].frameno = state->frameno;
}

#define BPF_MAIN_FUNC (-1)
static void init_func_state(struct bpf_verifier_env *env,
			    struct bpf_func_state *state,
			    int callsite, int frameno, int subprogno)
{
	state->callsite = callsite;
	state->frameno = frameno;
	state->subprogno = subprogno;
	init_reg_state(env, state);
}

static int _mybpf_verifier_init_env(MYBPF_VERIFIER_ENV_S *env, int subprog)
{
	MYBPF_VERIFIER_STATE_S *state;
	struct bpf_reg_state *regs;

	state = MEM_ZMalloc(sizeof(MYBPF_VERIFIER_STATE_S));
	if (!state) {
        RETURN(BS_NO_MEMORY);
    }

	state->frame[0] = MEM_ZMalloc(sizeof(MYBPF_BPF_FUNC_STATE_S));
	if (!state->frame[0]) {
		MEM_Free(state);
        RETURN(BS_NO_MEMORY);
	}

	env->cur_state = state;
	init_func_state(env, state->frame[0], BPF_MAIN_FUNC, 0, subprog);

	regs = state->frame[state->curframe]->regs;
	if (subprog) {
		for (int i = BPF_REG_1; i <= BPF_REG_5; i++) {
			if (regs[i].type == PTR_TO_CTX)
				mark_reg_known_zero(env, regs, i);
			else if (regs[i].type == SCALAR_VALUE)
				mark_reg_unknown(env, regs, i);
			else if (base_type(regs[i].type) == PTR_TO_MEM) {
				const u32 mem_size = regs[i].mem_size;
				mark_reg_known_zero(env, regs, i);
				regs[i].mem_size = mem_size;
				regs[i].id = ++env->id_gen;
			}
		}
	} else {
		regs[BPF_REG_1].type = PTR_TO_CTX;
		mark_reg_known_zero(env, regs, BPF_REG_1);
	}

    return 0;
}

static void free_func_state(struct bpf_func_state *state)
{
	if (!state)
		return;
	MEM_Free(state->refs);
	MEM_Free(state->stack);
	MEM_Free(state);
}

static void clear_jmp_history(struct bpf_verifier_state *state)
{
	MEM_Free(state->jmp_history);
	state->jmp_history = NULL;
	state->jmp_history_cnt = 0;
}


static void free_verifier_state(struct bpf_verifier_state *state,
				bool free_self)
{
	int i;

	for (i = 0; i <= state->curframe; i++) {
		free_func_state(state->frame[i]);
		state->frame[i] = NULL;
	}
	clear_jmp_history(state);
	if (free_self)
		MEM_Free(state);
}

static void free_states(struct bpf_verifier_env *env)
{
	struct bpf_verifier_state_list *sl, *sln;
	int i;

	sl = env->free_list;
	while (sl) {
		sln = sl->next;
		free_verifier_state(&sl->state, false);
		MEM_Free(sl);
		sl = sln;
	}
	env->free_list = NULL;

	if (!env->explored_states)
		return;

	for (i = 0; i < env->insn_cnt; i++) {
		sl = env->explored_states[i];

		while (sl) {
			sln = sl->next;
			free_verifier_state(&sl->state, false);
			MEM_Free(sl);
			sl = sln;
		}
		env->explored_states[i] = NULL;
	}
}

static int _mybpf_verifier_do_check(MYBPF_VERIFIER_ENV_S *env, void *insts, int len)
{
	int prev_insn_idx = -1;
	int insn_cnt = len / sizeof(MYBPF_INSN_S);
	//MYBPF_INSN_S *insns = insts;

    for (;;) {
        //MYBPF_INSN_S *insn;
        //u8 class;
        //int err;

        env->prev_insn_idx = prev_insn_idx;
        if (env->insn_idx >= insn_cnt) {
            RETURNI(BS_ERR, "invalid insn idx %d insn_cnt %d\n", env->insn_idx, insn_cnt);
        }

        //insn = &insns[env->insn_idx];
		//class = BPF_CLASS(insn->opcode);

		if (++env->insn_processed > BPF_COMPLEXITY_LIMIT_INSNS) {
			RETURNI(BS_OUT_OF_RANGE,
				"BPF program is too large. Processed %d insn\n",
				env->insn_processed);
		}

        //TODO

    }

    return 0;
}

int MYBPF_VERIFIER_DoCheck(void *insts, int len/* 字节数 */, int subprog)
{
    MYBPF_VERIFIER_ENV_S the_env = {0};
    MYBPF_VERIFIER_ENV_S *env = &the_env;
    int ret;

    ret = _mybpf_verifier_init_env(env, subprog);
    if (ret < 0) {
        return ret;
    }

    ret = _mybpf_verifier_do_check(env, insts, len);

	if (env->cur_state) {
		free_verifier_state(env->cur_state, true);
		env->cur_state = NULL;
	}

//	while (!pop_stack(env, NULL, NULL, false));

	free_states(env);

	return ret;

}

