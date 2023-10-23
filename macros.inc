;   Joe's PIC macros: hide the W register and make PIC look like a
;   two-address machine with mnemonics similar to 6800/6502/PDP-11

	nolist

; Copyright (C) 2013  Joseph H. Allen

; Permission is hereby granted, free of charge, to any person obtaining a
; copy of this software and associated documentation files (the
; "Software"), to deal in the Software without restriction, including
; without limitation the rights to use, copy, modify, merge, publish,
; distribute, sublicense, and/or sell copies of the Software, and to permit
; persons to whom the Software is furnished to do so, subject to the
; following conditions:

; The above copyright notice and this permission notice shall be included
; in all copies or substantial portions of the Software.

; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
; OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
; IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
; CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
; OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
; THE USE OR OTHER DEALINGS IN THE SOFTWARE.


; Multi-precision arithmetic house of cards:
;
;   inc: increment, leaves carry unchanged
;     incb: increment if bit is set
;       qaddc: quick add carry into dest [use for last byte]
;   qadc: quick add src to dest with carry [use for last byte]
;   qadci: immediate version of above
;
;   "add dest,1": increment, sets carry and z properly
;     addb: add bit, set carry properly
;       addc: add carry, set carry properly [use for middle bytes]
;   adc: add with carry, sets carry properly [use for middle bytes]
;   adci: immediate version of above
;
;   dec: decremenet, leaves carry unchanged
;     decnb: decremement if bit is clear
;       qsubb: quick subtract borrow from dest [use for last byte]
;   qsbc: quick subtract src from dest with borrow [use for last byte]
;   qsbci: immediate version of above
;
;   "subi dest, 1": decrement, sets carry and z properly
;     subnb: subtract negative of specified bit from dest
;        subb: subtract borrow flag from <dest>
;   sbc: subtract with carry, set borrow properly [use for middle bytes]
;   sbci: immediate version of above
;
; Note that multi-byte increment can be done quickly like this (does not affect any flags)
;		incfsz	dest, 1		; Inc dest, skip if result is zero (skip if we carried)
;		goto	done
;		incfsz	dest+1, 1	; Inc dest+1, skip if result 0
;		goto	done
;		incfsz	dest+2, 1
;		goto	done
;		incfsz	dest+3, 1
;		goto	overflow
;done
;overflow

;
;  Todo:
;      cmpi   a, #low: updates Z and C
;      cwci   b, #high: updates C, only can clear Z
;   It would be nice if all of these also set zero in a multi-precision way
;   Or maybe don't bother trying to keep Z up to date in sbc/adc

