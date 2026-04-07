/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 lowRISC Contributors
 *
 * Authors:
 *   Alice Ziuziakowska <a.ziuziakowska@lowrisc.org>
 */

#ifndef __TIMER_LOWRISC_RVTIMER_H__
#define __TIMER_LOWRISC_RVTIMER_H__

#define RVTIMER_REGION_ALIGN 0x1000

struct lowrisc_rvtimer_data {
	u32 hart_count;
	unsigned long base;
	unsigned long size;
	unsigned long timer_freq;
	volatile u32 *io_base;
};

int lowrisc_rvtimer_cold_timer_init(struct lowrisc_rvtimer_data *lr_timer);

int lowrisc_rvtimer_warm_timer_init(void);

#endif /* __TIMER_LOWRISC_RVTIMER_H__ */
