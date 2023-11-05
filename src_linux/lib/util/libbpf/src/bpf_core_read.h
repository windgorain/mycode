/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */
#ifndef __BPF_CORE_READ_H__
#define __BPF_CORE_READ_H__


enum bpf_field_info_kind {
	BPF_FIELD_BYTE_OFFSET = 0,	
	BPF_FIELD_BYTE_SIZE = 1,
	BPF_FIELD_EXISTS = 2,		
	BPF_FIELD_SIGNED = 3,
	BPF_FIELD_LSHIFT_U64 = 4,
	BPF_FIELD_RSHIFT_U64 = 5,
};


enum bpf_type_id_kind {
	BPF_TYPE_ID_LOCAL = 0,		
	BPF_TYPE_ID_TARGET = 1,		
};


enum bpf_type_info_kind {
	BPF_TYPE_EXISTS = 0,		
	BPF_TYPE_SIZE = 1,		
	BPF_TYPE_MATCHES = 2,		
};


enum bpf_enum_value_kind {
	BPF_ENUMVAL_EXISTS = 0,		
	BPF_ENUMVAL_VALUE = 1,		
};

#define __CORE_RELO(src, field, info)					      \
	__builtin_preserve_field_info((src)->field, BPF_FIELD_##info)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __CORE_BITFIELD_PROBE_READ(dst, src, fld)			      \
	bpf_probe_read_kernel(						      \
			(void *)dst,				      \
			__CORE_RELO(src, fld, BYTE_SIZE),		      \
			(const void *)src + __CORE_RELO(src, fld, BYTE_OFFSET))
#else

#define __CORE_BITFIELD_PROBE_READ(dst, src, fld)			      \
	bpf_probe_read_kernel(						      \
			(void *)dst + (8 - __CORE_RELO(src, fld, BYTE_SIZE)), \
			__CORE_RELO(src, fld, BYTE_SIZE),		      \
			(const void *)src + __CORE_RELO(src, fld, BYTE_OFFSET))
#endif


#define BPF_CORE_READ_BITFIELD_PROBED(s, field) ({			      \
	unsigned long long val = 0;					      \
									      \
	__CORE_BITFIELD_PROBE_READ(&val, s, field);			      \
	val <<= __CORE_RELO(s, field, LSHIFT_U64);			      \
	if (__CORE_RELO(s, field, SIGNED))				      \
		val = ((long long)val) >> __CORE_RELO(s, field, RSHIFT_U64);  \
	else								      \
		val = val >> __CORE_RELO(s, field, RSHIFT_U64);		      \
	val;								      \
})


#define BPF_CORE_READ_BITFIELD(s, field) ({				      \
	const void *p = (const void *)s + __CORE_RELO(s, field, BYTE_OFFSET); \
	unsigned long long val;						      \
									      \
									      \
	asm volatile("" : "=r"(p) : "0"(p));				      \
									      \
	switch (__CORE_RELO(s, field, BYTE_SIZE)) {			      \
	case 1: val = *(const unsigned char *)p; break;			      \
	case 2: val = *(const unsigned short *)p; break;		      \
	case 4: val = *(const unsigned int *)p; break;			      \
	case 8: val = *(const unsigned long long *)p; break;		      \
	}								      \
	val <<= __CORE_RELO(s, field, LSHIFT_U64);			      \
	if (__CORE_RELO(s, field, SIGNED))				      \
		val = ((long long)val) >> __CORE_RELO(s, field, RSHIFT_U64);  \
	else								      \
		val = val >> __CORE_RELO(s, field, RSHIFT_U64);		      \
	val;								      \
})

#define ___bpf_field_ref1(field)	(field)
#define ___bpf_field_ref2(type, field)	(((typeof(type) *)0)->field)
#define ___bpf_field_ref(args...)					    \
	___bpf_apply(___bpf_field_ref, ___bpf_narg(args))(args)


#define bpf_core_field_exists(field...)					    \
	__builtin_preserve_field_info(___bpf_field_ref(field), BPF_FIELD_EXISTS)


#define bpf_core_field_size(field...)					    \
	__builtin_preserve_field_info(___bpf_field_ref(field), BPF_FIELD_BYTE_SIZE)


#define bpf_core_field_offset(field...)					    \
	__builtin_preserve_field_info(___bpf_field_ref(field), BPF_FIELD_BYTE_OFFSET)


#define bpf_core_type_id_local(type)					    \
	__builtin_btf_type_id(*(typeof(type) *)0, BPF_TYPE_ID_LOCAL)


#define bpf_core_type_id_kernel(type)					    \
	__builtin_btf_type_id(*(typeof(type) *)0, BPF_TYPE_ID_TARGET)


#define bpf_core_type_exists(type)					    \
	__builtin_preserve_type_info(*(typeof(type) *)0, BPF_TYPE_EXISTS)


