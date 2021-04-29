#include <flash.h>

static int ls1f_spi_inited;

void ls1f_spi_init(void *unused)
{

if(ls1f_spi_inited) return;
ls1f_spi_inited = 1;

tgt_exec(
"#define SPI_BASE 0xbfe80000 \n"
"#define SPCR      0x0 \n"
"#define SPSR      0x1 \n"
"#define TXFIFO    0x2 \n"
"#define SPER      0x3 \n"
"#define PARAM     0x4 \n"
"#define SOFTCS    0x5 \n"
"#define PARAM2    0x6 \n"

".macro set_spi add,val\n"
"li v1,\val;\n"
"sb v1,\add(v0);\n"
".endm\n"

"1:mtc0    v0, $31;\n" 
"li    v0,0xff200000;\n" 
"sw    v1,0x1fc(v0);\n" 
"li	   v0,SPI_BASE;\n" /*spi base*/
"set_spi SPSR,0xc0;\n"
"set_spi PARAM,0x40;\n"
"set_spi SPER,0x5;\n"
"set_spi PARAM2,1;\n"
"set_spi SPCR,0x50;\n"
"li    v0,0xff200000;\n" 
"lw    v1,0x1fc(v0);\n" 
"mfc0    v0, $31;\n" 
"b 1b;\n" 
"nop;" 
,0x8000);
}

int cmd_spi_init(int argc,char **argv)
{
	ls1f_spi_init(0);
}


static int __spiromsyscall1(int type,unsigned long long addr,union commondata *mydata)
{
uififo_waddr=0;
ui_datamem[0] = addr;
ui_datamem[1] = type;

tgt_exec(\
"1:mtc0    s0, $31;\n" 
"li s0,0xff200000;\n"
"sw v0,0x1fc(s0);\n"
"sw v1,0x1fc(s0);\n"
"sw ra,0x1fc(s0);\n"
"sw t0,0x1fc(s0);\n"
"li v0,0xbfe80000;\n" 
"bal 102f;\n" /*wait sr */
"nop;\n" 
"li v1,1;\n"
"sb v1,5(v0);\n" /*enable and low cs*/
"li v1,3;\n"  /* read command */
"bal 101f;\n" 
"nop;\n" 
"lw t0,0(s0);\n" /*addr*/
"srl v1,t0,16;\n" /*addr*/
"bal 101f;\n" 
"nop;\n" 
"srl v1,t0,8;\n" 
"bal 101f;\n" 
"nop;\n" 
"move v1,t0;\n" 
"bal 101f;\n" 
"nop;\n" 
"lw t0,4(s0);\n" /*count*/
"2:bal 101f;\n" /*read 1 data*/ 
"nop;\n" 
"sb v1,0x1f8(s0);\n"
"addiu t0,-1;\n"
"bnez t0,2b;\n" 
"nop;\n" \
"li v1,0x11;\n"
"sb v1,5(v0);\n" /*enable and high cs*/
"li s0,0xff200000;\n"
"lw t0,0x1fc(s0);\n"
"lw ra,0x1fc(s0);\n"
"lw v1,0x1fc(s0);\n"
"lw v0,0x1fc(s0);\n"
"mfc0    s0, $31;\n" 
"b 1b;\n" 
"nop;\n" 
"101:sb v1,2(v0);\n" 
"1:lb v1,1(v0);\n" 
"andi v1,1;\n" 
"bnez v1,1b;\n"
"nop;\n"
"lb v1,2(v0);\n" 
"jr ra;\n" 
"nop;" 
"102:move t0,ra;\n" 
"li v1,1;\n"
"sb v1,5(v0);\n" /*enable and low cs*/
"1:li v1,5;\n"  /*wait read sr*/
"bal 101b;\n" 
"nop;\n" 
"andi v1,1;\n"
"bnez v1,1b;\n"
"nop;\n"
"li v1,0x11;\n"
"sb v1,5(v0);\n" /*enable and high cs*/
"jr t0;\n"
"nop;\n",
0x8000
);

memcpy(mydata->c,ui_fifomem,type);
return 0;
}

static int __spiromsyscall2(int type,unsigned long long addr,union commondata *mydata)
{
return -1;
}

