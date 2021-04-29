static int cmd_save(int argc,char **argv)
{
	context_save(context_reg);
return 0;
}

static int cmd_restore(int argc,char **argv)
{
	context_restore(context_reg);
return 0;
}

#define MTC0(RT,RD,SEL) ((0x408<<20)|((RT)<<16)|((RD)<<11)|SEL)
#define MFC0(RT,RD,SEL) ((0x400<<20)|((RT)<<16)|((RD)<<11)|SEL)
#define DMTC0(RT,RD,SEL) ((0x40a<<20)|((RT)<<16)|((RD)<<11)|SEL)
#define DMFC0(RT,RD,SEL) ((0x402<<20)|((RT)<<16)|((RD)<<11)|SEL)
static int cp0s_sel=0;
extern unsigned int  ui_datamem[];
static int __cp0syscall1(int type,unsigned long long addr,union commondata *mydata)
{
long data8;
unsigned int codebuf[12]=	tgt_compile(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "lui    $15,0xFF20;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "mfc0    $2,$31;\n" \
	  "sw    $2, ($15);\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n");

if(type!=8)return -1;
memset(mydata->data8,0,8);
addr=addr&0x1f;
//codebuf[3]=DMFC0(2,addr,cp0s_sel);
codebuf[3]=MFC0(2,addr,cp0s_sel);

fMIPS32_ExecuteDebugModule(codebuf,40,1);

*(long *)mydata->data8=ui_datamem[0];

return 0;
}

static int __cp0syscall2(int type,unsigned long long addr,union commondata *mydata)
{
unsigned int codebuf[12]=	tgt_compile(\
	  ".set noat;\n" \
	  "1:\n" \
	  "mtc0    $15, $31;\n" \
	  "lui    $15,0xFF20;\n" \
	  "sw    $2, 0x1fc($15);\n" \
	  "lw    $2, ($15);\n" \
	  "mtc0    $2,$31;\n" \
	  "lw    $2, 0x1fc($15);\n" \
	  "mfc0    $15, $31;\n" \
	  "b 1b;\n" \
	  "nop;\n");
if(type!=8)return -1;
addr=addr&0x1f;
//codebuf[4]=DMTC0(2,addr,cp0s_sel);
codebuf[4]=MTC0(2,addr,cp0s_sel);

ui_datamem[0]=*(long *)mydata->data8;
fMIPS32_ExecuteDebugModule(codebuf,40,1);


return 0;
}

static int mycp0s(int argc,char **argv)
{
syscall1=__cp0syscall1;
syscall2=__cp0syscall2;
syscall_addrwidth=8;
if(argc>1)cp0s_sel=strtoul(argv[1],0,0);
else cp0s_sel=0;
return 0;	
}

char *regname[]={
	"zero",       "at",       "v0",       "v1",       "a0",       "a1",       "a2",       "a3",
	"t0",       "t1",       "t2",       "t3",       "t4",       "t5",       "t6",       "t7",
	"s0",       "s1",       "s2",       "s3",       "s4",       "s5",       "s6",       "s7",
	"t8",       "t9",       "k0",       "k1",       "gp",       "sp",       "s8",       "ra",
	"status",       "lo",       "hi", "badvaddr",    "cause",       "pc"
};

static cmd_set(int argc,char **argv)
{
	if(argc==1)
	{
	int i=0;
	  uififo_raddr=0;
	  uififo_waddr=0;
	  memset(ui_fifomem,0,38*4);
	fMIPS32_ExecuteDebugModule(aui_readregisters_code,0x80000,1);
	
	for(i=0;i<dMIPS32_NUM_DEBUG_REGISTERS;i++)
	{
		if((i&3)==0)printf("\n");
		printf("%s:0x%x ",regname[i],ui_fifomem[i]);
	}
	printf("\n");
	return 0;
	}
	else
	{
	char *p;
	int regnum;
	p = &argv[1][0];
	if(argv[1][0]==':') p++; /*for old style*/

	if(p[0]>='0' && p[0]<='9')
	regnum=strtoul(p,0,0);
	else {
	 int i;
	 regnum=0;
	 for(i=0;i<sizeof(regname)/sizeof(regname[0]);i++)
	 if(!strcmp(regname[i],p)){
	 regnum=i;
	  break;
	}
	}

	if(argc==2)
	{
      // Go read all the registers from the processor

		if(regnum<31)
		{
	int reg32code[]=tgt_compile(\
		"sw $0,($31);\n" \
		"sw $1,($31);\n" \
		);
		 reg32code[1]=reg32code[0]+regnum*0x10000;
		  tgt_exec(\
		  "mtc0    $31, $31;\n" \
		  "lui    $31,0xFF20;\n" \
		  ,8);
		  fMIPS32_ExecuteDebugModule(&reg32code[1],4,1);
		  tgt_exec("mfc0    $31, $31;\n",4);
		}
		else if(regnum==31)
		tgt_exec(\
		  "mtc0    $2, $31;\n" \
		  "lui    $2,0xFF20;\n" \
		  "sw    $31, ($2);\n" \
		  "mfc0    $2, $31;\n" \
		  ,16);
		else if(regnum<38) //sr
		{
		int cpxbuf[]=tgt_compile(\
		  "mfc0 $2, $12;\n" \
		  "mflo	$2;\n" \
		  "mfhi	$2;\n" \
		  "mfc0	$2, $8;\n" \
		  "mfc0	$2, $13;\n" \
		  "mfc0	$2, $24;\n" \
		  );

		tgt_exec(\
		  "mtc0    $15, $31;\n" \
		  "lui    $15,0xFF20;\n" \
		  "sw    $2, 4($15);\n" \
		  ,12);
		fMIPS32_ExecuteDebugModule(&cpxbuf[regnum-32],4,1);
		tgt_exec(\
		  "sw    $2, ($15);\n" \
		  "lw    $2, 4($15);\n" \
		  "mfc0    $15, $31;\n" \
		  ,12);
		}
		  else 
		  ui_datamem[0]=0;

	  printf("%s=0x%x\n",argv[1],ui_datamem[0]);

	   }
	else
	{
	 	ui_datamem[0]=strtoul(argv[2],0,0);


		if(regnum<31)
		{
	int reg32code[]=tgt_compile(\
		"lw $0,($31);\n" \
		"lw $1,($31);\n" \
		);
		 reg32code[1]=reg32code[0]+regnum*0x10000;
		  tgt_exec(\
		  "mtc0    $31, $31;\n" \
		  "lui    $31,0xFF20;\n" \
		  ,8);
		  fMIPS32_ExecuteDebugModule(&reg32code[1],4,1);
		  tgt_exec("mfc0    $31, $31;\n",4);
		}
		else if(regnum==31)
		tgt_exec(\
		  "mtc0    $2, $31;\n" \
		  "lui    $2,0xFF20;\n" \
		  "lw    $31, ($2);\n" \
		  "mfc0    $2, $31;\n" \
		  ,16);
		else if(regnum<38) //sr
		{
		int cpxbuf[]=tgt_compile(\
		  "mtc0 $2, $12;\n" \
		  "mtlo	$2;\n" \
		  "mthi	$2;\n" \
		  "mtc0	$2, $8;\n" \
		  "mtc0	$2, $13;\n" \
		  "mtc0	$2, $24;\n" \
		  );

		tgt_exec(\
		  "mtc0    $15, $31;\n" \
		  "lui    $15,0xFF20;\n" \
		  "sw    $2, 4($15);\n" \
		  "lw    $2, ($15);\n" \
		  ,16);
		fMIPS32_ExecuteDebugModule(&cpxbuf[regnum-32],4,1);
		tgt_exec(\
		  "lw    $2, 4($15);\n" \
		  "mfc0    $15, $31;\n" \
		  ,8);
		}

      else
      {
        printf("Invalid register number %d.\n", regnum);
	return -1;
      }
	}
	return 0;
	}
return 0;
}

extern int ejtag_in_debug_mode;
static int cmd_cont(int argc,char **argv)
{
	jtag_quit();
	ejtag_in_debug_mode = 0;
	return 0;
}

static int cmd_goback(int argc,char **argv)
{
int op,address,data,state=3,address0=0;
unsigned int destaddr=0xff200200;
int buf[8]=tgt_compile(\
					"mtc0    $15, $31;\n" \
					"lui $15,0;\n" \
					"or $15,0;\n" \
					"jr $15;\n" \
					"nop;\n"); 

int buf1[4]=tgt_compile("mfc0 $15,$31;\n");

if (argc > 1)
	destaddr = strtoul(argv[1],0,0);

if(destaddr>0xff200000) destaddr -= 4;

buf[2] |= destaddr&0xffff;	
buf[1] |= (destaddr>>16)&0xffff;

 fMIPS32_Init();

while(1)
{
 int ret;
 ret=get_acc(&op,&address,&data);
 if(!(ret&1)){force_acc();fprintf(stderr,"conneting\n");state=0;continue;}
 if(state<1){fprintf(stderr,"conneted\n");state=1;}

 if(!(ret&2)){fprintf(stderr,"waiting\n");state=1;continue;}
 if (state<2) {fprintf(stderr,"acc\n"); state=2;}
 if(!address0)address0=address;

 if(address >=address0 && address < address0+20)feed_acc(buf[(address-address0)/4]);
 else if(address != destaddr) feed_acc(0);
 else { feed_acc(buf1[0]);break;}
}


return 0;
}

mycmd_init(set,cmd_set,"set [reg] [val]","set regs");
mycmd_init(save,cmd_save,"save","save regs");
mycmd_init(restore,cmd_restore,"restore","restore regs");
mycmd_init(cp0s,mycp0s,"cp0s","cp0 regs");
mycmd_init(cont,cmd_cont,"cont ","quit jtag and cont");
mycmd_init(goback,cmd_goback,"goback [address]","go back to 0xff200200 default");
//mycmd_init(cp0s,mycp0s,"[0|-1]","set m4,d4 to access cp0 regs");