#define bpf_core_type_matches(type)					    \
	__builtin_preserve_type_info(*(typeof(type) *)0, BPF_TYPE_MATCHES)


#define bpf_core_type_size(type)					    \
	__builtin_preserve_type_info(*(typeof(type) *)0, BPF_TYPE_SIZE)


#define bpf_core_enum_value_exists(enum_type, enum_value)		    \
	__builtin_preserve_enum_value(*(typeof(enum_type) *)enum_value, BPF_ENUMVAL_EXISTS)


#define bpf_core_enum_value(enum_type, enum_value)			    \
	__builtin_preserve_enum_value(*(typeof(enum_type) *)enum_value, BPF_ENUMVAL_VALUE)


#define bpf_core_read(dst, sz, src)					    \
	bpf_probe_read_kernel(dst, sz, (const void *)__builtin_preserve_access_index(src))


#define bpf_core_read_user(dst, sz, src)				    \
	bpf_probe_read_user(dst, sz, (const void *)__builtin_preserve_access_index(src))

#define bpf_core_read_str(dst, sz, src)					    \
	bpf_probe_read_kernel_str(dst, sz, (const void *)__builtin_preserve_access_index(src))


#define bpf_core_read_user_str(dst, sz, src)				    \
	bpf_probe_read_user_str(dst, sz, (const void *)__builtin_preserve_access_index(src))

#define ___concat(a, b) a ## b
#define ___apply(fn, n) ___concat(fn, n)
#define ___nth(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, __11, N, ...) N


#define ___narg(...) ___nth(_, ##__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define ___empty(...) ___nth(_, ##__VA_ARGS__, N, N, N, N, N, N, N, N, N, N, 0)

#define ___last1(x) x
#define ___last2(a, x) x
#define ___last3(a, b, x) x
#define ___last4(a, b, c, x) x
#define ___last5(a, b, c, d, x) x
#define ___last6(a, b, c, d, e, x) x
#define ___last7(a, b, c, d, e, f, x) x
#define ___last8(a, b, c, d, e, f, g, x) x
#define ___last9(a, b, c, d, e, f, g, h, x) x
#define ___last10(a, b, c, d, e, f, g, h, i, x) x
#define ___last(...) ___apply(___last, ___narg(__VA_ARGS__))(__VA_ARGS__)

#define ___nolast2(a, _) a
#define ___nolast3(a, b, _) a, b
#define ___nolast4(a, b, c, _) a, b, c
#define ___nolast5(a, b, c, d, _) a, b, c, d
#define ___nolast6(a, b, c, d, e, _) a, b, c, d, e
#define ___nolast7(a, b, c, d, e, f, _) a, b, c, d, e, f
#define ___nolast8(a, b, c, d, e, f, g, _) a, b, c, d, e, f, g
#define ___nolast9(a, b, c, d, e, f, g, h, _) a, b, c, d, e, f, g, h
#define ___nolast10(a, b, c, d, e, f, g, h, i, _) a, b, c, d, e, f, g, h, i
#define ___nolast(...) ___apply(___nolast, ___narg(__VA_ARGS__))(__VA_ARGS__)

#define ___arrow1(a) a
#define ___arrow2(a, b) a->b
#define ___arrow3(a, b, c) a->b->c
#define ___arrow4(a, b, c, d) a->b->c->d
#define ___arrow5(a, b, c, d, e) a->b->c->d->e
#define ___arrow6(a, b, c, d, e, f) a->b->c->d->e->f
#define ___arrow7(a, b, c, d, e, f, g) a->b->c->d->e->f->g
#define ___arrow8(a, b, c, d, e, f, g, h) a->b->c->d->e->f->g->h
#define ___arrow9(a, b, c, d, e, f, g, h, i) a->b->c->d->e->f->g->h->i
#define ___arrow10(a, b, c, d, e, f, g, h, i, j) a->b->c->d->e->f->g->h->i->j
#define ___arrow(...) ___apply(___arrow, ___narg(__VA_ARGS__))(__VA_ARGS__)

#define ___type(...) typeof(___arrow(__VA_ARGS__))

#define ___read(read_fn, dst, src_type, src, accessor)			    \
	read_fn((void *)(dst), sizeof(*(dst)), &((src_type)(src))->accessor)


#define ___rd_first(fn, src, a) ___read(fn, &__t, ___type(src), src, a);
#define ___rd_last(fn, ...)						    \
	___read(fn, &__t, ___type(___nolast(__VA_ARGS__)), __t, ___last(__VA_ARGS__));
