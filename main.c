#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mips32.h"
#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <myconfig.h>
#include <signal.h>
#include <setjmp.h>
/*
 *This is main.c
 *
 */

static int syscall_addrwidth=1;
unsigned long help_addr=0xa0020000;
int log_disasm = 0;
extern int enable_timer;
extern unsigned int timer_val;

#include "cmdparser.c"
#include "lib/echo.c"
#include "lib/mem.c"
#include "lib/reg.c"
#include "lib/jtagtest.c"
#include "lib/put.c"
#include "lib/sput.c"
#include "lib/fput.c"
#include "lib/get.c"
#include "lib/fget.c"
#include "lib/sget.c"
#include "lib/env.c"
#include "lib/run.c"
#include "lib/source.c"
#include "lib/flash.c"
#include "lib/linux.c"
#include "lib/import.c"
#include "lib/putelf.c"
#include "lib/cache.c"
#include "lib/checksum.c"
#include "lib/memtest.c"
#include "Targets/main.c"

myconfig_init("log.disas",log_disasm,CONFIG_I,"log disasm");
myconfig_init("helpaddr",help_addr,CONFIG_I,"helpaddr for download");

static int ejtag_quit = 0;

static int quit(int argc,char **argv)
{
	ejtag_quit = 1;
return 0;
}

mycmd_init(q,quit,"q","quit this shell");

static void print_help(char **argv)
{
	printf("usage: %s [-dlStch] [-e cmd] [-T n]\n"\
"-d: verbose on,show debug messags\n" \
"-e 'cmd': run cmd\n" 
"-l: do not use read line\n"
"-S: log disassemble info\n"
"-t: disable timer\n"
"-T n: set timer n ms\n"
"-c: do not load cfg file\n"
"-h: show this help\n",argv[0]
);
	printf("bellow is cmd on debug cmd line\n");
	do_cmd("h");
}

#include "readline.c"

sigjmp_buf main_jmpbuf;
static __sighandler_t oldhandler;

static void main_handleint(int sig)
{
	siglongjmp(main_jmpbuf,1);
}

/*
 * main function
 * @argc argc
 * #argv argv
 */
int main(int argc,char **argv)
{
static char str[1024];
int count;
int opt;
int no_use_readline = 0;
int disable_cfg = 0;


setbuffer(stdout,0,0);
ui_fifomem=malloc(ui_fifomem_size);

do_cmd("verbose off");


while ((opt=getopt(argc,argv,"ldSctT:e:h")) != -1 )
{
	switch (opt) {
		case 'd':
			do_cmd("verbose on");
			break;
		case 'e':
			do_cmd(optarg);
			break;
	    case 'l':
			no_use_readline = 1;
			break;
		case 'S':
			log_disasm = 1;
			break;
		case 't':
			enable_timer = -1;
			break;
		case 'T':
			timer_val = strtoul(optarg,0,0);
			break;
		case 'c':
			disable_cfg = 1;
			break;
		case 'h':
			print_help(argv);
			return 0;
			break;

	}
}

if(!disable_cfg)
{
int fd;
fd = open("ejtag.cfg",O_RDONLY);
if(fd>=0)
{
close(fd);
do_cmd("source ejtag.cfg");
}
}

oldhandler = signal(SIGINT,main_handleint);

if(no_use_readline)
{
	if(sigsetjmp(main_jmpbuf,3)) printf("break!\n");

 while(!ejtag_quit)
 {
 char *pstart=str,*pend;
 int len=1023;
    printf("\n-");	
    while(1)
    {
	if(enable_timer != -1) enable_timer = 1;
	count=read(0,pstart,len);
	if(enable_timer != -1) enable_timer = 0;
	if(count<=0)return;
	pstart[count]=0;
	pend=_do_cmd(str,1);
	if((pend-pstart)==count)break;
	strcpy(str,pend);
	pstart=str+strlen(str);
	len=1023-strlen(str);
    }
 }
}
else
{
	initialize_readline();

	read_history("ejtag.rc");

	if(sigsetjmp(main_jmpbuf,3)) printf("break!\n");

	while(!ejtag_quit)
	{
		char *line;
		if(enable_timer != -1) enable_timer = 1;
		line= readline("-");
		if(enable_timer != -1) enable_timer = 0;
		if(!line)break;
		add_history(line);
		_do_cmd(line,0);
		free(line);
	}
	write_history("ejtag.rc");
}
	signal(SIGINT,oldhandler);
 return 0;
}


	
