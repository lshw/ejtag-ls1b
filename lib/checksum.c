int cmd_checksum(int argc,char **argv)
{
	unsigned int addr,size;
	int context_reg[dMIPS32_NUM_DEBUG_REGISTERS];
	addr=strtoul(argv[1],0,0);
	size=strtoul(argv[2],0,0);
	size = (size+3)&~3;
	context_save(context_reg);
    
    {
	    unsigned int code_inst_download[]=
tgt_compile(\
"move a2,zero;\n" \
"1:lw v1,0(a0);\n" \
"addu a2,v1;\n" \
"addiu a1,-4;\n" \
"bnez a1,1b;\n" \
"addiu a0,a0,4;\n" \
"nop\n" \
"li v1,0xff200000;\n"
"sw a2,(v1);\n" \
"nop\n" \
"1:li v1,0xff200200 ;\n" \
"jr v1 ;\n" \
"nop;\n" \
);


	jtag_putmem(code_inst_download,help_addr,sizeof(code_inst_download));
	if(!(help_addr & 0x20000000)) cacheflush( help_addr, sizeof(code_inst_download));
    }

 {
unsigned int miro_inst_2[]=
tgt_compile(\
"bal 1f;\n" \
"nop;\n" \
"param:\n" \
".space	16 \n" \
"1:\n" \
"lw v0,(ra);\n" \
"lw a0,4(ra);\n" \
"lw a1,8(ra);\n" \
"jr v0;\n" \
"nop ;\n" \
"nop ;\n" \
"nop ;\n" \
);
	    miro_inst_2[2]=help_addr;
	    miro_inst_2[3]=addr;
	    miro_inst_2[4]=size;
    fMIPS32_ExecuteDebugModule(miro_inst_2,sizeof(miro_inst_2),1);
}

	ui_datamem[0]=-1;
	ui_datamem[1]=-1;
	wait_address(0xff200204,0,1);
	printf("checksum 0x%08x\n",ui_datamem[0]);

	context_restore(context_reg);

	return 0;
}

mycmd_init(checksum,cmd_checksum,"checksum","checksum address size");
