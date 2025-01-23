#pragma once

#include <uacpi/types.h>

uacpi_status uacpi_ininitialize_registers(void);
void uacpi_deininitialize_registers(void);

enum uacpi_register {
    UACPI_REGISTER_PM1_STS = 0,
    UACPI_REGISTER_PM1_EN,
    UACPI_REGISTER_PM1_CNT,
    UACPI_REGISTER_PM_TMR,
    UACPI_REGISTER_PM2_CNT,
    UACPI_REGISTER_SLP_CNT,
    UACPI_REGISTER_SLP_STS,
    UACPI_REGISTER_RESET,
    UACPI_REGISTER_SMI_CMD,
    UACPI_REGISTER_MAX = UACPI_REGISTER_SMI_CMD,
};

uacpi_status uacpi_read_register(enum uacpi_register, uacpi_u64*);

uacpi_status uacpi_write_register(enum uacpi_register, uacpi_u64);
uacpi_status uacpi_write_registers(enum uacpi_register, uacpi_u64, uacpi_u64);

enum uacpi_register_field {
    UACPI_REGISTER_FIELD_TMR_STS = 0,
    UACPI_REGISTER_FIELD_BM_STS,
    UACPI_REGISTER_FIELD_GBL_STS,
    UACPI_REGISTER_FIELD_PWRBTN_STS,
    UACPI_REGISTER_FIELD_SLPBTN_STS,
    UACPI_REGISTER_FIELD_RTC_STS,
    UACPI_REGISTER_FIELD_PCIEX_WAKE_STS,
    UACPI_REGISTER_FIELD_HWR_WAK_STS,
    UACPI_REGISTER_FIELD_WAK_STS,
    UACPI_REGISTER_FIELD_TMR_EN,
    UACPI_REGISTER_FIELD_GBL_EN,
    UACPI_REGISTER_FIELD_PWRBTN_EN,
    UACPI_REGISTER_FIELD_SLPBTN_EN,
    UACPI_REGISTER_FIELD_RTC_EN,
    UACPI_REGISTER_FIELD_PCIEXP_WAKE_DIS,
    UACPI_REGISTER_FIELD_SCI_EN,
    UACPI_REGISTER_FIELD_BM_RLD,
    UACPI_REGISTER_FIELD_GBL_RLS,
    UACPI_REGISTER_FIELD_SLP_TYP,
    UACPI_REGISTER_FIELD_HWR_SLP_TYP,
    UACPI_REGISTER_FIELD_SLP_EN,
    UACPI_REGISTER_FIELD_HWR_SLP_EN,
    UACPI_REGISTER_FIELD_ARB_DIS,
    UACPI_REGISTER_FIELD_MAX = UACPI_REGISTER_FIELD_ARB_DIS,
};

uacpi_status uacpi_read_register_field(enum uacpi_register_field, uacpi_u64*);
uacpi_status uacpi_write_register_field(enum uacpi_register_field, uacpi_u64);
