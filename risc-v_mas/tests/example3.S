.data
myvar:   .word 5, 5, 	15

.text
	addi a7, x1, 4 
	lw a0, 0(t0)
	ecall

	
	ret