;   Register bank switching is automatic: Macros expect that the register
;   bank is zero on entry, and leave it at zero on exit.  In this scheme,
;   access to bank 0 registers is definitely faster than accessing registers
;   in other banks.
;
;   Macros are provided for the following instruction formats:
;
;   Register-Register instruction format:
;       Format:  <insn>  <destination>, <src>
;       Example: add     0x12, 0x13   ; Add regiter 0x13 to register 0x12
;
;       Register-Register instructions:
;          ld  <dest>, <src>          ; Load (move <src> to <dest>)
;          add <dest>, <src>          ; Add <src> to <dest>
;          adc <dest>, <src>          ; Add with carry <src> to <dest>
;          sub <dest>, <src>          ; Subtract <src> from <dest>
;          sbc <dest>, <src>          ; Subtract with carry <src> from <dest>
;          cmp <dest>, <src>          ; Compare <dest> with <src>
;          and <dest>, <src>          ; Bitwise and <src> to <dest>
;          or <dest>, <src>           ; Bitwise or <src> to <dest>
;          xor <dest>, <src>          ; Bitwise exclusive-or <src> to <dest>
;
;   Register-Immediate instruction format:
;       Format:  <insn>i <destination>, <value>
;       Example: addi    0x12, 0x2    ; Add literal value 2 to register 0x12
;
;       Register-Immediate instructions:
;          ldi <dest>, <value>        ; Load immediate <value> into <dest>
;          addi <dest>, <value>       ; Add immediate <value> to <dest>
;          adci <dest>, <value>       ; Add immediate <value> with carry into <dest>
;          subi <dest>, <value>       ; Subtract immediate <value> from <dest>
;          sbci <dest>, <value>       ; Subtract immediate <value> and borrow from <dest>
;          cmpi <dest>, <value>       ; Compare <dest> with immediate <value>
;          andi <dest>, <valie>       ; And immediate <value> to <dest>
;          ori <dest>, <value>        ; Or immediate <value> to <dest>
;          xori <dest>, <value>       ; Exclusive-or immediate <value> to <dest>
;
;   Register instruction format:
;       Format:  <insn>  <destination>
;       Example: clr     0x13         ; Clear register 0x13
;
;       Register instructions:
;          clr <dest>	; Clear <dest>
;	   inc <dest>	; Increment <dest>
;	   dec <dest>	; Decrement <dest>
;	   rol <dest>	; Rotate <dest> left through carry
;	   lsl <dest>	; Logical shift left <dest>
;	   asl <dest>	; Arithmetic shift right <dest>
;	   ror <dest>	; Rotate <dest> right through carry
;	   lsr <dest>	; Logical shift right <dest>
;	   asr <dest>	; Arithmetic shift right <dest>
;	   com <dest>	; Bitwise complement <dest>
;          neg <dest>	; 2s complement negate <dest>
;	   swap <dest>	; Swap halves of <dest>
;          addc <dest>	; Add carry to <dest>
;          subb <dest>	; Subtract borrow (negative of carry flag) from <dest>
;
;   Inherent (operand is implied by instruction):
;       Format:  <insn>
;       Example: clc                  ; Clear carry
;
;       Inherent instructions:
;          clc		; Clear carry flag
;	   stc		; Set carry flag
;	   clz		; Clear zero flag
;	   stz		; Set zero flag
;
;   Conditional jumps:
;        Format: j<insn> <destination>
;       Example: jeq fred             ; Jump to fred if previous comparison
;                                     ; resulted in equality.
;
;       Conditional jump instructions:
;          jeq, jne, jcs, jcc, jas, jac, jlo, jls, jhs, jhi, 
;
;   Jumps and subroutines:
;          jsr <target>           ; Jump to subroutine in same page
;          jmp <target>           ; Jump to label in same page
;          farjsr <target>        ; Jump to subroutine in different page
;          farjmp <target>        ; Jump to labe in different page
;          rts                    ; Return from subroutine
;
;   Create a table in code memory:
;        fred:  table                  ; Table header
;               val <value>            ; Table entry 0
;               val <value>            ; Table entry 1
;               etc.
;
;   Index a table:
;          Index specified table by <src>, put result into <dest>.
;
;          short_lookup fred, <dest>, <src> ; Table is in same 256 byte page
;          lookup fred, <dest>, <src>       ; Table is in same 2KB page
;          far_lookup fred, <dest>, <src>   ; Table is in a different page
;
;   Bit instructions:
;         addb <dest>, <src>, <bit>   ; Add specified bit to register
;         subb <dest>, <src>, <bit>   ; Subtract bit from register
;         subnb <dest>, <src>, <bit>  ; Subtract inverse of bit from register
;         bic <dest>, <bit>           ; Clear a bit
;         bis <dest>, <bit>           ; Set a bit
;         jbc <dest>, <bit>, <target> ; Jump on clear bit
;         jbs <dest>, <bit>, <target> ; Jump on set bit
;         ldb <dest>, <dest-bit>, <src>, <src-bit>  ; Load bit
;         ldnb <dest>, <dest-bit>, <src>, <src-bit> ; Load negative bit
;
;   Instructions which expose W:
;	ldwi	<imm>
;	addwi	<imm>
;	subwi	<imm>
;	orwi	<imm>
;	andwi	<imm>
;	xorwi	<imm>
;	ldw	<src>
;	stw	<dest>
;	clrw

; PIC instructions which should just be Used directly:
;   nop
;   sleep
;   clrwdt
;   tris   (use with ldwi)
;   option (use with ldwi)

