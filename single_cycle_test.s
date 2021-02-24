.data
.text
#print: cycle 67, cycle 80, final state
main:
	addiu $t0, $t0, 5
	addiu $t1, $t1, 0xfffB
	addi $t2, $t2, 0xfffB
	and $v0, $zero, $zero
	and $v1, $zero, $zero
	and $a0, $zero, $zero
	and $a1, $zero, $zero
	and $a2, $zero, $zero
	and $a3, $zero, $zero
	and $t0, $zero, $zero
	and $t1, $zero, $zero
	and $t2, $zero, $zero
	and $t3, $zero, $zero
	and $t4, $zero, $zero
	and $t5, $zero, $zero
	and $t6, $zero, $zero
	and $t7, $zero, $zero
	and $s0, $zero, $zero
	and $s1, $zero, $zero
	and $s2, $zero, $zero
	and $s3, $zero, $zero
	and $s4, $zero, $zero
	and $s5, $zero, $zero
	and $s6, $zero, $zero
	and $s7, $zero, $zero
	and $t8, $zero, $zero
	and $t9, $zero, $zero
	and $k0, $zero, $zero
	and $k1, $zero, $zero
	#and $fp, $zero, $zero
	and $ra, $zero, $zero
	
	ori $t0, $t0, 0x1
	ori $t1, $t1, 0x2
	ori $t2, $t2, 0x3
	add $t3, $t2, $t1
	addi $t4, $t4, 0x10
	addiu $t5, $t4, 0xFFF
	addu $t6, $t5, $t0
	andi $t7, $t4, 0x1
	sw $t4, 0($gp)
	lw $s0, 0($gp)
	and $t3, $zero, $zero
	and $t4, $zero, $zero
	ori $t3, $t3, 0x3
	ori $t4, $t4, 0x4
back:	
	addi $t4, $t4, -1
	nor $s1, $t4, $t5
	or $s2, $t4, $t5
	ori $s1, $s1, 0xFFF
	sll $s1, $s1, 16
	ori $s1, $s1, 0xFFF
	ori $s0, $s0, 0xFFF
	sll $s0, $s0, 16
	ori $s0, $s0, 0xFFB
	slt $s3, $s0, $s1
	sltu $s6, $s0, $s1
	slti $s4, $t4, 0x0
	sltiu $s5, $t5, 0xFFF
	sll $s7, $s2, 6
	srl $t0, $s7, 6
	sub $t1, $s1, $t0
	subu $t2, $s2, $t0
	beq $t3, $t4, back
	and $t3, $zero, $zero
	and $t4, $zero, $zero
	ori $t3, $t3, 0x4
	ori $t4, $t4, 0x4
	bne $t3, $t4, init
ret:
	j end
init:
	and $v0, $zero, $zero
	and $v1, $zero, $zero
	and $a0, $zero, $zero
	and $a1, $zero, $zero
	and $a2, $zero, $zero
	and $a3, $zero, $zero
	and $t0, $zero, $zero
	and $t1, $zero, $zero
	and $t2, $zero, $zero
	and $t3, $zero, $zero
	and $t4, $zero, $zero
	and $t5, $zero, $zero
	and $t6, $zero, $zero
	and $t7, $zero, $zero
	and $s0, $zero, $zero
	and $s1, $zero, $zero
	and $s2, $zero, $zero
	and $s3, $zero, $zero
	and $s4, $zero, $zero
	and $s5, $zero, $zero
	and $s6, $zero, $zero
	and $s7, $zero, $zero
	and $t8, $zero, $zero
	and $t9, $zero, $zero
	and $k0, $zero, $zero
	and $k1, $zero, $zero
	and $ra, $zero, $zero
	j ret
end:
	sll $zero, $zero, 0