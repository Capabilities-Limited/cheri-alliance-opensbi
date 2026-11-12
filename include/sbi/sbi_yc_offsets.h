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

#ifndef __SBI_YC_OFFSETS_H__
#define __SBI_YC_OFFSETS_H__

/*
 * Offsets into the YC structures, shared by C and preprocessed assembly
 * (.S). Include the project's configuration before this header. They assume
 * the normal, unpacked RISC-V ABI layout, with no explicit over-alignment.
 */
#define YC_COMP_ENTRY_STRIDE		__SIZEOF_POINTER__
#define YC_COMP_ENTRY_ALIGN		__SIZEOF_POINTER__

#define YC_OPENSBI_CONTEXT_FP_BASE	(21 * __SIZEOF_POINTER__)
#define YC_OPENSBI_CONTEXT_FCSR		(YC_OPENSBI_CONTEXT_FP_BASE + 32 * 8)
#define YC_OPENSBI_CONTEXT_SIZE		(YC_OPENSBI_CONTEXT_FCSR + 8)

#define YC_COMP_PTR_BYTES		__SIZEOF_POINTER__
#define YC_COMP_LONG_BYTES		__SIZEOF_LONG__
#define YC_COMP_ALIGN_UP(n, a)		(((n) + (a) - 1) & ~((a) - 1))

#define YC_COMP_PCC			0
#define YC_COMP_BUF			(YC_COMP_PCC + YC_COMP_PTR_BYTES)
#define YC_COMP_MTVEC			(YC_COMP_BUF + YC_COMP_PTR_BYTES)
#define YC_COMP_GPRS			(YC_COMP_MTVEC + YC_COMP_PTR_BYTES)
#define YC_COMP_GPR_STRIDE		YC_COMP_PTR_BYTES
#define YC_COMP_GPRS_SIZE		(32 * YC_COMP_GPR_STRIDE)
#define YC_COMP_GPR(n)			(YC_COMP_GPRS + (n) * YC_COMP_GPR_STRIDE)

#define YC_COMP_ENTRY_COUNT		4
#define YC_COMP_ENTRIES			\
	YC_COMP_ALIGN_UP(YC_COMP_GPRS + YC_COMP_GPRS_SIZE, YC_COMP_ENTRY_ALIGN)
#define YC_COMP_ENTRIES_SIZE		(YC_COMP_ENTRY_COUNT * YC_COMP_ENTRY_STRIDE)
#define YC_COMP_ENTRY(n)		(YC_COMP_ENTRIES + (n) * YC_COMP_ENTRY_STRIDE)
#define YC_COMP_MSTATUS			\
	YC_COMP_ALIGN_UP(YC_COMP_ENTRIES + YC_COMP_ENTRIES_SIZE, \
			 YC_COMP_LONG_BYTES)

#if defined(CONFIG_SBI_YC_SUPPORT_FPU)
#define YC_COMP_FPRS			(YC_COMP_MSTATUS + YC_COMP_LONG_BYTES)
#define YC_COMP_FPR_STRIDE		YC_COMP_LONG_BYTES
#define YC_COMP_FPRS_SIZE		(32 * YC_COMP_FPR_STRIDE)
#define YC_COMP_FPR(n)			(YC_COMP_FPRS + (n) * YC_COMP_FPR_STRIDE)
#define YC_COMP_FCSR			(YC_COMP_FPRS + YC_COMP_FPRS_SIZE)
#define YC_COMP_PADDING			(YC_COMP_FCSR + YC_COMP_LONG_BYTES)
#define YC_COMP_AFTER_FPU		(YC_COMP_FCSR + YC_COMP_LONG_BYTES)
#else
#define YC_COMP_AFTER_FPU		(YC_COMP_MSTATUS + YC_COMP_LONG_BYTES)
#endif

#if defined(CONFIG_SBI_YC_SUPPORT_RVV)
#if !defined(RVV_VLEN) || RVV_VLEN <= 0 || (RVV_VLEN % 8) != 0
#error "Define RVV_VLEN in bits, positive and divisible by eight"
#endif
#define YC_COMP_VRS			YC_COMP_AFTER_FPU
#define YC_COMP_VR_STRIDE		(RVV_VLEN / 8)
#define YC_COMP_VRS_SIZE		(32 * YC_COMP_VR_STRIDE)
#define YC_COMP_VR(n)			(YC_COMP_VRS + (n) * YC_COMP_VR_STRIDE)
#define YC_COMP_VSTART			\
	YC_COMP_ALIGN_UP(YC_COMP_VRS + YC_COMP_VRS_SIZE, YC_COMP_LONG_BYTES)
#define YC_COMP_VL			(YC_COMP_VSTART + YC_COMP_LONG_BYTES)
#define YC_COMP_VTYPE			(YC_COMP_VL + YC_COMP_LONG_BYTES)
#define YC_COMP_VCSR			(YC_COMP_VTYPE + YC_COMP_LONG_BYTES)
#define YC_COMP_END			(YC_COMP_VCSR + YC_COMP_LONG_BYTES + YC_COMP_LONG_BYTES)
#else
#define YC_COMP_END			(YC_COMP_AFTER_FPU + YC_COMP_LONG_BYTES)
#endif

/*
 * Choose the alignment with #if, since a C conditional expression cannot be
 * used in assembly.
 */
#if YC_COMP_ENTRY_ALIGN > YC_COMP_PTR_BYTES && \
    YC_COMP_ENTRY_ALIGN > YC_COMP_LONG_BYTES
#define YC_COMP_ALIGN			YC_COMP_ENTRY_ALIGN
#elif YC_COMP_PTR_BYTES > YC_COMP_LONG_BYTES
#define YC_COMP_ALIGN			YC_COMP_PTR_BYTES
#else
#define YC_COMP_ALIGN			YC_COMP_LONG_BYTES
#endif
#define YC_COMP_SIZE			YC_COMP_ALIGN_UP(YC_COMP_END, YC_COMP_ALIGN)

#endif /* __SBI_YC_OFFSETS_H__ */
