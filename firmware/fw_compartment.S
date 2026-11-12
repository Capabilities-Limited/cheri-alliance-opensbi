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

#include "fw_base.S"
#include <sbi/sbi_yc_offsets.h>

	.section .entry, "ax", %progbits
	.align 3
	.global fw_boot_hart
fw_boot_hart:
	li	a0, -1
	ret

	.section .entry, "ax", %progbits
	.align 3
	.global fw_save_info
fw_save_info:
	ret

	.section .entry, "ax", %progbits
	.align 3
	.global fw_next_addr
fw_next_addr:
	PC_PTR_L	a0, payload_bin
#if defined(__CHERI__)
	yaddrw	a0, a3, a0
#endif /* defined(__CHERI__) */
	ret

	.section .entry, "ax", %progbits
	.align 3
	.global fw_next_mode
fw_next_mode:
	li	a0, PRV_S
	ret

	.section .entry, "ax", %progbits
	.align 3
	.global fw_options
fw_options:
	add	a0, zero, zero
	ret

	.section .payload, "ax", %progbits
	.align 4
	.globl payload_bin
payload_bin:
#ifndef FW_PAYLOAD_PATH
	wfi
	j	payload_bin
#else
	.incbin	FW_PAYLOAD_PATH
#endif

	.section .entry, "ax", %progbits
	.align 3
	.global fw_next_arg1
fw_next_arg1:
	add	a0, a1, zero
#if defined(__CHERI__)
	yaddrw	a0, a3, a0
	set_cap_perm	a0, a0, a1, CHERI_PERM_DATA	/* FDT is a read-write data */
#endif /* defined(__CHERI__) */
	ret

#if defined(__riscv_flen) && __riscv_flen == 64
#define FREG_L fld
#define FREG_S fsd
#elif defined(__riscv_flen) && __riscv_flen == 32
#define FREG_L flw
#define FREG_S fsw
#endif

.macro OPENSBI_YC_SAVE_FP
#if defined(__riscv_flen) && defined(CONFIG_SBI_YC_SUPPORT_FPU)
	li	t1, 0x6000
	csrs	mstatus, t1

	.irp r,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
		FREG_S	f\r, (YC_OPENSBI_CONTEXT_FP_BASE + \r*8)(t0)
	.endr

	csrr	t1, fcsr
	sw	t1, YC_OPENSBI_CONTEXT_FCSR(t0)
#endif
.endm

.macro OPENSBI_YC_LOAD_FP
#if defined(__riscv_flen) && defined(CONFIG_SBI_YC_SUPPORT_FPU)
	li	t1, 0x6000
	csrs	mstatus, t1

	.irp r,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
		FREG_L	f\r, (YC_OPENSBI_CONTEXT_FP_BASE + \r*8)(t0)
	.endr

	lw	t1, YC_OPENSBI_CONTEXT_FCSR(t0)
	csrw	fcsr, t1
#endif
.endm

.macro	OPENSBI_YC_SAVE_CONTEXT
	csrr	t0, mscratch
	yaddi	t0, t0, SBI_SCRATCH_YC_CONTEXT_OFFSET
	REG_S	a0, (0*SZREG)(t0)
	REG_S	s0, (1*SZREG)(t0)
	REG_S	s1, (2*SZREG)(t0)
	REG_S	s2, (3*SZREG)(t0)
	REG_S	s3, (4*SZREG)(t0)
	REG_S	s4, (5*SZREG)(t0)
	REG_S	s5, (6*SZREG)(t0)
	REG_S	s6, (7*SZREG)(t0)
	REG_S	s7, (8*SZREG)(t0)
	REG_S	s8, (9*SZREG)(t0)
	REG_S	s9, (10*SZREG)(t0)
	REG_S	s10, (11*SZREG)(t0)
	REG_S	s11, (12*SZREG)(t0)
	REG_S	sp, (13*SZREG)(t0)
	REG_S	ra, (14*SZREG)(t0)
	REG_S	gp, (15*SZREG)(t0)
	REG_S	tp, (16*SZREG)(t0)

	csrr	t1, mtvec
	REG_S	t1, (17*SZREG)(t0)
	csrr	t1, mscratch
	REG_S	t1, (18*SZREG)(t0)
	csrr	t1, mstatus
	REG_S	t1, (19*SZREG)(t0)
	csrr	t1, mepc
	REG_S	t1, (20*SZREG)(t0)
	OPENSBI_YC_SAVE_FP
