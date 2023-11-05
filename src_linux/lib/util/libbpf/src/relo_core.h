/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */
/* Copyright (c) 2019 Facebook */

#ifndef __RELO_CORE_H
#define __RELO_CORE_H

#include <linux/bpf.h>

struct bpf_core_cand {
	const struct btf *btf;
	__u32 id;
};


struct bpf_core_cand_list {
	struct bpf_core_cand *cands;
	int len;
};

#define BPF_CORE_SPEC_MAX_LEN 64


struct bpf_core_accessor {
	__u32 type_id;		
	__u32 idx;		
	const char *name;	
};

struct bpf_core_spec {
	const struct btf *btf;
	
	struct bpf_core_accessor spec[BPF_CORE_SPEC_MAX_LEN];
	
	__u32 root_type_id;
	
	enum bpf_core_relo_kind relo_kind;
	
	int len;
	
	int raw_spec[BPF_CORE_SPEC_MAX_LEN];
	
	int raw_len;
	
	__u32 bit_offset;
};

struct bpf_core_relo_res {
	
	__u64 orig_val;
	
	__u64 new_val;
	
	bool poison;
	
	bool validate;
	
	bool fail_memsz_adjust;
	__u32 orig_sz;
	__u32 orig_type_id;
	__u32 new_sz;
	__u32 new_type_id;
};

int __bpf_core_types_are_compat(const struct btf *local_btf, __u32 local_id,
				const struct btf *targ_btf, __u32 targ_id, int level);
int bpf_core_types_are_compat(const struct btf *local_btf, __u32 local_id,
			      const struct btf *targ_btf, __u32 targ_id);
int __bpf_core_types_match(const struct btf *local_btf, __u32 local_id, const struct btf *targ_btf,
			   __u32 targ_id, bool behind_ptr, int level);
int bpf_core_types_match(const struct btf *local_btf, __u32 local_id, const struct btf *targ_btf,
			 __u32 targ_id);

size_t bpf_core_essential_name_len(const char *name);

int bpf_core_calc_relo_insn(const char *prog_name,
			    const struct bpf_core_relo *relo, int relo_idx,
			    const struct btf *local_btf,
			    struct bpf_core_cand_list *cands,
			    struct bpf_core_spec *specs_scratch,
			    struct bpf_core_relo_res *targ_res);

int bpf_core_patch_insn(const char *prog_name, struct bpf_insn *insn,
			int insn_idx, const struct bpf_core_relo *relo,
			int relo_idx, const struct bpf_core_relo_res *res);

int bpf_core_parse_spec(const char *prog_name, const struct btf *btf,
		        const struct bpf_core_relo *relo,
		        struct bpf_core_spec *spec);

int bpf_core_format_spec(char *buf, size_t buf_sz, const struct bpf_core_spec *spec);

#endif
