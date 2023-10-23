# pic-macros

Assembly language macros that make programming PIC microcontrollers more
pleasant.  This is somewhat inspired by the Parallax PIC assembler.

The general idea of these macros is to hide the W register and make PIC look
like a two-address machine with mnemonics similar to 6800 / 6502 / PDP-11.

The syntax used is compatible with Microchip's [MPASM](https://ww1.microchip.com/downloads/en/DeviceDoc/30400g.pdf)
and the open source [GPASM](https://gputils.sourceforge.io/).

Simply include "macros.inc" into your assembly source file with:

~~~
	include p10f200.inc
	include macros.inc
~~~

## Features

* The macros make the PIC look like a two address machine.  This means that
dual-operand instruction expect two register arguments, or one register and
one immediate value:

~~~
	add	0x14, 0x18	; Add register 0x18 to register 0x14
	addi	0x14, 3		; Add immediate value 3 to register 0x14
~~~

* The macros hide register banking.  On entry, each macro assumes that the
current bank is bank 0 (the RP bits in STATUS are all set to zero).  If the
macro needs to switch banks to access a register which is located beyond
bank 0, it automatically does so.  It then restores the current bank back to 0.

* Macros are provided for multi-precision arithmetic, including ADC and
SBC (add with carry and subtract with carry).  For example, to add two
32-bit numbers, this sequence can be used:

~~~
a	equ	0x10	; Location of first 32-bit number
b	equ	0x14	; Location of second 32-bit number

	add	a+0, b+0	; Add, set carry correctly
	adc	a+1, b+1	; Add with carry, set carry correctly
	adc	a+2, b+2	; Add with carry, set carry correctly
	qadc	a+3, b+3	; Quick add with carry (does not set carry correctly)
~~~

* A complete set of branches are provided for unsigned comparisons, using
Motorola syntax (so you don't have to think about the skip instructions):

~~~
	cmp	a, b	; Compare a with b, then one of:
	jeq	target	; Jump to target if equal
	jne	target	; Jump to target if not equal
	jhi	target	; Jump to target if a is higher than b
	jhs	target	; Jump to target if a is higher or same as than b
	jlo	target	; Jump to target if a is lower than b
	jls	target	; Jump to target if a is lower than or same as b
~~~

* Macros are provided for proper code bank switching:

Use jsr when calling a subroutine that is located within the same bank:

~~~
	jsr	fred		; Jump to subroutine fred

fred	. . .			; Do something
	rts			; Return
~~~

Use farjsr when calling a subroutine that is located in a different bank:

~~~
	farjsr	bob		; Jump to subroutine bob

bob	. . .			; Do something
	rts			; Return

~~~

* Macros for flash-memory (code-space) based table indexing

Create a table of bytes using the "table" and "val" macros:

~~~
	mytable	table
		val	0x10	; Table index 0
		val	0x20	; Table index 1
		val	0x30	; Table index 2
~~~

Use short_lookup, lookup or far_lookup depending on the location of the
table:

~~~
		ldi	0x10, 2			; Table index of 2 in register 0x10
		lookup	mytable, 0x14, 0x10	; Index mytable
		; Register 0x14 now has 0x30
~~~

Which lookup should you use?

* short_lookup if table is in same 256 word page
* lookup if table is in same 2K word page
* farlookup if table is in a different page

# Macro reference

## Instructions that expose W

You do not have to use these except as a prefix for this OPTION or TRIS
instructions.  You may want to use them if you need to load many of the same
value into a set of registers.

~~~
	ldwi	<imm>	; Load immediate value <imm> into W

	addwi	<imm>	; Add immediate value <imm> into W

	subwi	<imm>	; Subtract immediate value <imm> from W

	orwi	<imm>	; Bit-wise OR immediate value <imm> into W

	andwi	<imm>	; Bit-wise AND immediate value <imm> into W

	xorwi	<imm>	; Bit-wise exclusive-OR immediate value <imm> into W

	ldw	<src>	; Load register <src> into W

	stw	<dest>	; Store W into register <dest>

	clrw		; Clear W
~~~

## Dual Operand Instructions

~~~
	ld	<dest>, <src>		; Load register <src> into register <dest>

	ldi	<dest>, <imm>		; Load immediate value <imm> into register <dest>

	add	<dest>, <src>		; Add register <src> to register <dest>

	addi	<dest>, <imm>		; Add immediate value <imm> to register <dest>

	adc	<dest>, <src>		; Add with carry register <src> to register <dest>

	adci	<dest>, <imm>		; Add with carry immediate value <imm> to register <dest>

	qadc	<dest>, <src>		; Quick version of adc: does not set resulting carry correctly

	qadci	<dest>, <imm>		; Quick version of adci: does not set resulting carry correctly

	sub	<dest>, <src>		; Subtract register <src> from register <dest>

	subi	<dest>, <imm>		; Subtract immediate value <imm> from register <dest>

	sbc	<dest>, <src>		; Subtract with carry register <src> from register <dest>

	sbci	<dest>, <imm>		; Subtract with carry immediate value <imm> from register <dest>

	qsbc	<dest>, <src>		; Quick version of sbc: does not set resulting carry correctly

	qsbci	<dest>, <imm>		; Quick version of sbci: does not set resulting carry correctly

	cmp	<dest>, <src>		; Compare register <dest> with register <src>

	cmpi	<dest>, <imm>		; Compare register <dest> with immediate value <imm>

	and	<dest>, <src>		; Bitwise-AND register <src> into register <dest>

	andi	<dest>, <imm>		; Bitwise-AND immediate value <imm> into register <dest>

	or	<dest>, <src>		; Bitwise-OR regiter <src> into register <dest>

	ori	<dest>, <imm>		; Bitwise-OR immediate value <imm> into register <dest>

	xor	<dest>, <src>		; Bitwise exclusive-OR register <src> into register <dest>

	xori	<dest>, <imm>		; Bitwise exclusive-OR immediate value <imm> into register <dest>
~~~

## Dual Operand Bit Instructions

~~~
	ldb	<dest>, <dest-b>, <src>, <src-b> 	; Load bit number <dest-b> of register <dest> from bit number <src-b> of register <src>

	ldnb	<dest>, <dest-b>, <src>, <src-b>	; Same as above, but load the complement of the bit
~~~

Incremement or decrement register depending on bit value:

~~~
	addb	<dest>, <src>, <b>	; Add bit number <b> of register <src> to register <dest>
					; This one affects Carry and Zero.

	incb	<dest>, <src>, <b>	; Increment register <dest> if bit number <b> of register <src> is set
					; This one does not affect Carry.  Zero is affected only if the bit was set.

	subnb	<dest>, <src>, <b>	; Subtract inverse of bit number <b> from register <src> from register <dest>
					; This one affects Carry and Zero.

	decb	<dest>, <src>, <b>	; Decrement register <dest> if bit number <b> or register <src> is clear
					; this one does not affect Carry.  Zero is affected only if the bit was clear.
~~~

## Single Operand Instructions

~~~
	clr	<dest>			; Clear register <dest>

	inc	<dest>			; Increment register <dest>

	dec	<dest>			; Decrement register <dest>

	rol	<dest>			; Rotate register <dest> left through carry

	lsl	<dest>			; Logical shift left register <dest>

	asl	<dest>			; Arithmetic shift right register <dest>

	ror	<dest>			; Rotate register <dest> right through carry

	lsr	<dest>			; Logical shift right register <dest>

	asr	<dest>			; Arithmetic shift right register <dest>

	com	<dest>			; Bitwise complement register <dest>

	neg	<dest>			; 2s complement negate register <dest>

	swap	<dest>			; Swap halves of register <dest>

	addc	<dest>			; Add carry to register <dest>

	qaddc	<dest>			; Quick version of above: does not set Carry correctly

	subb	<dest>			; Subtract borrow (negative of carry flag) from register <dest>

	qsubb	<dest>			; Quick verison of above: does not set Carry correctly
~~~

## Single Operand Bit Instructions

~~~
	bic	<dest>, <b>		; Clear bit number <b> of register <dest>

	bis	<dest>, <b>		; Set bit number <b> of register <dest>
~~~

## Implied Operand Instructions

~~~
	clc		; Clear carry flag

	stc		; Set carry flag

	clz		; Clear zero flag

	stz		; Set zero flag
~~~

## Conditional Jumps and Loops

~~~
	jbc	<src>, <b>, label	; Jump if bit number <b> of <src> is clear

	jbs	<src>, <b>, label	; Jump if bit number <b> of <src> is set

	jeq	label			; Jump if equal (Z set)

	jne	label			; Jump if not equal (Z clear)

	jcs	label			; Jump if Carry set

	jcc	label			; Jump if Carry clear

	jlo	label			; Jump if lower

	jls	label			; Jump if lower or same

	jhi	label			; Jump if higher

	jhs	label			; Jump if higher or same

	decjne	<dest>, label		; Decrement register <dest>, jump if it's not zero

	incjne	<dest>, label		; Increment register <dest>, jump if it's not zero
~~~

## Unconditional Jumps and Subroutines

~~~
	jsr	label			; Jump to subroutine at label (same page), (same as CALL)

	farjsr	label			; Jump to subroutine at label (different page)

	rts				; Return from subroutine (same as RETURN)

	jmp	label			; Jump to label (same as GOTO)

	farjmp	label			; Jump to label (different page)
~~~

## PIC instructions that should be used directly

~~~
	nop				; No Operation

	sleep				; Sleep until interrupt

	clrwdt				; Clear watchdog timer

	tris				; Load TRIS register (use with ldwi)

	option				; Load option register (use with ldwi)
~~~

## Table macros

~~~
		; Index <table> with regsiter <src>, but result into <dest>
		short_lookup	<table>, <dest>, <src>	; Use if table is in same 256 word page
		lookup		<table>, <dest>, <src>	; Use if table is in same 2K word page
		far_lookup	<table>, <dest>, <src>	; Use if table is in a different page

	fred	table			; Create a byte table called fred (alias for "addwf pcl, 1")
		val	1		; val is alias for "retlw"
		val	2
		val	3
		. . .
~~~
