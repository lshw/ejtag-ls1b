/*
 * put filename  addr 
 */

int cmd_run(int argc,char **argv)
{
	unsigned long download_addr;
	struct stat statbuf;
	char *buf;
	int fd;
	unsigned long before=0,after=0;
	int context_reg[dMIPS32_NUM_DEBUG_REGISTERS];
	if(stat(argv[1],&statbuf)!=0)
	{
		printf("error read file %s\n",argv[1]);
		exit(-1);
	}

	if(argc>2)before=strtoul(argv[2],0,0);
	if(argc>3)after=strtoul(argv[3],0,0);

printf("open file %s,size %d\n",argv[1],statbuf.st_size);
buf=malloc(statbuf.st_size+before+after);
if(!buf)printf("error allocate memory\n");
fd=open(argv[1],O_RDONLY);
read(fd,buf+before,statbuf.st_size);
close(fd);

printf("before=%x,after=%x\n",before,after);
fMIPS32_ExecuteDebugModule(buf+before,statbuf.st_size+after,4);

free(buf);

    return 0;
}

mycmd_init(run,cmd_run,"run","run filename");