static int cmd_spiroms(int argc,char **argv)
{
syscall1=__spiromsyscall1;
syscall2=__spiromsyscall2;
syscall_addrwidth=1;
if(argc>1)cp0s_sel=strtoul(argv[1],0,0);
else cp0s_sel=0;
ls1f_spi_init(0);
return 0;	
}


static unsigned int code_erase_st25p64[]=
tgt_compile(
"1:mtc0    s0, $31;\n" 
"li    s0,0xff200000;\n" 
"sw    v0,0x1fc(s0);\n" 
"sw    v1,0x1fc(s0);\n" 
"sw    ra,0x1fc(s0);\n" 
"sw    t0,0x1fc(s0);\n" 
"li s0,0xbfe80000;\n" 
"bal 102f;\n" /*wait sr */
"nop;\n" 
"li v0,6;\n"  /* write enable */
"bal 101f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"li v0,0xc7;\n" /*bulk erase */
"bal 101f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"bal 102f;\n" /*wait sr*/
"nop;\n"
"li    s0,0xff200000;\n" 
"lw    t0,0x1fc(s0);\n" 
"lw    ra,0x1fc(s0);\n" 
"lw    v1,0x1fc(s0);\n" 
"lw    v0,0x1fc(s0);\n" 
"mfc0    s0, $31;\n" 
"b 1b;\n" 
"nop;\n" 
"101:li v1,1;\n"
"sb v1,5(s0);\n" /*enable and low cs*/
"103:sb v0,2(s0);\n" 
"1:lb v0,1(s0);\n" 
"andi v0,1;\n" 
"bnez v0,1b;\n"
"nop;\n"
"lb v0,2(s0);\n" 
"jr ra;\n" 
"nop;" 
"102:move t0,ra;\n" 
"li v1,1;\n"
"sb v1,5(s0);\n" /*enable and low cs*/
"1:li v0,5;\n"  /*wait read sr*/
"bal 103b;\n" 
"nop;\n" 
"andi v0,1;\n"
"bnez v0,1b;\n"
"nop;\n"
"li v0,0x11;\n" /*high cs*/
"sb v0,5(s0);\n"
"jr t0;\n"
"nop;\n"
);




static unsigned int code_program_st25p64[]=
tgt_compile(\
"li v0,0xbfe80000;\n" \

//"li a0, 0xa1000000;\n"	\
//"li a1, 0x80000;\n"	\
//"li a2, 0x780000;\n"	\


"1:li v1,0x11;\n"
"sb v1,5(v0);\n" /*enable and low cs*/
"bal 102f;\n" /*wait sr */
"nop;\n" 
"li v1,6;\n"  /* write enable */
"bal 101f;\n" 
"nop;\n" 
"li t1,0x11;\n" /*high cs*/
"sb t1,5(v0);\n"
"bal 102f;\n" /*wait sr*/
"nop;"
"li v1,2;\n"  /*write sector*/
"bal 101f;\n" 
"nop;\n" 
"srl v1,a1,16;\n" /*addr*/
"bal 103f;\n" 
"nop;\n" 
"srl v1,a1,8;\n" 
"bal 103f;\n" 
"nop;\n" 
"move v1,a1;\n" 
"bal 103f;\n" 
"nop;\n" 
"2:lb v1,(a0)\n" /*write 1 data*/ 
"bal 103f;\n" 
"nop;\n" 
"addiu a0,1;\n" 
"addiu a1,1;\n" 
"addiu a2,-1;\n"
"beqz a2,3f;\n" 
"nop;\n" \
"andi v1,a1,0xff;\n" 
"beqz v1,1b;\n" 
"nop;\n"
"b 2b;\n" 
"nop;\n" 
"3:\n" 
"li v1,0x11;\n"
"sb v1,5(v0);\n" /*enable and low cs*/
"li ra,0xff200200 ;\n" 
"jr ra ;\n" 
"nop;\n" 
"101:li t1,1;\n"
"sb t1,5(v0);\n" /*enable and low cs*/
"103:sb v1,2(v0);\n" 
"1:lb v1,1(v0);\n" 
"andi v1,1;\n" 
"bnez v1,1b;\n"
"nop;\n"
"lb v1,2(v0);\n" 
"jr ra;\n" 
"nop;" 
"102:move t0,ra;\n" 
"1:li v1,5;\n"  /*wait read sr*/
"bal 101b;\n" 
"nop;\n" 
"andi v1,1;\n"
"bnez v1,1b;\n" /*can continue read sr,write to gen clock*/
"nop;\n"
"li t1,0x11;\n" /*high cs*/
"sb t1,5(v0);\n"
"jr t0;\n"
"nop;\n"
);

