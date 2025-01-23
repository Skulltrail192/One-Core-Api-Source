#pragma once

#include <uacpi/internal/types.h>
#include <uacpi/internal/helpers.h>
#include <uacpi/platform/libc.h>
#include <uacpi/kernel_api.h>

#ifndef uacpi_memcpy
void *uacpi_memcpy(void *dest, const void *src, uacpi_size count);
#endif

#ifndef uacpi_memmove
void *uacpi_memmove(void *dest, const void *src, uacpi_size count);
#endif

#ifndef uacpi_memset
void *uacpi_memset(void *dest, uacpi_i32 ch, uacpi_size count);
#endif

#ifndef uacpi_memcmp
uacpi_i32 uacpi_memcmp(const void *lhs, const void *rhs, uacpi_size count);
#endif

#ifndef uacpi_strlen
uacpi_size uacpi_strlen(const uacpi_char *str);
#endif

#ifndef uacpi_strnlen
uacpi_size uacpi_strnlen(const uacpi_char *str, uacpi_size max);
#endif

#ifndef uacpi_strcmp
uacpi_i32 uacpi_strcmp(const uacpi_char *lhs, const uacpi_char *rhs);
#endif

#ifndef uacpi_snprintf
UACPI_PRINTF_DECL(3, 4)
uacpi_i32 uacpi_snprintf(
    uacpi_char *buffer, uacpi_size capacity, const uacpi_char *fmt, ...
);
#endif

#ifndef uacpi_vsnprintf
uacpi_i32 uacpi_vsnprintf(
    uacpi_char *buffer, uacpi_size capacity, const uacpi_char *fmt,
    uacpi_va_list vlist
);
#endif

#ifdef UACPI_SIZED_FREES
#define uacpi_free(mem, size) uacpi_kernel_free(mem, size)
#else
#define uacpi_free(mem, _) uacpi_kernel_free(mem)
#endif

#define uacpi_memzero(ptr, size) uacpi_memset(ptr, 0, size)

#define UACPI_COMPARE(x, y, op) ((x) op (y) ? (x) : (y))
#define UACPI_MIN(x, y) UACPI_COMPARE(x, y, <)
#define UACPI_MAX(x, y) UACPI_COMPARE(x, y, >)

#define UACPI_ALIGN_UP_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define UACPI_ALIGN_UP(x, val, type) UACPI_ALIGN_UP_MASK(x, (type)(val) - 1)

#define UACPI_ALIGN_DOWN_MASK(x, mask) ((x) & ~(mask))
#define UACPI_ALIGN_DOWN(x, val, type) UACPI_ALIGN_DOWN_MASK(x, (type)(val) - 1)

#define UACPI_IS_ALIGNED_MASK(x, mask) (((x) & (mask)) == 0)
#define UACPI_IS_ALIGNED(x, val, type) UACPI_IS_ALIGNED_MASK(x, (type)(val) - 1)

#define UACPI_IS_POWER_OF_TWO(x, type) UACPI_IS_ALIGNED(x, x, type)

void uacpi_memcpy_zerout(void *dst, const void *src,
                         uacpi_size dst_size, uacpi_size src_size);

// Returns the one-based bit location of LSb or 0
uacpi_u8 uacpi_bit_scan_forward(uacpi_u64);

// Returns the one-based bit location of MSb or 0
uacpi_u8 uacpi_bit_scan_backward(uacpi_u64);

uacpi_u8 uacpi_popcount(uacpi_u64);

#ifndef UACPI_NATIVE_ALLOC_ZEROED
void *uacpi_builtin_alloc_zeroed(uacpi_size size);
#define uacpi_kernel_alloc_zeroed uacpi_builtin_alloc_zeroed
#endif
