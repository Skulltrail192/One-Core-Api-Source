#pragma once

#ifdef UACPI_OVERRIDE_LIBC
#include "uacpi_libc.h"
#else
/*
 * The following libc functions are used internally by uACPI and have a default
 * (sub-optimal) implementation:
 * - memcpy
 * - memset
 * - memcmp
 * - strcmp
 * - memmove
 * - strnlen
 * - strlen
 * - snprintf
 * - vsnprintf
 *
 * In case your platform happens to implement optimized verisons of the helpers
 * above, you are able to make uACPI use those instead by overriding them like so:
 *
 * #define uacpi_memcpy my_fast_memcpy
 * #define uacpi_snprintf my_fast_snprintf
 */
#endif
