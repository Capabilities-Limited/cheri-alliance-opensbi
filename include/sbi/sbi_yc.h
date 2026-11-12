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

#ifndef __SBI_YC_H__
#define __SBI_YC_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_trap.h>

#define YC_MAX_COMPARTMENTS 2
#define YC_MAX_PER_COMPARTMENT_ENTRIES 4
#define RVV_VLEN 128

#ifndef CONFIG_YC_BUFFER0_ADDRESS
#define CONFIG_YC_BUFFER0_ADDRESS 0xa0000000
#endif
#ifndef CONFIG_YC_BUFFER0_SIZE
#define CONFIG_YC_BUFFER0_SIZE 0x40000 /* 256 KiB */
#endif

#ifndef CONFIG_YC_BUFFER1_ADDRESS
#define CONFIG_YC_BUFFER1_ADDRESS \
	(CONFIG_YC_BUFFER0_ADDRESS + CONFIG_YC_BUFFER0_SIZE)
#endif
#ifndef CONFIG_YC_BUFFER1_SIZE
#define CONFIG_YC_BUFFER1_SIZE 0x40000 /* 256 KiB */
#endif

#define CONFIG_YC_SHMEM_ADDRESS CONFIG_YC_BUFFER0_ADDRESS
#define CONFIG_YC_SHMEM_SIZE (CONFIG_YC_BUFFER0_SIZE + CONFIG_YC_BUFFER1_SIZE)

typedef unsigned long (*compartment_entry)(uintptr_t input_buffer,
					   uintptr_t output_buffer);

typedef struct {
	uintptr_t pcc; /* Current PCC */
	uintptr_t buf; /* Data buffer shared with the OS */
	uintptr_t mtvec;
	uintptr_t gprs[32];
	compartment_entry entries[4]; /* Compartment entry points */

	unsigned long mstatus;

#if defined(CONFIG_SBI_YC_SUPPORT_FPU)
	unsigned long fprs[32];
	/* FPU status register */
	unsigned long fcsr;
#endif /* CONFIG_SBI_YC_SUPPORT_FPU */

#if defined(CONFIG_SBI_YC_SUPPORT_RVV)
	char vrs[32][RVV_VLEN / 8];
	/* Other RVV state */
	unsigned long vstart;
	unsigned long vl;
	unsigned long vtype;
	unsigned long vcsr; /* Includes vxrm and vxsat */
#endif /* CONFIG_SBI_YC_SUPPORT_RVV */

	/* Current HART a compartment is pinned to */
	long hartid;
} compartment_t;

_Static_assert(sizeof(compartment_t) == YC_COMP_SIZE,
	       "Compartment layout mismatch");

extern void *cheri_infinite_cap;
extern unsigned long compartment_enter(compartment_t *compartment);
extern unsigned long compartment0_enter(uintptr_t input_buffer,
					uintptr_t output_buffer);
extern unsigned long compartment1_enter(uintptr_t input_buffer,
					uintptr_t output_buffer);
extern unsigned long compartment_trap(void);
extern unsigned long compartment_return(void);

unsigned long sbi_yc_enter(unsigned long cid, unsigned long timeout,
			   unsigned long pa0, unsigned long size0,
			   unsigned long pa1, unsigned long size1);
unsigned long sbi_yc_resume(unsigned long id, unsigned long timeout);

#endif