.endm

.macro	COMPARTMENT_CLEAR_REGS
	/* Flush the instruction cache. */
	fence.i

	csrw	ddc, zero

	/* Skip a0/x10 as it will hold the compartment's pointer and will be overwritten */
	.irp r,1,2,3,4,5,6,7,8,9,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
		ymv	x\r, zero
	.endr

#if defined(__riscv_flen) && defined(CONFIG_SBI_YC_SUPPORT_FPU)
	/* Enable FP access: mstatus.FS = Dirty. */
	li	t0, 0x6000
	csrs	mstatus, t0

	.irp r,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
#if __riscv_flen == 64
	/* Full-width +0.0; works on RV32D and RV64D. */
	fcvt.d.w	f\r, zero, rne
#elif __riscv_flen == 32
	fmv.w.x	f\r, zero
#else
#error "Unsupported floating-point register width"
#endif
	.endr

	/* Clear rounding mode and accrued exception flags. */
	csrw	fcsr, zero
#endif

#if defined(__riscv_vector) && defined(CONFIG_SBI_YC_SUPPORT_RVV)
	/* Enable vector access: mstatus.VS = Dirty. */
	li	t0, 0x600
	csrs	mstatus, t0

	/* Clear every element, including after an interrupted operation. */
	csrw	vstart, zero
	vsetvli	t0, zero, e8, m8, tu, mu

	/* Each instruction clears a group of eight vector registers. */
	vmv.v.i	v0, 0
	vmv.v.i	v8, 0
	vmv.v.i	v16, 0
	vmv.v.i	v24, 0

	/* Clear vxrm/vxsat; finish with vl=0 and a known vtype. */
	csrw	vcsr, zero
	vsetivli	zero, 0, e8, m1, tu, mu
#endif

.endm

.macro	OPENSBI_YC_LOAD_CONTEXT
	csrr	t0, mscratch
	yaddi	t0, t0, SBI_SCRATCH_YC_CONTEXT_OFFSET
	OPENSBI_YC_LOAD_FP
	REG_L	s0, (1*SZREG)(t0)
	REG_L	s1, (2*SZREG)(t0)
	REG_L	s2, (3*SZREG)(t0)
	REG_L	s3, (4*SZREG)(t0)
	REG_L	s4, (5*SZREG)(t0)
	REG_L	s5, (6*SZREG)(t0)
	REG_L	s6, (7*SZREG)(t0)
	REG_L	s7, (8*SZREG)(t0)
	REG_L	s8, (9*SZREG)(t0)
	REG_L	s9, (10*SZREG)(t0)
	REG_L	s10, (11*SZREG)(t0)
	REG_L	s11, (12*SZREG)(t0)
	REG_L	sp, (13*SZREG)(t0)
	REG_L	ra, (14*SZREG)(t0)
	REG_L	gp, (15*SZREG)(t0)
	REG_L	tp, (16*SZREG)(t0)
	REG_L	t1, (17*SZREG)(t0)
	csrw	mtvec, t1
	REG_L	t1, (18*SZREG)(t0)
	csrw	mscratch, t1
	REG_L	t1, (19*SZREG)(t0)
	csrw	mstatus, t1
	REG_L	t1, (20*SZREG)(t0)
	csrw	mepc, t1
.endm

.macro	YC_COMPARTMENT_SAVE_REGS
	/* Save the GPRs, except x10 (a0). */
	.irp r,1,2,3,4,5,6,7,8,9,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
		REG_S	x\r, YC_COMP_GPR(\r)(a0)
	.endr

#if defined(__riscv_flen) && defined(CONFIG_SBI_YC_SUPPORT_FPU)
	/* Save the FPRs. */
	csrr	t0, fcsr
	sw	t0, YC_COMP_FCSR(a0)
	.irp r,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
		FREG_S	f\r, YC_COMP_FPR(\r)(a0)
	.endr
#endif

	csrr	t0, mstatus
	XREG_S	t0, (YC_COMP_MSTATUS)(a0)
	csrr	t0, mepc
	REG_S	t0, (YC_COMP_PCC)(a0)
	csrr	t0, mtvec
	REG_S	t0, (YC_COMP_MTVEC)(a0)
