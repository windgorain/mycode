/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#ifndef _MYBPF_BASEOS_TNUM_H
#define _MYBPF_BASEOS_TNUM_H
#ifdef __cplusplus
extern "C"
{
#endif

#define TNUM(_v, _m)	(struct tnum){.value = _v, .mask = _m}

struct tnum {
	u64 value;
	u64 mask;
};

static inline struct tnum tnum_const(u64 value)
{
	return TNUM(value, 0);
}

#ifdef __cplusplus
}
#endif
#endif 
