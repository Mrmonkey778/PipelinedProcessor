.data
myvar:   .word 5, 5, 	15

.text
	lw t1, 0(t0)
	lw t2, 4(t0)
	beq t1, t2, _start
	lw t1, 0(t0)
	lw t2, 4(t0)
_start:
	and x8, t1, t2
	xori x9, t1, 31
	slli x10, t1, 1

	
	ret