static unsigned int code_erase_area_st25p64[]=
tgt_compile(
"1:mtc0    s0, $31;\n" 
"li    s0,0xff200000;\n" 
"sw    v0,0x1fc(s0);\n" 
"sw    v1,0x1fc(s0);\n" 
"sw    a0,0x1fc(s0);\n" 
"sw    a1,0x1fc(s0);\n" 
"sw    a2,0x1fc(s0);\n" 
"sw    ra,0x1fc(s0);\n" 
"sw    t0,0x1fc(s0);\n" 
"lw a0,0(s0);\n"
"lw a1,4(s0);\n"
"lw a2,8(s0);\n"

//"li a0,0x80000;\n"
//"li a1,0x800000;\n"
//"li a2,0x10000;\n"

"li s0,0xbfe80000;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"bal 102f;\n" /*wait sr */
"nop;\n" 
"2:li v0,6;\n"  /* write enable */
"bal 101f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"li v0,0xd8;\n" /*bulk erase */
"bal 101f;\n" 
"nop;\n" 
"srl v0,a0,16;\n" /*addr*/
"bal 103f;\n" 
"nop;\n" 
"srl v0,a0,8;\n" 
"bal 103f;\n" 
"nop;\n" 
"move v0,a0;\n" 
"bal 103f;\n" 
"nop;\n" 
"li v0,0x11;\n" /*high cs*/
"sb v0,5(s0);\n"
"bal 102f;\n" /*wait sr*/
"nop;\n"
"addu a0,a2;\n"
"slt v0,a1,a0;\n"
"beqz v0,2b;\n"
"nop;\n"
"li    s0,0xff200000;\n" 
"lw    t0,0x1fc(s0);\n" 
"lw    ra,0x1fc(s0);\n" 
"lw    a2,0x1fc(s0);\n" 
"lw    a1,0x1fc(s0);\n" 
"lw    a0,0x1fc(s0);\n" 
"lw    v1,0x1fc(s0);\n" 
"lw    v0,0x1fc(s0);\n" 
"mfc0    s0, $31;\n" 
"b 1b;\n" 
"nop;\n" 
"101:li v1,1;\n"
"sb v1,5(s0);\n" /*enable and low cs*/
"103:sb v0,2(s0);\n" 
"1:lb v0,1(s0);\n" 
"andi v0,1;\n" 
"bnez v0,1b;\n"
"nop;\n"
"lb v0,2(s0);\n" 
"jr ra;\n" 
"nop;" 
"102:move t0,ra;\n" 
"li v1,1;\n"
"sb v1,5(s0);\n" /*enable and low cs*/
"1:li v0,5;\n"  /*wait read sr*/
"bal 103b;\n" 
"nop;\n" 
"andi v0,1;\n"
"bnez v0,1b;\n"
"nop;\n"
"li v0,0x11;\n" /*high cs*/
"sb v0,5(s0);\n"
"jr t0;\n"
"nop;\n"
);


