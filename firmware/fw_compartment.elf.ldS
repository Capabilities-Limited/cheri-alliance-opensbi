/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
	#include "fw_base.ldS"

	.compartments :
	{
		PROVIDE(_compartments_start = .);
		*(.compartments)
		PROVIDE(_compartments_end = .);
	}
	_fw_end = .;

	/* Smallest power of two covering the firmware's size. */
	_fw_pmp_size = 1 << LOG2CEIL(MAX(8, _fw_end - _fw_start));

	/* NAPOT also requires the base to be aligned to that size. */
	ASSERT((_fw_start & (_fw_pmp_size - 1)) == 0,
	       "Firmware base is not aligned to its PMP size")

	_fw_pmp_end = _fw_start + _fw_pmp_size;

	/* Place the payload beyond the entire protected region. */
	. = _fw_pmp_end;
	. = ALIGN(FW_PAYLOAD_ALIGN);

	.payload :
	{
		PROVIDE(_payload_start = .);
		*(.payload)
		. = ALIGN(8);
		PROVIDE(_payload_end = .);
	}

	PROVIDE(_fw_reloc_end = .);
}
