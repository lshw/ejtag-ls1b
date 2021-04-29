/*
 * put filename  addr 
 */
#include <unistd.h>
/*
 * It seems that fast  put by usb has bug.
 * when progam run on ram at 0xa0020000(set helpaddr 0xa0200000),
 * the first data is missing,maybe fetched by cpu random fetch,or some other reason.
 * when progam run on ram at 0xa0300000,then the first data is ok.
 * Now we add a 16 byte header before data,0~11 is 0xff,12~15 is 0.
 * program on run check 0,then begin to fetch real data.
 * the other bug:usb always send one more data.
 */

int cmd_put(int argc,char **argv)
{
	unsigned long download_addr;
	struct stat statbuf;
	char *buf;
	int fd;
	int context_reg[dMIPS32_NUM_DEBUG_REGISTERS];
	if(stat(argv[1],&statbuf)!=0)
	{
		printf("error read file %s\n",argv[1]);
		exit(-1);
	}
printf("open file %s,size %d\n",argv[1],statbuf.st_size);
buf=malloc(statbuf.st_size+4);
if(!buf)printf("error allocate memory\n");
fd=open(argv[1],O_RDONLY);
read(fd,buf,statbuf.st_size);
close(fd);
download_addr=strtoul(argv[2],0,0);


	context_save(context_reg);
    
    {
	    unsigned int code_inst_download[]=
tgt_compile(\
"move a3,a2;\n" \
"1:lw v1,0(v0);\n" \
"subu a2,v1;\n" \
"sw v1,0(a0);\n" \
"lw v1,0(a0);\n" \
"subu a3,v1;\n" \
"addiu a1,-4;\n" \
"addiu a0,a0,4;\n" \
"bnez a1,1b;\n" \
"nop ;\n" \
"li ra,0xff200200 ;\n" \
"jr ra ;\n" \
"nop;\n" \
);


	jtag_putmem(code_inst_download,help_addr,sizeof(code_inst_download));
	if(!(help_addr & 0x20000000)) cacheflush( help_addr, sizeof(code_inst_download));
    }


 {
unsigned int checksum,i;
unsigned int miro_inst_2[]=
tgt_compile(\
"bal 1f;\n" \
"nop;\n" \
"param:\n" \
".space	16 \n" \
"1:\n" \
"lw v1,(ra);\n" \
"lw a0,4(ra);\n" \
"lw a1,8(ra);\n" \
"lw a2,12(ra);\n" \
"li v0,0xff20fe04;\n" \
"jr v1;\n" \
"nop ;\n" \
);

	for(i=0,checksum=0;i<((statbuf.st_size+3)&~3);i+=4)
	{
	  checksum += *(unsigned int *)(buf+i);
	}
	printf("checksum should be %x\n",checksum);

	    miro_inst_2[2]=help_addr;
	    miro_inst_2[3]=download_addr;
	    miro_inst_2[4]=(statbuf.st_size+3)&~3;
	    miro_inst_2[5]=checksum;
    fMIPS32_ExecuteDebugModule(miro_inst_2,0x200,3);
}

		ui_datamem[0]=-1;
		ui_datamem[1]=-1;
		wait_address(0xff20fe04,0,1);

	fMIPS32_USB_DownloadModule(buf,((statbuf.st_size+3)&~3));

	tgt_exec(\
"1:lui v0,0xff20;\n" \
"sw a2,0(v0);sw a3,4(v0);nop;\n" ,16); /*fetch instruct is before commit*/

	printf("transfer checksum %s\n",ui_datamem[0]?"error":"ok");
	printf("mem checksum %s\n",ui_datamem[1]?"error":"ok");

	context_restore(context_reg);

free(buf);


    return 0;
}

mycmd_init(put,cmd_put,"put","put filename address,\tenv: put_speed");

