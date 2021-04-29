#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "crc32.c"

#define MICRO_INST_SIZE 10
#define MICRO_INST_SIZE_2 11
//#define MICRO_INSTCP0_SIZE 15
#define MICRO_INST_SIZE_INITRAM 16



#define SDRAM_SIZE_256M
//#define SDRAM_SIZE_128M
//#define SDRAM_SIZE_64M

int main(int argc,char **argv)
{
int fd;
int fd2;
int i=0;
char *buf;
char *buf2;
struct stat statbuf;
struct stat statbuf2;
char op_mathod[10];
char download_filename[40];
unsigned int checksum_crc=0x0;
unsigned int crc_target=0x0;
char buf_crc[8];
int ii=0;
unsigned int ui_data[6000]={0};
unsigned int upload_len=0;

extern unsigned int miro_inst[];


extern unsigned int miro_inst_2[];


extern unsigned int miro_initsdram_inst[];


extern unsigned int miro_initsdram_inst_2[];


extern unsigned int miro_download_os_ret_inst[];

extern unsigned int miro_savedepc_inst[];
extern unsigned int miro_runcrc_inst[];


extern unsigned int miro_initcp0_inst[];

setbuffer(stdout,0,0);

re_input:
printf("Please select operation mathod[Download/Upload]:");
gets(op_mathod);
op_mathod[0]='D' ;
if(op_mathod[0]=='U' || op_mathod[0] == 'u')
{
printf("jtag_quit 1\n");
jtag_quit();
printf("Save the context!\n");

    fMIPS32_ExecuteDebugModule(miro_savedepc_inst,0x8000,1); //Save the context
    download_context_save();



	upload_len =0x200000 ; // 0x180 ; // 0x100000 ;
    jtag_download_length(0xa5300000,upload_len);
    jtag_self_down_up(0xa5000000);
	printf("upload-1 \n");
    fMIPS32_ExecuteDebugModule(miro_inst_2,0x20,1);
	printf("upload-2 \n");

 fJTAG_UPLOAD(upload_len);


	printf("upload-3 \n");
}
else if(op_mathod[0]=='D' || op_mathod[0] == 'd')
{
if(argc!=2){
	printf("usage:%s gzrom.bin\npmon zloader/Makefile.inc change GZROMSTARTADDR to GZROMSTARTADDR?=0xffffffff81000200\n",argv[0]);

	printf("Please the need to download filename:");
	gets(download_filename);
	if(stat(download_filename,&statbuf)!=0)
	{
		printf("error read file %s\n",download_filename);
		exit(-1);
	}
}
else{
	if(stat(argv[1],&statbuf)!=0)
	{
		printf("error read file %s\n",argv[1]);
		return -1;
	}
if(getenv("PROGRAM_FLASH"))
{

	if(stat(argv[2],&statbuf2)!=0)
	{
		printf("error read file %s\n",argv[2]);
		return -1;
	}
}
}
printf("open file %s,size %d\n",argv[1],statbuf.st_size);
buf=malloc(statbuf.st_size);
if(!buf)printf("error allocate memory\n");
fd=open(argv[1],O_RDONLY);
read(fd,(buf),statbuf.st_size);
close(fd);

if(getenv("RUN_CRC_CHECKSUM"))
{
checksum_crc = download_crc32(buf,statbuf.st_size);
printf("checksum_crc : 0x%x \n",checksum_crc);
buf_crc[0]=statbuf.st_size&0xFF;
buf_crc[1]=(statbuf.st_size&0xFF00) >>8;
buf_crc[2]=(statbuf.st_size&0xFF0000) >>16;
buf_crc[3]=(statbuf.st_size&0xFF000000) >>24;
buf_crc[4]=checksum_crc&0xFF;
buf_crc[5]=(checksum_crc&0xFF00) >>8;
buf_crc[6]=(checksum_crc&0xFF0000) >>16;
buf_crc[7]=(checksum_crc&0xFF000000) >>24;
}

printf("jtag_quit 1\n");
jtag_quit();
printf("jtag_quit 2\n");
//zgj 0116 fMIPS32_ExecuteDebugModule(buf,statbuf.st_size,0);
if(getenv("PMON_BOOT_FLAG"))
{
    fMIPS32_ExecuteDebugModule(miro_initcp0_inst,0x8000,1);   //Bios init CPU CP0 registers
printf("before init SDRAM!\n");
    fMIPS32_ExecuteDebugModule(miro_initsdram_inst,0x8000,1); //Init the Sdram controller
printf("after  init SDRAM!\n");
}
else
{
printf("Save the context!\n");

    //fMIPS32_ExecuteDebugModule(miro_savedepc_inst,0x8000,1); //Save the context
    download_context_save();
}

      jtag_download_length(0xa5300000,statbuf.st_size);
if(getenv("RUN_CRC_CHECKSUM"))
{
    jtag_download_crc_checksum(0xa5300000 + 4,checksum_crc);
    jtag_crc_down(0xa5400000);
    jtag_crc_table_down(0xa5000100);
}
    
    jtag_self_down(0xa5000000);
    fMIPS32_ExecuteDebugModule(miro_inst_2,0x8000,3);

	fMIPS32_USB_DownloadModule(buf,statbuf.st_size);

if(getenv("PROGRAM_FLASH"))
{
      jtag_download_length(0xa5300000,statbuf.st_size);
    jtag_self_down(0xa5000000);
    fMIPS32_ExecuteDebugModule(miro_inst_2,0x8000,1);
	fMIPS32_USB_DownloadModule(buf,statbuf.st_size);
}

free(buf);

if(getenv("RUN_CRC_CHECKSUM"))
{
printf("jtag_quit crc-excute\n");
jtag_quit();
    fMIPS32_ExecuteDebugModule(miro_runcrc_inst,0x8000,1); //zgj jump to the 0xa5400000 to execute the program.
//sleep(2);
printf("jtag_quit crc-excute 1 \n");
    crc_target = jtag_wait_k0_flag(0xFF2FFF00+26*4);
    if(crc_target == checksum_crc)
    {
    	printf("Download successful!\n");
    }
    else
    {
	    printf("real-crc : 0x%x , error-crc : 0x%x \n",checksum_crc,crc_target);
	    printf("CRC checksum is error,Please retry to download\n");
	    goto re_input ;
    }
}

if(getenv("PMON_BOOT_FLAG"))
{
    fMIPS32_ExecuteDebugModule(miro_initsdram_inst_2,0x8000,1); //zgj jump to the 0xa0200000 to execute the program.
}
else
{
    download_context_restore();
	jtag_quit();
 //   fMIPS32_ExecuteDebugModule(miro_download_os_ret_inst,0x8000,1); //zgj jump right addr to execute the program.
}
    
printf("jtag_quit 4\n");

}
else
{
	printf("You input the unimplement operation mathod!\n");
	goto re_input;
}
return 0;
}

