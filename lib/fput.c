/*
 * put filename  addr 
 */

int cmd_fput(int argc,char **argv)
{
	unsigned long download_addr;
	struct stat statbuf;
	unsigned int size;
	unsigned int checksum,i;
	char *buf;
	int fd;
	int context_reg[dMIPS32_NUM_DEBUG_REGISTERS];
	if(stat(argv[1],&statbuf)!=0)
	{
		printf("error read file %s\n",argv[1]);
		exit(-1);
	}
printf("open file %s,size %d\n",argv[1],statbuf.st_size);
size = (statbuf.st_size + 3) & ~3;
buf=malloc(size);
if(!buf)printf("error allocate memory\n");
fd=open(argv[1],O_RDONLY);
read(fd,(buf),statbuf.st_size);
close(fd);
download_addr=strtoul(argv[2],0,0);
	context_save(context_reg);

    
    {
	    unsigned int code_inst_download[]=
tgt_compile(\
"li v0,0xff20fe04;\n" \
"move a3,a2;\n" \
"1:lw v1,0(v0);\n" \
"subu a2,v1;\n" \
"sw v1,0(a0);\n" \
"lw v1,0(a0);\n" \
"subu a3,v1;\n" \
"addiu a1,-4;\n" \
"bnez a1,1b;\n" \
"addiu a0,a0,4;\n" \
"nop\n" \
"nop\n" \
"nop\n" \
"li ra,0xff200200 ;\n" \
"jr ra ;\n" \
"nop;\n" \
);


	jtag_putmem(code_inst_download,help_addr,sizeof(code_inst_download));
	if(!(help_addr & 0x20000000)) cacheflush( help_addr, sizeof(code_inst_download));
    }

	for(i=0,checksum=0;i<((statbuf.st_size+3)&~3);i+=4)
	{
	  checksum += *(unsigned int *)(buf+i);
	}
	printf("checksum should be %x\n",checksum);

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
"lw a2,12(ra);\n" \
"jr v0;\n" \
"nop ;\n" \
);
	    miro_inst_2[2]=help_addr;
	    miro_inst_2[3]=download_addr;
	    miro_inst_2[4]=size;
	    miro_inst_2[5]=checksum;
    fMIPS32_ExecuteDebugModule(miro_inst_2,sizeof(miro_inst_2),1);
}

	wait_address(0xff20fe04,0,1);
	fMIPS32_Transfer(buf,size);
	wait_address(0xff200200,0,1);

	ui_datamem[0]=-1;
	ui_datamem[1]=-1;
	tgt_exec(\
"1:lui v0,0xff20;\n" \
"sw a2,0(v0);sw a3,4(v0);nop;\n" ,16); /*fetch instruct is before commit*/
	printf("transfer checksum %s\n",ui_datamem[0]?"error":"ok");
	printf("mem checksum %s\n",ui_datamem[1]?"error":"ok");

	context_restore(context_reg);


free(buf);

    return 0;
}

int cmd_fputflash(int argc,char **argv)
{
	unsigned long download_addr;
	struct stat statbuf;
	unsigned int size;
	char *buf;
	int fd;
	int context_reg[dMIPS32_NUM_DEBUG_REGISTERS];
	if(stat(argv[1],&statbuf)!=0)
	{
		printf("error read file %s\n",argv[1]);
		exit(-1);
	}
printf("open file %s,size %d\n",argv[1],statbuf.st_size);
size = (statbuf.st_size + 3) & ~3;
buf=malloc(size);
if(!buf)printf("error allocate memory\n");
fd=open(argv[1],O_RDONLY);
read(fd,(buf),statbuf.st_size);
close(fd);
download_addr=strtoul(argv[2],0,0);

    context_save(context_reg);

    
    {
	    unsigned int code_inst_download[]=
tgt_compile(\
"li t0,0xff20fe04;\n" \
"1:\n" \
"lw t1,(t0);\n" \
"li t2,1;\n" \
"2:\n" \
"li v0,0xbfc05554;\n" \
"li v1,0xaa\n" \
"sh v1,0x5556(v0);\n" \
"li v1,0x55;\n" \
"sh v1,(v0);\n" \
"li v1,0xa0\n" \
"sh v1,0x5556(v0);\n" \
"sh t1,(a0);\n" \
"3:\n" \
"lh v0,(a0);\n" \
"lh v1,(a0);\n" \
"bne v0,v1,3b;\n" \
"nop;\n" \
"srl t1,16;\n" \
"addiu a0,2;\n" \
"bnez t2,2b;\n" \
"addiu t2,-1;\n" \
"addiu a1,-4;\n" \
"bnez a1,1b;\n" \
"nop;\n" \
"nop;\n" \
"nop;\n" \
"li ra,0xff200200 ;\n" \
"jr ra ;\n" \
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
".space	12 \n" \
"1:\n" \
"lw v0,(ra);\n" \
"lw a0,4(ra);\n" \
"lw a1,8(ra);\n" \
"jr v0;\n" \
"nop ;\n" \
);
	    miro_inst_2[2]=help_addr;
	    miro_inst_2[3]=download_addr;
	    miro_inst_2[4]=size;
    fMIPS32_ExecuteDebugModule(miro_inst_2,0x8000,3);
}

	wait_address(0xff20fe04,0,1);
	fMIPS32_Transfer(buf,size);
	wait_address(0xff200200,0,1);

	context_restore(context_reg);

free(buf);


    return 0;
}
mycmd_init(fput,cmd_fput,"fput","fput filename address");
mycmd_init(fputflash,cmd_fputflash,"fputflash","fputflash filename address");