; PIC facts to remember:
;
;  There are up to 512 registers [? depends on PIC].
;
;  The IND/FSR indirect addressing mechanism can reach all 512 of them, but
;  with two banks: the IRP bit of the status register supplies the upper
;  bit of the address.
;
;  Direct addressing is only 7 bits [? depends on PIC], so there are
;  multiple direct address banks.  The RP bits of the status register
;  selects the bank.  These registers are aliased into all of the
;  banks:
;
;     indf
;     pcl
;     status
;     fsr
;     pclath
;     intcon
;     general purpose registers 0x70 - 0x7F
;
;  For other registers, you use the "banksel" pseudo-instruction:
;
;    banksel ANSELA
;    clrf ANSELA
;    banksel PORTA
;
;  Borrow is inverted (0 - 1 give a clear carry).
;
;  There are two code page sizes to worry about: 256 bytes for adding to PC
;  for table lookup.  2K bytes for jmp (goto).  Can use farjmp for jumps out
;  of current page.  PCLATH supplies the upper bits so that the entire
;  memory map can be reached with goto.
;
;  mpasm has "pagesel" to help deal with this.  I provide farjmp and farjsr.
;
;  Code assumes pclath is correct, so if you set it for a farjsr and then
;  return, it needs to be restored to its original value.  The farjsr macro
;  does this for you.  Rts (return) does not- it just restores the PC, not
;  pclath.
;
;  [PIC16F720 has only a single 2K page, so no need to worry about farjsr..]
;
;  With mpasm, correct sequence is:
;     pagesel dest
;     call dest
;     pagesel here
;  here
;
;  Pagesel is stupid: it always sets the page bits (as long as code is
;  larger than 2K).  It does not know to not set them if the target is in
;  the same page.
;
;  Interrupt handlers must save context in registers 0x70 - 0x7F..  problem
;  is there is no way to know which is the current register bank, so only
;  these registers (which are aliased in each bank) can be used for this.
;
; If you do a bit operation on a PORT, the other bits could be affected
; since it's a RMW operation (for example if input give different value
; than output, you will end up changing the output).
;
; I/O bits may seem to not work because:
;
; Don't forget to write to ANSEL bits.. pins selected for analog input
; (which is the default) always read 0.
;
; On 10F200, you must clear the T0CS bit of OPTION since it prevents GP2
; from being used as an output.

; Direct address bank mask.  The PICs I've used have 7 direct address bits.

dabmask		equ	(~0x7F)	; 7 bits for direct address

; Registers
; This is processor dependent!

ind	equ	0	; [index]
rtcc	equ	1	; timer
pcl	equ	2	; Low half of PC
status	equ	3	; status flags
fsr	equ	4	; index register
pclath	equ	0xa	; Supply upper PC bits for goto and call
intcon	equ	0xb

; Most PICs
porta	equ	5
portb	equ	6
portc	equ	7

; For 10F200:
osccal	equ	5
gpio	equ	6

; Flag bits
;   7   6   5   4   3 2  1 0
; IRP RP1 RP0 ~TO ~PD Z DC C

c_flag	equ	0
dc_flag	equ	1
z_flag	equ	2
pd_flag	equ	3
to_flag	equ	4
;pa0_flag	equ	5
;pa1_flag	equ	6
;pa2_flag	equ	7

; GP registers

r0	equ	0x10
r1	equ	0x11
r2	equ	0x12
r3	equ	0x13
r4	equ	0x14
r5	equ	0x15
;r6	equ	0x16
;r7		equ	0x17

irq_arg1	equ	0x16
irq_rtn	equ	0x17

; For subroutine calls

;r8	equ	0x78
;r9	equ	0x79
;r10	equ	0x7a
;r11	equ	0x7b

arg3	equ	0x18
arg2	equ	0x19
arg1	equ	0x1a
rtn	equ	0x1b

; Reserved for interrupt context saving

;r12	equ	0x7c
;r13	equ	0x7d
;r14	equ	0x7e
;r15	equ	0x7f

saved_fsr	equ	0x7c
saved_pclath	equ	0x7d
saved_status	equ	0x7e
saved_w		equ	0x7f

;; Macros that don't hide W

; Load register into w
ldw	macro	src
		if	((src) & dabmask) != 0
			; Source is not in bank 0
			banksel	src
			movf	src, 0
			banksel	0
		else
			; Source is in bank 0
			movf	src, 0
		endif
	endm

; Store w into register
stw	macro	dest
		if	((dest) & dabmask) != 0
			; Dest is not in bank 0
			banksel	dest
			movwf	dest
			banksel	0
		else
			; Dest is in bank 0
			movwf	dest
		endif
	endm

; Load immediate into W
ldwi	macro	imm
		movlw	imm
	endm

; Add immediate into W
addwi	macro	imm
		addlw	imm
	endm

; Subtract immediate from W
subwi	macro
		sublw	imm
	endm

; Bitwise AND immediate into W
andwi	macro	imm
		andlw	imm
	endm

; Bitwide OR immediate into W
orwi	macro	imm
		iorlw	imm
	endm

; Bitwise XOR immediate into W
xorwi	macro	imm
		xorlw	imm
	endm

;; Bit operations
; clear a bit
; OK farok
bic	macro	dest, bit
		if	(((dest) & dabmask) != 0)

; If dest is not in zero bank
			banksel	dest
			bcf	dest, bit
			banksel	0
		else
; Dest is in zero bank
			bcf	dest, bit
		endif
	endm