static unsigned int code_program_st25vf080[]=
tgt_compile(\
"li s0,0xbfe80000;\n" \



#if 0
/*******************************************/		//lxy
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"bal 102f;\n" /*wait sr */
"nop;\n" 
"li v0,6;\n"  /* write enable */
"bal 101f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"li v0,1;\n"  /* write status*/
"bal 101f;\n" 
"nop;\n" 
"li v0,0;\n"  /* write 0*/	//double '0'  to status register for winbond spi flash
"bal 103f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"bal 102f;\n" /*wait sr */
"nop;\n" 
/*******************************************/
#endif



"1:li v0,0x11;\n"
"sb v0,5(s0);\n" /*high cs*/
"bal 102f;\n" /*wait sr */
"nop;\n" 
"li v0,6;\n"  /* write enable */
"bal 101f;\n" 
"nop;\n" 
"li t1,0x11;\n" /*high cs*/
"sb t1,5(s0);\n"
"bal 102f;\n" /*wait sr*/
"nop;"
"li v0,2;\n"  /*write sector*/
"bal 101f;\n" 
"nop;\n" 
"srl v0,a1,16;\n" /*addr*/
"bal 103f;\n" 
"nop;\n" 
"srl v0,a1,8;\n" 
"bal 103f;\n" 
"nop;\n" 
"move v0,a1;\n" 
"bal 103f;\n" 
"nop;\n" 
"2:lb v0,(a0)\n" /*write 1 data*/ 
"bal 103f;\n" 
"nop;\n" 
"addiu a0,1;\n" 
"addiu a1,1;\n" 
"addiu a2,-1;\n"
"beqz a2,3f;\n" 
"nop;\n" 
"b 1b;\n" 
"nop;\n" 
"3:\n" 
"li v0,0x11;\n"
"sb v0,5(s0);\n" /*high cs*/
"li ra,0xff200200 ;\n" 
"jr ra ;\n" 
"nop;\n" 
"101:li t1,1;\n"
"sb t1,5(s0);\n" /*enable and low cs*/
"103:sb v0,2(s0);\n" 
"1:lb v0,1(s0);\n" 
"andi v0,1;\n" 
"bnez v0,1b;\n"
"nop;\n"
"lb v0,2(s0);\n" 
"jr ra;\n" 
"nop;" 
"102:move t0,ra;\n" 
"1:li v0,5;\n"  /*wait read sr*/
"bal 101b;\n" 
"nop;\n" 
"andi v0,1;\n"
"bnez v0,1b;\n" /*can continue read sr,write to gen clock*/
"nop;\n"
"li t1,0x11;\n" /*high cs*/
"sb t1,5(s0);\n"
"jr t0;\n"
"nop;\n"
);




static unsigned int code_erase_area_win25x64[]=
tgt_compile(
"1:mtc0    s0, $31;\n" 
"li    s0,0xff200000;\n" 
"sw    v0,0x1fc(s0);\n" 
"sw    v1,0x1fc(s0);\n" 
"sw    a0,0x1fc(s0);\n" 
"sw    a1,0x1fc(s0);\n" 
"sw    a2,0x1fc(s0);\n" 
"sw    ra,0x1fc(s0);\n" 
"sw    t0,0x1fc(s0);\n" 
"lw a0,0(s0);\n"
"lw a1,4(s0);\n"
"lw a2,8(s0);\n"
"li s0,0xbfe80000;\n" 

#if 1
/*******************************************/		//lxy
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"bal 102f;\n" /*wait sr */
"nop;\n" 
"li v0,6;\n"  /* write enable */
"bal 101f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"li v0,1;\n"  /* write status*/
"bal 101f;\n" 
"nop;\n" 
"li v0,0;\n"  /* write 0*/	//double '0'  to status register for winbond spi flash
"bal 103f;\n" 
"nop;\n" 
"li v0,0;\n"  /* write 0*/
"bal 103f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"bal 102f;\n" /*wait sr */
"nop;\n" 
/*******************************************/
#endif

"2:li v0,6;\n"  /* write enable */
"bal 101f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"li v0,0xd8;\n" /*bulk erase */
"bal 101f;\n" 
"nop;\n" 
"srl v0,a0,16;\n" /*addr*/
"bal 103f;\n" 
"nop;\n" 
"srl v0,a0,8;\n" 
"bal 103f;\n" 
"nop;\n" 
"move v0,a0;\n" 
"bal 103f;\n" 
"nop;\n" 
"li v0,0x11;\n" /*high cs*/
"sb v0,5(s0);\n"
"bal 102f;\n" /*wait sr*/
"nop;\n"
"addu a0,a2;\n"
"slt v0,a1,a0;\n"
"beqz v0,2b;\n"
"nop;\n"
"li    s0,0xff200000;\n" 
"lw    t0,0x1fc(s0);\n" 
"lw    ra,0x1fc(s0);\n" 
"lw    a2,0x1fc(s0);\n" 
"lw    a1,0x1fc(s0);\n" 
"lw    a0,0x1fc(s0);\n" 
"lw    v1,0x1fc(s0);\n" 
"lw    v0,0x1fc(s0);\n" 
"mfc0    s0, $31;\n" 
"b 1b;\n" 
"nop;\n" 
"101:li v1,1;\n"
"sb v1,5(s0);\n" /*enable and low cs*/
"103:sb v0,2(s0);\n" 
"1:lb v0,1(s0);\n" 
"andi v0,1;\n" 
"bnez v0,1b;\n"
"nop;\n"
"lb v0,2(s0);\n" 
"jr ra;\n" 
"nop;" 
"102:move t0,ra;\n" 
"li v1,1;\n"
"sb v1,5(s0);\n" /*enable and low cs*/
"1:li v0,5;\n"  /*wait read sr*/
"bal 103b;\n" 
"nop;\n" 
"andi v0,1;\n"
"bnez v0,1b;\n"
"nop;\n"
"li v0,0x11;\n" /*high cs*/
"sb v0,5(s0);\n"
"jr t0;\n"
"nop;\n"
);



