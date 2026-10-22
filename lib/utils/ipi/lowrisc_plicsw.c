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
#include <sbi/sbi_ipi.h>
#include <sbi_utils/ipi/lowrisc_plicsw.h>

struct lowrisc_plicsw_data lr_plicsw;

int lowrisc_plicsw_cold_ipi_init(struct lowrisc_plicsw_data *lr_plicsw)
{
	int rc;

	/* Add PLICSW region to the root domain */
	rc = sbi_domain_root_add_memrange((unsigned long)lr_plicsw->addr,
					  lr_plicsw->size,
					  LOWRISC_PLICSW_REGION_ALIGN,
					  SBI_DOMAIN_MEMREGION_MMIO |
					  SBI_DOMAIN_MEMREGION_M_READABLE |
					  SBI_DOMAIN_MEMREGION_M_WRITABLE);
	if (rc)
		return rc;

	return 0;
}
