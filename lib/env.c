#include  <unistd.h>
extern char **environ;
int cmd_setenv(int argc,char **argv)
{
	int i;
	if(argc==1)
	{
	for(i=0; environ[i];i++)
		printf("%s\n",environ[i]);
	}
	else if(argc==2)
	{
	 char *env=getenv(argv[1]);
	 if(env)printf("%s=%s\n",argv[1],env);
	 else printf("%s is not set\n",argv[1]);
	}
	else 
	{
		setenv(argv[1],argv[2],1);
	}

	return 0;
}

int cmd_msleep(int argc,char **argv)
{
usleep(strtoul(argv[1],0,0)*1000);
return 0;
}

mycmd_init(setenv,cmd_setenv,"setenv [env] [val]","set environment");
mycmd_init(msleep,cmd_msleep,"msleep ms","sleep ms");
