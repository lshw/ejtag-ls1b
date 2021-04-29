#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mips32.h"
#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <cmdparser.h>
#include <stdio.h>

/*
 * 0xff200000-0xff2001f0: ui_datamem
 * 0xff2001fc-0xff2001ff: stack
 * 0xff2001f8-0xff2001fb: ui_fifomem(uififo_raddr is read index,uififo_waddr is write index)
 * 0xff2001f4: output msg,display byte write as char val
 * 0xff2001f5: output msg,use byte write as index of ui_datamem,ui_datamem[byte] is format,ui_datamem[byte+1]... as params.

functions to access target memory:

unsigned char jtag_inb(unsigned long addr);
unsigned short jtag_inw(unsigned long addr);
unsigned int jtag_inl(unsigned long addr);
void jtag_outb(unsigned long addr,unsigned char val);
void jtag_outw(unsigned long addr,unsigned short val);
void jtag_outl(unsigned long addr,unsigned int val);
void jtag_getmem(char *dst,unsigned long addr,unsigned int len);
void jtag_putmem(char *src,unsigned long addr,unsigned int len);

functions to execute code on target:
tgt_exec(char *asmcode,int len);
 exec end when exceed len,or backto begin addr.

 */

/*
 *here is a example write data argv[2] to address argv[1] 
 */
int cmd_example(int argc,char **argv)
{

	uififo_raddr=0;
	uififo_waddr=0;
	ui_datamem[0] = strtoul(argv[1],0,0);
	ui_datamem[1] = strtoul(argv[2],0,0);

tgt_exec(\
"1:mtc0    v0, $31;\n" \
"li    v0,0xff200000;\n" \
"sw    v1,0x1fc(v0);\n" \
"sw    t1,0x1fc(v0);\n" \
"sw    t2,0x1fc(v0);\n" \
"lw    t1,(v0);\n" \
"lw    t2,4(v0);\n" \
"sw    t2,(t1);\n" \
"lw      t2, 0x1fc(v0);\n"
"lw      t1, 0x1fc(v0);\n"
"lw      v1, 0x1fc(v0);\n"
"mfc0    v0, $31;\n" \
"b 1b;\n"
"nop\n"
,0x8000);

return 0;
}

mycmd_init(example,cmd_example,"","this a example");