.endm

.macro	YC_COMPARTMENT_LOAD_REGS
	/* Load the GPRs, except x10 (a0). */
	.irp r,1,2,3,4,6,7,8,9,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
		REG_L	x\r, YC_COMP_GPR(\r)(a0)
	.endr

	REG_L	t0, (YC_COMP_PCC)(a0)
	csrw	mepc, t0
	REG_L	t0, (YC_COMP_MTVEC)(a0)
	csrw	mtvec, t0

	XREG_L	t0, (YC_COMP_MSTATUS)(a0)
	csrw	mstatus, t0

#if defined(__riscv_flen) && defined(CONFIG_SBI_YC_SUPPORT_FPU)
	/* Load the FPRs. */
	.irp r,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
		FREG_L	f\r, YC_COMP_FPR(\r)(a0)
	.endr
	lw	t0, YC_COMP_FCSR(a0)
	csrw	fcsr, t0
#endif

	REG_L	t0, (YC_COMP_GPR(5))(a0)
	REG_L	a0, (YC_COMP_GPR(10))(a0)
.endm

	.align 4
	.globl compartment_enter
	.type  compartment_enter, @function
compartment_enter:
	OPENSBI_YC_SAVE_CONTEXT
	/* Make sure DDC is cleared here; we do not need full register clearing since
	 * YC_COMPARTMENT_LOAD_REGS will reload all registers anyway.
	 */
	csrw	ddc, zero
	YC_COMPARTMENT_LOAD_REGS
	mret

	.globl compartment_return
	.type  compartment_return, @function
compartment_return:
	COMPARTMENT_CLEAR_REGS
	OPENSBI_YC_LOAD_CONTEXT
	li	a0, 0
	ret
	.size compartment_enter, . - compartment_enter

	.globl compartment_trap
	.type  compartment_trap, @function
	.balign 4
compartment_trap:
	/* a0 = original sbi_scratch; mscratch = the compartment's a0. */
	csrrw	a0, mscratch, a0

	/* Save the compartment's t0 before using it. */
	REG_S	t0, SBI_SCRATCH_TMP0_OFFSET(a0)

	csrr	t0, mscratch
	/* Restore mscratch to the original sbi_scratch pointer. */
	csrw	mscratch, a0

	/* Recover this hart's active compartment context. */
	REG_L	a0, (SBI_SCRATCH_YC_CONTEXT_OFFSET)(a0)

	/* Save the compartment's a0. */
	REG_S	t0, YC_COMP_GPR(10)(a0)

	csrr	t0, mscratch
	REG_L	t0, SBI_SCRATCH_TMP0_OFFSET(t0)

	YC_COMPARTMENT_SAVE_REGS
	COMPARTMENT_CLEAR_REGS
	OPENSBI_YC_LOAD_CONTEXT

	csrr	t0, mcause
	bgez	t0, .Lcompartment_fault		/* Synchronous exception */

	slli	t0, t0, 1			/* Clear the interrupt bit */
	srli	t0, t0, 1
	li	t1, 7				/* Machine timer interrupt */
	bne	t0, t1, .Lcompartment_fault

	li	a0, -12				/* SBI_ERR_TIMEOUT */
	ret

.Lcompartment_fault:
	li	a0, -1				/* SBI_ERR_FAILED */
	ret
	.size compartment_trap, . - compartment_trap

/* First compartment's content (its binary: code, data, etc.) */
	.section .compartments, "ax", %progbits
	.globl compartment0_enter
	.type  compartment0_enter, @object
	.balign 1024*1024
compartment0_enter:
#ifndef FW_COMPARTMENT0_PATH
	j	compartment0_enter
#else
	.incbin	FW_COMPARTMENT0_PATH
#endif
compartment0_end:
	.size compartment0_enter, . - compartment0_enter

/* Second compartment's content (its binary: code, data, etc.) */
	.globl compartment1_enter
	.type  compartment1_enter, @object
	.balign 1024*1024
compartment1_enter:
#ifndef FW_COMPARTMENT1_PATH
	j	compartment1_enter
#else
	.incbin	FW_COMPARTMENT1_PATH
#endif
	.balign 1024*1024
compartment1_end:
	.size compartment1_enter, . - compartment1_enter
