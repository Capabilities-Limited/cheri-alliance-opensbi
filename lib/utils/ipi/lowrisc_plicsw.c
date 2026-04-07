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

static void lowrisc_plicsw_ipi_send(u32 hart_index)
{
	u32 target_hart = sbi_hartindex_to_hartid(hart_index);
	void *reg = lr_plicsw.addr + (8u * target_hart);

	if (lr_plicsw.hart_count <= target_hart)
		ebreak();

	writel(1, reg);
}

static void lowrisc_plicsw_ipi_clear(u32 hart_index)
{
	u32 target_hart = sbi_hartindex_to_hartid(hart_index);
	void *reg = lr_plicsw.addr + (8u * target_hart);

	if (lr_plicsw.hart_count <= target_hart)
		ebreak();

	writel(0, reg);
}

static struct sbi_ipi_device lowrisc_plicsw_ipi = {
	.name      = "lowrisc_plicsw",
	.ipi_send  = lowrisc_plicsw_ipi_send,
	.ipi_clear = lowrisc_plicsw_ipi_clear
};

int lowrisc_plicsw_warm_ipi_init(void)
{
	u32 hartid = current_hartid();

	/* Clear PLICSW IPI */
	lowrisc_plicsw_ipi_clear(hartid);

	return 0;
}

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

	sbi_ipi_set_device(&lowrisc_plicsw_ipi);

	return 0;
}
