	.equ	STACK, 0x8000
	.section .init
	.global _start
_start:
	mov	sp, #STACK	@ initialize the stack pointer
	bl	main		@ invoke the main function
0:	b	0b		@ stay here forever when the main ends
