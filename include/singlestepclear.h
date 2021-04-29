unsigned int aui_singlestepclear_code[]=
tgt_compile(\
"1:\n" \
"	mtc0	$15, $31 ;\n" \
"	lui	$15, 0xFF20;\n" \
"	sw	$1,  0x1fc($15);\n" \
"	sw	$2,  0x1fc($15);\n" \
"	# Load R1 with the debug register (CP0 reg 23 select 0);\n" \
"	mfc0	$1,  $23;\n" \
"	# Load the AND mask into R2;\n" \
"	lui	$2, 0xFFFF;\n" \
"	ori	$2, $2, 0xFEFF;\n" \
"	# Clear the SSt bit in the debug register;\n" \
"	and	$1, $1, $2;\n" \
"	# Put the modified debug register back;\n" \
"	mtc0	$1, $23;\n" \
"	# Pop R1 and R2 off the debug stack;\n" \
"	lw	$2,  0x1fc($15);\n" \
"	lw	$1,  0x1fc($15);\n" \
"	mfc0	$15, $31 ;\n" \
"	b 1b;\n" \
"	nop;\n" \
);