;;; set a bit
; OK farok
bis	macro	dest, bit
		if	(((dest) & dabmask) != 0)

; If dest is not in zero bank
			banksel	dest
			bsf	dest, bit
			banksel	0
		else
; Dest is in zero bank
			bsf	dest, bit
		endif
	endm

; Clear carry (or set borrow)
; OK
clc	macro
		bic	status, c_flag
	endm

; Set carry (or clear borrow)
; OK
stc	macro
		bis	status, c_flag
	endm

; Clear zero
; OK
clz	macro
		bic	status, z_flag
	endm

; Set zero
; OK
stz	macro
		bis	status, z_flag
	endm

;;; Jump if bit is clear
; OK farok
jbc	macro	addr, bit, br
		local	?over
		if	(((addr) & dabmask) != 0)

; If addr is not in zero bank
			banksel	addr
			btfsc	addr, bit
			goto	?over
			banksel	0
			goto	br
?over
			banksel	0
		else
; If addr is in zero bank
			btfss	addr, bit
			goto	br
		endif
	endm

; Jump if bit is set
; OK farok
jbs	macro	addr, bit, br
		local	?over
		if	(((addr) & dabmask) != 0)
; If addr is not in zero bank
			banksel	addr
			btfss	addr, bit
			goto	?over
			banksel	0
			goto	br
?over
			banksel	0
		else
; If addr is in zero bank
			btfsc	addr, bit
			goto	br
		endif
	endm

; Load bit into another bit
; OK farok
ldb	macro	dest, dest_bt, src, src_bt
		local	?over1, ?over2
		if ((src) & dabmask) != 0
			banksel	src
			btfsc	src, src_bt
			goto	?over1
			banksel	dest
			bcf	dest, dest_bt
			banksel	src
?over1
			btfss	src, src_bt
			goto	?over2
			banksel	dest
			bsf	dest, dest_bt
?over2
			banksel	0
		else
			if ((dest) & dabmask) != 0
				btfsc	src, src_bt
				goto	?over1
				banksel	dest
				bcf	dest, dest_bt
				banksel	0
?over1
				btfss	src, src_bt
				goto	?over2
				banksel	dest
				bsf	dest, dest_bt
				banksel	0
?over2
			else

				btfss	src, src_bt
				bcf	dest, dest_bt
				btfsc	src, src_bt
				bsf	dest, dest_bt
			endif
		endif
	endm

; Load inversre of a bit
; OK farok
ldnb	macro	dest, dest_bt, src, src_bt
		local	?over1, ?over2
		if ((src) & dabmask) != 0
				banksel	src
				btfsc	src, src_bt
				goto	?over1
				banksel	dest
				bsf	dest, dest_bt
				banksel	src
?over1
				btfss	src, src_bt
				goto	?over2
				banksel	dest
				bcf	dest, dest_bt
?over2
				banksel	0
		else
			if ((dest) & dabmask) != 0
				btfsc	src, src_bt
				goto	?over1
				banksel	dest
				bsf	dest, dest_bt
				banksel	0
?over1
				btfss	src, src_bt
				goto	?over2
				banksel	dest
				bcf	dest, dest_bt
				banksel	0
?over2
			else

				btfss	src, src_bt
				bsf	dest, dest_bt
				btfsc	src, src_bt
				bcf	dest, dest_bt
			endif
		endif
	endm

; Increment dest if bit is set
; Z flag is updated if increment happens otherwise it's left unchanged
; OK farok
incb	macro	dest, src, bit
		local	?done1, ?done2
		if ((src) & dabmask) != 0
			if ((dest) & dabmask) == ((src) & dabmask)
				banksel	src
				btfsc	src, bit
				incf	dest, 1
				banksel	0
			else
				banksel	src
				btfss	src, bit
				goto	?done2
				banksel	dest
				incf	dest, 1
?done2
				banksel	0
			endif
		else
			if ((dest) & dabmask) != 0
				btfss	src, bit
				goto	?done1
				banksel	dest
				incf	dest, 1
				banksel	0
?done1
			else
				btfsc	src, bit
				incf	dest, 1
			endif
		endif
	endm

; Quick add carry flag to <dest>
; Z flag is updated if increment happens otherwise it's left unchanged
qaddc	macro dest
		incb	dest, status, c_flag
	endm

; *dest = *dest + 1, Z
; Warning: this does not affect carry
;   Use incc if you need C set
; OK farok
inc	macro	dest
		if	((dest) & dabmask) != 0
			banksel	dest
			incf	dest, 1
			banksel	0
		else
			incf	dest, 1
		endif
	endm

