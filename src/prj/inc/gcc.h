#pragma once

#include <stddef.h>

#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#define container_of(ptr, type, member) ({          \
	const typeof(((type *)0)->member)*__mptr = (ptr);    \
		     (type *)((char *)__mptr - offsetof(type, member)); })

#define compiler_barrier() do { \
	__asm__ __volatile__ ("" ::: "memory"); \
} while (0)

static ALWAYS_INLINE void __write_once_size(volatile void *p, void *res, int size)
{
	switch (size) {
	case 1: *(volatile unsigned char *)p  = *(unsigned char *)res;  break;
	case 2: *(volatile unsigned short *)p = *(unsigned short *)res; break;
	case 4: *(volatile unsigned int *)p   = *(unsigned int *)res;   break;
	case 8: *(volatile unsigned long long *)p = *(unsigned long long *)res; break;
	default:
		compiler_barrier();
		__builtin_memcpy((void *)p, (const void *)res, size);
		compiler_barrier();
	}
}

static ALWAYS_INLINE void __read_once_size(const volatile void *p, void *res, int size)
{
	switch (size) {
	case 1: *(unsigned char *)res  = *(volatile unsigned char *)p;  break;
	case 2: *(unsigned short *)res = *(volatile unsigned short *)p; break;
	case 4: *(unsigned int *)res   = *(volatile unsigned int *)p;   break;
	case 8: *(unsigned long long *)res = *(volatile unsigned long long *)p; break;
	default:
		compiler_barrier();
		__builtin_memcpy((void *)res, (const void *)p, size);
		compiler_barrier();
	}
}

#define WRITE_ONCE(x, val) \
({									\
	union { typeof(x) __val; char __c[1]; } __u =			\
		{ .__val = (typeof(x))(val) };				\
	__write_once_size(&(x), __u.__c, sizeof(x));			\
	__u.__val;							\
})

#define READ_ONCE(x) \
({									\
	union { typeof(x) __val; char __c[1]; } __u;			\
	__read_once_size(&(x), __u.__c, sizeof(x));			\
	__u.__val;							\
})

#ifndef __aligned
#define __aligned(x)	__attribute__((__aligned__(x)))
#endif

#define Z_STRINGIFY(x) #x
#define STRINGIFY(s) Z_STRINGIFY(s)
#define __GENERIC_SECTION(segment) __attribute__((section(STRINGIFY(segment))))
#define GENERIC_SECTION(segment) __GENERIC_SECTION(segment)
