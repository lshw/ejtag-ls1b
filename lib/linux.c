#include  <unistd.h>
#include <myconfig.h>
extern char **environ;
static unsigned int config_karg_bootparam_addr = 0xa4000000-512;
static unsigned int config_karg_rd_start = 0x84000000;
static unsigned int config_karg_rd_size = 0;
static unsigned int config_karg_memsize = 256;

myconfig_init("karg.bootparam_addr",config_karg_bootparam_addr,CONFIG_I,"bootparam address on memory");
myconfig_init("karg.rd_start",config_karg_rd_start,CONFIG_I,"rd_start");
myconfig_init("karg.rd_size",config_karg_rd_size,CONFIG_I,"rd_size");
myconfig_init("karg.memsize",config_karg_memsize,CONFIG_I,"memsize");

static int cmd_karg(int argc,char **argv)
{
	char memenv[32];
	char highmemenv[32];
	char *pmonenv[]={"cpuclock=200000000",memenv,highmemenv};
	char params_buf[512];
	int params_size=512;
	unsigned int *parg_env=params_buf;
	char *p;
    unsigned long bootparam_addr;
	int i;
	int ret;
	int nkargs ;
	unsigned long memsize;
	long a0,a1,a2;
	/*
	 * pram buf like this:
	 *argv[0] argv[1] 0 env[0] env[1] ...env[i] ,0, argv[0]'s data , argv[1]'s data ,env[0]'data,...,env[i]'s dat,0
	 */

	bootparam_addr = config_karg_bootparam_addr;

	memsize = config_karg_memsize;

	if(config_karg_rd_size) nkargs = argc + 1;
	else nkargs = argc;

	a0 = nkargs;

	//*count user special env
	for(ret=0,i=0;environ[i];i++)
		if(!strncmp(environ[i],"ENV_",4))ret+=4;

	//jump over argv and env area
	ret +=(nkargs+sizeof(pmonenv)/sizeof(char *)+2)*4;

	a1 = bootparam_addr;

	//argv0
	*parg_env++=bootparam_addr+ret;
	ret +=1+snprintf(params_buf+ret,params_size-ret,"g");
	//argv1

	if(config_karg_rd_size)
	{
	int rd_start;
	int rd_size;
	rd_start=config_karg_rd_start;
	rd_size=config_karg_rd_size;

	*parg_env++=bootparam_addr+ret;
	ret +=1+snprintf(params_buf+ret,params_size-ret,"rd_start=0x%x rd_size=%lu",rd_start,rd_size);
	}

	for(i=1;i<argc;i++)
	{
	*parg_env++=bootparam_addr+ret;
	ret +=1+snprintf(params_buf+ret,params_size-ret,"%s",argv[i]);
	}


	//argv2
	*parg_env++=0;

	a2 = bootparam_addr + (long)parg_env - (long)params_buf;

	//env
	sprintf(memenv,"memsize=%d",memsize>256?256:memsize);
	sprintf(highmemenv,"highmemsize=%d",memsize>256?memsize-256:0);


	for(i=0;i<sizeof(pmonenv)/4;i++)
	{
		*parg_env++=bootparam_addr+ret;
		ret +=1+snprintf(params_buf+ret,params_size-ret,"%s",pmonenv[i]);
	}

	for(i=0;environ[i];i++)
	{
		if(!strncmp(environ[i],"ENV_",4)){
			*parg_env++=bootparam_addr+ret;
			ret +=1+snprintf(params_buf+ret,params_size-ret,"%s",&environ[i][4]);
		}
	}

	*parg_env++=0;


	jtag_putmem(params_buf,bootparam_addr,ret);

	ui_datamem[0] = a0;
	ui_datamem[1] = a1;
	ui_datamem[2] = a2;

		tgt_exec(\
		  "1:mtc0    $2, $31;\n" \
		  "lui    $2,0xFF20;\n" \
		  "lw    $4, ($2);\n" \
		  "lw    $5, 4($2);\n" \
		  "lw    $6, 8($2);\n" \
		  "mfc0    $2, $31;\n" \
		  "b 1b;\n" \
		  "nop;\n" \
		  ,0x8000);

	return 0;
}


mycmd_init(karg,cmd_karg,"karg [arg0] [...]","set kernel argument,\tenv:bootparam_addr,memsize,rd_size,rd_start,ENV_xxx(pmon env) ");