; Add bit to dest: uses jbc, incc, ldw, clc
; Sets C [and Z] correctly
; This does not set Z correctly.

addb	macro	dest, src, bit
		local	?no_carry, ?done
		jbc	?no_carry, src, bit	; Jump if bit is clear
		; Bit is set
		addi	dest, 1	; Increment dest, update carry
		goto	?done
?no_carry
;		ldw	dest	; Check dest: update Z
		clc		; Clear carry
?done
	endm

; Add carry flag to <dest>
; Sets C correctly
; This does not set Z correctly.
addc	macro dest
		local	?done
		jcc	?done	; Jump if carry is clear
		; Carry is set
		addi	dest, 1	; Increment dest, update carry
?done
	endm

; Decrement dest if bit is set
; Z flag is updated if decrement happens otherwise it's left unchanged
; OK farok
decb	macro	dest, src, bit
		local	?done1, ?done2
		if ((src) & dabmask) != 0
			if ((dest) & dabmask) == ((src) & dabmask)
				banksel	src
				btfsc	src, bit
				decf	dest, 1
				banksel	0
			else
				banksel	src
				btfss	src, bit
				goto	?done2
				banksel	dest
				decf	dest, 1
?done2
				banksel	0
			endif
		else
			if ((dest) & dabmask) != 0
				btfss	src, bit
				goto	?done1
				banksel	dest
				decf	dest, 1
				banksel	0
?done1
			else
				btfsc	src, bit
				decf	dest, 1
			endif
		endif
	endm

; Subtract inverse of bit from dest
; Z flag is updated if decrement happens otherwise it's left unchanged
; OK farok
decnb	macro	dest, src, bit
		local	?done1, ?done2
		if ((src) & dabmask) != 0
			if ((dest) & dabmask) == ((src) & dabmask)
				banksel	src
				btfss	src, bit
				decf	dest, 1
				banksel	0
			else
				banksel	src
				btfsc	src, bit
				goto	?done2
				banksel	dest
				decf	dest, 1
?done2
				banksel	0
			endif
		else
			if ((dest) & dabmask) != 0
				btfsc	src, bit
				goto	?done1
				banksel	dest
				decf	dest, 1
				banksel	0
?done1
			else
				btfss	src, bit
				decf	dest, 1
			endif
		endif
	endm

; Quick subtrace borrow from <dest>
; Z flag is updated if decrement happens otherwise it's left unchanged
qsubb	macro dest
		decnb	dest, status, c_flag
	endm

; *dest = *dest - 1, Z
; OK farok
; Warning: this does not affect carry
dec	macro	dest
		if	((dest) & dabmask) != 0
			banksel	dest
			decf	dest, 1
			banksel	0
		else
			decf	dest, 1
		endif
	endm

; Subtract negative of specified bit from dest: uses jbs, decc, ldw, stc
; Sets C [and Z] correctly
; This does not set Z correctly

subnb	macro	dest, src, bit
		local	?no_norrow, ?done
		jbs	?no_borrow, src, bit	; Jump if bit is clear
		; Bit is set
		subi	dest, 1
		goto	?done
?no_borrow
;		ldw	dest	; Check dest: update Z
		stc		; Clear borrow
?done
	endm

; Subtract borrow flag from <dest>
; Sets C correctly
subb	macro dest
		local	?done
		jbs	?done, status, c_flag	; Jump if bit is set
		; Carry is clear (borrow is set)
		subi	dest, 1
?done
	endm

;; RR group
; *dest = *src, Z
; OK farok
ld	macro	dest, src

		if	((src) & dabmask) != 0
			; Source is not in bank 0
			banksel	src
			movf	src, 0
			if	((dest) & dabmask) != 0
				; Dest is not in bank 0
				if	((dest) & dabmask) != ((src) & dabmask)
					; Source and dest are in different
					; banks
					banksel	dest
				endif

				movwf	dest
				banksel	0

			else
				; Dest is in bank 0
				banksel	0
				movwf	dest

			endif


		else
			; Source is in bank 0

			movf	src, 0

			if	((dest) & dabmask) != 0
				; Dest is not in bank 0
				banksel	dest
				movwf	dest
				banksel	0

			else
				; Dest is in bank 0
				movwf	dest

			endif

		endif
	endm

