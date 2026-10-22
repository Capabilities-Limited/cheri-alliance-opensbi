/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 lowRISC Contributors
 *
 * Authors:
 *   Alice Ziuziakowska <a.ziuziakowska@lowrisc.org>
 */

#include <sbi/riscv_io.h>
#include <sbi_utils/fdt/fdt_driver.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/ipi/lowrisc_plicsw.h>

extern struct lowrisc_plicsw_data lr_plicsw;

int fdt_lowrisc_plicsw_cold_ipi_init(const void *fdt, int nodeoff,
				     const struct fdt_match *match)
{
	int rc;
	unsigned long addr;

	rc = fdt_parse_plicsw_node(fdt, nodeoff, &addr, &lr_plicsw.size,
				   &lr_plicsw.hart_count);
	if (rc)
		return rc;

	lr_plicsw.addr = ioremap(addr, lr_plicsw.size);

	rc = lowrisc_plicsw_cold_ipi_init(&lr_plicsw);
	if (rc)
		return rc;

	return 0;
}

static const struct fdt_match ipi_lowrisc_plicsw_match[] = {
	{ .compatible = "lowrisc,opentitan-plicsw" },
	{},
};

struct fdt_driver fdt_ipi_lowrisc_plicsw = {
	.match_table = ipi_lowrisc_plicsw_match,
	.init   = fdt_lowrisc_plicsw_cold_ipi_init,
};
