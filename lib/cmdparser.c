#include "argvize.c"
#define LINESZ	200		/* max command line length */

static char *expand (char *);
static char *expand1 (char *);
static int help(int argc,char **argv);

static int showcmd=0;

#include "cmdparser.h"
#include <myconfig.h>


extern struct mycmd __mycmd_begin;
extern struct mycmd __mycmd_end;
struct mycmd *mycmd_head;
struct mycmd *mycmd_tail;

static void init_cmd(void) __attribute__ ((constructor));
static void init_cmd()
{
struct mycmd *pcmd;
 for(pcmd=&__mycmd_begin+1;pcmd!=&__mycmd_end;pcmd++)
 {
   pcmd->next = pcmd+1;
 }
 mycmd_head = &__mycmd_begin+1;
 mycmd_tail = &__mycmd_end-1;
 mycmd_tail->next = 0;
}

extern struct mysymbol __mysymbol_begin;
extern struct mysymbol __mysymbol_end;
struct mysymbol *mysymbol_head;
struct mysymbol *mysymbol_tail;

static void init_symbol(void) __attribute__ ((constructor));
static void init_symbol()
{
struct mysymbol *psymbol;
 for(psymbol=&__mysymbol_begin+1;psymbol!=&__mysymbol_end;psymbol++)
 {
   psymbol->next = psymbol+1;
 }
 mysymbol_head = &__mysymbol_begin+1;
 mysymbol_tail = &__mysymbol_end;
}

void add_cmd(struct mycmd *mycmd)
{
 printf("add cmd %x\n",mycmd);
 mycmd_tail->next = mycmd;
 mycmd_tail = mycmd;
}

mysymbol_export(add_cmd);

extern struct myconfig __myconfig_begin;
extern struct myconfig __myconfig_end;
struct myconfig *myconfig_head;
struct myconfig *myconfig_tail;

static void init_config(void) __attribute__ ((constructor));
static void init_config()
{
struct myconfig *pconfig;
 for(pconfig=&__myconfig_begin+1;pconfig!=&__myconfig_end;pconfig++)
 {
	 if(pconfig->type == CONFIG_S)
	 {
		 char *p;
		 p = malloc(strlen(*(char **)pconfig->config)+1);
		 strcpy(p,*(char **)pconfig->config);
		 *(char **)pconfig->config = p;
	 }
	 pconfig->next = pconfig+1;
 }
 pconfig--;
 pconfig->next = 0;
 myconfig_head = &__myconfig_begin+1;
 myconfig_tail = pconfig;
}

static int cmd_setconfig(int argc,char **argv)
{
struct myconfig *pconfig;
char str[20];
int i;
if(argc==1)
{
   for(pconfig=myconfig_head,i=0;pconfig;pconfig=pconfig->next,i++)
	{
	if(pconfig->type == CONFIG_I)
	printf("%-30s: 0x%08x:%-30s\n",pconfig->name,*(unsigned int *)pconfig->config,pconfig->note);
	else
	printf("%-30s: %30s:%-30s\n",pconfig->name,*(char **)pconfig->config,pconfig->note);
	if(i%16==15){printf("press enter to continue...\n");read(0,str,20);}
	}
}
else if(argc==2)
{
 for(pconfig=myconfig_head;pconfig;pconfig=pconfig->next)
	if(!strcmp(pconfig->name,argv[1]))
    {
	if(pconfig->type == CONFIG_I)
	printf("%-30s: %08x:%-30s\n",pconfig->name,*(unsigned int *)pconfig->config,pconfig->note);
	else
	printf("%-30s:%30s:%s\n",pconfig->name,*(char **)pconfig->config,pconfig->note);
	break;
	}
 if(!pconfig)printf("can not find config %s\n",argv[1]);
}
else if(argc == 3)
	{
 for(pconfig=myconfig_head;pconfig;pconfig=pconfig->next)
	if(!strcmp(pconfig->name,argv[1]))
    {
	if(pconfig->type == CONFIG_I)
	 *(unsigned int *)pconfig->config = strtoul(argv[2],0,0);
	else
	{
	 *(char **)pconfig->config = realloc(*(char **)pconfig->config,strlen(argv[2])+1);
	 strcpy(*(char **)pconfig->config,argv[2]);
	}
	break;
	}
	}
	
else return -1;

return 0;
}

char *dupstr (int);

/* Generator function for command completion.  STATE lets us
   know whether to start from scratch; without any state
   (i.e. STATE == 0), then we start at the top of the list. */
char *
config_generator (text, state)
     const char *text;
     int state;
{
  static int list_index, len;
  char *name;
 static struct myconfig *pconfig;

  /* If this is a new word to complete, initialize now.  This
     includes saving the length of TEXT for efficiency, and
     initializing the index variable to 0. */
  if (!state)
    {
	  pconfig = myconfig_head;
      len = strlen (text);
    }

  /* Return the next name which partially matches from the
     command list. */



  while (pconfig && (name = pconfig->name))
    {
	  pconfig = pconfig->next;

      if (strncmp (name, text, len) == 0)
        return (dupstr(name));
    }

  /* If no names matched, then return NULL. */
  return ((char *)NULL);

}

