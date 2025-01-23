#pragma once

#include <uacpi/types.h>
#include <uacpi/acpi.h>

#ifdef __cplusplus
extern "C" {
#endif

uacpi_status uacpi_gas_read(const struct acpi_gas *gas, uacpi_u64 *value);
uacpi_status uacpi_gas_write(const struct acpi_gas *gas, uacpi_u64 value);

#ifdef __cplusplus
}
#endif
