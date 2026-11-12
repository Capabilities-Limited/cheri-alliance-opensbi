/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Capabilities Limited
 *
 * This software/hardware was developed in part by Capabilities Limited,
 * lowRISC CIC, Sigil Logic Inc., and Cherified Systems LLC under Advanced
 * Research and Invention Agency (ARIA) Contract PR-SGAI-00358, "AI Verified
 * Hardware for AI (AIVHAI)".
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sbi/sbi_string.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_yc.h>
#include <sbi/sbi_yc_offsets.h>
#include <sbi/sbi_timer.h>
#include <sbi/riscv_cheri.h>

compartment_t compartments[YC_MAX_COMPARTMENTS] = {
	[0] = { .hartid = -1 },
	[1] = { .hartid = -1 },
};

/*
 * Fallbacks for firmware that links this file but not fw_compartment.S (where
 * the real definitions live: compartment_enter/return/trap, and the incbin'd
 * compartment0/1_enter).  Defined weak here, once, rather than in the header.
 */
unsigned long __attribute__((weak))
compartment_enter(compartment_t *compartment)
{
	return SBI_ENOTSUPP;
}

unsigned long __attribute__((weak)) compartment_trap(void)
{
	return SBI_ENOTSUPP;
}

unsigned long __attribute__((weak)) compartment_return(void)
{
	return SBI_ENOTSUPP;
}

unsigned long __attribute__((weak)) compartment0_enter(uintptr_t input_buffer,
						       uintptr_t output_buffer)
{
	return SBI_ENOTSUPP;
}

unsigned long __attribute__((weak)) compartment1_enter(uintptr_t input_buffer,
						       uintptr_t output_buffer)
{
	return SBI_ENOTSUPP;
}