static char *config_complete(int argc,char **argv,char *text,int start,int end)
{
if(argc == 1)
return rl_completion_matches (text, config_generator);
if(argc == 2)
 {
	struct myconfig *pconfig;
   for(pconfig=myconfig_head;pconfig && strcmp(argv[1],pconfig->name);pconfig=pconfig->next);
   if(pconfig && pconfig->complete)
    return rl_completion_matches(text,pconfig->complete);
   else return NULL;
 }
else return NULL;
}

mycmd_init1(setconfig,cmd_setconfig,"setconfig [configname] [val]","setconfig for command",config_complete);

static int help(int argc,char **argv)
{
struct mycmd *pcmd;
char str[20];
int i;
if(argc==1)
{
   for(pcmd=mycmd_head,i=0;pcmd;pcmd=pcmd->next,i++)
	{
	printf("%-10s:%-30s:%s\n",pcmd->name,pcmd->usage,pcmd->note);
	if(i%16==15){printf("press enter to continue...\n");read(0,str,20);}
	}
}
else if(argc==2)
{
 for(pcmd=mycmd_head;pcmd;pcmd=pcmd->next)
	if(!strcmp(pcmd->name,argv[1]))
    {
	printf("%-10s:%-30s:%s\n",pcmd->name,pcmd->usage,pcmd->note);
	break;
	}
 if(!pcmd)printf("can not find cmd %s\n",argv[1]);
}
else return -1;

return 0;
}

/*
 *  Execute command
 *  ---------------
 *
 *  Execute the comman string given. If the command string given is
 *  empty, check the 'rptcmd' environment var and repeat the last
 *  command accordingly. If value is 'off' no repeat takes place. If
 *  the value is 'on' all commands repeatable can be repeated. If
 *  value is 'trace' only the 't' and 'to' commands will be repeated.
 */
char *_do_cmd(char *p_in,int need_end);

void do_cmd(const char *p)
{
char *cmd=malloc(strlen(p)+1);
strcpy(cmd,p);
_do_cmd(cmd,0);
free(cmd);
}

char *_do_cmd(char *p_in,int need_end)
{
	char	*av[MAX_AC];	/* argument holder */
	int32_t	ac;		/* # of arguments */
	int	 nc, j, c;
	struct mycmd *pcmd;
	char *p,*p_cmd;

cmd_loop:
	p_cmd=p_in;
	for (; *p_in;) {
		c = *p_in;
		if(c == '\'' || c == '"') {
			p_in++;
			while (*p_in && *p_in != c) {
				++p_in;
			}
			if(*p_in) {
				p_in++;
			}
			else {
				printf("unbalanced %c\n", c);
				return p;
			}
		}
		else if((c == ';')||(c == '\n')) {
			*p_in++ = 0;
			goto run_a_cmd;
		}
		else {
			p_in++;
		}
	}
	if(need_end)return p_cmd;
run_a_cmd:

	/* Expand command substituting $vars. Breakup into separate cmds */

	if(!(p = expand (p_cmd))) {
		return p_cmd;
	}

	if(!(p = expand1 (p))) {
		return p_cmd;
	}

	nc = 0;

	/*
	 *  Lookup command in command list and dispatch.
	 */
	do{
		int stat = -1;
		if(showcmd)printf("#%s\n",p);
		ac = argvize (av, p);
		if(ac == 0|| av[0][0]=='#') {
			break;
		}


   for(pcmd=mycmd_head;pcmd;pcmd=pcmd->next)
   if(!strcmp(av[0],pcmd->name))break;
   if(pcmd)
	stat = pcmd->func(ac, av);
   else 
   {printf("syntax error %d\n",stat);stat=0;}

		if(stat < 0) {
		printf("usage:%-10s:%-30s:%s\n",pcmd->name,pcmd->usage,pcmd->note);
		}

		if(stat != 0) {
			return p_in;	/* skip commands after ';' */
		}
	}while(0);

if(*p_in)goto cmd_loop;
return p_in;
}

/*
 * expand(cmdline) - expand environment variables
 * entry:
 *	char *cmdline pointer to input command line
 * returns:
 *	pointer to static buffer containing expanded line.
 */
