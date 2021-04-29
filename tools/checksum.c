#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc,char **argv)
{
	int fd;
	struct stat statbuf;
	unsigned int checksum,i;
	unsigned int seek,len,size;
	char *buf;
	seek = strtoul(argv[2],0,0);
	len = strtoul(argv[3],0,0);
	if(stat(argv[1],&statbuf)!=0)
	{
		printf("error read file %s\n",argv[1]);
		exit(-1);
	}
	printf("open file %s,size %d\n",argv[1],statbuf.st_size);
	buf=malloc(statbuf.st_size+4);
	if(!buf)printf("error allocate memory\n");
	fd=open(argv[1],O_RDONLY);
	read(fd,buf,statbuf.st_size);
	close(fd);
	checksum = 0;
	size = (statbuf.st_size+3)&~3;
	if(len+seek>size) len=size;

	for(i=seek,checksum=0;i<seek+len;i+=4)
	{
		checksum += *(unsigned int *)(buf+i);
	}
	printf("checksum 0x%08x\n",checksum);

	return 0;
}
