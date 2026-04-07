/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 lowRISC Contributors
 *
 * Authors:
 *   Alice Ziuziakowska <a.ziuziakowska@lowrisc.org>
 */

#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/timer/fdt_timer.h>
#include <sbi_utils/timer/lowrisc_rvtimer.h>
#include <sbi/riscv_io.h>

extern struct lowrisc_rvtimer_data lr_timer;

static int fdt_lowrisc_rvtimer_cold_timer_init(void *fdt, int nodeoff,
					       const struct fdt_match *match)
{
	int rc;

	rc = fdt_parse_lowrisc_rvtimer_node(fdt, nodeoff, &lr_timer.base,
					    &lr_timer.size, &lr_timer.hart_count);
	if (rc)
		return rc;


	lr_timer.io_base = (u32 *)ioremap(lr_timer.base, lr_timer.size);

	rc = fdt_parse_timebase_frequency(fdt, &lr_timer.timer_freq);
	if (rc)
		return rc;

	rc = lowrisc_rvtimer_cold_timer_init(&lr_timer);
	if (rc)
		return rc;

	return 0;
}

static const struct fdt_match timer_lowrisc_rvtimer_match[] = {
	{ .compatible = "lowrisc,opentitan-rv-timer-v1" },
	{},
};

struct fdt_timer fdt_lowrisc_rvtimer = {
	.match_table = timer_lowrisc_rvtimer_match,
	.cold_init   = fdt_lowrisc_rvtimer_cold_timer_init,
	.warm_init   = lowrisc_rvtimer_warm_timer_init,
	.exit	     = NULL,
};