static char *
expand(cmdline)
	char *cmdline;
{
	char *ip, *op, *v;
	char var[256];
	static char expline[LINESZ + 8];

	if(!strchr (cmdline, '$')) {
		return cmdline;
	}

	ip = cmdline;
	op = expline;
	while (*ip) {
		if(op >= &expline[sizeof(expline) - 1]) {
			printf ("Line too long after expansion\n");
			return (0);
		}

		if(*ip != '$') {
			*op++ = *ip++;
			continue;
		}

		ip++;
		if(*ip == '$') {
			*op++ = '$';
			ip++;
			continue;
		}

		/* get variable name */
		v = var;
		if(*ip == '{') {
			/* allow ${xxx} */
			ip++;
			while (*ip && *ip != '}') {
				*v++ = *ip++;
			}
			if(*ip && *ip != '}') {
				printf ("Variable syntax\n");
				return (0);
			}
			ip++;
		} else {
			/* look for $[A-Za-z0-9]* */
			while (isalpha(*ip) || isdigit(*ip)) {
				*v++ = *ip++;
			}
		}

		*v = 0;
		if(!(v = getenv (var))) {
			printf ("'%s': undefined\n", var);
			return (0);
		}

		if(op + strlen(v) >= &expline[sizeof(expline) - 1]) {
			printf ("Line expansion ovf.\n");
			return (0);
		}

		while (*v) {
			*op++ = *v++;
		}
	}
	*op = '\0';
	return (expline);
}

static char *expr(char *cmd)
{
char *token;
unsigned int op='+';
unsigned long result=0;
static char cmd1[0x100];
char *inp=cmd;
char *outp=cmd1;
unsigned long data;
char c=0x20;
int deep;
const char *fmt="+-*/\\%|&^hpod";

while(*inp)
{
	if(strchr(fmt,*inp)){
	if(c!=0x20){*outp++=0x20;}
	*outp++=*inp++;*outp++=0x20;c=0x20;
	continue;
	}
	if(*inp==0x20||*inp=='\t'){inp++;continue;}
	*outp++=c=*inp++;
}

*outp=0;
inp=cmd1;

   while(*inp)
   {
	 while(*inp==' '||*inp=='\t')inp++;

	 if(*inp=='('){
		 token=++inp;
		 deep=1;
		 while(1)
		 {
			 if(deep<1 || !*inp){printf("error synax");return 0;}
			 if(*inp==')' && deep==1){*inp=0;inp++;break;}
		 	 if(*inp=='('){inp++;deep++;continue;}
		 	 if(*inp==')'){inp++;deep--;continue;}
			 ++inp;
		 }
		 token=expr(token);
	 }
	 else {
	 token=inp;
	 while(*inp!=' ' && *inp!='\t' && *inp!=0)inp++;
	 if(*inp){*inp=0;inp++;}
	 }

	if(!token||!*token)break;
	if(token[1]==0 && strchr(fmt,*token)){op=*token;continue;}
	data=strtoul(token,0,0);
	switch(op)
	{
		case '+':result=result+data;break;
		case '-':result=result-data;break;
		case '*':result=result*data;break;
		case '/':result=result/data;break;
		case '\\':result=data/result;break;
		case '%':result=result%data;break;
		case '|':result=result|data;break;
		case '&':result=result&data;break;
		case '^':result=result^data;break;
		case '~':result=~result;break;
	}

   }
   switch(op)
   {
	   case 'd':sprintf(cmd1,"%ld",result);break;
	   case 'p':sprintf(cmd1,"%p",(void *)result);break;
	   case 'o':sprintf(cmd1,"0%lo",result);break;
	   default:sprintf(cmd1,"0x%lx",result);break;
   }
   return cmd1;
}

static char *expand1(char *cmdline)
{
	char *ip, *op, *v;
	char var[256];
	static char expline[LINESZ + 8];

	if(!strchr (cmdline, '`')) {
		return cmdline;
	}

	ip = cmdline;
	op = expline;
	while (*ip) {
		if(op >= &expline[sizeof(expline) - 1]) {
			printf ("Line too long after expansion\n");
			return (0);
		}

		if(*ip != '`') {
			*op++ = *ip++;
			continue;
		}

		ip++;
		if(*ip == '`') {
			*op++ = '`';
			ip++;
			continue;
		}

		/* get variable name */
		v = var;
			while (*ip && *ip != '`') {
				*v++ = *ip++;
			}
			if(*ip && *ip != '`') {
				printf ("Variable syntax\n");
				return (0);
			}
			ip++;

		*v = 0;
		if(!(v = expr (var))) {
			printf ("'%s': undefined\n", var);
			return (0);
		}

		if(op + strlen(v) >= &expline[sizeof(expline) - 1]) {
			printf ("Line expansion ovf.\n");
			return (0);
		}

		while (*v) {
			*op++ = *v++;
		}
	}
	*op = '\0';
	return (expline);
}

int cmd_expr(int argc,char **argv)
{
	if(argc!=2)return -1;
	printf("%s\n",expr(argv[1]));
	return 0;
}

mycmd_init(h,help,"h","show this help");
mycmd_init(expr,cmd_expr,"expr","expr");
