unsigned int aui_readword_code[]=
	tgt_compile(\
	"1:mtc0	$15, $31 ;\n" \
	"lui	$15, 0xFF20;\n" \
	"sw	$2,  0x1fc($15);\n" \
	"lw	$2,  ($15);\n" \
	"lw	$2, 0($2);\n" \
	"sw	$2,  4($15);\n" \
	"lw	$2,  0x1fc($15);\n" \
	"mfc0	$15, $31 ;\n" \
	"b 1b;\n" \
	"nop;"); 
