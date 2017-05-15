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

.if 0
	.global	ledSetup
	.global	ledSend
	.global pinModePwm
	.global pwmSetClock
	.global pwmSetRange
	.global pwmWriteFifo

	@ Control registers
	.equ	PERIPHERAL_BASE, 0x3f000000

	@ GPIO
	.equ	GPIO_BASE,   0x00200000 + PERIPHERAL_BASE
	.equ	GPFSEL1,     0x04	@ function select 1
	.equ	GPFSEL_ALT5, 0x2

	@ Clock manager
	.equ	PWMCLK_BASE,   0x00101000 + PERIPHERAL_BASE
	.equ	PWMCLK_CTL,    0xa0
	.equ	PWMCLK_DIV,    0xa4
	.equ	PWMCLK_PASSWD, 0x5a000000

	@ PWM generator
	.equ	PWM_BASE, 0x0020c000 + PERIPHERAL_BASE
	.equ	PWM_CTL,  0x00
	.equ	PWM_STA,  0x04
	.equ	PWM_RNG1, 0x10		@ range
	.equ	PWM_DAT1, 0x14		@ data	(duty cycle = data/range)
	.equ	PWM_FIFO, 0x18
	.equ	PWM_RNG2, 0x20		@ range
	.equ	PWM_DAT2, 0x24		@ data	(duty cycle = data/range)

ledSetup:
	push	{lr}
	bl	pinModePwm
	bl	pwmSetClock
	bl	pwmSetRange
	mov	r0, #0
	pop	{lr}
	bx	lr

	.macro	fifo8
	bl	pwmWriteFifo
	bl	pwmWriteFifo
	bl	pwmWriteFifo
	bl	pwmWriteFifo

	bl	pwmWriteFifo
	bl	pwmWriteFifo
	bl	pwmWriteFifo
	bl	pwmWriteFifo
	.endm

ledSend:
	push	{lr}

	ldr	r1, =PWM_BASE
	mov	r3, #4
1:
	mov	r0, #8
	fifo8

	mov	r0, #3
	fifo8
	fifo8

	fifo8
	fifo8
	fifo8

	subs	r3, r3, #1
	bne	1b

	@ Send RET
	mov	r0, #0
	mov	r3, #50
2:
	bl	pwmWriteFifo
	subs	r3, r3, #1
	bne	2b

	pop	{lr}
	bx	lr

pinModePwm:
	ldr	r0, =GPIO_BASE
	ldr	r1, =(GPFSEL_ALT5 << (8 * 3))	@ GPIO 18
	str	r1, [r0, #GPFSEL1]
	bx	lr

pwmSetClock:
	push	{lr}

	@ PWM の動作モードを設定する
	ldr	r0, =PWM_BASE
	ldr	r1, =(1<<7)|(1<<5)|(1<<0)	@ MSEN1=1, USEF1=1, PWEN1=1
	str	r1, [r0, #PWM_CTL]

	@ PWM のクロックソースを設定する
	ldr	r0, =PWMCLK_BASE
	ldr	r1, =PWMCLK_PASSWD|0x01		@ src=osc, enable=false
	str	r1, [r0, #PWMCLK_CTL]

	ldr	r0, =110
	bl	delayMicroseconds

1:	@ wait for busy bit to be cleared
	ldr	r0, =1
	bl	delayMicroseconds

	ldr	r0, =PWMCLK_BASE
	ldr	r1, [r0, #PWMCLK_CTL]
	tst	r1, #0x80
	bne	1b

	ldr	r1, =PWMCLK_PASSWD|(2 << 12)	@ div = 2.0
	str	r1, [r0, #PWMCLK_DIV]
	ldr	r1, =PWMCLK_PASSWD|0x11		@ src=osc, enable=true
	str	r1, [r0, #PWMCLK_CTL]

	ldr	r0, =110
	bl	delayMicroseconds

	@ PWM の動作モードを設定する
	ldr	r0, =PWM_BASE
	ldr	r1, =(1<<6)			@ clearfifo
	str	r1, [r0, #PWM_CTL]
	ldr	r1, =(1<<7)|(1<<5)|(1<<0)	@ MSEN1=1, USEF1=1, PWEN1=1
	str	r1, [r0, #PWM_CTL]

	pop	{lr}
	bx	lr

pwmSetRange:
	push	{lr}

	ldr	r0, =PWM_BASE
	mov	r1, #12
	str	r1, [r0, #PWM_RNG1]

	ldr	r0, =10
	bl	delayMicroseconds

	pop	{lr}
	bx	lr

pwmWriteFifo:
	ldr	r1, =PWM_BASE
fifo_full:
	ldr	r2, [r1, #PWM_STA]
	tst	r2, #1			@ full?
	beq	fifo_not_full
	ands	r2, r2, #0x1fc		@ error flags
	strne	r2, [r1, #PWM_STA]	@ clear error flags
	b	fifo_full
fifo_not_full:
	str	r0, [r1, #PWM_FIFO]	@ write to fifo
	bx	lr

.endif