static unsigned int code_program_win25x64[]=
tgt_compile(\
"li s0,0xbfe80000;\n" \
"1:li v0,0x11;\n"
"sb v0,5(s0);\n" /*high cs*/
"bal 102f;\n" /*wait sr */
"nop;\n" 
"li v0,6;\n"  /* write enable */
"bal 101f;\n" 
"nop;\n" 
"li t1,0x11;\n" /*high cs*/
"sb t1,5(s0);\n"
"bal 102f;\n" /*wait sr*/
"nop;"
"li v0,2;\n"  /*write sector*/
"bal 101f;\n" 
"nop;\n" 
"srl v0,a1,16;\n" /*addr*/
"bal 103f;\n" 
"nop;\n" 
"srl v0,a1,8;\n" 
"bal 103f;\n" 
"nop;\n" 
"move v0,a1;\n" 
"bal 103f;\n" 
"nop;\n" 
"2:lb v0,(a0)\n" /*write 1 data*/ 
"bal 103f;\n" 
"nop;\n" 
"addiu a0,1;\n" 
"addiu a1,1;\n" 
"addiu a2,-1;\n"
"beqz a2,3f;\n" 
"nop;\n" \

/**************************/		//lxy
"andi v1,a1,0xff;\n" 
"beqz v1,1b;\n" 
"nop;\n"
"b 2b;\n" 
"nop;\n" 
/**************************/

//"b 1b;\n" 
//"nop;\n" 
"3:\n" 
"li v0,0x11;\n"
"sb v0,5(s0);\n" /*high cs*/
"li ra,0xff200200 ;\n" 
"jr ra ;\n" 
"nop;\n" 
"101:li t1,1;\n"
"sb t1,5(s0);\n" /*enable and low cs*/
"103:sb v0,2(s0);\n" 
"1:lb v0,1(s0);\n" 
"andi v0,1;\n" 
"bnez v0,1b;\n"
"nop;\n"
"lb v0,2(s0);\n" 
"jr ra;\n" 
"nop;" 
"102:move t0,ra;\n" 
"1:li v0,5;\n"  /*wait read sr*/
"bal 101b;\n" 
"nop;\n" 
"andi v0,1;\n"
"bnez v0,1b;\n" /*can continue read sr,write to gen clock*/
"nop;\n"
"li t1,0x11;\n" /*high cs*/
"sb t1,5(s0);\n"
"jr t0;\n"
"nop;\n"
);



