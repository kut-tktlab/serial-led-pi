	.section .init
	.global _start
_start:
	@ 起動時はキャッシュがoffになっている.
	@ さらにRPi3は起動時にHYPモードになっており, SVCモードに
	@ 遷移しないとキャッシュの設定が有効にならない.
	@ https://www.raspberrypi.org/forums/viewtopic.php?f=72&t=161885

	@ leave HYP-mode
	mrs	r0, cpsr	@ read current-PSR

	eor	r0, r0, #0x1a
	tst	r0, #0x1f	@ mode bits == 0x1a (hyp) ?

	bic	r0, r0, #0x1f	@ clear mode bits
	orr	r0, r0, #0x13	@ set SVC mode
	orr	r0, r0, #0xc0	@ mask IRQ and FIQ

	bne	1f		@ branch if not HYP mode

	orr	r0, r0, #0x100	@ mask ABORT (SError)
	msr	spsr_cxsf, r0	@ write to saved-PSR all bits (31:0)

	ldr	r1, =2f
	msr	elr_hyp, r1
	eret			@ (bx elr_hyp) go down to SVC mode

1:	msr	cpsr_c, r0	@ write to current-PSR control bits (7:0)
2:	isb			@ inst sync barrier (flush pipeline)

	@ initialize translation table
	ldr	r1, =0		@ for base in 0..<1024-16:
	ldr	r2, =0x4000	@ addr of pagetable (traditionally 0x4000)
	ldr	r3, =0x50c0e	@ pagetable flags - 0x10c0e also possible

1:	mov	r0, r1, lsl #20
	orr	r0, r0, r3	@ r0 = base << 20 | flags
	str	r0, [r2], #4	@ pagetable[base] = r0
	add	r1, r1, #1
	cmp	r1, #1024-16
	bne	1b

	ldr	r3, =0x40c06	@ flags for rest of the pagetable
2:	mov	r0, r1, lsl #20
	orr	r0, r0, r3	@ r0 = base << 20 | flags
	str	r0, [r2], #4	@ pagetable[base] = r0
	add	r1, r1, #1
	cmp	r1, #4096
	bne	2b

	@ activate mmu:
	mov	r0, #0x0		@ always use TTBR0
	mcr	p15, 0, r0, c2, c0, 2

	ldr	r0, =0 | 0x4000		@ table walk etc.
	mcr	p15, 0, r0, c2, c0, 0

	ldr	r0, =0x55555555		@ needed for Raspi3?
	mcr	p15, 0, r0, c3, c0, 0

	@ start L1 cache and mmu
	mrc	p15, 0, r0, c1, c0, 0	@ read System Control Register (SCTLR)
	orr	r0, r0, #0x1000		@ enable instruction cache
	orr	r0, r0, #0x800		@ enable branch prediction
	orr	r0, r0, #0x4		@ enable data cache
	orr	r0, r0, #0x1		@ enable mmu
	mcr	p15, 0, r0, c1, c0, 0	@ write back

	@ initialize .bss
	ldr	r0, =__bss_start__
	ldr	r1, =__bss_end__
	mov	r2, #0
	b	2f
1:	str	r2, [r0], #4
2:	cmp	r0, r1
	blo	1b

	@ initialize the stack pointer
	ldr	sp, =0x8000

	@ invoke the main func
	bl	main

	b	.		@ stay forever
