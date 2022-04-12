


.data
empty:  .space 32

myvar:   .word 5, -10, 	15

.align 4 #align to 16 bytes
astring: .asciiz "Hello, world!"

.text
_start:
	la t0, myvar
	lw	t1,    0(t0) # myvar[0]
lw      t2,	4(t0)		# myvar[1]
	add	x2, t1, t2 
	lw	t3, 8(t1)	
	beq	x2, t3, _start# 	better not be equal
	nop
  mv a0, t2
  jal x0, abs
	ret

abs:

  srai t0,a0,31
  	     add  a0, a0, t0
  xor a0, a0, t0
  
  ret

  

