/*
 * put filename  addr 
 */
#include <unistd.h>

static int fast_get_once(unsigned int download_addr,char *buf,unsigned int size)
{
unsigned int checksum,i;
unsigned int miro_inst_2[]=
tgt_compile(\
"bal 1f;\n" \
"nop;\n" \
"param:\n" \
".space	12 \n" \
"1:\n" \
"lw v1,(ra);\n" \
"lw a0,4(ra);\n" \
"lw a1,8(ra);\n" \
"li v0,0xff20fe04;\n" \
"jr v1;\n" \
"nop ;\n" \
);

	    miro_inst_2[2]=help_addr;
	    miro_inst_2[3]=download_addr;
	    miro_inst_2[4]=(size+3)&~3;
    fMIPS32_ExecuteDebugModule(miro_inst_2,sizeof(miro_inst_2),3);

		ui_datamem[0]=-1;
		ui_datamem[1]=-1;
		wait_address(0xff20fe04,1,1);

	fJTAG_UPLOAD(buf,(size+3)&~3);
		wait_address(0xff200204,0,1);
ui_datamem[0]=-1;

	tgt_exec(\
"1:lui v0,0xff20;\n" \
"sw a2,0(v0);nop;\n" ,12); /*fetch instruct is before commit*/

	checksum=ui_datamem[0];

	printf("downloadaddr 0x%x,checksum shoule be %x\n",download_addr,checksum);
	for(i=0;i<((size+3)&~3);i+=4)
	{
	  checksum -= *(unsigned int *)(buf+i);
	}
	printf("transfer checksum %s\n",checksum?"error":"ok");
	return checksum;
}

/*
 * It seems that fast  put by usb has bug.
 * when progam run on ram at 0xa0020000(set helpaddr 0xa0200000),
 * the first data is missing,maybe fetched by cpu random fetch,or some other reason.
 * when progam run on ram at 0xa0300000,then the first data is ok.
 * Now we add a 16 byte header before data,0~11 is 0xff,12~15 is 0.
 * program on run check 0,then begin to fetch real data.
 * the other bug:usb always send one more data.
 */

int cmd_get(int argc,char **argv)
{
	unsigned long download_addr;
	char *buf;
	int fd;
	int context_reg[dMIPS32_NUM_DEBUG_REGISTERS];
	unsigned int size,realsize,oncesize;
	unsigned int checksum;
	int i;

if(argc<4)return -1;
size=strtoul(argv[3],0,0);


buf=malloc(size+16);
if(!buf)printf("error allocate memory\n");
download_addr=strtoul(argv[2],0,0);


	context_save(context_reg);

{
	    unsigned int code_inst_download[]=
tgt_compile(\
"move a2,zero;\n" \
"sw zero,0(v0);\n" \
"sw zero,0(v0);\n" \
"1:lw v1,0(a0);\n" \
"sw v1,0(v0);\n" \
"addu a2,v1;\n" \
"addiu a1,-4;\n" \
"bnez a1,1b;\n" \
"addiu a0,a0,4;\n" \
"li v0,100;\n" \
"2:addiu v0,-1;\n" \
"bnez v0,2b;\n" \
"nop;\n" \
"li ra,0xff200200 ;\n" \
"jr ra ;\n" \
"nop;\n" \
);


	jtag_putmem(code_inst_download,help_addr,sizeof(code_inst_download));
	if(!(help_addr & 0x20000000)) cacheflush( help_addr, sizeof(code_inst_download));

}


realsize=((size+3)&~3);
#define GETLEN 0x100000
for(i=0;i<realsize;)
{
oncesize=(i+GETLEN)<realsize?GETLEN:realsize-i;
	if(!fast_get_once(download_addr+i,buf+i,oncesize))
	i+=oncesize;
}

fd=open(argv[1],O_WRONLY|O_CREAT|O_TRUNC);
if(fd){
write(fd,buf,size);
close(fd);
}
else
{
printf("open file %s for write error\n",argv[1]); 
}

free(buf);
	context_restore(context_reg);


    return 0;
}

mycmd_init(get,cmd_get,"get","get filename address size");

