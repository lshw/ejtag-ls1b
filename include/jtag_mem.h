unsigned char jtag_inb(unsigned long addr)
{
      ui_datamem[0]=addr;
	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "sw    $3, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lb    $3, ($2);\n" \
	  "sb    $3, 4($15);\n" \
	  "lw    $3, 0x1fc($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
	  return (unsigned char)(ui_datamem[1]&0xff);
}

unsigned short jtag_inw(unsigned long addr)
{
      ui_datamem[0]=addr;
	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "sw    $3, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lh    $3, ($2);\n" \
	  "sh    $3, 4($15);\n" \
	  "lw    $3, 0x1fc($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
	  return (unsigned short)(ui_datamem[1]&0xffff);
}

unsigned int jtag_inl(unsigned long addr)
{
      ui_datamem[0]=addr;
	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "sw    $3, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lw    $3, ($2);\n" \
	  "sw    $3, 4($15);\n" \
	  "lw    $3, 0x1fc($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
	  return (unsigned int)ui_datamem[1];
}

void jtag_outb(unsigned long addr,unsigned char val)
{
      ui_datamem[0]=addr;
      ui_datamem[1]=val;

	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "sw    $3, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lw    $3, 0x4($15);\n" \
	  "sb    $3, ($2);\n" \
	  "lw    $3, 0x1fc($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
}

void jtag_outw(unsigned long addr,unsigned short val)
{
      ui_datamem[0]=addr;
      ui_datamem[1]=val;
	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "sw    $3, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lw    $3, 0x4($15);\n" \
	  "sh    $3, ($2);\n" \
	  "lw    $3, 0x1fc($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
}

void jtag_outl(unsigned long addr,unsigned int val)
{
      ui_datamem[0]=addr;
      ui_datamem[1]=val;

	  tgt_exec(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "sw    $3, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lw    $3, 0x4($15);\n" \
	  "sw    $3, ($2);\n" \
	  "lw    $3, 0x1fc($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);

}

#define min(a,b) (a<b?a:b)
void jtag_getmem(char *dst,unsigned long addr,unsigned int len)
{
	  int once;
     while(len)
     {
	  uififo_raddr=0;
	  uififo_waddr=0;
	  once=min(len,ui_fifomem_size);
	  if(once<4)once=4;
      ui_datamem[0]=addr&~3;
      ui_datamem[1]=(addr+once-4)&~3;
      once=ui_datamem[1]-ui_datamem[0]+4-(addr&3);

	  tgt_exec(\
	  ".set noat;\n" \
	  ".set noreorder;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $1, 0x1FC($15);\n" \
	  "sw    $2, 0x1FC($15);\n" \
	  "sw    $3, 0x1FC($15);\n" \
	  "lw    $1, ($15);\n" \
	  "lw    $2, 0x4($15);\n" \
	  "2:\n" \
	  "lw    $3, ($1);\n" \
	  "sw    $3, 0x1F8($15);\n" \
	  "bne   $1,$2,2b;\n" \
	  "addu  $1, 4;\n" \
	  "lw    $3, ($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lw    $1, ($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
	  once=min(once,len);
	  memcpy(dst,(char *)ui_fifomem+(addr&3),once);
	  len-=once;
	  dst+=once;
	  addr+=once;
   }

}

void jtag_putmem(char *src,unsigned long addr,unsigned int len)
{
	  int once;
     while(len)
     {
	  uififo_raddr=0;
	  uififo_waddr=0;
	  once=min(len,ui_fifomem_size);
	  if(once<4)once=4;
      ui_datamem[0]=addr&~3;
      ui_datamem[1]=(addr+once-4)&~3;
      once=ui_datamem[1]-ui_datamem[0]+4-(addr&3);
      printf("addr=%x,len=%x\n",addr,len);

	  memcpy((char *)ui_fifomem+(addr&3),src,once);

	  tgt_exec(\
	  ".set noat;\n" \
	  ".set noreorder;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "li    $15,0xFF200000;\n" \
	  "sw    $1, 0x1FC($15);\n" \
	  "sw    $2, 0x1FC($15);\n" \
	  "sw    $3, 0x1FC($15);\n" \
	  "lw    $1, ($15);\n" \
	  "lw    $2, 0x4($15);\n" \
	  "2:\n" \
	  "lw    $3, 0x1F8($15);\n" \
	  "sw    $3, ($1);\n" \
	  "bne   $1,$2,2b;\n" \
	  "addu  $1, 4;\n" \
	  "lw    $3, ($15);\n" \
	  "lw    $2, ($15);\n" \
	  "lw    $1, ($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n" \
	  ,0x80000);
	  once=min(once,len);
	  len-=once;
	  src+=once;
	  addr+=once;
	}
}
