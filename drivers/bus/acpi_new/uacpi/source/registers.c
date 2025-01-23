#include <uacpi/internal/registers.h>
#include <uacpi/internal/stdlib.h>
#include <uacpi/internal/context.h>
#include <uacpi/internal/io.h>
#include <uacpi/acpi.h>

enum register_kind {
    REGISTER_KIND_GAS,
    REGISTER_KIND_IO,
};

enum register_access_kind {
    REGISTER_ACCESS_KIND_PRESERVE,
    REGISTER_ACCESS_KIND_WRITE_TO_CLEAR,
    REGISTER_ACCESS_KIND_NORMAL,
};

struct register_spec {
    uacpi_u8 kind;
    uacpi_u8 access_kind;
    uacpi_u8 access_width; // only REGISTER_KIND_IO
    void *accessor0, *accessor1;
    uacpi_u64 write_only_mask;
    uacpi_u64 preserve_mask;
};

static const struct register_spec registers[UACPI_REGISTER_MAX + 1] = {
    [UACPI_REGISTER_PM1_STS] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_WRITE_TO_CLEAR,
        .accessor0 = &g_uacpi_rt_ctx.pm1a_status_blk,
        .accessor1 = &g_uacpi_rt_ctx.pm1b_status_blk,
        .preserve_mask = ACPI_PM1_STS_IGN0_MASK,
    },
    [UACPI_REGISTER_PM1_EN] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_PRESERVE,
        .accessor0 = &g_uacpi_rt_ctx.pm1a_enable_blk,
        .accessor1 = &g_uacpi_rt_ctx.pm1b_enable_blk,
    },
    [UACPI_REGISTER_PM1_CNT] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_PRESERVE,
        .accessor0 = &g_uacpi_rt_ctx.fadt.x_pm1a_cnt_blk,
        .accessor1 = &g_uacpi_rt_ctx.fadt.x_pm1b_cnt_blk,
        .write_only_mask = ACPI_PM1_CNT_SLP_EN_MASK |
                           ACPI_PM1_CNT_GBL_RLS_MASK,
        .preserve_mask = ACPI_PM1_CNT_PRESERVE_MASK,
    },
    [UACPI_REGISTER_PM_TMR] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_PRESERVE,
        .accessor0 = &g_uacpi_rt_ctx.fadt.x_pm_tmr_blk,
    },
    [UACPI_REGISTER_PM2_CNT] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_PRESERVE,
        .accessor0 = &g_uacpi_rt_ctx.fadt.x_pm2_cnt_blk,
        .preserve_mask = ACPI_PM2_CNT_PRESERVE_MASK,
    },
    [UACPI_REGISTER_SLP_CNT] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_PRESERVE,
        .accessor0 = &g_uacpi_rt_ctx.fadt.sleep_control_reg,
        .write_only_mask = ACPI_SLP_CNT_SLP_EN_MASK,
        .preserve_mask = ACPI_SLP_CNT_PRESERVE_MASK,
    },
    [UACPI_REGISTER_SLP_STS] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_WRITE_TO_CLEAR,
        .accessor0 = &g_uacpi_rt_ctx.fadt.sleep_status_reg,
        .preserve_mask = ACPI_SLP_STS_PRESERVE_MASK,
    },
    [UACPI_REGISTER_RESET] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_NORMAL,
        .accessor0 = &g_uacpi_rt_ctx.fadt.reset_reg,
    },
    [UACPI_REGISTER_SMI_CMD] = {
        .kind = REGISTER_KIND_IO,
        .access_kind = REGISTER_ACCESS_KIND_NORMAL,
        .access_width = 1,
        .accessor0 = &g_uacpi_rt_ctx.fadt.smi_cmd,
    },
};

static const struct register_spec *get_reg(uacpi_u8 idx)
{
    if (idx > UACPI_REGISTER_MAX)
        return UACPI_NULL;

    return &registers[idx];
}

static uacpi_status read_one(
    enum register_kind kind, void *reg, uacpi_u8 byte_width,
    uacpi_u64 *out_value
)
{
    if (kind == REGISTER_KIND_GAS) {
        struct acpi_gas *gas = reg;

        if (!gas->address)
            return UACPI_STATUS_OK;

        return uacpi_gas_read(reg, out_value);
    }

    return uacpi_system_io_read(*(uacpi_u32*)reg, byte_width, out_value);
}

static uacpi_status write_one(
    enum register_kind kind, void *reg, uacpi_u8 byte_width,
    uacpi_u64 in_value
)
{
    if (kind == REGISTER_KIND_GAS) {
        struct acpi_gas *gas = reg;

        if (!gas->address)
            return UACPI_STATUS_OK;

        return uacpi_gas_write(reg, in_value);
    }

    return uacpi_system_io_write(*(uacpi_u32*)reg, byte_width, in_value);
}

