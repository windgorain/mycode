/*
 * =====================================================================================
 *       Filename:  types.h
 *       Created:  2018/12/24 10时45分52秒
 * =====================================================================================
 */
#ifndef _TYPES_H
#define _TYPES_H

#ifndef uint8_t 
#define uint8_t unsigned char
#endif
#ifndef uint16_t 
#define uint16_t unsigned short
#endif
#ifndef uint32_t 
#define uint32_t unsigned int
#endif

#ifndef NULL 
#define NULL 0
#endif

#ifndef likely
#ifndef __glibc_likely
#define __glibc_unlikely(cond) __builtin_expect((cond), 0)
#define __glibc_likely(cond) __builtin_expect((cond), 1)
#endif
#define likely(__expr)      __glibc_likely(!!(__expr))
#define unlikely(__expr)    __glibc_unlikely(!!(__expr))
#endif

#define bit_value(_vtype, bit) ((_vtype)1 << bit)
#endif
