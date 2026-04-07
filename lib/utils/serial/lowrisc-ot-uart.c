/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 lowRISC Contributors
 *
 * Authors:
 *   Alice Ziuziakowska <a.ziuziakowska@lowrisc.org>
 */

#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/serial/lowrisc-ot-uart.h>

#define UART_REG_INTR 1
#define UART_REG_CTRL 4
#define UART_REG_STATUS 5
#define UART_REG_RDATA 6
#define UART_REG_WDATA 7

#define UART_CTRL_TX_RX 0x3
#define UART_STATUS_TXFULL 0x1
#define UART_STATUS_RXEMPTY 0x20

static volatile u32 *uart_base;

static u32 get_reg(u32 num)
{
	return readl(uart_base + num);
}

static void set_reg(u32 num, u32 val)
{
	writel(val, uart_base + num);
}

static void lowrisc_ot_putc(char ch) {
	while (get_reg(UART_REG_STATUS) & UART_STATUS_TXFULL)
		;

	set_reg(UART_REG_WDATA, ch);
}

static int lowrisc_ot_getc(void) {
	if (get_reg(UART_REG_STATUS) & UART_STATUS_RXEMPTY)
		return -1;

	return (u8)get_reg(UART_REG_RDATA);
}

static struct sbi_console_device lowrisc_ot_console = {
	.name = "lowrisc_opentitan_uart",
	.console_putc = lowrisc_ot_putc,
	.console_getc = lowrisc_ot_getc,
};

int lowrisc_ot_uart_init(void *base, u32 freq, u32 baud)
{
	uart_base = (volatile u32 *)base;

	/* Disable UART interrupts */
	set_reg(UART_REG_INTR, 0);

	/*
	 * Configure UART baud rate.
	 * nco = (baud * 2^20) / clock_freq
	 */
	u32 nco = ((u64)baud << 20) / freq;
	u32 ctrl = (nco << 16) | UART_CTRL_TX_RX;
	set_reg(UART_REG_CTRL, ctrl);

	sbi_console_set_device(&lowrisc_ot_console);
	return 0;
}
