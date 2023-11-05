// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
/* Copyright (c) 2021 Facebook */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <linux/err.h>
#include "hashmap.h"
#include "libbpf_internal.h"
#include "strset.h"

struct strset {
	void *strs_data;
	size_t strs_data_len;
	size_t strs_data_cap;
	size_t strs_data_max_len;

	
	struct hashmap *strs_hash;
};

static size_t strset_hash_fn(long key, void *ctx)
{
	const struct strset *s = ctx;
	const char *str = s->strs_data + key;

	return str_hash(str);
}

static bool strset_equal_fn(long key1, long key2, void *ctx)
{
	const struct strset *s = ctx;
	const char *str1 = s->strs_data + key1;
	const char *str2 = s->strs_data + key2;

	return strcmp(str1, str2) == 0;
}

struct strset *strset__new(size_t max_data_sz, const char *init_data, size_t init_data_sz)
{
	struct strset *set = calloc(1, sizeof(*set));
	struct hashmap *hash;
	int err = -ENOMEM;

	if (!set)
		return ERR_PTR(-ENOMEM);

	hash = hashmap__new(strset_hash_fn, strset_equal_fn, set);
	if (IS_ERR(hash))
		goto err_out;

	set->strs_data_max_len = max_data_sz;
	set->strs_hash = hash;

	if (init_data) {
		long off;

		set->strs_data = malloc(init_data_sz);
		if (!set->strs_data)
			goto err_out;

		memcpy(set->strs_data, init_data, init_data_sz);
		set->strs_data_len = init_data_sz;
		set->strs_data_cap = init_data_sz;

		for (off = 0; off < set->strs_data_len; off += strlen(set->strs_data + off) + 1) {
			
			err = hashmap__add(hash, off, off);
			if (err == -EEXIST)
				continue; 
			if (err)
				goto err_out;
		}
	}

	return set;
err_out:
	strset__free(set);
	return ERR_PTR(err);
}

void strset__free(struct strset *set)
{
	if (IS_ERR_OR_NULL(set))
		return;

	hashmap__free(set->strs_hash);
	free(set->strs_data);
	free(set);
}

size_t strset__data_size(const struct strset *set)
{
	return set->strs_data_len;
}

const char *strset__data(const struct strset *set)
{
	return set->strs_data;
}

static void *strset_add_str_mem(struct strset *set, size_t add_sz)
{
	return libbpf_add_mem(&set->strs_data, &set->strs_data_cap, 1,
			      set->strs_data_len, set->strs_data_max_len, add_sz);
}


int strset__find_str(struct strset *set, const char *s)
{
	long old_off, new_off, len;
	void *p;

	
	len = strlen(s) + 1;
	p = strset_add_str_mem(set, len);
	if (!p)
		return -ENOMEM;

	new_off = set->strs_data_len;
	memcpy(p, s, len);

	if (hashmap__find(set->strs_hash, new_off, &old_off))
		return old_off;

	return -ENOENT;
}


int strset__add_str(struct strset *set, const char *s)
{
	long old_off, new_off, len;
	void *p;
	int err;

	
	len = strlen(s) + 1;
	p = strset_add_str_mem(set, len);
	if (!p)
		return -ENOMEM;

	new_off = set->strs_data_len;
	memcpy(p, s, len);

	
	err = hashmap__insert(set->strs_hash, new_off, new_off,
			      HASHMAP_ADD, &old_off, NULL);
	if (err == -EEXIST)
		return old_off; 
	if (err)
		return err;

	set->strs_data_len += len; 
	return new_off;
}