; *dest = *dest + *src, Z DC C
; OK farok
add	macro	dest, src

		if	((src) & dabmask) != 0

			banksel	src
			movf	src, 0

			if	((dest) & dabmask) != 0

				if	((dest) & dabmask) != ((src) & dabmask)
					banksel	dest
				endif

				addwf	dest, 1
				banksel	0

			else

				banksel	0
				addwf	dest, 1

			endif


		else

			movf	src, 0

			if	((dest) & dabmask) != 0
				banksel	dest
				addwf	dest, 1
				banksel	0

			else

				addwf	dest, 1

			endif

		endif
	endm

; OK farok
; *dest = *dest + *src + c
; Does not update C and Z
qadc	macro	dest, src
		qaddc	dest
		add	dest, src
	endm

; OK farok
; *dest = *dest + *src + c
; Correctly update C [and Z]
; Does not update Z
adc	macro	dest, src
		local	?c_is_set, ?c_is_clear, ?done
		jcs	?c_is_set	; Jump if carry is set
		; Carry is clear
		add	dest, src	; Add normally
		goto	?done		; Bye
?c_is_set
		; Carry is set
		add	dest, src	; Add normally
		jcc	?c_is_clear	; Jump if no carry from add
		; Carry is set from add
		inc	dest		; Bump up by 1, leaving carry set
		goto	?done
?c_is_clear	addi	dest, 1		; Bump up by 1, set carry if we wrap, clear it otherwise
?done
	endm

; *dest = *dest - *src, Z DC C
; OK farok
sub	macro	dest, src
		if	((src) & dabmask) != 0

			banksel	src
			movf	src, 0

			if	((dest) & dabmask) != 0

				if	((dest) & dabmask) != ((src) & dabmask)
					banksel	dest
				endif

				subwf	dest, 1
				banksel	0

			else

				banksel	0
				subwf	dest, 1

			endif


		else

			movf	src, 0

			if	((dest) & dabmask) != 0
				banksel	dest
				subwf	dest, 1
				banksel	0

			else

				subwf	dest, 1

			endif

		endif
	endm

; OK farok
; *dest = *dest + *src + c
; Does not correctly update C and Z
qsbc	macro	dest, src
		qsubb	dest
		sub	dest, src
	endm

; *dest = *dest - *src - ~c
; OK farok
sbc	macro	dest, src
		local	?b_is_set, ?b_is_clear, ?done
		jcc	?b_is_set	; Jump if borrow is set
		; Borrow is clear
		sub	dest, src
		goto	?done
?b_is_set
		; Borrow is set
		sub	dest, src	; Subtract normally
		jcs	?b_is_clear
		; Borrow is set from sub
		dec	dest		; Decrememnt by 1, leave borrow set
		goto	?done
?b_is_clear	subi	dest, 1		; Decrement by 1, set borrow if we wrap, clear it otherwise
?done
	endm

; *dest - *src, Z DC C
; OK farok
cmp	macro	dest, src
		if	((src) & dabmask) != 0

			banksel	src
			movf	src, 0

			if	((dest) & dabmask) != 0

				if	((dest) & dabmask) != ((src) & dabmask)
					banksel	dest
				endif

				subwf	dest, 0
				banksel	0

			else

				banksel	0
				subwf	dest, 0

			endif


		else

			movf	src, 0

			if	((dest) & dabmask) != 0
				banksel	dest
				subwf	dest, 0
				banksel	0

			else

				subwf	dest, 0

			endif

		endif
	endm

; *dest = *dest & *src, Z
; OK farok
and	macro	dest, src
		if	((src) & dabmask) != 0

			banksel	src
			movf	src, 0

			if	((dest) & dabmask) != 0

				if	((dest) & dabmask) != ((src) & dabmask)
					banksel	dest
				endif

				andwf	dest, 1
				banksel	0

			else

				banksel	0
				andwf	dest, 1

			endif


		else

			movf	src, 0

			if	((dest) & dabmask) != 0
				banksel	dest
				andwf	dest, 1
				banksel	0

			else

				andwf	dest, 1

			endif

		endif
	endm

; *dest = *dest | *src, Z
; OK farok
or	macro	dest, src
		if	((src) & dabmask) != 0

			banksel	src
			movf	src, 0

			if	((dest) & dabmask) != 0

				if	((dest) & dabmask) != ((src) & dabmask)
					banksel	dest
				endif

				iorwf	dest, 1
				banksel	0

			else

				banksel	0
				iorwf	dest, 1

			endif


		else

			movf	src, 0

			if	((dest) & dabmask) != 0
				banksel	dest
				iorwf	dest, 1
				banksel	0

			else

				iorwf	dest, 1

			endif

		endif
	endm

