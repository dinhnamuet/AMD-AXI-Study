#pragma once
#include "gcc.h"
#include "xil_types.h"

static ALWAYS_INLINE void io_write32(UINTPTR addr, u32 data)
{
	compiler_barrier();
	*(volatile u32 *)addr = data;
	compiler_barrier();
}

static ALWAYS_INLINE u32 io_read32(UINTPTR addr)
{
	u32 value;

	compiler_barrier();
	value = *(volatile u32 *)addr;
	compiler_barrier();

	return value;
}
