#pragma once

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define ROUND_UP(x, align)                                                                         \
	((((unsigned long)(x) + ((unsigned long)(align) - 1)) / (unsigned long)(align)) *          \
	 (unsigned long)(align))

#define BIT(x)          (1UL << (x))

#define EOK				0
#define EFAULT			1
#define ENODEV			2
#define ENOMEM			3
#define EEXIST			4
#define EIO				5
#define EINVAL			6
