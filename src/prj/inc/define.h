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
#define EBUSY           7

/** number of nanoseconds per microsecond */
#define NSEC_PER_USEC 1000U

/** number of nanoseconds per millisecond */
#define NSEC_PER_MSEC 1000000U

/** number of microseconds per millisecond */
#define USEC_PER_MSEC 1000U

/** number of milliseconds per second */
#define MSEC_PER_SEC 1000U

/** number of seconds per minute */
#define SEC_PER_MIN 60U

/** number of seconds per hour */
#define SEC_PER_HOUR 3600U

/** number of seconds per day */
#define SEC_PER_DAY 86400U

/** number of minutes per hour */
#define MIN_PER_HOUR 60U

/** number of hours per day */
#define HOUR_PER_DAY 24U

/** number of microseconds per second */
#define USEC_PER_SEC ((USEC_PER_MSEC) * (MSEC_PER_SEC))

/** number of nanoseconds per second */
#define NSEC_PER_SEC ((NSEC_PER_USEC) * (USEC_PER_MSEC) * (MSEC_PER_SEC))
