#save word
addi $t1,$zero,10
#pseudocode code : need lui and ori
la   $t0, 268435456       # load address 0x10000000
sw   $t1, 0($t0)	#save word 
lw   $t2, 0($t0)	#load word