; *dest = *dest ^ *src, Z
; OK farok
xor	macro	dest, src
		if	((src) & dabmask) != 0

			banksel	src
			movf	src, 0

			if	((dest) & dabmask) != 0

				if	((dest) & dabmask) != ((src) & dabmask)
					banksel	dest
				endif

				xorwf	dest, 1
				banksel	0

			else

				banksel	0
				xorwf	dest, 1

			endif


		else

			movf	src, 0

			if	((dest) & dabmask) != 0
				banksel	dest
				xorwf	dest, 1
				banksel	0

			else

				xorwf	dest, 1

			endif

		endif
	endm

; *dest = imm
; OK farOK
ldi	macro	dest, imm

		movlw	imm

		if	((dest) & dabmask) != 0
			banksel	dest
			movwf	dest
			banksel	0
		else
			movwf	dest
		endif

	endm

; *dest = *dest + imm, Z DC C
; OK farOK
addi	macro	dest, imm
		movlw	imm
		if	((dest) & dabmask) != 0
			banksel	dest
			addwf	dest, 1
			banksel	0
		else
			addwf	dest, 1
		endif
	endm

; *dest = *dest + imm + c
; OK farOK
; This does not set carry or half-carry correctly- only use for final
; add in multi-precision arithmatic (not mid), and only then if you do not
; care about carry out of the multi-precision result.
qadci	macro	dest, imm
		qaddc	dest
		addi	dest, imm
	endm

; This one does set carry correctly, but it's slower
; It does not set digit-carry correctly.
adci	macro	dest, imm
		local	?c_is_set, ?c_is_clear, ?done
		jcs	?c_is_set	; Jump if carry is set
		; Carry is clear
		addi	dest, imm	; Add normally
		goto	?done		; Bye
?c_is_set
		; Carry is set
		addi	dest, imm	; Add normally
		jcc	?c_is_clear	; Jump if no carry from add
		; Carry is set from add
		inc	dest		; Bump up by 1, leave carry set
		goto	?done
?c_is_clear	addi	dest, 1		; Bump up by 1, set carry if we wrap, clear it otherwise
?done
	endm

; *dest = *dest - imm, Z DC C
; OK farOK
subi	macro	dest, imm
		movlw	imm
		if	((dest) & dabmask) != 0
			banksel	dest
			subwf	dest, 1
			banksel	0
		else
			subwf	dest, 1
		endif
	endm

; OK farok
; *dest = *dest - imm - ~c
; This does not set carry or half-carry correctly- only use for final
; subtract in multi-precision subtract (not mid), and only then if you do not
; care about carry out of the multi-precision result.
qsbci	macro	dest, imm
		qsubb	dest
		subi	dest, imm
	endm

sbci	macro	dest, imm
		local	?b_is_set, ?b_is_clear, ?done
		jcc	?b_is_set	; Jump if borrow is set
		; Borrow is clear
		subi	dest, imm
		goto	?done
?b_is_set
		; Borrow is set
		subi	dest, imm	; Subtract normally
		jcs	?b_is_clear
		; Borrow is set from sub
		dec	dest		; Decrememnt by 1, leave borrow set
		goto	?done
?b_is_clear	subi	dest, 1		; Decrement by 1, set borrow if we wrap, clear it otherwise
?done
	endm

; *dest - imm, Z DC C
; OK farok
cmpi	macro	dest, imm
		movlw	imm
		if	((dest) & dabmask) != 0
			banksel	dest
			subwf	dest, 0
			banksel	0
		else
			subwf	dest, 0
		endif
	endm

; *dest = *dest & imm, Z
; OK farok
andi	macro	dest, imm
		movlw	imm
		if	((dest) & dabmask) != 0
			banksel	dest
			andwf	dest, 1
			banksel	0
		else
			andwf	dest, 1
		endif
	endm

; *dest = *dest | imm, Z
; OK farok
ori	macro	dest, imm
		movlw	imm
		if	((dest) & dabmask) != 0
			banksel	dest
			iorwf	dest, 1
			banksel	0
		else
			iorwf	dest, 1
		endif
	endm

; *dest = *dest ^ imm, Z
; OK farok
xori	macro	dest, imm
		movlw	imm
		if	((dest) & dabmask) != 0
			banksel	dest
			xorwf	dest, 1
			banksel	0
		else
			xorwf	dest, 1
		endif
	endm

; *dest = 0, Z=1
; OK farok
clr	macro	dest
		if	((dest) & dabmask) != 0
			banksel	dest
			clrf	dest
			banksel	0
		else
			clrf	dest
		endif
	endm

