/*
 * r2300_fpu.S: Save/restore floating point context for signal handlers.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 by Ralf Baechle
 *
 * Multi-arch abstraction and asm macros for easier reading:
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 *
 * $Id: r2300_fpu.S,v 1.1.1.1 1999/02/08 06:21:22 linas Exp $
 */
#include <asm/asm.h>
#include <asm/fpregdef.h>
#include <asm/mipsregs.h>
#include <asm/offset.h>
#include <asm/regdef.h>

	.set	mips3
	.set	noreorder
	/* Save floating point context */
	.align	5
	LEAF(r2300_save_fp_context)
	mfc0	t0,CP0_STATUS
	sll	t0,t0,2

	bgez	t0,1f
	 nop

	cfc1	t0,fcr31
	/* Store the 32 single precision registers */
	swc1	$f0,(SC_FPREGS+0)(a0)
	swc1	$f1,(SC_FPREGS+8)(a0)
	swc1	$f2,(SC_FPREGS+16)(a0)
	swc1	$f3,(SC_FPREGS+24)(a0)
	swc1	$f4,(SC_FPREGS+32)(a0)
	swc1	$f5,(SC_FPREGS+40)(a0)
	swc1	$f6,(SC_FPREGS+48)(a0)
	swc1	$f7,(SC_FPREGS+56)(a0)
	swc1	$f8,(SC_FPREGS+64)(a0)
	swc1	$f9,(SC_FPREGS+72)(a0)
	swc1	$f10,(SC_FPREGS+80)(a0)
	swc1	$f11,(SC_FPREGS+88)(a0)
	swc1	$f12,(SC_FPREGS+96)(a0)
	swc1	$f13,(SC_FPREGS+104)(a0)
	swc1	$f14,(SC_FPREGS+112)(a0)
	swc1	$f15,(SC_FPREGS+120)(a0)
	swc1	$f16,(SC_FPREGS+128)(a0)
	swc1	$f17,(SC_FPREGS+136)(a0)
	swc1	$f18,(SC_FPREGS+144)(a0)
	swc1	$f19,(SC_FPREGS+152)(a0)
	swc1	$f20,(SC_FPREGS+160)(a0)
	swc1	$f21,(SC_FPREGS+168)(a0)
	swc1	$f22,(SC_FPREGS+176)(a0)
	swc1	$f23,(SC_FPREGS+184)(a0)
	swc1	$f24,(SC_FPREGS+192)(a0)
	swc1	$f25,(SC_FPREGS+200)(a0)
	swc1	$f26,(SC_FPREGS+208)(a0)
	swc1	$f27,(SC_FPREGS+216)(a0)
	swc1	$f28,(SC_FPREGS+224)(a0)
	swc1	$f29,(SC_FPREGS+232)(a0)
	swc1	$f30,(SC_FPREGS+240)(a0)
	swc1	$f31,(SC_FPREGS+248)(a0)
	sw	t0,SC_FPC_CSR(a0)
	cfc1	t0,$0				# implementation/version
	jr	ra
	.set	nomacro
	 sw	t0,SC_FPC_EIR(a0)
	.set	macro
1:
	jr	ra
	.set	nomacro
	 nop
	.set	macro
	END(r2300_save_fp_context)

/*
 * Restore fpu state:
 *  - fp gp registers
 *  - cp1 status/control register
 *
 * We base the decission which registers to restore from the signal stack
 * frame on the current content of c0_status, not on the content of the
 * stack frame which might have been changed by the user.
 */
	LEAF(r2300_restore_fp_context)
	mfc0	t0,CP0_STATUS
	sll	t0,t0,2

	bgez	t0,1f
	 nop

	 lw	t0,SC_FPC_CSR(a0)
	/* Restore the 16 odd double precision registers only
	 * when enabled in the cp0 status register.
	 */
	ldc1	$f0,(SC_FPREGS+0)(a0)
	ldc1	$f1,(SC_FPREGS+8)(a0)
	ldc1	$f2,(SC_FPREGS+16)(a0)
	ldc1	$f3,(SC_FPREGS+24)(a0)
	ldc1	$f4,(SC_FPREGS+32)(a0)
	ldc1	$f5,(SC_FPREGS+40)(a0)
	ldc1	$f6,(SC_FPREGS+48)(a0)
	ldc1	$f7,(SC_FPREGS+56)(a0)
	ldc1	$f8,(SC_FPREGS+64)(a0)
	ldc1	$f9,(SC_FPREGS+72)(a0)
	ldc1	$f10,(SC_FPREGS+80)(a0)
	ldc1	$f11,(SC_FPREGS+88)(a0)
	ldc1	$f12,(SC_FPREGS+96)(a0)
	ldc1	$f13,(SC_FPREGS+104)(a0)
	ldc1	$f14,(SC_FPREGS+112)(a0)
	ldc1	$f15,(SC_FPREGS+120)(a0)
	ldc1	$f16,(SC_FPREGS+128)(a0)
	ldc1	$f17,(SC_FPREGS+136)(a0)
	ldc1	$f18,(SC_FPREGS+144)(a0)
	ldc1	$f19,(SC_FPREGS+152)(a0)
	ldc1	$f20,(SC_FPREGS+160)(a0)
	ldc1	$f21,(SC_FPREGS+168)(a0)
	ldc1	$f22,(SC_FPREGS+176)(a0)
	ldc1	$f23,(SC_FPREGS+184)(a0)
	ldc1	$f24,(SC_FPREGS+192)(a0)
	ldc1	$f25,(SC_FPREGS+200)(a0)
	ldc1	$f26,(SC_FPREGS+208)(a0)
	ldc1	$f27,(SC_FPREGS+216)(a0)
	ldc1	$f28,(SC_FPREGS+224)(a0)
	ldc1	$f29,(SC_FPREGS+232)(a0)
	ldc1	$f30,(SC_FPREGS+240)(a0)
	ldc1	$f31,(SC_FPREGS+248)(a0)
	jr	ra
	.set	nomacro
	 ctc1	t0,fcr31
	.set	macro
1:
	jr	ra
	.set	nomacro
	 nop
	.set	macro
	END(r2300_restore_fp_context)