static uacpi_status do_read_register(
    const struct register_spec *reg, uacpi_u64 *out_value
)
{
    uacpi_status ret;
    uacpi_u64 value0, value1 = 0;

    ret = read_one(reg->kind, reg->accessor0, reg->access_width, &value0);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (reg->accessor1) {
        ret = read_one(reg->kind, reg->accessor1, reg->access_width, &value1);
        if (uacpi_unlikely_error(ret))
            return ret;
    }

    *out_value = value0 | value1;
    if (reg->write_only_mask)
        *out_value &= ~reg->write_only_mask;

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_read_register(
    enum uacpi_register reg_enum, uacpi_u64 *out_value
)
{
    const struct register_spec *reg;

    reg = get_reg(reg_enum);
    if (uacpi_unlikely(reg == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    return do_read_register(reg, out_value);
}

static uacpi_status do_write_register(
    const struct register_spec *reg, uacpi_u64 in_value
)
{
    uacpi_status ret;

    if (reg->preserve_mask) {
        in_value &= ~reg->preserve_mask;

        if (reg->access_kind == REGISTER_ACCESS_KIND_PRESERVE) {
            uacpi_u64 data;

            ret = do_read_register(reg, &data);
            if (uacpi_unlikely_error(ret))
                return ret;

            in_value |= data & reg->preserve_mask;
        }
    }

    ret = write_one(reg->kind, reg->accessor0, reg->access_width, in_value);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (reg->accessor1)
        ret = write_one(reg->kind, reg->accessor1, reg->access_width, in_value);

    return ret;
}

uacpi_status uacpi_write_register(
    enum uacpi_register reg_enum, uacpi_u64 in_value
)
{
    const struct register_spec *reg;

    reg = get_reg(reg_enum);
    if (uacpi_unlikely(reg == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    return do_write_register(reg, in_value);
}

uacpi_status uacpi_write_registers(
    enum uacpi_register reg_enum, uacpi_u64 in_value0, uacpi_u64 in_value1
)
{
    uacpi_status ret;
    const struct register_spec *reg;

    reg = get_reg(reg_enum);
    if (uacpi_unlikely(reg == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    ret = write_one(reg->kind, reg->accessor0, reg->access_width, in_value0);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (reg->accessor1)
        ret = write_one(reg->kind, reg->accessor1, reg->access_width, in_value1);

    return ret;
}

struct register_field {
    uacpi_u8 reg;
    uacpi_u8 offset;
    uacpi_u16 mask;
};

static const struct register_field fields[UACPI_REGISTER_FIELD_MAX + 1] = {
    [UACPI_REGISTER_FIELD_TMR_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_TMR_STS_IDX,
        .mask = ACPI_PM1_STS_TMR_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_BM_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_BM_STS_IDX,
        .mask = ACPI_PM1_STS_BM_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_GBL_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_GBL_STS_IDX,
        .mask = ACPI_PM1_STS_GBL_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_PWRBTN_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_PWRBTN_STS_IDX,
        .mask = ACPI_PM1_STS_PWRBTN_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_SLPBTN_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_SLPBTN_STS_IDX,
        .mask = ACPI_PM1_STS_SLPBTN_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_RTC_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_RTC_STS_IDX,
        .mask = ACPI_PM1_STS_RTC_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_HWR_WAK_STS] = {
        .reg = UACPI_REGISTER_SLP_STS,
        .offset = ACPI_SLP_STS_WAK_STS_IDX,
        .mask = ACPI_SLP_STS_WAK_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_WAK_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_WAKE_STS_IDX,
        .mask = ACPI_PM1_STS_WAKE_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_PCIEX_WAKE_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_PCIEXP_WAKE_STS_IDX,
        .mask = ACPI_PM1_STS_PCIEXP_WAKE_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_TMR_EN] = {
        .reg = UACPI_REGISTER_PM1_EN,
        .offset = ACPI_PM1_EN_TMR_EN_IDX,
        .mask = ACPI_PM1_EN_TMR_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_GBL_EN] = {
        .reg = UACPI_REGISTER_PM1_EN,
        .offset = ACPI_PM1_EN_GBL_EN_IDX,
        .mask = ACPI_PM1_EN_GBL_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_PWRBTN_EN] = {
        .reg = UACPI_REGISTER_PM1_EN,
        .offset = ACPI_PM1_EN_PWRBTN_EN_IDX,
        .mask = ACPI_PM1_EN_PWRBTN_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_SLPBTN_EN] = {
        .reg = UACPI_REGISTER_PM1_EN,
        .offset = ACPI_PM1_EN_SLPBTN_EN_IDX,
        .mask = ACPI_PM1_EN_SLPBTN_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_RTC_EN] = {
        .reg = UACPI_REGISTER_PM1_EN,
        .offset = ACPI_PM1_EN_RTC_EN_IDX,
        .mask = ACPI_PM1_EN_RTC_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_PCIEXP_WAKE_DIS] = {
        .reg = UACPI_REGISTER_PM1_EN,
        .offset = ACPI_PM1_EN_PCIEXP_WAKE_DIS_IDX,
        .mask = ACPI_PM1_EN_PCIEXP_WAKE_DIS_MASK,
    },
    [UACPI_REGISTER_FIELD_SCI_EN] = {
        .reg = UACPI_REGISTER_PM1_CNT,
        .offset = ACPI_PM1_CNT_SCI_EN_IDX,
        .mask = ACPI_PM1_CNT_SCI_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_BM_RLD] = {
        .reg = UACPI_REGISTER_PM1_CNT,
        .offset = ACPI_PM1_CNT_BM_RLD_IDX,
        .mask = ACPI_PM1_CNT_BM_RLD_MASK,
    },
    [UACPI_REGISTER_FIELD_GBL_RLS] = {
        .reg = UACPI_REGISTER_PM1_CNT,
        .offset = ACPI_PM1_CNT_GBL_RLS_IDX,
        .mask = ACPI_PM1_CNT_GBL_RLS_MASK,
    },
    [UACPI_REGISTER_FIELD_SLP_TYP] = {
        .reg = UACPI_REGISTER_PM1_CNT,
        .offset = ACPI_PM1_CNT_SLP_TYP_IDX,
        .mask = ACPI_PM1_CNT_SLP_TYP_MASK,
    },
    [UACPI_REGISTER_FIELD_SLP_EN] = {
        .reg = UACPI_REGISTER_PM1_CNT,
        .offset = ACPI_PM1_CNT_SLP_EN_IDX,
        .mask = ACPI_PM1_CNT_SLP_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_HWR_SLP_TYP] = {
        .reg = UACPI_REGISTER_SLP_CNT,
        .offset = ACPI_SLP_CNT_SLP_TYP_IDX,
        .mask = ACPI_SLP_CNT_SLP_TYP_MASK,
    },
    [UACPI_REGISTER_FIELD_HWR_SLP_EN] = {
        .reg = UACPI_REGISTER_SLP_CNT,
        .offset = ACPI_SLP_CNT_SLP_EN_IDX,
        .mask = ACPI_SLP_CNT_SLP_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_ARB_DIS] = {
        .reg = UACPI_REGISTER_PM2_CNT,
        .offset = ACPI_PM2_CNT_ARB_DIS_IDX,
        .mask = ACPI_PM2_CNT_ARB_DIS_MASK,
    },
};

static uacpi_handle g_reg_lock;

uacpi_status uacpi_ininitialize_registers(void)
{
    g_reg_lock = uacpi_kernel_create_spinlock();
    if (uacpi_unlikely(g_reg_lock == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    return UACPI_STATUS_OK;
}

void uacpi_deininitialize_registers(void)
{
    if (g_reg_lock != UACPI_NULL) {
        uacpi_kernel_free_spinlock(g_reg_lock);
        g_reg_lock = UACPI_NULL;
    }
}

uacpi_status uacpi_read_register_field(
    enum uacpi_register_field field_enum, uacpi_u64 *out_value
)
{
    uacpi_status ret;
    uacpi_u8 field_idx = field_enum;
    const struct register_field *field;
    const struct register_spec *reg;

    if (uacpi_unlikely(field_idx > UACPI_REGISTER_FIELD_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;

    field = &fields[field_idx];
    reg = &registers[field->reg];

    ret = do_read_register(reg, out_value);
    if (uacpi_unlikely_error(ret))
        return ret;

    *out_value = (*out_value & field->mask) >> field->offset;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_write_register_field(
    enum uacpi_register_field field_enum, uacpi_u64 in_value
)
{
    uacpi_status ret;
    uacpi_u8 field_idx = field_enum;
    const struct register_field *field;
    const struct register_spec *reg;
    uacpi_u64 data;
    uacpi_cpu_flags flags;

    if (uacpi_unlikely(field_idx > UACPI_REGISTER_FIELD_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;

    field = &fields[field_idx];
    reg = &registers[field->reg];

    in_value = (in_value << field->offset) & field->mask;

    flags = uacpi_kernel_lock_spinlock(g_reg_lock);

    if (reg->kind == REGISTER_ACCESS_KIND_WRITE_TO_CLEAR) {
        if (in_value == 0) {
            ret = UACPI_STATUS_OK;
            goto out;
        }

        ret = do_write_register(reg, in_value);
        goto out;
    }

    ret = do_read_register(reg, &data);
    if (uacpi_unlikely_error(ret))
        goto out;

    data &= ~field->mask;
    data |= in_value;

    ret = do_write_register(reg, data);

out:
    uacpi_kernel_unlock_spinlock(g_reg_lock, flags);
    return ret;
}