#define ___rd_p1(fn, ...) const void *__t; ___rd_first(fn, __VA_ARGS__)
#define ___rd_p2(fn, ...) ___rd_p1(fn, ___nolast(__VA_ARGS__)) ___rd_last(fn, __VA_ARGS__)
#define ___rd_p3(fn, ...) ___rd_p2(fn, ___nolast(__VA_ARGS__)) ___rd_last(fn, __VA_ARGS__)
#define ___rd_p4(fn, ...) ___rd_p3(fn, ___nolast(__VA_ARGS__)) ___rd_last(fn, __VA_ARGS__)
#define ___rd_p5(fn, ...) ___rd_p4(fn, ___nolast(__VA_ARGS__)) ___rd_last(fn, __VA_ARGS__)
#define ___rd_p6(fn, ...) ___rd_p5(fn, ___nolast(__VA_ARGS__)) ___rd_last(fn, __VA_ARGS__)
#define ___rd_p7(fn, ...) ___rd_p6(fn, ___nolast(__VA_ARGS__)) ___rd_last(fn, __VA_ARGS__)
#define ___rd_p8(fn, ...) ___rd_p7(fn, ___nolast(__VA_ARGS__)) ___rd_last(fn, __VA_ARGS__)
#define ___rd_p9(fn, ...) ___rd_p8(fn, ___nolast(__VA_ARGS__)) ___rd_last(fn, __VA_ARGS__)
#define ___read_ptrs(fn, src, ...)					    \
	___apply(___rd_p, ___narg(__VA_ARGS__))(fn, src, __VA_ARGS__)

#define ___core_read0(fn, fn_ptr, dst, src, a)				    \
	___read(fn, dst, ___type(src), src, a);
#define ___core_readN(fn, fn_ptr, dst, src, ...)			    \
	___read_ptrs(fn_ptr, src, ___nolast(__VA_ARGS__))		    \
	___read(fn, dst, ___type(src, ___nolast(__VA_ARGS__)), __t,	    \
		___last(__VA_ARGS__));
#define ___core_read(fn, fn_ptr, dst, src, a, ...)			    \
	___apply(___core_read, ___empty(__VA_ARGS__))(fn, fn_ptr, dst,	    \
						      src, a, ##__VA_ARGS__)


#define BPF_CORE_READ_INTO(dst, src, a, ...) ({				    \
	___core_read(bpf_core_read, bpf_core_read,			    \
		     dst, (src), a, ##__VA_ARGS__)			    \
})


#define BPF_CORE_READ_USER_INTO(dst, src, a, ...) ({			    \
	___core_read(bpf_core_read_user, bpf_core_read_user,		    \
		     dst, (src), a, ##__VA_ARGS__)			    \
})


#define BPF_PROBE_READ_INTO(dst, src, a, ...) ({			    \
	___core_read(bpf_probe_read_kernel, bpf_probe_read_kernel,	    \
		     dst, (src), a, ##__VA_ARGS__)			    \
})


#define BPF_PROBE_READ_USER_INTO(dst, src, a, ...) ({			    \
	___core_read(bpf_probe_read_user, bpf_probe_read_user,		    \
		     dst, (src), a, ##__VA_ARGS__)			    \
})


#define BPF_CORE_READ_STR_INTO(dst, src, a, ...) ({			    \
	___core_read(bpf_core_read_str, bpf_core_read,			    \
		     dst, (src), a, ##__VA_ARGS__)			    \
})


#define BPF_CORE_READ_USER_STR_INTO(dst, src, a, ...) ({		    \
	___core_read(bpf_core_read_user_str, bpf_core_read_user,	    \
		     dst, (src), a, ##__VA_ARGS__)			    \
})


#define BPF_PROBE_READ_STR_INTO(dst, src, a, ...) ({			    \
	___core_read(bpf_probe_read_kernel_str, bpf_probe_read_kernel,	    \
		     dst, (src), a, ##__VA_ARGS__)			    \
})


#define BPF_PROBE_READ_USER_STR_INTO(dst, src, a, ...) ({		    \
	___core_read(bpf_probe_read_user_str, bpf_probe_read_user,	    \
		     dst, (src), a, ##__VA_ARGS__)			    \
})


#define BPF_CORE_READ(src, a, ...) ({					    \
	___type((src), a, ##__VA_ARGS__) __r;				    \
	BPF_CORE_READ_INTO(&__r, (src), a, ##__VA_ARGS__);		    \
	__r;								    \
})


#define BPF_CORE_READ_USER(src, a, ...) ({				    \
	___type((src), a, ##__VA_ARGS__) __r;				    \
	BPF_CORE_READ_USER_INTO(&__r, (src), a, ##__VA_ARGS__);		    \
	__r;								    \
})


#define BPF_PROBE_READ(src, a, ...) ({					    \
	___type((src), a, ##__VA_ARGS__) __r;				    \
	BPF_PROBE_READ_INTO(&__r, (src), a, ##__VA_ARGS__);		    \
	__r;								    \
})


#define BPF_PROBE_READ_USER(src, a, ...) ({				    \
	___type((src), a, ##__VA_ARGS__) __r;				    \
	BPF_PROBE_READ_USER_INTO(&__r, (src), a, ##__VA_ARGS__);	    \
	__r;								    \
})

#endif

