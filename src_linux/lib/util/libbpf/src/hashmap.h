/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */

/*
 * Generic non-thread safe hash map implementation.
 *
 * Copyright (c) 2019 Facebook
 */
#ifndef __LIBBPF_HASHMAP_H
#define __LIBBPF_HASHMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

static inline size_t hash_bits(size_t h, int bits)
{
	
	if (bits == 0)
		return 0;

#if (__SIZEOF_SIZE_T__ == __SIZEOF_LONG_LONG__)
	
	return (h * 11400714819323198485llu) >> (__SIZEOF_LONG_LONG__ * 8 - bits);
#elif (__SIZEOF_SIZE_T__ <= __SIZEOF_LONG__)
	return (h * 2654435769lu) >> (__SIZEOF_LONG__ * 8 - bits);
#else
#	error "Unsupported size_t size"
#endif
}


static inline size_t str_hash(const char *s)
{
	size_t h = 0;

	while (*s) {
		h = h * 31 + *s;
		s++;
	}
	return h;
}

typedef size_t (*hashmap_hash_fn)(long key, void *ctx);
typedef bool (*hashmap_equal_fn)(long key1, long key2, void *ctx);


struct hashmap_entry {
	union {
		long key;
		const void *pkey;
	};
	union {
		long value;
		void *pvalue;
	};
	struct hashmap_entry *next;
};

struct hashmap {
	hashmap_hash_fn hash_fn;
	hashmap_equal_fn equal_fn;
	void *ctx;

	struct hashmap_entry **buckets;
	size_t cap;
	size_t cap_bits;
	size_t sz;
};

#define HASHMAP_INIT(hash_fn, equal_fn, ctx) {	\
	.hash_fn = (hash_fn),			\
	.equal_fn = (equal_fn),			\
	.ctx = (ctx),				\
	.buckets = NULL,			\
	.cap = 0,				\
	.cap_bits = 0,				\
	.sz = 0,				\
}

void hashmap__init(struct hashmap *map, hashmap_hash_fn hash_fn,
		   hashmap_equal_fn equal_fn, void *ctx);
struct hashmap *hashmap__new(hashmap_hash_fn hash_fn,
			     hashmap_equal_fn equal_fn,
			     void *ctx);
void hashmap__clear(struct hashmap *map);
void hashmap__free(struct hashmap *map);

size_t hashmap__size(const struct hashmap *map);
size_t hashmap__capacity(const struct hashmap *map);


enum hashmap_insert_strategy {
	HASHMAP_ADD,
	HASHMAP_SET,
	HASHMAP_UPDATE,
	HASHMAP_APPEND,
};

#define hashmap_cast_ptr(p) ({								\
	_Static_assert((__builtin_constant_p((p)) ? (p) == NULL : 0) ||			\
				sizeof(*(p)) == sizeof(long),				\
		       #p " pointee should be a long-sized integer or a pointer");	\
	(long *)(p);									\
})


int hashmap_insert(struct hashmap *map, long key, long value,
		   enum hashmap_insert_strategy strategy,
		   long *old_key, long *old_value);

#define hashmap__insert(map, key, value, strategy, old_key, old_value) \
	hashmap_insert((map), (long)(key), (long)(value), (strategy),  \
		       hashmap_cast_ptr(old_key),		       \
		       hashmap_cast_ptr(old_value))

#define hashmap__add(map, key, value) \
	hashmap__insert((map), (key), (value), HASHMAP_ADD, NULL, NULL)

#define hashmap__set(map, key, value, old_key, old_value) \
	hashmap__insert((map), (key), (value), HASHMAP_SET, (old_key), (old_value))

#define hashmap__update(map, key, value, old_key, old_value) \
	hashmap__insert((map), (key), (value), HASHMAP_UPDATE, (old_key), (old_value))

#define hashmap__append(map, key, value) \
	hashmap__insert((map), (key), (value), HASHMAP_APPEND, NULL, NULL)

bool hashmap_delete(struct hashmap *map, long key, long *old_key, long *old_value);

#define hashmap__delete(map, key, old_key, old_value)		       \
	hashmap_delete((map), (long)(key),			       \
		       hashmap_cast_ptr(old_key),		       \
		       hashmap_cast_ptr(old_value))

bool hashmap_find(const struct hashmap *map, long key, long *value);

#define hashmap__find(map, key, value) \
	hashmap_find((map), (long)(key), hashmap_cast_ptr(value))


#define hashmap__for_each_entry(map, cur, bkt)				    \
	for (bkt = 0; bkt < map->cap; bkt++)				    \
		for (cur = map->buckets[bkt]; cur; cur = cur->next)


#define hashmap__for_each_entry_safe(map, cur, tmp, bkt)		    \
	for (bkt = 0; bkt < map->cap; bkt++)				    \
		for (cur = map->buckets[bkt];				    \
		     cur && ({tmp = cur->next; true; });		    \
		     cur = tmp)


#define hashmap__for_each_key_entry(map, cur, _key)			    \
	for (cur = map->buckets						    \
		     ? map->buckets[hash_bits(map->hash_fn((_key), map->ctx), map->cap_bits)] \
		     : NULL;						    \
	     cur;							    \
	     cur = cur->next)						    \
		if (map->equal_fn(cur->key, (_key), map->ctx))

#define hashmap__for_each_key_entry_safe(map, cur, tmp, _key)		    \
	for (cur = map->buckets						    \
		     ? map->buckets[hash_bits(map->hash_fn((_key), map->ctx), map->cap_bits)] \
		     : NULL;						    \
	     cur && ({ tmp = cur->next; true; });			    \
	     cur = tmp)							    \
		if (map->equal_fn(cur->key, (_key), map->ctx))

#endif 
