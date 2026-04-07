/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 lowRISC Contributors
 *
 * Authors:
 *   Alice Ziuziakowska <a.ziuziakowska@lowrisc.org>
 */

#include <sbi/sbi_error.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/serial/fdt_serial.h>
#include <sbi_utils/serial/lowrisc-ot-uart.h>
#include <sbi/riscv_io.h>

static int serial_lowrisc_init(void *fdt, int nodeoff,
			       const struct fdt_match *match)
{
	int rc;
	uint64_t reg_addr, reg_size;
	struct platform_uart_data uart = { 0 };

	if (nodeoff < 0 || !fdt)
		return SBI_ENODEV;

	rc = fdt_get_node_addr_size(fdt, nodeoff, 0, &reg_addr, &reg_size);
	if (rc < 0 || !reg_addr || !reg_size)
		return SBI_ENODEV;

	rc = fdt_parse_lowrisc_ot_uart_node(fdt, nodeoff, &uart);
	if (rc)
		return rc;

	void *mmio = ioremap(reg_addr, reg_size);

	return lowrisc_ot_uart_init(mmio, uart.freq, uart.baud);
}

static const struct fdt_match serial_lowrisc_match[] = {
	{ .compatible = "lowrisc,opentitan-uart-v2" },
	{ },
};

struct fdt_serial fdt_serial_lowrisc = {
	.match_table = serial_lowrisc_match,
	.init = serial_lowrisc_init,
};