static unsigned int code_erase_area_st25vf080[]=
tgt_compile(
"1:mtc0    s0, $31;\n" 
"li    s0,0xff200000;\n" 
"sw    v0,0x1fc(s0);\n" 
"sw    v1,0x1fc(s0);\n" 
"sw    a0,0x1fc(s0);\n" 
"sw    a1,0x1fc(s0);\n" 
"sw    a2,0x1fc(s0);\n" 
"sw    ra,0x1fc(s0);\n" 
"sw    t0,0x1fc(s0);\n" 
"lw a0,0(s0);\n"
"lw a1,4(s0);\n"
"lw a2,8(s0);\n"
"li s0,0xbfe80000;\n" 

#if 1
/*******************************************/		//lxy
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"bal 102f;\n" /*wait sr */
"nop;\n" 
"li v0,6;\n"  /* write enable */
"bal 101f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"li v0,1;\n"  /* write status*/
"bal 101f;\n" 
"nop;\n" 
"li v0,0;\n"  /* write 0*/	//double '0'  to status register for winbond spi flash
"bal 103f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"bal 102f;\n" /*wait sr */
"nop;\n" 
/*******************************************/
#endif

"2:li v0,6;\n"  /* write enable */
"bal 101f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"li v0,0xd8;\n" /*bulk erase */
"bal 101f;\n" 
"nop;\n" 
"srl v0,a0,16;\n" /*addr*/
"bal 103f;\n" 
"nop;\n" 
"srl v0,a0,8;\n" 
"bal 103f;\n" 
"nop;\n" 
"move v0,a0;\n" 
"bal 103f;\n" 
"nop;\n" 
"li v0,0x11;\n" /*high cs*/
"sb v0,5(s0);\n"
"bal 102f;\n" /*wait sr*/
"nop;\n"
"addu a0,a2;\n"
"slt v0,a1,a0;\n"
"beqz v0,2b;\n"
"nop;\n"
"li    s0,0xff200000;\n" 
"lw    t0,0x1fc(s0);\n" 
"lw    ra,0x1fc(s0);\n" 
"lw    a2,0x1fc(s0);\n" 
"lw    a1,0x1fc(s0);\n" 
"lw    a0,0x1fc(s0);\n" 
"lw    v1,0x1fc(s0);\n" 
"lw    v0,0x1fc(s0);\n" 
"mfc0    s0, $31;\n" 
"b 1b;\n" 
"nop;\n" 
"101:li v1,1;\n"
"sb v1,5(s0);\n" /*enable and low cs*/
"103:sb v0,2(s0);\n" 
"1:lb v0,1(s0);\n" 
"andi v0,1;\n" 
"bnez v0,1b;\n"
"nop;\n"
"lb v0,2(s0);\n" 
"jr ra;\n" 
"nop;" 
"102:move t0,ra;\n" 
"li v1,1;\n"
"sb v1,5(s0);\n" /*enable and low cs*/
"1:li v0,5;\n"  /*wait read sr*/
"bal 103b;\n" 
"nop;\n" 
"andi v0,1;\n"
"bnez v0,1b;\n"
"nop;\n"
"li v0,0x11;\n" /*high cs*/
"sb v0,5(s0);\n"
"jr t0;\n"
"nop;\n"
);

static unsigned int code_erase_st25vf080[]=
tgt_compile(
"1:mtc0    s0, $31;\n" 
"li    s0,0xff200000;\n" 
"sw    v0,0x1fc(s0);\n" 
"sw    v1,0x1fc(s0);\n" 
"sw    ra,0x1fc(s0);\n" 
"sw    t0,0x1fc(s0);\n" 
"li s0,0xbfe80000;\n" 
"bal 102f;\n" /*wait sr */
"nop;\n" 
"li v0,6;\n"  /* write enable */
"bal 101f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"li v0,1;\n"  /* write status*/
"bal 101f;\n" 
"nop;\n" 
"li v0,0;\n"  /* write 0*/
"bal 103f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"bal 102f;\n" /*wait sr */
"nop;\n" 
"li v0,6;\n"  /* write enable */
"bal 101f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"li v0,0xc7;\n" /*bulk erase */
"bal 101f;\n" 
"nop;\n" 
"li v1,0x11;\n" /*high cs*/
"sb v1,5(s0);\n"
"bal 102f;\n" /*wait sr*/
"nop;\n"
"li    s0,0xff200000;\n" 
"lw    t0,0x1fc(s0);\n" 
"lw    ra,0x1fc(s0);\n" 
"lw    v1,0x1fc(s0);\n" 
"lw    v0,0x1fc(s0);\n" 
"mfc0    s0, $31;\n" 
"b 1b;\n" 
"nop;\n" 
"101:li v1,1;\n"
"sb v1,5(s0);\n" /*enable and low cs*/
"103:sb v0,2(s0);\n" 
"1:lb v0,1(s0);\n" 
"andi v0,1;\n" 
"bnez v0,1b;\n"
"nop;\n"
"lb v0,2(s0);\n" 
"jr ra;\n" 
"nop;" 
"102:move t0,ra;\n" 
"li v1,1;\n"
"sb v1,5(s0);\n" /*enable and low cs*/
"1:li v0,5;\n"  /*wait read sr*/
"bal 103b;\n" 
"nop;\n" 
"andi v0,1;\n"
"bnez v0,1b;\n"
"nop;\n"
"li v0,0x11;\n" /*high cs*/
"sb v0,5(s0);\n"
"jr t0;\n"
"nop;\n"
);

