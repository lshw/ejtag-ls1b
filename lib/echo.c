static int echo(int argc,char **argv)
{
	int i;
if(!strcmp(argv[0],"echo_on")){showcmd=1;return 0;}
else if(!strcmp(argv[0],"echo_off")){showcmd=0;return 0;}

	for(i=1;i<argc;i++)
	printf("%s ",argv[i]);
	printf("\n");
return 0;	
}

static int verbose(int argc,char **argv)
{
if (argc<2) return -1;
 if (!strcmp(argv[1],"off")) {
	 int fd;
	 fd=open("/dev/null",O_WRONLY);
	 close(2);
	 dup2(fd,2);
	 close(fd);
	 return 0;
 }
 else if (!strcmp(argv[1],"on")) {
	 close(2);
	 dup(1,2);
	 return 0;
 }
else {
	 close(2);
	 dup(strtoul(argv[1],0,0),2);
	 return 0;
 }
}

static int loop(int argc,char **argv)
{
unsigned int i;
char buf[100]="";
if (argc<3) return -1;
for(i=2;i<argc;i++)
{
 strcat(buf,argv[i]);
 strcat(buf," ");
}

	for(i=0;i<strtoul(argv[1],0,0);i++)
	do_cmd(buf);
}


mycmd_init(echo,echo,"cmds","echo");
mycmd_init(echo_on,echo,"","echo on");
mycmd_init(echo_off,echo,"","echo off");
mycmd_init(verbose,verbose,"cmds","verbose [on|off|fd number]");
mycmd_init(loop,loop,"cmds","loop");

