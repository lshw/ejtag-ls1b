/*
 * put filename  addr 
 */

int cmd_source(int argc,char **argv)
{
	unsigned long download_addr;
	struct stat statbuf;
	char *buf;
	int fd;
	if(stat(argv[1],&statbuf)!=0)
	{
		printf("error read file %s\n",argv[1]);
		exit(-1);
	}
printf("open file %s,size %d\n",argv[1],statbuf.st_size);
buf=malloc(statbuf.st_size+1);
if(!buf)printf("error allocate memory\n");
fd=open(argv[1],O_RDONLY);
read(fd,(buf),statbuf.st_size);
close(fd);
buf[statbuf.st_size] = 0;
_do_cmd(buf,0);
free(buf);

    return 0;
}

mycmd_init(source,cmd_source,"source","source filename");