unsigned long sbi_yc_enter(unsigned long id, unsigned long timeout,
			   unsigned long pa0, unsigned long size0,
			   unsigned long pa1, unsigned long size1)
{
	unsigned long ret = 0;
	unsigned long eid = (id >> 8) & 0xff;
	long expected_hart_init = -1;
	id &= 0xff;

	if (id >= YC_MAX_COMPARTMENTS || eid >= YC_MAX_PER_COMPARTMENT_ENTRIES)
		return SBI_EINVAL;

	if (!__atomic_compare_exchange_n(&compartments[id].hartid,
					 &expected_hart_init,
					 current_hartindex(), false,
					 __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
		return SBI_EALREADY_STARTED;

	sbi_memset(&compartments[id], 0, sizeof(compartments[id]));

	/* Pin this compartment to the current HART until it returns */
	compartments[id].hartid = current_hartindex();

	if (id == 0) {
		/* Build PCC */
		compartments[id].pcc = (uintptr_t)cheri_infinite_cap;
		compartments[id].pcc = __builtin_cheri_address_set(
			compartments[id].pcc,
			(unsigned long)compartment0_enter);
		compartments[id].pcc = __builtin_cheri_bounds_set(
			compartments[id].pcc,
			__builtin_cheri_length_get(compartment0_enter));
		compartments[id].pcc = __builtin_cheri_perms_and(
			compartments[id].pcc, ~CHERI_PERM_SYSTEM_REGS);
		compartments[id].pcc =
			__builtin_cheri_flags_set(compartments[id].pcc, 0);
		compartments[id].pcc =
			__builtin_cheri_seal_entry(compartments[id].pcc);
		compartments[id].entries[eid] =
			(compartment_entry)compartments[id].pcc;

		/* Build shared buffer capability */
		compartments[id].buf = (uintptr_t)cheri_infinite_cap;
		compartments[id].buf = __builtin_cheri_address_set(
			compartments[id].buf, CONFIG_YC_BUFFER0_ADDRESS);
		compartments[id].buf = __builtin_cheri_bounds_set(
			compartments[id].buf, CONFIG_YC_BUFFER0_SIZE);
		compartments[id].buf = __builtin_cheri_perms_and(
			compartments[id].buf, CHERI_PERM_DATA);
		compartments[id].buf =
			__builtin_cheri_flags_set(compartments[id].buf, 0);
	} else if (id == 1) {
		/* Build PCC */
		compartments[id].pcc = (uintptr_t)cheri_infinite_cap;
		compartments[id].pcc = __builtin_cheri_address_set(
			compartments[id].pcc,
			(unsigned long)compartment1_enter);
		compartments[id].pcc = __builtin_cheri_bounds_set(
			compartments[id].pcc,
			__builtin_cheri_length_get(compartment1_enter));
		compartments[id].pcc = __builtin_cheri_perms_and(
			compartments[id].pcc, ~CHERI_PERM_SYSTEM_REGS);
		compartments[id].pcc =
			__builtin_cheri_flags_set(compartments[id].pcc, 0);
		compartments[id].pcc =
			__builtin_cheri_seal_entry(compartments[id].pcc);
		compartments[id].entries[eid] =
			(compartment_entry)compartments[id].pcc;

		/* Build shared buffer capability */
		compartments[id].buf = (uintptr_t)cheri_infinite_cap;
		compartments[id].buf = __builtin_cheri_address_set(
			compartments[id].buf, CONFIG_YC_BUFFER1_ADDRESS);
		compartments[id].buf = __builtin_cheri_bounds_set(
			compartments[id].buf, CONFIG_YC_BUFFER1_SIZE);
		compartments[id].buf = __builtin_cheri_perms_and(
			compartments[id].buf, CHERI_PERM_DATA);
		compartments[id].buf =
			__builtin_cheri_flags_set(compartments[id].buf, 0);
	} else {
		return SBI_EINVAL;
	}

	compartments[id].mtvec = (uintptr_t)cheri_infinite_cap;
	compartments[id].mtvec = __builtin_cheri_address_set(
		compartments[id].mtvec, (unsigned long)compartment_trap);
	compartments[id].mtvec =
		__builtin_cheri_flags_set(compartments[id].mtvec, 0);

	compartments[id].gprs[1] = (uintptr_t)cheri_infinite_cap;
	compartments[id].gprs[1] = __builtin_cheri_address_set(
		compartments[id].gprs[1], (unsigned long)compartment_return);
	compartments[id].gprs[1] =
		__builtin_cheri_flags_set(compartments[id].gprs[1], 0);
	compartments[id].gprs[1] =
		__builtin_cheri_seal_entry(compartments[id].gprs[1]);
	compartments[id].gprs[10] =
		__builtin_cheri_address_set(compartments[id].buf, pa0);
	compartments[id].gprs[10] =
		__builtin_cheri_bounds_set(compartments[id].gprs[10], size0);
	/* Read-only input buffer */
	compartments[id].gprs[10] = __builtin_cheri_perms_and(
		compartments[id].gprs[10], CHERI_PERM_R_DATA);
	compartments[id].gprs[11] =
		__builtin_cheri_address_set(compartments[id].buf, pa1);
	compartments[id].gprs[11] =
		__builtin_cheri_bounds_set(compartments[id].gprs[11], size1);
	/* Write-only output buffer */
	compartments[id].gprs[11] = __builtin_cheri_perms_and(
		compartments[id].gprs[11], ~CHERI_PERM_READ);

	asm volatile("csrr %0, mstatus" : "=r"(compartments[id].mstatus)::);
	compartments[id].mstatus |= MSTATUS_MPP;
	compartments[id].mstatus &= ~(MSTATUS_MIE | MSTATUS_MPIE);
#if defined(CONFIG_SBI_YC_SUPPORT_FPU)
	compartments[id].mstatus |= MSTATUS_FS;
#endif /* CONFIG_SBI_YC_SUPPORT_FPU */
#if defined(CONFIG_SBI_YC_SUPPORT_RVV)
	compartments[id].mstatus |= MSTATUS_VS;
#endif /* CONFIG_SBI_YC_SUPPORT_RVV */

	if (timeout) {
		sbi_timer_event_start(sbi_timer_value() + timeout);
		compartments[id].mstatus |= MSTATUS_MPIE;
	}

	ret = compartment_enter(&compartments[id]);

	if (ret == SBI_OK)
		__atomic_store_n(&compartments[id].hartid, -1, __ATOMIC_RELEASE);

	return ret;
}

unsigned long sbi_yc_resume(unsigned long id, unsigned long timeout)
{
	unsigned long ret;
	unsigned long eid = (id >> 8) & 0xff;
	id &= 0xff;

	if (id >= YC_MAX_COMPARTMENTS || eid >= YC_MAX_PER_COMPARTMENT_ENTRIES)
		return SBI_EINVAL;

	/* Only allow resuming on the same core a compartment entered */
	if (__atomic_load_n(&compartments[id].hartid, __ATOMIC_ACQUIRE) != current_hartindex())
		return SBI_EALREADY_STARTED;

	compartments[id].mstatus |= MSTATUS_MPP;
	compartments[id].mstatus &= ~(MSTATUS_MIE | MSTATUS_MPIE);

	if (timeout) {
		sbi_timer_event_start(sbi_timer_value() + timeout);
		compartments[id].mstatus |= MSTATUS_MPIE;
	}

	ret = compartment_enter(&compartments[id]);
	if (ret == SBI_OK)
		__atomic_store_n(&compartments[id].hartid, -1, __ATOMIC_RELEASE);
	return ret;
}
