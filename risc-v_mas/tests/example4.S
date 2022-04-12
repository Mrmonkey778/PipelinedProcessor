.data
myvar:   .word 5, 5, 	15

.text
	srli x1, x2, 5
	sra x3, x4, x5
	
	lw t3, 8(t0)
_start:
	lw t1, 0(t0)
	lw t2, 4(t0)
	add x2, t1, t2
	sw x2, 0(t0)
	bne x2, t3, _start
	xor x3, t1, t2
	sub x4, t3, t1
	ret
