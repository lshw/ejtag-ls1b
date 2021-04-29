/*
 * put filename  addr 
 */

int cmd_sget(int argc,char **argv)
{
	unsigned long download_addr;
	char *buf;
	int fd;
	unsigned int size;

if(argc<4)return -1;
size=strtoul(argv[3],0,0);


buf=malloc(size);
if(!buf)printf("error allocate memory\n");

	download_addr=strtoul(argv[2],0,0);

	jtag_getmem(buf,download_addr,size);

fd=open(argv[1],O_WRONLY|O_CREAT|O_TRUNC);
if(fd){
write(fd,buf,size);
close(fd);
}
else
{
printf("open file %s for write error\n",argv[1]); 
}

free(buf);

	return 0;
}

mycmd_init(sget,cmd_sget,"sget","sget filename address size");
