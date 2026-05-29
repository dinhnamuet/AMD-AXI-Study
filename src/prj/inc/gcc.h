#pragma once

#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#define container_of(ptr, type, member) ({          \
	const typeof(((type *)0)->member)*__mptr = (ptr);    \
		     (type *)((char *)__mptr - offsetof(type, member)); })

#define compiler_barrier() do { \
	__asm__ __volatile__ ("" ::: "memory"); \
} while (0)

#ifndef __aligned
#define __aligned(x)	__attribute__((__aligned__(x)))
#endif

#define Z_STRINGIFY(x) #x
#define STRINGIFY(s) Z_STRINGIFY(s)
#define __GENERIC_SECTION(segment) __attribute__((section(STRINGIFY(segment))))
#define GENERIC_SECTION(segment) __GENERIC_SECTION(segment)