static struct flash_function st25p65_flash_func __attribute__((section(".flash.init"))) =
	{"st25p64",code_program_st25p64,sizeof(code_program_st25p64),code_erase_st25p64,ls1f_spi_init,code_erase_area_st25p64};
static struct flash_function st25vf080_flash_func __attribute__((section(".flash.init"))) =
	{"st25vf080",code_program_st25vf080,sizeof(code_program_st25vf080),code_erase_st25vf080,ls1f_spi_init,code_erase_area_st25vf080};
static struct flash_function win25x64_flash_func __attribute__((section(".flash.init"))) =
	{"win25x64",code_program_win25x64,sizeof(code_program_win25x64),code_erase_st25vf080,ls1f_spi_init,code_erase_area_win25x64};


static int cmd_test_program_spi(int argc,char **argv)
{
	int context_reg[dMIPS32_NUM_DEBUG_REGISTERS];
	int i;
	ls1f_spi_init(0);
	context_save(context_reg);
	ui_datamem[0] = argc>1?strtoul(argv[1],0,0):0xbfc00000;
	ui_datamem[1] = argc>2?strtoul(argv[2],0,0):0;
	ui_datamem[2] = argc>3?strtoul(argv[3],0,0):0x60000;
	tgt_exec(
			"1:li    v0,0xff200000;\n" 
			"lw    a0,0(v0);\n" 
			"lw    a1,4(v0);\n" 
			"lw    a2,8(v0);\n" 
			"b 1b;\n"
			"nop;"
		,0x8000);

	if(argc > 4 && ui_datamem[0]>=0xff201000)
	{
	unsigned long download_addr;
	struct stat statbuf;
	char *buf;
	int fd;
	unsigned long before=0,after=0;
	if(stat(argv[3],&statbuf)!=0)
	{
		printf("error read file %s\n",argv[1]);
		exit(-1);
	}


	printf("open file %s,size %d,placed on 0xff201000\n",argv[3],statbuf.st_size);
	buf=malloc(statbuf.st_size+0x1000);
	if(!buf)printf("error allocate memory\n");
	fd=open(argv[1],O_RDONLY);
	read(fd,buf+0x1000-0x200,statbuf.st_size);
	close(fd);
	memcpy(buf,code_program_st25p64,sizeof(code_program_st25p64));
	do_cmd("goback");
	fMIPS32_ExecuteDebugModule(buf,statbuf.st_size+0x1000,1);
	}
	else
	fMIPS32_ExecuteDebugModule(code_program_st25p64,sizeof(code_program_st25p64),1);
    	context_restore(context_reg);
}


static int cmd_test_erase_spi_area(int argc,char **argv)
{
	int context_reg[dMIPS32_NUM_DEBUG_REGISTERS];
	int i;
	ls1f_spi_init(0);
	context_save(context_reg);
	ls1f_spi_init(0);
	ui_datamem[0] = argc>1?strtoul(argv[1],0,0):0x00000;
	ui_datamem[1] = argc>2?strtoul(argv[2],0,0):0x7ffff;
	ui_datamem[2] = argc>3?strtoul(argv[3],0,0):0x10000;
	fMIPS32_ExecuteDebugModule(code_erase_area_st25p64,sizeof(code_erase_area_st25p64),1);
    	context_restore(context_reg);
}

mycmd_init(test_erase_spi_area,cmd_test_erase_spi_area,"[start] [end] [blocksize]","test erase spi");
mycmd_init(test_program_spi,cmd_test_program_spi,"[form] [to] [size]","test program spi");
mycmd_init(spiroms,cmd_spiroms,"","select spi rom for read");
mycmd_init(spi_init,cmd_spi_init,"spi_init","init spi");

