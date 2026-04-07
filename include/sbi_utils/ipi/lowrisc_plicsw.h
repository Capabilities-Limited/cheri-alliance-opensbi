/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 lowRISC Contributors
 *
 * Authors:
 *   Alice Ziuziakowska <a.ziuziakowska@lowrisc.org>
 */

#ifndef _IPI_LOWRISC_PLICSW_H_
#define _IPI_LOWRISC_PLICSW_H_

#define LOWRISC_PLICSW_REGION_ALIGN	0x1000

struct lowrisc_plicsw_data {
	void *addr;
	unsigned long size;
	uint32_t hart_count;
};

int lowrisc_plicsw_warm_ipi_init(void);

int lowrisc_plicsw_cold_ipi_init(struct lowrisc_plicsw_data *lr_plicsw);

#endif /* _IPI_LOWRISC_PLICSW_H_ */