; Rotate left, C
; OK farok
rol	macro	dest
		if	((dest) & dabmask) != 0
			banksel	dest
			rlf	dest, 1
			banksel	0
		else
			rlf	dest, 1
		endif
	endm

; OK farok
lsl	macro	dest
		clc
		rol	dest
	endm

; OK farok
asl	macro	dest
		clc
		rol	dest
	endm

; Rotate right, C
; OK farok
ror	macro	dest
		if	((dest) & dabmask) != 0
			banksel	dest
			rrf	dest, 1
			banksel	0
		else
			rrf	dest, 1
		endif
	endm

; OK farok
lsr	macro	dest
		clc
		ror	dest
	endm

; OK farok
asr	macro	dest
		ldb	status, c_flag, dest, 7
		ror	dest
	endm

; Complement, Z
; OK farok
com	macro	dest
		if	((dest) & dabmask) != 0
			banksel	dest
			comf	dest, 1
			banksel	0
		else
			comf	dest, 1
		endif
	endm

; *dest = -*dest, Z
; OK farok
; Does not set C correctly
neg	macro	dest
		if	((dest) & dabmask) != 0
			banksel	dest
			comf	dest, 1
			incf	dest, 1
			banksel	0
		else
			comf	dest, 1
			incf	dest, 1
		endif
	endm

; Swap nibbles
; OK farok
swap	macro	dest
		if	((dest) & dabmask) != 0
			banksel	dest
			swapf	dest, 1
			banksel	0
		else
			swapf	dest, 1
		endif
	endm

; Jump to subroutine
; OK
jsr	macro	dest
		call	dest
	endm

; Far jump to subroutine (subroutine is in different page)
farjsr	macro	dest
		local	?here
		movlw	HIGH dest
		movwf	pclath
		call	dest
		movlw	HIGH ?here	; Restore page in pclath
		movwf	pclath
?here
	endm

; Return from subroutine
; OK
rts	macro
		return
	endm

; Decrement branch ne
; OK farok
decjne	macro	dest, br
		local	?over, ?out
		if ((dest) & dabmask) != 0
			banksel	dest
			decfsz	dest, 1
			goto	?over
			banksel	0
			goto	?out
?over
			banksel	0
			goto	br
?out
		else
			decfsz	dest, 1
			goto	br
		endif
	endm

; Increment branch ne
; OK farok
incjne	macro	dest, br
		local	?over, ?out
		if ((dest) & dabmask) != 0
			banksel	dest
			incfsz	dest, 1
			goto	?over
			banksel	0
			goto	?out
?over
			banksel	0
			goto	br
?out
		else
			incfsz	dest, 1
			goto	br
		endif
	endm

; Jump
; OK
jmp	macro	dest
		goto	dest
	endm

; Far jump
farjmp	macro	dest
		movlw	HIGH dest
		movwf	pclath
		goto	dest
	endm

; OK
jeq	macro	dest
		jbs		status, z_flag, dest
	endm

; OK
jne	macro	dest
		jbc		status, z_flag, dest
	endm

; OK
jcs	macro	dest
		jbs		status, c_flag, dest
	endm

; OK
jcc	macro	dest
		jbc		status, c_flag, dest
	endm

; OK
jas	macro	dest
		jbs		status, dc_flag, dest
	endm

; OK
jac	macro	dest
		jbc		status, dc_flag, dest
	endm

; Comparison
; OK
jlo	macro	dest
		jbc		status, c_flag, dest
	endm

; OK
jls	macro	dest
		jbc		status, c_flag, dest
		jbs		status, z_flag, dest
	endm

; OK
jhs	macro	dest
		jbs		status, c_flag, dest
	endm

; OK
jhi	macro	dest
		local	?skipit
		jbs		status, z_flag, ?skipit
		jbs		status, c_flag, dest
?skipit
	endm

; Short table lookup: the table is in the same 256 byte page

short_lookup	macro table, dest, src
		movf	src, 0
		call	table
		movwf	dest
	endm

; Normal table lookup: the table is in the same 2KB page

lookup	macro	table, dest, src
		movlw	HIGH table
		movwf	pclath
		movf	src, 0
		call	table
		movwf	dest
	endm

; Far table lookup: the table is in a different page

far_lookup	macro	table, dest, src
		local	?here
		movlw	HIGH table
		movwf	pclath
		movf	src, 0
		call	table
		movwf	dest
		movlw	HIGH ?here	; Restore page in pclath
		movwf	pclath
?here
	endm

; Table header
; Max table size is 255 bytes
table	macro
		addwf	pcl, 1
	endm

; Table entry

val	macro	value
		retlw	value
	endm

	list
