static int config_icache_size = 0x4000;
static int config_icache_ways = 4;
static int config_icache_waybit = 12;

static int config_dcache_size = 0x4000;
static int config_dcache_ways = 4;
static int config_dcache_waybit = 12;


myconfig_init("icache.size",config_icache_size,CONFIG_I,"icache size");
myconfig_init("icache.ways",config_icache_ways,CONFIG_I,"icache ways");
myconfig_init("icache.waybit",config_icache_waybit,CONFIG_I,"icache waybit");
myconfig_init("dcache.size",config_dcache_size,CONFIG_I,"dcache size");
myconfig_init("dcache.ways",config_dcache_ways,CONFIG_I,"dcache ways");
myconfig_init("dcache.waybit",config_dcache_waybit,CONFIG_I,"dcache waybit");

int cacheflush(unsigned int start,unsigned int size)
{
int i;

if(size < config_icache_size)
{
unsigned int codebuf[] = tgt_compile(
	  ".set mips32;\n"
	  "1:mtc0 ra,c0_desave;\n"
	  "bal 2f;\n"
	  "nop;\n"
	  ".space 20;\n"
	  "2:\n"
	  "sw v0,(ra);\n"
	  "sw v1,4(ra);\n"
	  "lw v0,8(ra);\n" /*start address*/
	  "lw v1,12(ra);\n" /*end address*/
	  "3:cache    0x15,(v0);\n"
	  "cache    0x10,(v0);\n" 
	  "addu v0,32;\n"
	  "bne v0,v1,3b;\n"
	  "nop;\n"
	  "lw v1,4(ra);\n"
	  "lw v0,(ra);\n"
	  "mfc0 ra,c0_desave;\n"
	  "b 1b;\n"
	   "nop;\n");
   
    codebuf[5] = start;
    codebuf[6] = (start + size + 32) & ~ 31;
	fMIPS32_ExecuteDebugModule(codebuf,sizeof(codebuf),1);
}
else
{
unsigned int cacheinst;
unsigned int codebuf[] = tgt_compile(
	  ".set mips32;\n"
	  "1:mtc0 ra,c0_desave;\n"
	  "bal 2f;\n"
	  "nop;\n"
	  ".space 20;\n"
	  "2:\n"
	  "sw v0,(ra);\n"
	  "sw v1,4(ra);\n"
	  "lw v0,8(ra);\n" /*start address*/
	  "lw v1,12(ra);\n" /*end address*/
	  "3:cache    0,(v0);\n"
	  "addu v0,32;\n"
	  "bne v0,v1,3b;\n"
	  "nop;\n"
	  "lw v1,4(ra);\n"
	  "lw v0,(ra);\n"
	  "mfc0 ra,c0_desave;\n"
	  "b 1b;\n"
	   "nop;\n");

cacheinst = codebuf[12];


for(i=0;i<config_dcache_ways;i++)
{
	codebuf[12] = cacheinst|(1<<16);
	codebuf[5] = 0x80000000 + (i<<config_dcache_waybit);
	codebuf[6] = 0x80000000 +  config_dcache_size/config_dcache_ways + (i<<config_dcache_waybit);
	fMIPS32_ExecuteDebugModule(codebuf,sizeof(codebuf),1);
}


for(i=0;i<config_icache_ways;i++)
{
	codebuf[12] = cacheinst|(0<<16);
	codebuf[5] = 0x80000000 + (i<<config_icache_waybit);
	codebuf[6] = 0x80000000 +  config_icache_size/config_icache_ways + (i<<config_icache_waybit);
	fMIPS32_ExecuteDebugModule(codebuf,sizeof(codebuf),1);
}


}

return 0;
}

int cmd_cacheflush(int argc,char **argv)
{
unsigned int start;
unsigned int size;
int i;
start = strtoul(argv[1],0,0);
size = strtoul(argv[2],0,0);
cacheflush(start,size);
return 0;
}


int cmd_cache(int argc,char **argv)
{
unsigned int start;
unsigned int end;
unsigned int size;
unsigned int cacheinst;
int op;
unsigned int codebuf[] = tgt_compile(
		".set mips32;\n"
		"1:mtc0 ra,c0_desave;\n"
		"bal 2f;\n"
		"nop;\n"
		".space 20;\n"
		"2:\n"
		"sw v0,(ra);\n"
		"sw v1,4(ra);\n"
		"lw v0,8(ra);\n" /*start address*/
		"lw v1,12(ra);\n" /*end address*/
		"3:cache    0,(v0);\n"
		"addu v0,32;\n"
		"bne v0,v1,3b;\n"
		"nop;\n"
		"lw v1,4(ra);\n"
		"lw v0,(ra);\n"
		"mfc0 ra,c0_desave;\n"
		"b 1b;\n"
		"nop;\n");

if(argc < 4) return -1;

op = strtoul(argv[1],0,0);
start = strtoul(argv[2],0,0);
size = strtoul(argv[3],0,0);

	codebuf[12] |= (op<<16);
	codebuf[5] = start;
	codebuf[6] = (start + size + 31) & ~31;

printf("start = %x\n",start);
	fMIPS32_ExecuteDebugModule(codebuf,sizeof(codebuf),1);

return 0;
}

mycmd_init(cache,cmd_cache,"cache op addr size","flush dache ");
mycmd_init(cacheflush,cmd_cacheflush,"cacheflush addr size","flush dache and invalid icache");

