/*********************************************************
*   Copyright (C) LiXingang
*   Description: 给ULC用户使用的头文件
*
********************************************************/
#ifndef _ULC_USER_H
#define _ULC_USER_H

#ifdef IN_ULC_USER

#include <stddef.h>
#include "utl/int_types.h"
#include "utl/bpf_helper_utl.h"
#include "utl/int_types.h"
#include "utl/ulc_def.h"
#include "utl/ulc_user_def.h"
#include "utl/ulc_user_base.h"
#include "utl/ulc_user_sys.h"
#include "utl/ulc_user_user.h"

#ifndef noinline
#define noinline __attribute__((noinline))
#endif

#undef SEC
#define SEC(NAME) __attribute__((section(NAME), used))

#ifndef NULL
#define NULL 0
#endif


#endif

#endif //ULC_USER_H_
