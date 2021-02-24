.data
.text
#print: cycles 31 & 91 and final state
main:

	ori $v0, $zero, 0x1
	ori $v1, $zero, 0x2
	ori $a0, $zero, 0x3
	ori $a1, $zero, 0x4
	ori $a2, $zero, 0x5
	ori $a3, $zero, 0x6
	ori $t0, $zero, 0x0
	ori $t1, $zero, 0x1
	ori $t2, $zero, 0x2
	ori $t3, $zero, 0x3
	ori $t4, $zero, 0x4
	ori $t5, $zero, 0x5
	ori $t6, $zero, 0x6
	ori $t7, $zero, 0x7
	ori $s0, $zero, 0x0
	ori $s1, $zero, 0x1
	ori $s2, $zero, 0x2
	ori $s3, $zero, 0x3
	ori $s4, $zero, 0x4
	ori $s5, $zero, 0x5
	ori $s6, $zero, 0x6
	ori $s7, $zero, 0xa
	ori $t8, $zero, 0x8
	ori $t9, $zero, 0x9
	ori $k0, $zero, 0x0
	ori $k1, $zero, 0x0
	ori $ra, $zero, 0x0
	
loop_2:
	addi $t2, $t2, 1
	addi $t2, $t2, 1
	bne $t2, $s7, loop_2

loop_0:
	addi $t0, $t0, 1
	or $s7, $s7, $s7
	bne $t0, $s7, loop_0
	
	sw $s7, 0($gp)
loop_9:
	lw $s6, 0($gp)
	beq $s6, $t9, loop_x
	addi $t9, $t9, 1
	nor $t4, $t4, $t9
	bne $t9, $s6, loop_9
	
	add $s7, $s6, $t9
	sub $s0, $s7, $s1
	sw $s0, 4($gp)
	lw $s7, 4($gp)
	or $t4, $zero, $gp
	
loop_x:
	beq $t1, $s7, loop_y
	addi $t0, $t0, 0x1
	addi $t1, $t1, 0x1
	lw   $s0, 0($gp)
	add $t2, $t2, $s0
	addi $t3, $t3, 0x7
	addi $t4, $t4, 0x4
	addi $t5, $t5, 0x6
	addi $t6, $t6, 0x4
	addi $t7, $t7, 0x3
	sw $t3, 0($t4)
	beq $zero, $zero, loop_x
	
loop_y:	
	beq $s1, $s7, exit
	addi $s1, $s1, 0x1
	addi $s2, $s1, 0x2
	addi $s3, $s3, 0x8
	addi $s4, $s4, 0x7
	addi $s5, $s5, 0x6
	addi $s6, $s5, 0x5
	addi $t8, $t8, 0x4
	addi $t9, $t8, 0x3
	addi $t4, $t4, 0x4
	sw $s2, 0($t4)
	beq $zero, $zero, loop_y

exit:
	slt $t0, $t0, $t9	
	sll $t0, $t0, 10
	
end:	
	sll $zero, $zero, 0			#THIS IS MANDATORY