#include <stdio.h>
#include "elf.h"
#include <myconfig.h>

static int putelf_uncached = 1;
myconfig_init("putelf.uncached",putelf_uncached,CONFIG_I,"putelf uncached addr");

static struct ehdr 	exec;
static struct phdr	prgm;
static struct shdr	sect;
static struct sym	symb;


static int fast_bzero(unsigned int start_addr,unsigned int size)
{
unsigned int code_inst_download[]=
tgt_compile(
"1:sw zero,0(a0);\n" 
"addiu a1,-4;\n" 
"bnez a1,1b;\n" 
"addiu a0,a0,4;\n" 
"li v1,0xff200200;\n" 
"jr v1;\n"
"nop;\n" 
);

	jtag_putmem(code_inst_download,help_addr,sizeof(code_inst_download));
	if(!(help_addr & 0x20000000)) cacheflush( help_addr, sizeof(code_inst_download));
 {
unsigned int checksum,i;
unsigned int miro_inst_2[]=
tgt_compile(\
".set mips32;\n"
"mtc0 ra,c0_desave;\n"
"bal 1f;\n"
"lui v0,0xff20;\n"
".space 20;\n"
"1:sw a0,0x1fc(v0);\n"
"sw a1,0x1fc(v0);\n"
"lw a0,4(ra);\n" \
"lw a1,8(ra);\n" \
"lw ra,(ra);\n" \
"jr ra;\n" \
"nop ;\n" \
);

	miro_inst_2[3]=help_addr;
	miro_inst_2[4]=start_addr;
	miro_inst_2[5]=(size+3)&~3;

    fMIPS32_ExecuteDebugModule(miro_inst_2,0x200,3);
	wait_address(0xff200204,0,1);
tgt_exec(
"1:li v0,0xff200000;\n"
"lw a1,0x1fc(v0);\n"
"lw a0,0x1fc(v0);\n"
"mfc0 v0,c0_desave;\n" 
"b 1b;\n"
"nop;"
,0x8000);
}
return 0;
}

/*---------------------------------------------------------------------------
 * bootelf -- strip and reorganize the elf executable into format
 * 	      suitable for blasting into PROM.
 *
 * args:	infile	- the elf executable file
 *		outfile - name of file to write striped data to
 *
 * There is also a problem in that the constant initialized data follows the code
 * but the 'etext' symbol only reflects the end of the code.  Hence it is impossible
 * for the ROM code to know where the end of the constant initialized data ends so
 * it can copy the normal (non-constant) initialized data out to RAM.
 *
 *	[CODE SEGMENT][CONST INIT DATA]  [INIT DATA][UNINIT DATA]
 *	\_____________________________/  \______________________/
 *		READ ONLY		      WRITEABLE DATA
 *	   Program Header #1		     Program Header #0
 *
 * Since the bootelf converter knows the size of Program Headers, we insert this
 * value into a known variable (reotext) where the rom code can read it and make
 * use of the value. (Yes this is an aweful hack!)
 *---------------------------------------------------------------------------*/
int cmd_putelf(int argc,char **argv)
{
	FILE	*in, *out;
	int 	i;
	long	len;
	char 	tmpfile[100];

	if(argv[0][0] == 'p') argv[0][3] = 0;
	else argv[0][4] = 0;

	if( (in = fopen(argv[1], "rb")) == NULL ) {
		perror(argv[1]);
		return -1;
	}

	strcpy(tmpfile,"/tmp/ejtagbin_XXXXXX");
    if(mktemp(tmpfile) == NULL)
	{
		perror(tmpfile);
		return -1;
	}


	if( fread(&exec, sizeof(exec), 1, in) == 0 ) {
		fprintf(stderr, "fread fails\n");
		return -3;
	}

	/*---------------------------------------------------------------------------
	 * For the BOOTROM, the code is compiled to 0xbfc00000 while the data is usually
	 * compiled to address 0xa0000200 which is uncached DRAM or 0x00000200 which is
	 * the cached DRAM.  This causes a problem as the elf headers appear to be ordering
	 * in accending order (hence the data will appear before the code).  Thus we process
	 * the headers in reverse order.
	 *---------------------------------------------------------------------------*/
	for ( i =0 ; i < exec.phcount; i++ ) {
		char *buf;

		fseek(in,exec.phoff + i* exec.phsize,SEEK_SET);
	
		if( fread(&prgm, sizeof(prgm), 1, in) == 0 ) {
			fprintf(stderr, "fread fails\n");
			return -3;
		}

		if(prgm.type != PT_LOAD)
		 continue;

		if( (out = fopen(&tmpfile, "wb")) == NULL ) {
			perror(&tmpfile);
			fclose(in);
			return -2;
		}

		len = prgm.filesz;
		fseek(in, prgm.offset , SEEK_SET);
		buf = malloc(len);
		fread(buf,len,1,in);
		fwrite(buf,len,1,out);
		fclose(out);
		sprintf(buf,"%s %s 0x%x",argv[0],tmpfile,prgm.vaddr|(putelf_uncached?0xa0000000:0));
		printf("%s\n",buf);
		if(putelf_uncached) cacheflush(prgm.vaddr&0xdfffffff,len);
		do_cmd(buf);
		if(!putelf_uncached) cacheflush(prgm.vaddr,len);
		if(prgm.filesz < prgm.memsz)
		fast_bzero(prgm.vaddr+prgm.filesz,prgm.memsz-prgm.filesz);
		unlink(&tmpfile);
		free(buf);
	}

	fclose(in);

	ui_datamem[0] = exec.entry;

		tgt_exec(\
		  "1:mtc0    $2, $31;\n" \
		  "lui    $2,0xFF20;\n" \
		  "lw    $2, ($2);\n" \
		  "mtc0    $2, $24;\n" \
		  "mfc0    $2, $31;\n" \
		  "b 1b;\n" \
		  "nop;\n" \
		  ,0x8000);

	return 0;
}

mycmd_init(putelf,cmd_putelf,"putelf filename","put kernel");
mycmd_init(fputelf,cmd_putelf,"fputelf filename","put kernel");
mycmd_init(sputelf,cmd_putelf,"sputelf filename","put kernel");

