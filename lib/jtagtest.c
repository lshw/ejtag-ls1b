#include <linux/types.h>
#include <myconfig.h>
static int config_jtagregs_jtagdata = 0;
static int config_jtagregs_jtagdata_val = 0;
myconfig_init("jtagregs.jtagdata",config_jtagregs_jtagdata,CONFIG_I,"jtagregs d1 will input special data");
myconfig_init("jtagregs.jtagdata.val",config_jtagregs_jtagdata_val,CONFIG_I,"jtagregs input data'svalue");
static int __jtagreg_syscall1(int type,unsigned long long addr,union commondata *mydata)
{
    int code = addr&0x1f;
    unsigned int data;
    if(type!=4)return -1;
    switch(code)
    {
	    case 0xa: data=0x4c000; break;
		case 0xb:mydata->data4 =0;return 0;break;
	    default: data=0; break;
    }
	if(config_jtagregs_jtagdata)data=config_jtagregs_jtagdata_val;
    fJTAG_Instruction(code);
    mydata->data4 = fJTAG_Data(data);
    fJTAG_Data(mydata->data4);
return 0;
}

static int __jtagreg_syscall2(int type,unsigned long long addr,union commondata *mydata)
{
    int code = addr&0x1f;
    unsigned int data;
    if(type!=4)return -1;
    fJTAG_Instruction(code);
    data=fJTAG_Data(mydata->data4);
    printf("\n%02x: %02x<->",code,data);
    data=fJTAG_Data(mydata->data4);
    printf("%02x\n",data);
return 0;
}


static int jtagregs(int argc,char **argv)
{
syscall1=__jtagreg_syscall1;
syscall2=__jtagreg_syscall2;
syscall_addrwidth=4;
fMIPS32_Init();
return 0;	
}

static int waitreg(int argc,char **argv)
{
    int reg;
    unsigned int data,data0,wantdata;
	if(argc!=3)return -1;

	fMIPS32_Init();

	reg = strtoul(argv[1],0,0) & 0x1f;
	wantdata = strtoul(argv[2],0,0);
	
    switch(reg)
    {
	    case 0xa: data0=0x4c000; break;
	    default: data0=0; break;
    }

  do{
    fJTAG_Instruction(reg);
    data = fJTAG_Data(data0);
    fJTAG_Data(data);
	} while(data != wantdata);

return 0;
}

static int cmd_waitfacc(int argc,char **argv)
{
int op,address,data,state=3;

 fMIPS32_Init();

while(1)
{
 int ret;
 ret=get_acc(&op,&address,&data);
 if(!(ret&1)){force_acc();fprintf(stderr,"conneting\n");state=0;continue;}
 if(state<1){fprintf(stderr,"conneted\n");state=1;}

 if(!(ret&2)){fprintf(stderr,"waiting\n");state=1;continue;}
 if (state<2) {fprintf(stderr,"acc\n"); state=2;}
 if(address==0xff200200)break;
}

return 0;
}

static int jtaginit(int argc,char **argv)
{
 fMIPS32_Init();
 if(argc>1)
 setenv("put_speed",argv[1],0);
 return 0;
}

mycmd_init(jtagregs,jtagregs,"jtagregs","jtagregs");
mycmd_init(waitreg,waitreg,"waitreg reg data","waitreg reg equal to be data");
mycmd_init(waitfacc,cmd_waitfacc,"waitfacc ","wait first acc after reset");
mycmd_init(jtaginit,jtaginit,"jtaginit [speed]","jtaginit [speed]");

