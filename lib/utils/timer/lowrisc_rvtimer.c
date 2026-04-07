/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 lowRISC Contributors
 *
 * Authors:
 *   Alice Ziuziakowska <a.ziuziakowska@lowrisc.org>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_timer.h>
#include <sbi_utils/timer/lowrisc_rvtimer.h>

#define TIMER_REG_CTRL	0x4
#define TIMER_REG_HART_OFFSET	0x100
#define TIMER_REG_HART_ENABLE	0x0
#define TIMER_REG_HART_STATE	0x4
#define TIMER_REG_HART_CFG	0xc
#define TIMER_REG_HART_TIMEVAL_LO	0x10
#define TIMER_REG_HART_TIMEVAL_HI	0x14
#define TIMER_REG_HART_TIMECMP_LO	0x18
#define TIMER_REG_HART_TIMECMP_HI	0x1c

#define TIMER_REG_HART_CFG_STEP	16

#define rvtimer_hart_offset(hart, reg_offset) \
	(((1U + (hart)) * TIMER_REG_HART_OFFSET) + (reg_offset))

struct lowrisc_rvtimer_data lr_timer;

static u32 get_reg(u32 offset)
{
	return readl((void *)lr_timer.io_base + offset);
}

static void set_reg(u32 offset, u32 val)
{
	writel(val, (void *)lr_timer.io_base + offset);
}

static u64 lowrisc_rvtimer_value(void)
{
	u32 target_hart = current_hartid();
	if (lr_timer.hart_count <= target_hart)
		ebreak();

	u32 lo, hi;
	do {
		hi = get_reg(rvtimer_hart_offset(target_hart,
						 TIMER_REG_HART_TIMEVAL_HI));
		lo = get_reg(rvtimer_hart_offset(target_hart,
						 TIMER_REG_HART_TIMEVAL_LO));
	} while (hi != get_reg(rvtimer_hart_offset(target_hart,
						   TIMER_REG_HART_TIMEVAL_HI)));

	return ((u64)hi << 32) | (u64)lo;
}

static void lowrisc_rvtimer_event_stop(void)
{
	u32 target_hart = current_hartid();
	if (lr_timer.hart_count <= target_hart)
		ebreak();

	set_reg(rvtimer_hart_offset(target_hart, TIMER_REG_HART_TIMECMP_HI), -1U);
	set_reg(rvtimer_hart_offset(target_hart, TIMER_REG_HART_TIMECMP_LO), -1U);
}

static void lowrisc_rvtimer_event_start(u64 next_event)
{
	u32 target_hart = current_hartid();
	if (lr_timer.hart_count <= target_hart)
		ebreak();

	set_reg(rvtimer_hart_offset(target_hart, TIMER_REG_HART_TIMECMP_LO), -1U);
	set_reg(rvtimer_hart_offset(target_hart, TIMER_REG_HART_TIMECMP_HI), (u32)(next_event >> 32u));
	set_reg(rvtimer_hart_offset(target_hart, TIMER_REG_HART_TIMECMP_LO), (u32)next_event);
}

static struct sbi_timer_device lowrisc_rvtimer = {
	.name		   = "lowrisc_rvtimer",
	.timer_freq	   = 0,
	.timer_value	   = lowrisc_rvtimer_value,
	.timer_event_start = lowrisc_rvtimer_event_start,
	.timer_event_stop  = lowrisc_rvtimer_event_stop,
};

int lowrisc_rvtimer_cold_timer_init(struct lowrisc_rvtimer_data *lr_timer)
{
	int rc;

	/* Add RV Timer region to the root domain */
	rc = sbi_domain_root_add_memrange(
		lr_timer->base, lr_timer->size,
		RVTIMER_REGION_ALIGN,
		SBI_DOMAIN_MEMREGION_MMIO |
		SBI_DOMAIN_MEMREGION_M_READABLE |
		SBI_DOMAIN_MEMREGION_M_WRITABLE);

	if (rc)
		return rc;

	for (int i = 0; i < lr_timer->hart_count; i++) {
		/* Set all hart timers to tick at timer_freq */
		set_reg(rvtimer_hart_offset(i, TIMER_REG_HART_CFG),
			1U << TIMER_REG_HART_CFG_STEP);
		/* Enable the timer interrupt on each hart timer */
		set_reg(rvtimer_hart_offset(i, TIMER_REG_HART_ENABLE), 1U);
	}

	/* Enable all hart timers at once */
	u32 enable = (1U << lr_timer->hart_count) - 1U;
	set_reg(TIMER_REG_CTRL, enable);

	lowrisc_rvtimer.timer_freq = lr_timer->timer_freq;

	sbi_timer_set_device(&lowrisc_rvtimer);

	return 0;
}

int lowrisc_rvtimer_warm_timer_init(void)
{
	if (!lr_timer.io_base)
		return SBI_ENODEV;

	lowrisc_rvtimer_event_stop();

	return 0;
}
