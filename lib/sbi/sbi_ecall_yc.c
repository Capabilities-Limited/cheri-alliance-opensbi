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

#include <sbi/sbi_error.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_yc.h>

static int sbi_ecall_yc_handler(unsigned long extid, unsigned long funcid,
				struct sbi_trap_regs *regs,
				struct sbi_ecall_return *out)
{
	int ret = 0;

	if (funcid == SBI_EXT_YC_ENTER) {
		ret = sbi_yc_enter((u64)regs->a0, (u64)regs->a1, (u64)regs->a2,
				   (u64)regs->a3, (u64)regs->a4, (u64)regs->a5);
		out->value = 0;
	} else if (funcid == SBI_EXT_YC_RESUME) {
		ret = sbi_yc_resume((u64)regs->a0, (u64)regs->a1);
		out->value = 0;
	} else {
		ret = SBI_ENOTSUPP;
	}

	return ret;
}

struct sbi_ecall_extension ecall_yc;

static int sbi_ecall_yc_register_extensions(void)
{
	return sbi_ecall_register_extension(&ecall_yc);
}

struct sbi_ecall_extension ecall_yc = {
	.name			= "yc",
	.extid_start		= SBI_EXT_YC,
	.extid_end		= SBI_EXT_YC,
	.experimental		= true,
	.register_extensions	= sbi_ecall_yc_register_extensions,
	.handle			= sbi_ecall_yc_handler,
};
